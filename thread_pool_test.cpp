#include "thread_pool_normal.hpp"
#include "thread_pool_fast.hpp"
#include "test_unit.hpp"
#include "test_util.hpp"
#include "lock.hpp"
#include "callback.hpp"
#include <signal.h>
 
#include<iostream>
#include<ctime>
 
namespace {

using base::ThreadPool;
using base::ThreadPoolNormal;
using base::ThreadPoolFast;
using base::Callback;
using base::Notification;
using test::Counter;
using test::TaskAdderNormal;
using test::TaskAdderFast;
using test::LongTask;
using test::TaskStopNormal;
using test::TaskStopFast;
using base::makeCallableOnce;
using base::makeCallableMany;


TEST(AddTaskFastPool, SingleThread) {
  ThreadPoolFast* tpn = new ThreadPoolFast(1);
  Counter c;
  
  for(int i=0; i<10; i++) {
   Callback<void>* cb = makeCallableMany(&Counter::inc, &c);
   tpn->addTask(cb);
  }

  for(int k=0; k<10; k++) {
   Callback<void>* cb1 = makeCallableMany(&Counter::incBy, &c, 3);
   tpn->addTask(cb1);
  }

  for(int m=0; m<10; m++) {
   Callback<void,int>* cb2 = makeCallableOnce(&Counter::incBy, &c);
   Callback<void>* cb3 = makeCallableOnce(&Callback<void,int>::operator(), cb2, 4);
   tpn->addTask(cb3);
  }

  sleep(1);

  tpn->stop();
  EXPECT_EQ(c.count(), 80);

  delete tpn;

}

// Single Thread in the Pool
TEST(AddTaskNormalPool, SingleThread) {
  ThreadPoolNormal* tpn = new ThreadPoolNormal(1); 
  Counter c;

  //Pool 1
  for(int i=0; i<10; i++) {
   Callback<void>* cb = makeCallableMany(&Counter::inc, &c);
   tpn->addTask(cb);   
  }

  for(int k=0; k<10; k++) {
   Callback<void>* cb1 = makeCallableMany(&Counter::incBy, &c, 3);
   tpn->addTask(cb1);
  }

  for(int m=0; m<10; m++) {
   Callback<void,int>* cb2 = makeCallableOnce(&Counter::incBy, &c);
   Callback<void>* cb3 = makeCallableOnce(&Callback<void,int>::operator(), cb2, 4);
   tpn->addTask(cb3);
  }

  sleep(1);
  tpn->stop();
  EXPECT_EQ(c.count(), 80);
   
}


// More than one thread in the pool
TEST(AddTaskNormalPool, MoreThanOneThread) {  
  ThreadPoolNormal* tpn = new ThreadPoolNormal(5);
  Counter c;
  Counter c1;

  for(int i=0; i<50000; i++) {
   Callback<void>* cb = makeCallableOnce(&Counter::inc, &c);
   Callback<void>* cb1 = makeCallableOnce(&Counter::incBy, &c1, 1);
   tpn->addTask(cb);
   tpn->addTask(cb1);
  }
  	   
  sleep(1);

  tpn->stop();
  delete tpn;

  EXPECT_EQ(c.count(), 50000);
  EXPECT_EQ(c1.count(), 50000);
}


// More than one thread in the pool
TEST(AddTaskFastPool, MoreThanOneThread) {
  ThreadPoolFast* tpn = new ThreadPoolFast(5);
  Counter c;
  Counter c1;

  for(int i=0; i<10000; i++) {
   Callback<void>* cb = makeCallableOnce(&Counter::inc, &c);
   Callback<void>* cb1 = makeCallableOnce(&Counter::incBy, &c1, 1);
   tpn->addTask(cb);
   tpn->addTask(cb1);
  }
  
  sleep(4);

  tpn->stop();
  delete tpn;

  EXPECT_EQ(c.count(), 10000);
  EXPECT_EQ(c1.count(), 10000);
}





//Threads adding tasks to pool
TEST(AddTaskNormalPool, ThreadsAddingTask) {
  TaskAdderNormal ta;
  ThreadPoolNormal* tpn = new ThreadPoolNormal(5);
  Counter c;
  Counter c1;

  for(int i=0; i<40000; i++) {
   Callback<void>* cb = makeCallableOnce(&Counter::inc, &c);
   Callback<void>* cb1 = makeCallableOnce(&Counter::incBy, &c1, 1);
   tpn->addTask(cb);
   tpn->addTask(cb1);
  }
   
   Callback<void>* cb = makeCallableOnce(&Counter::inc, &c);
   Callback<void>* cb1 = makeCallableOnce(&TaskAdderNormal::add, &ta, &tpn, &cb);
   tpn->addTask(cb1);

   Callback<void>* cb2 = makeCallableOnce(&Counter::incBy, &c1, 10);
   Callback<void>* cb3 = makeCallableOnce(&TaskAdderNormal::add, &ta, &tpn, &cb2);
   tpn->addTask(cb3);
 

  sleep(1);

  tpn->stop();
  delete tpn;
  EXPECT_EQ(c.count(), 40001);
  EXPECT_EQ(c1.count(), 40010);
}


//Threads adding tasks to pool
TEST(AddTaskFastPool, ThreadsAddingTask) {
  TaskAdderFast ta;
  ThreadPoolFast* tpn = new ThreadPoolFast(5);
  Counter c;
  Counter c1;

  for(int i=0; i<10000; i++) {
   Callback<void>* cb = makeCallableOnce(&Counter::inc, &c);
   Callback<void>* cb1 = makeCallableOnce(&Counter::incBy, &c1, 1);
   tpn->addTask(cb);
   tpn->addTask(cb1);
  }

   Callback<void>* cb = makeCallableOnce(&Counter::inc, &c);
   Callback<void>* cb1 = makeCallableOnce(&TaskAdderFast::add, &ta, &tpn, &cb);
   tpn->addTask(cb1);

   Callback<void>* cb2 = makeCallableOnce(&Counter::incBy, &c1, 10);
   Callback<void>* cb3 = makeCallableOnce(&TaskAdderFast::add, &ta, &tpn, &cb2);
   tpn->addTask(cb3);


  sleep(4);

  tpn->stop();
  delete tpn;
  
 
  EXPECT_EQ(c.count(), 10001);
  EXPECT_EQ(c1.count(), 10010);
}




// Main Threads adds stops tasks to the current pool
TEST(StopNormalPool, ThreadOutsidePool) {
  ThreadPoolNormal* tpn = new ThreadPoolNormal(10);
  Counter c;

  for(int i=0; i<100000; i++) {
   Callback<void>* cb = makeCallableOnce(&Counter::inc, &c);
   tpn->addTask(cb);
  }

  sleep(1);
  tpn->stop();

  //No Further tasks added
  for(int i=0; i<10000; i++) {
   Callback<void>* cb = makeCallableOnce(&Counter::inc, &c);
   tpn->addTask(cb);
   delete cb;
  }
  
  delete tpn;
  EXPECT_EQ(c.count(), 100000);
}


// Main Threads adds stops tasks to the current pool
TEST(StopFastPool, ThreadOutsidePool) {
  ThreadPoolFast* tpn = new ThreadPoolFast(3);
  Counter c;

  for(int i=0; i<1000; i++) {
   Callback<void>* cb = makeCallableOnce(&Counter::inc, &c);
   tpn->addTask(cb);
  }

  sleep(4);
  tpn->stop();

  //No Further tasks added
  for(int i=0; i<1000; i++) {
   Callback<void>* cb = makeCallableOnce(&Counter::inc, &c);
   tpn->addTask(cb);
  }

  delete tpn;
  EXPECT_EQ(c.count(), 1000);
}


// Threads adds stops tasks to the same pool 
TEST(StopNormalPool, ThreadInsideSamePool) {
  TaskAdderNormal ta;
  TaskStopNormal ts;
  ThreadPoolNormal* tpn = new ThreadPoolNormal(2);
  Counter c;

  for(int i=0; i<100000; i++) {
   Callback<void>* cb = makeCallableOnce(&Counter::inc, &c);
   tpn->addTask(cb);
  }


  Callback<void>* cb1 = makeCallableOnce(&TaskStopNormal::stopTask, &ts, &tpn);
  tpn->addTask(cb1);
   
  sleep(1);

  for(int i=0; i<10000; i++) {
   Callback<void>* cb = makeCallableOnce(&Counter::inc, &c);
   tpn->addTask(cb);
  }

  delete tpn;
  EXPECT_EQ(c.count(), 100000);
}

// Threads adds stops tasks to the same pool 
TEST(StopFastPool, ThreadInsideSamePool) {
  TaskAdderFast ta;
  TaskStopFast ts;
  ThreadPoolFast* tpn = new ThreadPoolFast(2);
  Counter c;

  for(int i=0; i<1000; i++) {
   Callback<void>* cb = makeCallableOnce(&Counter::inc, &c);
   tpn->addTask(cb);
  }


  Callback<void>* cb1 = makeCallableOnce(&TaskStopFast::stopTask, &ts, &tpn);

  tpn->addTask(cb1);

  sleep(3);

  for(int i=0; i<1000; i++) {
   Callback<void>* cb = makeCallableOnce(&Counter::inc, &c);
   tpn->addTask(cb);
  }

  delete tpn;
  EXPECT_EQ(c.count(), 1000);
}


//
TEST(DestructorNormal, checkPendingTask) {
  ThreadPoolNormal* tpn = new ThreadPoolNormal(10);
  LongTask lt;
  Counter c;
 
  for(int i=0; i<10000; i++) {
   Callback<void>* cb = makeCallableOnce(&Counter::inc, &c);
   tpn->addTask(cb);
  }

  for(int k=0; k<5; k++) {  
   Callback<void>* cb1 = makeCallableOnce(&LongTask::task, &lt, &c);
   tpn->addTask(cb1);
  }

  sleep(1);
  delete tpn; 
  EXPECT_EQ(c.between(0, 10000000), 1);

}

//
TEST(DestructorFast, checkPendingTask) {
  ThreadPoolFast* tpn = new ThreadPoolFast(10);
  LongTask lt;
  Counter c;

  for(int i=0; i<10000; i++) {
   Callback<void>* cb = makeCallableOnce(&Counter::inc, &c);
   tpn->addTask(cb);
  }

  for(int k=0; k<5; k++) {
   Callback<void>* cb1 = makeCallableOnce(&LongTask::task, &lt, &c);
   tpn->addTask(cb1);
  }

  sleep(5);
  tpn->stop();
  delete tpn;
  EXPECT_EQ(c.between(0, 10000000), 1);

}

  
} // unnammed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}
