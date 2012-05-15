#ifndef MCP_BASE_TEST_UTIL_HEADER
#define MCP_BASE_TEST_UTIL_HEADER


#include <iostream>
#include "lock.hpp"
#include "thread_pool_normal.hpp"
#include "thread_pool_fast.hpp"
#include "callback.hpp"


namespace test {


using base::Callback;
using base::ThreadPool;
using base::ThreadPoolNormal;
using base::ThreadPoolFast;
using base::Mutex;
// A simple counter class used often in unit test (and not supposed to
// be used outside *_test.cpp files.

class Counter {
public:
  Counter()          { reset(); }
  ~Counter()         { }

  int  count() const { 
    this->locker_.lock();
    return count_;
    this->locker_.unlock();
  } 
  void set(int i) { 
    this->locker_.lock();
    count_ = i;
    this->locker_.unlock();
  }
  void reset() { 
    this->locker_.lock();
    count_ = 0;
    this->locker_.unlock();
  }
  void inc() { 
    this->locker_.lock();
    ++count_;
    this->locker_.unlock();
  } 
  void incBy(int i) { 
    this->locker_.lock();
    count_ += i; 
    this->locker_.unlock();
  } 
 

  bool between(int i, int j) {
    if (i > count_) return false;
    if (j < count_) return false;
    return true;
  }

private:
  int  count_;
  mutable Mutex locker_;
};

class LongTask {
public:
  void task(Counter* c) { 
    for(int i=0; i<100000; i++) {
      c->inc();
    }
    c->reset(); 
    for(int k=0; k<100000; k++) { 

      if(c->between(0,50000)) {
        c->incBy(10);
      } else {
      c->incBy(20);
      }

    } 
  }
};

class ShortTask {
public:
  void task(Counter* c) {   c->inc();  }
};

class TaskAdderNormal {
public:
  void add(ThreadPoolNormal** tp,Callback<void>** task) { 
   this->locker.lock(); 
   (*tp)->addTask(*task);
   this->locker.unlock();
  }
private:
  Mutex locker; 
};

class TaskAdderFast {
public:
  void add(ThreadPoolFast** tp,Callback<void>** task) {
   this->locker.lock();
   (*tp)->addTask(*task);
   this->locker.unlock();
  }
private:
  Mutex locker;
};


class TaskStopNormal {
public:
  void stopTask(ThreadPoolNormal** tp) { 
    this->locker.lock(); 
    (*tp)->stop();
    this->locker.unlock();   
  }
private:
  Mutex locker;
};

class TaskStopFast {
public:
  void stopTask(ThreadPoolFast** tp) {
    this->locker.lock();
    (*tp)->stop();
    this->locker.unlock();
  }
private:
  Mutex locker;
};

}  // namespace base

#endif // MCP_BASE_TEST_UTIL_HEADER
