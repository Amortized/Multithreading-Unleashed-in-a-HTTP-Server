#ifndef MCP_BASE_THREAD_POOL_NORMAL_HEADER
#define MCP_BASE_THREAD_POOL_NORMAL_HEADER

#include "callback.hpp"
#include "thread_pool.hpp"
#include "lock.hpp"
//#include "timer.hpp"


#include <queue>
#include <vector>
#include <pthread.h>


namespace base {

using std::vector;
using std::queue;

class ThreadPoolNormal : public ThreadPool {
public:
  // ThreadPoolNormal interface
  explicit ThreadPoolNormal(int num_workers);
  virtual ~ThreadPoolNormal();

  virtual void addTask(Callback<void>* task);
  virtual void stop();
  virtual int count() const;

private:

  // Non-copyable, non-assignable.
  ThreadPoolNormal(const ThreadPoolNormal&);
  ThreadPoolNormal& operator=(const ThreadPoolNormal&);

  bool flagstop_;
  vector<pthread_t> threads_;
  queue<Callback<void>*> tasks_;  
  mutable Mutex queue_lock_;
  ConditionVar queue_empty_;

  void ExecuteTask();
};

} // namespace base

#endif // MCP_BASE_THREAD_POOL_NORMAL_HEADER

