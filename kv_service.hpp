#ifndef MCP_KV_SERVICE_HEADER
#define MCP_KV_SERVICE_HEADER

#include <string>
#include "service_manager.hpp"
#include "http_response.hpp"
#include "lock_free_hash_table.hpp"
#include "request_stats.hpp"

namespace kv {

using std::string;
using base::AcceptCallback;
using base::ServiceManager;
using base::Notification;
using http::Response;
using base::RequestStats;
using lock_free_hash_table::LockFreeHashTable;

class KVClientConnection;
class Response;
typedef base::Callback<void, KVClientConnection*> ConnectCallback;
typedef base::Callback<void, Response*> ResponseCallback;


class KVService {
public:
  KVService(int port, ServiceManager* service_manager);
  ~KVService();

  // Asks the service manager to stop all the registered services.
  void stop();

  // Client side

  // Tries to connect to 'host:port' and issues 'cb' with the
  // resulting attempt. ConnectCallback takes an HTTPConnection as
  // parameter.
  void asyncConnect(const string& host, int port, ConnectCallback* cb);

  // Synchronous dual of asyncConnect. The ownership of
  // HTTPClientConnection is transfered to the caller.
  void connect(const string& host, int port, KVClientConnection** conn);

  //accessors
  ServiceManager* service_manager() { return service_manager_; }
  LockFreeHashTable* hash_table() { return hash_table_; } 
  RequestStats* stats() { return &stats_; }

private:
  ServiceManager* service_manager_;  // not owned here
  LockFreeHashTable* hash_table_;
  RequestStats    stats_;

  // Starts the server-side of a new HTTP connection established on
  // socket 'client_fd'.
  void acceptConnection(int client_fd);

  void buildHashTable();

  // Completion call used in synchronous connection.
  void connectInternal(Notification* n,
                       KVClientConnection** user_conn,
                       KVClientConnection* new_conn);


  // Non-copyable, non-assignable.
  KVService(const KVService&);
  KVService& operator=(const KVService&);
};

} // namespace kv

#endif //  MCP_KV_SERVICE_HEADER
