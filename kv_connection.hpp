#ifndef MCP_KV_CONNECTION_HEADER
#define MCP_KV_CONNECTION_HEADER

#include <queue>
#include <string>
#include "connection.hpp"
#include "kv_service.hpp"
#include "callback.hpp"
#include "http_request.hpp"
#include "lock.hpp"

namespace kv {

using std::queue;
using std::string;
using base::Mutex;
using base::Notification;
using http::Request;
using base::Buffer;


class KVServerConnection : public base::Connection {
public:
  KVServerConnection(KVService* service, int client_fd);

private:
  KVService* my_service_;
  Request      request_;
  LockFreeHashTable* hash_table_;  //Not owned here

  // base::Connection is ref counted. Use release() to
  // delete. Normally, you won't need to because the io_manager will
  // do that for you.
  virtual ~KVServerConnection() {}

  // Parses as many requests as there are in the input buffer and
  // generates responses for each of them.
  virtual bool readDone();

  bool handleRequest(Request* request);

  // Non-copyable, non-assignalble
  KVServerConnection(const KVServerConnection&);
  KVServerConnection& operator=(const KVServerConnection&);
};

class KVClientConnection : public base::Connection {
public:
  KVClientConnection(KVService* service);
  // Copies 'request' into the connection output buffer and schedules
  // a write. Enqueues 'cb' for when a reponse arrives.
  void asyncSend(Request* request, ResponseCallback* cb);

  // Issues 'request' and blocks until 'response' arrives. This is the
  // synchronous version of send.
  void send(Request* request, Response** response);

private:
  friend class KVService;
  KVService* my_service_;

  // The connect callback is always issued, independently of the
  // result of the connect itself. 'connect_cb_' would thus self
  // delete if it is a once only callback.
  ConnectCallback*         connect_cb_;

  Mutex                    m_response_;    // protects the queue below
  queue<ResponseCallback*> response_cbs_;  // owned here


  // base::Connection is ref counted so the destructor shoudn't be
  // issued directly. The connection would get deleted if 'connDone()'
  // below doesn't start reading or if 'readDone()' returns false (to
  // stop reading).
  virtual ~KVClientConnection() {}

  // Tries to connect with a server sitting on 'host:port' and issue
  // 'cb' with the resulting attempt.
  void connect(const string& host, int port, ConnectCallback* cb);



  // If the connect request went through, start reading from the
  // connection. In any case, issue the callback that was registered.
  virtual void connDone();

  // Parses as many responses as there are in the input buffer for
  // this connection and issues one callback per parsed response.
  virtual bool readDone();

  bool handleResponse(Response* response);

  // Completion call used in sync receive().
  void receiveInternal(Notification* n,
                       Response** user_response,
                       Response* new_response);



  // Non-copyable, non-assignalble
  KVClientConnection(const KVClientConnection&);
  KVClientConnection& operator=(const KVClientConnection&);
};

} // namespace kv

#endif // MCP_KV_CONNECTION_HEADER
