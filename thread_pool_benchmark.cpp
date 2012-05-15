/* In order to test the benchmark the behaviour of the pool
   was when stop is called finish all the pending tasks so 
   that fast consumer and slow consumer do the same no of tasks */

#include <iostream>
#include <sstream>
#include <vector>

#define NO_OF_CORES 8

#include "callback.hpp"
#include "thread_pool_normal.hpp"
#include "thread_pool_fast.hpp"
#include "timer.hpp"
#include "test_util.hpp"

namespace {

using base::Callback;
using base::makeCallableMany;
using base::makeCallableOnce;
using base::ThreadPoolNormal;
using base::ThreadPoolFast;
using base::Timer;
using test::LongTask;
using test::ShortTask;
using test::Counter;

// Thread Pool increases ; Tasks are smaller in size
template<typename PoolType>
void FastConsumer() {
  std::cout << "Fast Consumer:\t";
  std::cout << std::endl;
    
  int no_of_threads,initial_no_of_threads = 1, factor=1;
  std::vector<double> speedup;
  speedup.push_back(1);

  for(int i=1; i<=NO_OF_CORES; i++) {
    no_of_threads = initial_no_of_threads * factor;
    PoolType* tpn = new PoolType(no_of_threads);
    ShortTask st;
    Counter c;
    Timer t;
    double timeElapsed;
  
    t.start();
    for(int i=0; i<30; i++) {
     Callback<void>* cb1 = makeCallableOnce(&ShortTask::task, &st, &c);
     tpn->addTask(cb1);
    }
    
    sleep(6);
    tpn->stop();
 
    delete tpn;  
    t.end();
    timeElapsed = t.elapsed();
    speedup.push_back(timeElapsed);  
    factor+=1; 
  }

  std::cout << "Speed up as pool size increases" << std::endl;
  for(int i=1; i<(int)speedup.size(); i++) {
    std::cout << "No of threads:" << i << " " << (speedup.at(i)/speedup.at(i-1)) << std::endl;
  }  
}

//Thread pool increases; Tasks are larger in size
template<typename PoolType>
void SlowConsumer() {
  std::cout << "Slow Consumer:\t";
  std::cout << std::endl;
 
  int no_of_threads,initial_no_of_threads = 1, factor=1;
  std::vector<double> speedup;
  speedup.push_back(1);

  for(int i=1; i<=NO_OF_CORES; i++) {
    no_of_threads = initial_no_of_threads * factor;
    PoolType* tpn = new PoolType(no_of_threads);
    LongTask lt;
    Counter c;
    Timer t;
    double timeElapsed;
    t.start(); //Start the timer
   
    for(int i=0; i<30; i++) {
     Callback<void>* cb1 = makeCallableOnce(&LongTask::task, &lt, &c);
     tpn->addTask(cb1);
    }
   
    sleep(15);
    tpn->stop();

    delete tpn;
    t.end();
    timeElapsed = t.elapsed();
      
    speedup.push_back(timeElapsed);
    factor+=1;  //Threads increase by a factor of 1
  }

  std::cout << "Speed up as Pool size increases" << std::endl;
  for(int i=1; i<(int)speedup.size(); i++) {
    std::cout << "No of threads:" << i << " " << (speedup.at(i)/speedup.at(i-1)) << std::endl;
  }
}

}  // unnamed namespace

void usage(int argc, char* argv[]) {
  std::cout << "Usage: " << argv[0] << " [1 | 2]" << std::endl;
  std::cout << "  1 is normal thread pool" << std::endl;
  std::cout << "  2 is fast thread pool" << std::endl;
  std::cout << "  defaul is to run both " << std::endl;
}

int main(int argc, char* argv[]) {
  bool all = false;
  bool num[4] = {false};
  if (argc == 1) {
    all = true;
  } else if (argc == 2) {
    std::istringstream is(argv[1]);
    int i = 0;
    is >> i;
    if (i>0 && i<=4) {
      num[i-1] = true;
    } else {
      usage(argc, argv);
      return -1;
    }
  } else {
    usage(argc, argv);
    return -1;
  }

  // no queueing of tasks really
  if (all || num[0]) {
   FastConsumer<ThreadPoolNormal>();
  }
  if (all || num[1]) {
    FastConsumer<ThreadPoolFast>();
  }

  // force queue building
  if (all || num[0]) {
    SlowConsumer<ThreadPoolNormal>();
  }
  if (all || num[1]) {
    SlowConsumer<ThreadPoolFast>();
  }

  return 0;
}
