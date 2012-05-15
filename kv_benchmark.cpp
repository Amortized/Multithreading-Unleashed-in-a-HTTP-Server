// Benchmarking KV Protocol for async and sync clients

#include "callback.hpp"
#include "kv_connection.hpp"
#include "http_response.hpp"
#include "kv_service.hpp"
#include "io_manager.hpp"
#include "http_connection.hpp"
#include "http_service.hpp"
#include "ticks_clock.hpp"
#include <time.h>
#include <sstream>
#include "request_stats.hpp"

using base::AcceptCallback;
using base::Callback;
using base::IOManager;
using base::ServiceManager;
using base::makeCallableOnce;
using base::makeCallableMany;
using kv::ConnectCallback;
using http::HTTPClientConnection;
using http::HTTPService;
using http::Request;
using http::Response;
using http::ResponseCallback;
using kv::KVService;
using kv::KVClientConnection;
using base::TicksClock;
using base::RequestStats;

class ProgressMeter {
public:
  ProgressMeter(ServiceManager* service, KVService* kvservice) : service_(service), kvservice_(kvservice) {};
  ~ProgressMeter() {}

  void check();

private:
  ServiceManager* service_; // not owned here
  KVService* kvservice_; // not owned here

};

void ProgressMeter::check() {
  // TODO
  // if made progress since last time
  //   print progress
  //   reschedule
  // else
  // server->stop();
  // For now, just stop after 10 seconds.
  static int i=0;
  if (++i == 10) {
    std::cout << "stop" << std::endl;
    service_->stop();
    return;
  }

  std::cout << "." << std::flush;
 
  RequestStats* stats = kvservice_->stats();
  uint32_t reqsLastSec;
  stats->getStats(TicksClock::getTicks(), &reqsLastSec, NULL, NULL, NULL);
  std::ostringstream stats_stream;
  stats_stream << reqsLastSec;
  std::cout << "\n No of Requests served in last sec " << reqsLastSec << std::endl;

  Callback<void>* cb = makeCallableOnce(&ProgressMeter::check, this);
  IOManager* io_manager = service_->io_manager();
  io_manager->addTimer(1.0 /*sec*/ , cb);
}

class AsyncClient {
public:
  AsyncClient();
  ~AsyncClient();

  void start(KVClientConnection* conn);
  void doRequest();
  void requestDone(Response* response);

private:
  KVClientConnection* conn_;
  Callback<void>*       request_cb_;   // owned here
  ResponseCallback*     response_cb_;  // owned here
  
  // non-copyable, non-assignable
  AsyncClient(const AsyncClient&);
  AsyncClient& operator=(const AsyncClient&);
};

AsyncClient::AsyncClient() : conn_(NULL), request_cb_(NULL), response_cb_(NULL) {
 srand(time(NULL));
 
}

AsyncClient::~AsyncClient() {
  delete request_cb_;
  delete response_cb_;
}

void AsyncClient::start(KVClientConnection* conn) {
  if (! conn->ok()) {
    LOG(LogMessage::ERROR) << "Cannot connect: " << conn->errorString();
    return;
  }

  conn_ = conn;
  request_cb_ = makeCallableMany(&AsyncClient::doRequest, this);
  response_cb_ = makeCallableMany(&AsyncClient::requestDone, this);

  // Issue the first request. acquire() here is necessary because
  // doRequest() always release()s. And that makes it possible for it
  // to be scheduled using a timer.
  conn_->acquire();
  doRequest();
}

void AsyncClient::doRequest() {
  if (! conn_->ok()) {
    LOG(LogMessage::ERROR) << "In new request: " << conn_->errorString();

  } else {
    http::Request request;
    request.method = "GET";
    
    int temp = rand() % 300 +1;
    std::ostringstream buffer;
    buffer << temp;	
    request.address = "/" + buffer.str();
    request.version = "HTTP/1.1";
    conn_->asyncSend(&request, response_cb_);
  }
  conn_->release();
}

void AsyncClient::requestDone(Response* response) {
  if (! conn_->ok()) {
    LOG(LogMessage::ERROR) << "In requestDone: " << conn_->errorString();
    return;
  }

  conn_->acquire();
  IOManager* io_manager = conn_->io_manager();
  io_manager->addTask(request_cb_);
}

int main(int argc, char* argv[]) {
  // Even if using just client calls, we set up a service. Client calls
  // will use the service's io_manager and use it to collect stats
  // automatically.
  ServiceManager service(8 /* num_workers */);
  KVService kv_service(16111, &service);

  // Open and "fire" N parallel clients.
  std::cout << " For 32 Async Clients " << std::endl;
  const int parallel = 32;
  AsyncClient clients[parallel];
  for (int i=0; i<parallel; ++i) {
    ConnectCallback* connect_cb = makeCallableOnce(&AsyncClient::start, &clients[i]);
    kv_service.asyncConnect("127.0.0.1", 16111, connect_cb);
  }
 
  // Launch a progress meter in the background. If things hang, let
  // the meter kill this process.
  ProgressMeter meter(&service, &kv_service);
  Callback<void>* progress_cb = makeCallableOnce(&ProgressMeter::check, &meter);
  IOManager* io_manager = service.io_manager();
  io_manager->addTimer(1.0 /*sec*/ , progress_cb);

  // We'd need a '/quit' request to break out of the server.
  // TODO have SIGINT break out nicely as well.
  service.run();
  return 0;
}
