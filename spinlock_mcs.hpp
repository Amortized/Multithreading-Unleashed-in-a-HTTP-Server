
/***

spinlock_mcs_test.cpp --  Tests the Correctness of Spinlocks MCS
lock_benchmark_MCS.cpp -- Benchmark between BackOff Spin Locks and MCS Locks 

***/


#include <ctime>
#include <iostream>

namespace base {

// A spinning lock with backoff.
//
// This class predates C++11 atomics (and can be compiled in early
// versions). We use GCC built-in atomics instead. The lock state is
// represented by a boolean and all accesses to that state are
// explicit in terms of atomicity and memory ordering expected.
//

struct Node {
    struct Node* next;
    volatile bool locked;
};

class SpinlockMCS {
public:
   SpinlockMCS() : tail_(NULL) {  }

  ~SpinlockMCS() {  }

  void lock(struct Node* my_node) {
    struct Node* predecessor;
    my_node->next = NULL;
    predecessor = __sync_val_compare_and_swap(&tail_,tail_,my_node);
    if (predecessor != NULL) {
	my_node->locked = true;
	predecessor->next = my_node; 
	while(my_node->locked) {  }
    } 
  } 

  void unlock(struct Node* my_node) {
    if(my_node->next == NULL) {
        std::cout << "\nExecuting.."<<std::endl;
	if(__sync_bool_compare_and_swap(&tail_,my_node,NULL)) {
	  return;
	}
	while(my_node->next == NULL) { }
    }
    my_node->next->locked = false;	
    std::cout << "\nExecuting.." << std::endl;
  }	

private:
  struct Node* tail_;

  // Non-copyable, non-assignable
  SpinlockMCS(SpinlockMCS&);
  SpinlockMCS& operator=(SpinlockMCS&);
};

}  // namespace base

