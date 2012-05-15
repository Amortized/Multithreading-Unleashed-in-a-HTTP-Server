
#include "callback.hpp"
#include "spinlock.hpp"
#include "thread.hpp"
#include "test_unit.hpp"
#include "lock.hpp"
#include "timer.hpp"
#include <vector>
namespace {

using base::Callback;
using base::makeCallableOnce;
using base::makeThread;
using base::Spinlock;
using base::Mutex;
using base::Timer;
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


// *************** Blocking Locks *********
class BlockingLockTester {
public:
  BlockingLockTester(Mutex* mymutex, int* counter) 
   : mymutex_(mymutex), counter_(counter), requests_(0) {
  }
  ~BlockingLockTester() { }

  void start(int increments) {
    Callback<void>* body = makeCallableOnce(&BlockingLockTester::test, this, increments);
    tid_ = makeThread(body);
  }
  void join() {
    pthread_join(tid_, NULL);
  }

  int requests() const { return requests_; }
  virtual void test(int increments) = 0;
protected:
  Mutex* mymutex_;
  int*      counter_;
  int       requests_;
  pthread_t tid_;

  // Non-copyable, non-assignable
  BlockingLockTester(BlockingLockTester&);
  BlockingLockTester& operator=(BlockingLockTester&);
}; 

//Small Critical Section
class SmallCriticalBlockingLockTester: public BlockingLockTester {
public:
  SmallCriticalBlockingLockTester(Mutex& mymutex, int& counter)
    : BlockingLockTester(&mymutex, &counter) {
  }

  void test(int increments) {
    while (increments-- > 0) {
      mymutex_->lock();
      ++(*counter_);
      ++requests_;
      mymutex_->unlock();
    }
  }
};

//Large Critical Section
class LargeCriticalBlockingLockTester: public BlockingLockTester {
public:
  LargeCriticalBlockingLockTester(Mutex& mymutex, int& counter)
    : BlockingLockTester(&mymutex, &counter) {
  }

  void test(int increments) {
    mymutex_->lock();
    while (increments-- > 0) {
      ++(*counter_);
      ++requests_;
      int temp = 100;
      while(temp > 0) {
        temp--;
      }
    }
    mymutex_->unlock();
  }
};



// 
// Test cases
//

class Test {
public:
  void SpinLocksShortCritical();
  void SpinLocksLongCritical();
  void BlockingLocksShortCritical();
  void BlockingLocksLongCritical();
  void SmallCriticalResults();
  void LargeCriticalResults();

  double findLarger(double,double,int*);
private:
  std::vector<double> SpinLocksShort_;
  std::vector<double> SpinLocksLong_;
  std::vector<double> BlockingLocksShort_;
  std::vector<double> BlockingLocksLong_;
};

void Test::SpinLocksShortCritical() {   
  Spinlock spin;
  int counter = 0;
  int threads = 2;
  const int incs = 50000;
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
  const int incs = 50000;
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


void Test::BlockingLocksShortCritical() {
  Mutex mymutex;
  int counter = 0;
  int threads = 2;
  const int incs = 50000;
  const int MAX_THREADS = 8;
  for(; threads <= MAX_THREADS; threads++) {
    Timer t;
    counter = 0;
    std::vector<BlockingLockTester*> testers;
    for (int i = 0; i < threads; i++) {
      testers.push_back(new SmallCriticalBlockingLockTester(mymutex, counter));
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
    BlockingLocksShort_.push_back(t.elapsed());
  }
}

void Test::BlockingLocksLongCritical() {
  Mutex mymutex;
  int counter = 0;
  int threads = 2;
  const int incs = 50000;
  const int MAX_THREADS = 8;
  for(; threads <= MAX_THREADS; threads++) {
    Timer t;
    counter = 0;
    std::vector<BlockingLockTester*> testers;
    for (int i = 0; i < threads; i++) {
      testers.push_back(new LargeCriticalBlockingLockTester(mymutex, counter));
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
    BlockingLocksLong_.push_back(t.elapsed());
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
  std::cout << "No of threads:" << "   Spin Locks " << "  Blocking Locks    " << std::endl;
  for(int i=0; i < (int)SpinLocksShort_.size(); i++) {
    std::cout << (2+i) << "                  " << SpinLocksShort_.at(i) << "        " << BlockingLocksShort_.at(i) ;

    double percentage =  findLarger(SpinLocksShort_.at(i), BlockingLocksShort_.at(i),&larger);

    if(larger == 2) {
       std::cout << " SpinLocks Faster by " << percentage << "%" << std::endl;
    }
    else {
       std::cout << " Blocking Faster by " << percentage << "%" << std::endl;
    }
 
  }

  std::cout << "\n ***** Conclusion: Spin Locks are better than Blocking Locks when lock is held for a short duration *** " << std::endl;
  std::cout <<"\n *****  Note: If Spin locks sleep for a long time and critical section is small, Blocking locks perform better **" << std::endl;
  std::cout <<"\n *****  Note: If Spin locks sleep too fast/slow  and critical section is small, Spin locks perform better **" << std::endl;
}

void Test::LargeCriticalResults() {
  int larger = 0;
  std::cout << "\n ---------------------------------" << std::endl;
  std::cout << "**** LONG CRITICAL SECTION ****" << std::endl;
  std::cout << "The following table shows the total time taken by all threads to complete their tasks for both locks" << std::endl;
  std::cout << "No of threads:" << "   Spin Locks " << "  Blocking Locks" << std::endl;
  for(int i=0; i < (int)SpinLocksLong_.size(); i++) {
    std::cout << (2+i) << "                  " << SpinLocksLong_.at(i) << "        " << BlockingLocksLong_.at(i);
   
    double percentage =  findLarger(SpinLocksLong_.at(i), BlockingLocksLong_.at(i),&larger);

    if(larger == 2) {
       std::cout << " SpinLocks Faster by " << percentage << "%" << std::endl;
    }
    else {
       std::cout << " Blocking Faster by " << percentage << "%" << std::endl;
    }
  }

  std::cout << "\n ***** Conclusion: Blocking Locks are better than Spin Locks when lock is held for a long duration *** " << std::endl;
  std::cout <<"\n *****  Note: Even If Spin locks sleep for a long/short time or too fast/slow , Blocking locks perform better **" << std::endl;

}

}  // unnamed namespace

int main(int argc, char *argv[]) {
  Test t;
  std::cout << "Executing ...." <<  std::endl;
  t.SpinLocksShortCritical();
  t.SpinLocksLongCritical();
  t.BlockingLocksShortCritical();
  t.BlockingLocksLongCritical();
  t.SmallCriticalResults();
  t.LargeCriticalResults();
  return 0;
}
