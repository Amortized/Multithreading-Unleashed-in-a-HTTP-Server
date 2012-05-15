#include "thread_pool_normal.hpp"
#include "thread.hpp"
#include <pthread.h>
#include <signal.h>
#include <iostream>

namespace base {

ThreadPoolNormal::ThreadPoolNormal(int num_workers) 
  : flagstop_(false) {
  for(int i=0; i<num_workers; i++) {
   Callback<void>* cb = makeCallableOnce(&ThreadPoolNormal::ExecuteTask,this);
   threads_.push_back(makeThread(cb));
  }
}	

ThreadPoolNormal::~ThreadPoolNormal() { 
  // Clear tasks 
  queue_lock_.lock();
  while(!tasks_.empty()) {
    Callback<void>* task = tasks_.front();
    tasks_.pop();  
    if(task && task->once()) {
      delete task;
    }
  }
  queue_lock_.unlock(); 
}

void ThreadPoolNormal::ExecuteTask() {
  while(1) { 
    queue_lock_.lock(); 

    while(tasks_.empty() && flagstop_ == false) {        
      queue_empty_.wait(&queue_lock_);     
    }

    /* Either pool was stopped or task was added */

    if(flagstop_ == true) {               //Stop the Pool
      break; 
    } 
    else {                               // Pop from the queue
        Callback<void>* task = tasks_.front();  
        bool task_type = task->once();
        tasks_.pop();
        queue_lock_.unlock();

        (*task)();    //Execute the task

        if(task_type == false) { // Delete the task if of type CallbackMany
          delete task;  
        }
    }
  }  
  queue_lock_.unlock();
}


void ThreadPoolNormal::stop() {
  queue_lock_.lock();
  if(flagstop_ == true) {
    queue_lock_.unlock();
    return;
  }
  flagstop_ = true; 
  queue_empty_.signalAll();
  queue_lock_.unlock();

  //Wait for threads to Die;
  for(int i=0; i<(int)threads_.size(); i++) {
   pthread_join(threads_.at(i), NULL);
  }
  return;
}

void ThreadPoolNormal::addTask(Callback<void>* task) {
  queue_lock_.lock();
  if(flagstop_ == true) {
     queue_lock_.unlock();  
     return;
  } 
  tasks_.push(task);
  queue_empty_.signal();   //Signal to threads waiting for tasks
  queue_lock_.unlock();
}

int ThreadPoolNormal::count() const {
  queue_lock_.lock();
  int size = (int)tasks_.size();
  queue_lock_.unlock(); 
  return size;
}

} // namespace base
