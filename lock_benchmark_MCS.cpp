
#include "spinlock_mcs.hpp"
#include "callback.hpp"
#include "spinlock.hpp"
#include "thread.hpp"
#include "test_unit.hpp"
#include "lock.hpp"
#include "timer.hpp"
#include "thread_local.hpp"

#include <vector>

namespace {

using base::Callback;
using base::makeCallableOnce;
using base::makeThread;
using base::Spinlock;
using base::Timer;
using base::ThreadLocal;
using base::SpinlockMCS;
using base::Node;
// ************************************************************
// Spin Locks
//
   
class SpinLockTester {
public:
  SpinLockTester(Spinlock* spin, int* counter)
    : spin_(spin), counter_(counter), requests_(0) {
  } 
  ~SpinLockTester() { }

  void start(int increments) {
    Callback<void>* body = makeCallableOnce(&SpinLockTester::test, this, increments);
    tid_ = makeThread(body);
  }

  void join() {
    pthread_join(tid_, NULL);
  }

  int requests() const { return requests_; }
  virtual void test(int increments) = 0;

protected:
  Spinlock* spin_;
  int*      counter_;
  int       requests_;
  pthread_t tid_;

  // Non-copyable, non-assignable
  SpinLockTester(SpinLockTester&);
  SpinLockTester& operator=(SpinLockTester&);
};

//Small Critical Region
class SmallCriticalSpinLockTester: public SpinLockTester {
public:
  SmallCriticalSpinLockTester(Spinlock& spin, int& counter)
    : SpinLockTester(&spin, &counter) {
  }
  
  void test(int increments) {	
    while (increments-- > 0) {
      spin_->lock();
      ++(*counter_);
      ++requests_;
      spin_->unlock();
    }
  } 
};  
  
//Larger Critical Region
class LargeCriticalSpinLockTester: public SpinLockTester {
public:
  LargeCriticalSpinLockTester(Spinlock& spin, int& counter)
    : SpinLockTester(&spin, &counter) {
  }

  void test(int increments) {
    spin_->lock();
    while (increments-- > 0) {
      ++(*counter_);
      ++requests_;
      int temp = 1000;
      while(temp > 0) {
        temp--;
      }
    }
    spin_->unlock();  
  }
};


// *************** MCS Spin Locks *********

class SpinLockMCSTester {
public: 
  SpinLockMCSTester(SpinlockMCS* spin, int* counter)
    : spin_(spin), counter_(counter), requests_(0) {
  }

  ~SpinLockMCSTester() { } 

  void start(int increments) {
    Callback<void>* body = makeCallableOnce(&SpinLockMCSTester::test, this, increments);
    tid_ = makeThread(body);
  }

  void join() {
    pthread_join(tid_, NULL);
  }

  int requests() const { return requests_; }
  virtual void test(int increments) = 0;

protected:
  SpinlockMCS* spin_; 
  int*      counter_;
  int       requests_;
  pthread_t tid_;
    
  ThreadLocal<struct Node> local_;
    

  // Non-copyable, non-assignable
  SpinLockMCSTester(SpinLockMCSTester&);
  SpinLockMCSTester& operator=(SpinLockMCSTester&);
};



//Small Critical Region
class SmallCriticalSpinLockMCSTester: public SpinLockMCSTester {
public:
  SmallCriticalSpinLockMCSTester(SpinlockMCS& spin, int& counter)
    : SpinLockMCSTester(&spin, &counter) {
  }

  void test(int increments) {
    while (increments-- > 0) {
      spin_->lock(local_.getAddr());
      ++(*counter_);
      ++requests_;
      spin_->unlock(local_.getAddr());
    }
  }
};

//Larger Critical Region
class LargeCriticalSpinLockMCSTester: public SpinLockMCSTester {
public:
  LargeCriticalSpinLockMCSTester(SpinlockMCS& spin, int& counter)
    : SpinLockMCSTester(&spin, &counter) {
  }

  void test(int increments) {
    spin_->lock(local_.getAddr());
    while (increments-- > 0) {
      ++(*counter_);
      ++requests_;
      int temp = 1000;
      while(temp > 0) {
        temp--;
      }
    }
    spin_->unlock(local_.getAddr());
  }
};







// 
// Test cases
//

class Test {
public:
  void SpinLocksShortCritical();
  void SpinLocksLongCritical();
  void SpinLocksMCSShortCritical();
  void SpinLocksMCSLongCritical();
  void SmallCriticalResults();
  void LargeCriticalResults();

  double findLarger(double,double,int*);
private:
  std::vector<double> SpinLocksShort_;
  std::vector<double> SpinLocksLong_;
  std::vector<double> SpinLocksMCSShort_;
  std::vector<double> SpinLocksMCSLong_;
};

void Test::SpinLocksShortCritical() {   
  Spinlock spin;
  int counter = 0;
  int threads = 2;
  const int incs = 200;
  const int MAX_THREADS = 8;
  for(; threads <= MAX_THREADS; threads++) {
    Timer t;
    counter = 0;
    std::vector<SpinLockTester*> testers;
    for (int i = 0; i < threads; i++) {
      testers.push_back(new SmallCriticalSpinLockTester(spin, counter));
    }

    t.start();
    for (int i = 0; i < threads; i++) {
      testers.at(i)->start(incs);
    }
    for (int i = 0; i < threads; i++) {
      testers.at(i)->join();
      delete testers.at(i);    
    }
    t.end();
    SpinLocksShort_.push_back(t.elapsed());
  } 
} 

void Test::SpinLocksLongCritical() {
  Spinlock spin;
  int counter = 0;
  int threads = 2;
  const int incs = 200;
  const int MAX_THREADS = 8;
  for(; threads <= MAX_THREADS; threads++) {
    Timer t;
    counter = 0;
    std::vector<SpinLockTester*> testers;
    for (int i = 0; i < threads; i++) {
      testers.push_back(new LargeCriticalSpinLockTester(spin, counter));
    }
    t.start();
    for (int i = 0; i < threads; i++) {
      testers.at(i)->start(incs);
    }
    for (int i = 0; i < threads; i++) { 
      testers.at(i)->join();
      delete testers.at(i);
    }
    t.end();
    SpinLocksLong_.push_back(t.elapsed());
  }
} 


void Test::SpinLocksMCSShortCritical() {
  SpinlockMCS spin;
  int counter = 0;
  int threads = 2;
  const int incs = 200;
  const int MAX_THREADS = 8;
  for(; threads <= MAX_THREADS; threads++) {
    Timer t;
    counter = 0;
    std::vector<SpinLockMCSTester*> testers;
    for (int i = 0; i < threads; i++) {
      testers.push_back(new SmallCriticalSpinLockMCSTester(spin, counter));
    }
    t.start();
    for (int i = 0; i < threads; i++) {
      testers.at(i)->start(incs);
    }
    for (int i = 0; i < threads; i++) {
      testers.at(i)->join();
      delete testers.at(i);
    }
    t.end();
    SpinLocksMCSShort_.push_back(t.elapsed());
  }
}

void Test::SpinLocksMCSLongCritical() {
  SpinlockMCS spin;
  int counter = 0;
  int threads = 2;
  const int incs = 200;
  const int MAX_THREADS = 8;
  for(; threads <= MAX_THREADS; threads++) {
    Timer t;
    counter = 0;
    std::vector<SpinLockMCSTester*> testers;
    for (int i = 0; i < threads; i++) {
      testers.push_back(new LargeCriticalSpinLockMCSTester(spin, counter));
    }
    t.start();
    for (int i = 0; i < threads; i++) {
      testers.at(i)->start(incs);
    }
    for (int i = 0; i < threads; i++) {
      testers.at(i)->join();
      delete testers.at(i);
    }
    t.end();
    SpinLocksMCSLong_.push_back(t.elapsed());
  }
} 

double Test::findLarger(double arg1, double arg2,int* largest) {
  if(arg1 >= arg2) {
    *largest = 1;
    return ((1 - (arg2/arg1)) * 100);
  }
  else {
   *largest = 2;
   return ((1-(arg1/arg2)) * 100);
  }
}

void Test::SmallCriticalResults() {
  int larger = 0;
  std::cout << " ---------------------------------" << std::endl;
  std::cout << "**** SHORT CRITICAL SECTION ****" << std::endl;
  std::cout << "The following table shows the total time taken by all threads to complete their tasks for both locks" << std::endl;
  std::cout << "No of threads:" << "   Backoff Spin Locks " << "  Queue Based(MCS) Spin Locks    " << std::endl;
  for(int i=0; i < (int)SpinLocksShort_.size(); i++) {
    std::cout << (2+i) << "                  " << SpinLocksShort_.at(i) << "                  " << SpinLocksMCSShort_.at(i) ;

    double percentage =  findLarger(SpinLocksShort_.at(i), SpinLocksMCSShort_.at(i),&larger);

    if(larger == 2) {
       std::cout << "               SpinLocks Faster by " << percentage << "%" << std::endl;
    }
    else {
       std::cout << "               MCS Locks Faster by " << percentage << "%" << std::endl;
    }
 
  }

}

void Test::LargeCriticalResults() {
  int larger = 0;
  std::cout << "\n ---------------------------------" << std::endl;
  std::cout << "**** LONG CRITICAL SECTION ****" << std::endl;
  std::cout << "The following table shows the total time taken by all threads to complete their tasks for both locks" << std::endl;
  std::cout << "No of threads:" << "   Backoff Spin Locks " << "  Queue Based(MCS) Spin Locks" << std::endl;
  for(int i=0; i < (int)SpinLocksLong_.size(); i++) {
    std::cout << (2+i) << "                  " << SpinLocksLong_.at(i) << "                   " << SpinLocksMCSLong_.at(i);  
    double percentage =  findLarger(SpinLocksLong_.at(i), SpinLocksMCSLong_.at(i),&larger);

    if(larger == 2) {
       std::cout << "              SpinLocks Faster by " << percentage << "%" << std::endl;
    }
    else {
       std::cout << "              MCS Faster by " << percentage << "%" << std::endl;
    }
  }


}

}  // unnamed namespace

int main(int argc, char *argv[]) {
  Test t;
  std::cout << "Executing ...." <<  std::endl;
  t.SpinLocksShortCritical();
  t.SpinLocksLongCritical();
  t.SpinLocksMCSShortCritical();
  t.SpinLocksMCSLongCritical();
  t.SmallCriticalResults();
  t.LargeCriticalResults();
  return 0;
}
