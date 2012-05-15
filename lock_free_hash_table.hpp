#ifndef MCP_LOCK_FREE_HASH_TABLE_HEADER
#define MCP_LOCK_FREE_HASH_TABLE_HEADER

#include "lock_free_list.hpp"
#include <vector>

namespace lock_free_hash_table {

using lock_free::LockFreeList;
using base::ThreadLocal;

class LockFreeHashTable {
public:

  // ThreadPool's worker threads would comply.
  explicit LockFreeHashTable(int num_threads);

  // The destructor is not thread-safe
  ~LockFreeHashTable();
         
  // Returns true if 'data' does not yet exist on the set and inserts
  // it. Otherwise returns false.
  bool insertKey(const unsigned int& data, int value);

  // Returns true if 'data' exists on the set and remove it. Otherwise
  // returns false.
  bool removeKey(const unsigned int& data);

  // Returns true if 'data' exists on the set.
  bool findKey(const unsigned int& data, int* value);

  //Wrapper for accessing checkIntegrity() of lockfreelist_
  bool checkIntegrity() const;

  //Accessor for size_
  uint32_t getSize() const;
  
  //Accessor for count_
  uint32_t getCount() const;
private:

  LockFreeList<unsigned int>* lockfreelist_;  //Owned here
  
  std::vector<LockFreeList<unsigned int>::Node*> buckets_; //Buckets
  uint32_t count_;  //   Total Item Count
  uint32_t size_;   //	Current Table Size
  int loadFactor_;

  // Helper Functions

  // Initialize the bucket and insert the appropriate dummy key
  void initialize_bucket(uint32_t bucketNo);

  // Unsets the first turned on MSB
  uint32_t getParent(uint32_t bucketNo);

  // Sets the MSB before reversing the bits	
  uint32_t reverseRegularKey(uint32_t);  

  // Does not set the MSB ; reverses the bits 	
  uint32_t reverseDummyKey(uint32_t);		

  // Reverses the bits
  uint32_t reverse(register uint32_t);
  
  // Non-copyable, non-assignable.
  LockFreeHashTable(LockFreeHashTable&);
  LockFreeHashTable& operator=(const LockFreeHashTable&);
};






} // namespace lock_free_hash_table

#endif  // MCP_LOCK_FREE_HASH_TABLE_HEADER
