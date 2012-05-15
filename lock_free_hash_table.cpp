#include "lock_free_hash_table.hpp"
#include "cmath"
#include <sstream>
#include <string>
#include <algorithm>
       
namespace lock_free_hash_table {

LockFreeHashTable::LockFreeHashTable(int num_threads) {
  lockfreelist_ = new LockFreeList<uint32_t>(num_threads);
  count_ = 0;
  size_ = 2;
  loadFactor_ = 10;
  // Creating the Initial Buckets
  for(int i=0; i<(int)size_; i++) {
    buckets_.push_back(NULL);
  }
  // Insert the Dummy Node '0'. First Bucket gets initialized
  insertKey(0,0); 
}    

LockFreeHashTable::~LockFreeHashTable() {
  buckets_.clear();
  delete lockfreelist_;
}  

bool LockFreeHashTable::insertKey(const uint32_t& data, int value=0) {
  uint32_t bucket = data % size_;
  if(buckets_.at(bucket) == NULL)  
    initialize_bucket(bucket);
  if(lockfreelist_->insert(reverseRegularKey(data),&value,(buckets_.at(bucket)))==false) 
    return false; 
  uint32_t csize = size_;
  __sync_fetch_and_add(&count_, 1);
  
  if ((count_ / csize) > (uint32_t)loadFactor_) {
    for(int i=0; i<(int)size_; i++) {
      buckets_.push_back(NULL);
    }
    __sync_bool_compare_and_swap(&size_, csize, 2 * csize);
  }
  return true;
} 

bool LockFreeHashTable::removeKey(const uint32_t& data) {
  uint32_t bucket = data % size_;
  if(buckets_.at(bucket) == NULL) 
    initialize_bucket(bucket);
  if(!lockfreelist_->remove(reverseRegularKey(data),(buckets_.at(bucket)))) 
    return false;	
  __sync_fetch_and_sub(&count_, 1);
  return true;	
}

bool LockFreeHashTable::findKey(const uint32_t& data,int* value=NULL) {
  uint32_t bucket = data % size_;	
  if(buckets_.at(bucket) == NULL) 
    initialize_bucket(bucket);
  return (lockfreelist_->lookup(reverseRegularKey(data),value,(buckets_.at(bucket))));
}

void LockFreeHashTable::initialize_bucket(uint32_t bucketNo) {
  uint32_t parent;
  if(bucketNo!=0) { 
    parent = getParent(bucketNo);
    if(buckets_.at(parent) == NULL)   
      initialize_bucket(parent);
  }
  else { //Parent of 0 is 0
    parent = 0;
  }
  LockFreeList<unsigned int>::Node* dummy = new LockFreeList<unsigned int>::Node;
  lockfreelist_->insert(reverseDummyKey(bucketNo),0,(buckets_.at(parent)), &dummy);
  buckets_.at(bucketNo) = dummy;
}  

uint32_t LockFreeHashTable::getParent(uint32_t bucketNo) {
  int pos = __builtin_clz(bucketNo); //Get the no of leading zeros
  int parent =  bucketNo ^ 1 << (8*sizeof(bucketNo) - 1 - pos);  //Unset the turned on MSB bit using pos
  return parent;
}  

uint32_t LockFreeHashTable::reverse(register uint32_t x)
{
  x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
  x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
  x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
  x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
  return((x >> 16) | (x << 16));
}

uint32_t LockFreeHashTable::reverseRegularKey(uint32_t key) {
  uint32_t keyWithMSBSet = (key|=1 << 31); //Setting the MSB to 1
  return (reverse(keyWithMSBSet));
}

uint32_t LockFreeHashTable::reverseDummyKey(uint32_t key) {
  return (reverse(key));
}

bool LockFreeHashTable::checkIntegrity() const {
  return lockfreelist_->checkIntegrity();
}

uint32_t LockFreeHashTable::getSize() const {
  return size_;
}
     
uint32_t LockFreeHashTable::getCount() const {
  return count_;
}

} //namespace
