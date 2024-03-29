#ifndef MCP_BASE_FILE_CACHE_HEADER
#define MCP_BASE_FILE_CACHE_HEADER

#include <string>
#include <tr1/unordered_map>
#include "buffer.hpp"
#include "lock.hpp"
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <list>

namespace base {

using std::string;
using std::tr1::hash;
using std::tr1::unordered_map;
using std::list;

// The FileCache maintains a map from file names to their contents,
// stored as 'Buffer's. The sum of all Buffer's stored in the map
// never exceeds the size determined for the cache (at construction
// time).
//
// A request to pin a file that is already in the cache is supposed to
// be very fast. We do so by leveraging reader-writer locks.A cache
// hit needs only to grab a read lock on the map -- it doesn't change
// it -- and to increment the pin count for that file -- which can be
// done with an atomic fetch-and-add.
//  
// A cache miss is slower, but we expect them to be less frequent. The
// buffers are connected together in a single-linked FIFO list. That's
// right, for the lab 2 there is no special eviction policy. When
// space is needed, we traverse the list and evict the first
// non-pinned buffer we find. And repeat until enough space was
// cleared.
// 
// If not enough unpinned space is found to fit a new request, then
// the pin request may fail.
//
// Thread safety:
//   + pin() and unpin() can be done from different threads
//   + ~FileCache is NOT thread-safe. The caller has to be sure there
//     are no ongoing cache readers before disposing of the cache
//
// Usage:
//   FileCache my_cache(50<<20 /* 50MB */);
//
//   Buffer* buf;
//   int error;
//   CacheHandle h = my_cache.pin("a_file.html", &buf, &error);
//   if (h != 0) {
//     can read contents of *buf
//   } else if (error == 0) {
//     no space on cache. will need to read the file on your own
//   } else /* error < 0 */ {
//     other error reading the file; error contains errno
//   }
//

class FileCache {
public:
  typedef const string* CacheHandle;

  explicit FileCache(int max_size);
  ~FileCache();

  CacheHandle pin(const string& file_name, Buffer** buf, int* error);
  void unpin(CacheHandle h);

  // accessors

  int maxSize() const;
  int bytesUsed() const;
  int pins() const;
  int hits() const;      
  int failed() const;    

private:
  // Non-copyable, non-assignable
  FileCache(FileCache&);
  FileCache& operator=(FileCache&);

 struct Node {
   const string file_name;
   Buffer*      buf;             // owned here
   int  count_unpin;
   Node(const string& FileName,Buffer* buffer,int count) 
      :file_name(FileName), buf(buffer),count_unpin(count) {
   } 
 }; 
 struct HashStrPtr {
   size_t operator()(const string* a) const {
     hash<string> hasher;
     return hasher(*a);
   }
  };
 struct EqStrPtr {
   bool operator()(const string* a, const string* b) const {
     return *a == *b;
   }
 };
 typedef unordered_map<const string*, Node*, HashStrPtr, EqStrPtr> CacheMap;
 CacheMap cache_map_;

 
 list<string> PinnedFiles_;   //FOR FIFO EVICTION

 int CacheSize_;
 RWMutex CacheLock_; 
 int pins_total_;
 int hits_total_;
 int failed_total_;  
 int bytes_used_;    
 int bytes_unpinned_files_;
};   

} // namespace base

#endif // MCP_BASE_FILE_CACHE_HEADER
