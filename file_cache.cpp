#include "file_cache.hpp"
#include "logging.hpp"


namespace base {
  
FileCache::FileCache(int max_size){
  CacheSize_ = max_size;
  pins_total_ = 0;
  failed_total_ = 0;
  hits_total_ = 0;
  bytes_used_ = 0;                 
  bytes_unpinned_files_ = 0;
}                                                  

// REQUIRES: No ongoing pin. This code assumes no one is using the
// cache anymore
FileCache::~FileCache() {
  CacheMap::iterator it;
  for(it = cache_map_.begin(); it != cache_map_.end() ;it++) {
    Node* node = it->second;
    delete node->buf;
  }
  PinnedFiles_.clear();
} 

FileCache::CacheHandle FileCache::pin(const string& file_name,
                                      Buffer** buf,
                                      int* error) {
  CacheHandle h = 0;
  CacheLock_.rLock();
  CacheMap::iterator it = cache_map_.find(&file_name);
  if (it != cache_map_.end()) {         //File Pinned Before
    h = it->first;
    Node* node = it->second;
    *buf = node->buf;
    CacheLock_.unlock();
    __sync_fetch_and_add(&node->count_unpin, 1);
    __sync_fetch_and_add(&hits_total_, 1);
  } 
  else {                             //File Not Pinned Before
    CacheLock_.unlock();
    int fd = ::open(file_name.c_str(), O_RDONLY);
    if (fd < 0) {
      LOG(LogMessage::WARNING) << "could not open " << file_name << ": " << strerror(errno);
      error = &errno;   //Unable to open the file
    }
    else {
      struct stat stat_buf;
      fstat(fd, &stat_buf);  
      Buffer* new_buf = new Buffer;
      size_t to_read = stat_buf.st_size;
      bool error_Read = false; 
      while (to_read > 0) {
        new_buf->reserve(Buffer::BlockSize);
        int bytes_read = read(fd, new_buf->writePtr(), new_buf->writeSize());
        if (bytes_read < 0 ) {
          LOG(LogMessage::ERROR) << "cant read file " << file_name << ": " << strerror(errno);
          close(fd);
          delete new_buf;
          error = &errno; 
          error_Read = true;
          break;
        }
          to_read -= bytes_read;
    }    

    if(error_Read) { //File couldn't be read;
      return h;    
    }

    if (to_read != 0) {
      LOG(LogMessage::WARNING) << "file change while reading " << file_name;
    }  
    new_buf->advance(stat_buf.st_size - to_read);
    //File has been successfully read

    CacheLock_.wLock();
    int free_space = maxSize() - bytesUsed();
    if( free_space >= stat_buf.st_size) { //If Space Exists 
      Node* new_node = new Node(file_name, new_buf,1);
      *buf = new_buf;
      std::pair<CacheMap::iterator, bool> it;
      it = cache_map_.insert(make_pair(&new_node->file_name, new_node));
      if (it.second) {
        h = reinterpret_cast<CacheHandle>(&new_node->file_name);
        bytes_used_+= new_buf->byteCount();
        PinnedFiles_.push_back(new_node->file_name);
        CacheLock_.unlock();
      } 
      else { 
        delete new_buf;
        CacheLock_.unlock();
        __sync_fetch_and_add(&hits_total_, 1);          
      }
    }   
    else {  //Try to free space
      if(free_space + bytes_unpinned_files_ < stat_buf.st_size) {  //Check if request can be satisfied by removing unpinned files and freespace
        error = 0;   //No space on cache         
        *buf = 0;
        CacheLock_.unlock();
        __sync_fetch_and_add(&failed_total_, 1);
      } 
      else {
        list<string>::iterator list_iterator;   
        for(list_iterator = PinnedFiles_.begin(); list_iterator!=PinnedFiles_.end(); list_iterator++) {        
          string filename = *list_iterator;
          CacheMap::iterator it = cache_map_.find(&filename);
          Node* existing_node = it->second;  
          if(existing_node->count_unpin == 0) {  //Evict the File only if unpinned count is zero
            int space_freed = existing_node->buf->byteCount();
            bytes_used_-= space_freed;
            cache_map_.erase(&existing_node->file_name);
            free_space = maxSize() - bytes_used_;
            PinnedFiles_.erase(list_iterator);
          } 
          if(free_space >= stat_buf.st_size) { //If Enough space has been freed
            break;
          }     
        } 
  
        if(free_space >= stat_buf.st_size) {
          Node* new_node = new Node(file_name, new_buf, 1);
          *buf = new_node->buf;
          std::pair<CacheMap::iterator, bool> it;
          it = cache_map_.insert(make_pair(&new_node->file_name, new_node));
          if (it.second) {
            h = reinterpret_cast<CacheHandle>(&new_node->file_name);
            PinnedFiles_.push_back(new_node->file_name);  //Unpinned Files to be used for eviction
            bytes_used_+= new_buf->byteCount();
            CacheLock_.unlock();
          } 
          else {  //File Already in the cache
            delete new_buf;
            CacheLock_.unlock();
             __sync_fetch_and_add(&hits_total_, 1);
          }
        } 
        else {  //Still no space
          error = 0;         
          *buf = 0;
          CacheLock_.unlock();
           __sync_fetch_and_add(&failed_total_, 1); 
        }
 
     }
    } 
   }
  }
  __sync_fetch_and_add(&pins_total_, 1);
  return h;
}

void FileCache::unpin(CacheHandle h) {
  CacheLock_.wLock();    
  if(h!=0) {    
    string temp = *h;
    const string& ref = temp;
    CacheMap::iterator it = cache_map_.find(&ref);
    if (it != cache_map_.end()) { 
      Node* node = it->second;
      if(node->count_unpin > 0) {  //If File has been unpinned before 
        node->count_unpin--;
        bytes_unpinned_files_+=node->buf->byteCount();
      } 
      else {  
        LOG(LogMessage::FATAL) << "File:" << temp << "has been unpinned before:";   
      }
    } 
    else {
      LOG(LogMessage::FATAL) << "File:" << temp << "has been evicted";
    }
    CacheLock_.unlock();  
  }
}   

int FileCache::maxSize() const {
  return CacheSize_;
}

int FileCache::bytesUsed() const { 
  return bytes_used_;
} 

int FileCache::pins() const {
  return pins_total_;
}                                           

int FileCache::hits() const {                                    
  return hits_total_;
} 

int FileCache::failed() const {
  return failed_total_;
}                         

}  // namespace base
