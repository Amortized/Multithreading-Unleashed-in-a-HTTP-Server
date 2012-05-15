#include <fcntl.h>
#include <errno.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>
#include <string>
#include <tr1/random>
#include "buffer.hpp"
#include "callback.hpp"
#include "file_cache.hpp"
#include "logging.hpp"
#include "thread.hpp"
#include "test_unit.hpp"

namespace {

using std::string;
using std::vector;
using base::Buffer;
using base::Callback;
using base::FileCache;
using base::LogMessage;
using base::makeCallableOnce;
using base::makeThread;
//
// Support for creating test files
//

class FileFixture {
public:
  FileFixture(const char** files, size_t files_size)
    : files_(files), files_size_(files_size) {  }
  ~FileFixture() { }

  void startUp();
  void tearDown();

private:
  const char** files_;
  size_t files_size_;

  vector<int> fds_;
  vector<string> names_;

  void createFile(const char* file, size_t size, char c);
  void deleteFiles();
};               

void FileFixture::startUp() {
  for (size_t i = 0; i < files_size_; i += 3) {
    size_t size = atoi(files_[i+1]);
    createFile(files_[i], size, *files_[i+2]);
  }
}

void FileFixture::tearDown() {
  for (size_t i = 0; i < fds_.size(); i++) {
    close(fds_[i]);
    unlink(names_[i].c_str());
  }
}

void FileFixture::createFile(const char* name, size_t size, char c) {
  unlink(name);
  int fd = creat(name, S_IRUSR | S_IRGRP);
  if (fd < 0) {
    LOG(LogMessage::ERROR) << "can't create file " << string(name)
                           << ": " << strerror(errno);
    close(fd);
    unlink(name);
    exit(1);
  } 

  string content(size, c);
  int bytes = ::write(fd, content.c_str(), size);
  if (bytes < 0) {
    close(fd);
    unlink(name);
    LOG(LogMessage::ERROR) << "can't write to " << string(name)
                           << ": " << *strerror(errno);
    exit(1);
  }
  fds_.push_back(fd);
  names_.push_back(name);
}

//
// Test cases
//
// Sequential: Files aren't pinned if there is no space
TEST(CacheSizeSequential, NoSpace) {
  FileCache my_cache((5 * 10<<10) /10);  //5kb cache size
  Buffer* buf;
  int error;
  my_cache.pin("a.html", &buf, &error);
  EXPECT_EQ(my_cache.failed(), 0);
  EXPECT_EQ(my_cache.bytesUsed(), 2500);
  
  my_cache.pin("b.html", &buf, &error);
  EXPECT_EQ(my_cache.failed(), 0); 
  EXPECT_EQ(my_cache.bytesUsed(), 5000);

  for(int i=0; i<5; i++) {
    my_cache.pin("c.html", &buf, &error);
    EXPECT_EQ(my_cache.failed(), i*2 + 1);  //Pin request fails 
    EXPECT_EQ(my_cache.bytesUsed(), 5000);
    my_cache.pin("2.html", &buf, &error);
    EXPECT_EQ(my_cache.failed(), i*2 + 2);  //Pin request fails 
    EXPECT_EQ(my_cache.bytesUsed(), 5000);
  } 
}  

void* threads_Pin1(void* cache);
void* threads_Pin2(void* cache);
void* threads_Pin3(void* cache);
void* threads_Pin4(void* cache);

// Concurrent: Files aren't pinned if there is no space
TEST(CacheSizeConcurrent, NoSpace) {
  FileCache my_cache((5 * 10<<10) /10);  //5kb cache size
  pthread_t thread1, thread2, thread3, thread4;
  Buffer* buf;
  int error;
  my_cache.pin("a.html", &buf, &error);
  EXPECT_EQ(my_cache.failed(), 0);
  EXPECT_EQ(my_cache.bytesUsed(), 2500);
  my_cache.pin("b.html", &buf, &error);
  EXPECT_EQ(my_cache.failed(), 0);
  EXPECT_EQ(my_cache.bytesUsed(), 5000);

  pthread_create(&thread1, NULL, &threads_Pin1, (void*)&my_cache);
  pthread_create(&thread2, NULL, &threads_Pin2, (void*)&my_cache);
  pthread_create(&thread3, NULL, &threads_Pin3, (void*)&my_cache);
  pthread_create(&thread4, NULL, &threads_Pin4, (void*)&my_cache);

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
  pthread_join(thread3, NULL);
  pthread_join(thread4, NULL);

  EXPECT_EQ(my_cache.bytesUsed(), 5000);
  EXPECT_EQ(my_cache.failed(), 40);
} 

void* threads_Pin1(void* cache) {
  FileCache* s=(FileCache*)cache;
  for(int i=0; i<10; i++) {
    Buffer* buf;
    int error;
    s->pin("c.html", &buf, &error);
  }
  return NULL;
}

void* threads_Pin2(void* cache) {
  FileCache* s=(FileCache*)cache;
  for(int i=0; i<10; i++) {
     Buffer* buf;
     int error;
     s->pin("1.html", &buf, &error);
  }
  return NULL;
}

void* threads_Pin3(void* cache) {
  FileCache* s=(FileCache*)cache;
  for(int i=0; i<10; i++) {
     Buffer* buf;
     int error;
     s->pin("2.html", &buf, &error);
  }
  return NULL;
}

void* threads_Pin4(void* cache) {
  FileCache* s=(FileCache*)cache;
  for(int i=0; i<10; i++) {
     Buffer* buf;
     int error;
     s->pin("3.html", &buf, &error);
  }
  return NULL;
}

//Sequential : Space is freed after unpinning
TEST(CacheSizeSequential, PinAndUnpin) {
  FileCache my_cache((8 * 10<<10)/10);  //10kb
  Buffer *buf1,*buf2,*buf3;
  int error;
  typedef const string* CacheHandle;
  CacheHandle h = my_cache.pin("a.html", &buf1, &error);
  EXPECT_EQ(my_cache.failed(), 0);
  my_cache.pin("b.html", &buf2, &error);
  EXPECT_EQ(my_cache.failed(), 0);
  my_cache.pin("1.html", &buf3, &error);
  EXPECT_EQ(my_cache.failed(), 0);
  EXPECT_EQ(my_cache.bytesUsed(), 7048);

  my_cache.pin("2.html", &buf1, &error);
  EXPECT_EQ(my_cache.failed(), 1);  //Pin request fails 
  EXPECT_EQ(my_cache.bytesUsed(), 7048);
  my_cache.unpin(h);
  my_cache.pin("2.html", &buf1, &error);
  EXPECT_EQ(my_cache.failed(), 1);
  EXPECT_EQ(my_cache.bytesUsed(), 6596);
}


//Concurrent : Unpinning by threads in parallel which allows pinning;
typedef const string* CacheHandle;
struct Thread1{
  CacheHandle H1;
  CacheHandle H2;
  FileCache* cache;
}ThreadStruct1;

struct Thread2{
  CacheHandle H1;
  CacheHandle H2;
  FileCache* cache;
}ThreadStruct2;

struct Thread3{
  CacheHandle H1;
  CacheHandle H2;
  FileCache* cache;
}ThreadStruct3;


void* threads_UnPin1(void* threadstate);
void* threads_UnPin2(void* threadstate);
void* threads_UnPin3(void* threadstate);

TEST(CacheSizeConcurrent, PinAndUnpin) {
  FileCache my_cache((10 * 10<<10)/10);  //17kb
  Buffer  *buf1,*buf2,*buf3,*buf4;
  int error;
  typedef const string* CacheHandle;
  pthread_t thread1, thread2, thread3;

  CacheHandle h0 = my_cache.pin("a.html", &buf1, &error);
  EXPECT_EQ(my_cache.failed(), 0);
  CacheHandle h1 = my_cache.pin("b.html", &buf2, &error);
  EXPECT_EQ(my_cache.failed(), 0);
  CacheHandle h2 = my_cache.pin("2.html", &buf3, &error);
  EXPECT_EQ(my_cache.failed(), 0);
  CacheHandle h3 = my_cache.pin("3.html", &buf4, &error);
  EXPECT_EQ(my_cache.failed(), 0);

  EXPECT_EQ(my_cache.bytesUsed(), 9096);

  my_cache.pin("1.html", &buf1, &error);
  EXPECT_EQ(my_cache.failed(), 1);  //Pin request fails 
  my_cache.pin("4.html", &buf2, &error);
  EXPECT_EQ(my_cache.failed(), 2); //Pin request fails 
  my_cache.pin("5.html", &buf3, &error);
  EXPECT_EQ(my_cache.failed(), 3); //Pin request fails 
  
  EXPECT_EQ(my_cache.bytesUsed(), 9096);


  ThreadStruct1.H1 = h0;
  ThreadStruct1.H2 = h1;
  ThreadStruct1.cache = &my_cache;

  ThreadStruct2.H1 = h2;
  ThreadStruct2.cache = &my_cache;

  ThreadStruct3.H1 = h3;
  ThreadStruct3.cache = &my_cache;

  pthread_create(&thread1, NULL, &threads_UnPin1, (void*)&ThreadStruct1);
  pthread_create(&thread2, NULL, &threads_UnPin2, (void*)&ThreadStruct2);
  pthread_create(&thread3, NULL, &threads_UnPin3, (void*)&ThreadStruct3);

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
  pthread_join(thread3, NULL);


  my_cache.pin("1.html", &buf1, &error);
  my_cache.pin("4.html", &buf2, &error);
  my_cache.pin("5.html", &buf3, &error);
  my_cache.pin("2.html", &buf4, &error);

  EXPECT_EQ(my_cache.bytesUsed(), 8192);
}

void* threads_UnPin1(void* threadstate) {
  Thread1* s = (Thread1*)threadstate;
  (s->cache)->unpin(s->H1);
  (s->cache)->unpin(s->H2);
  return NULL;
}

void* threads_UnPin2(void* threadstate) {
  Thread2* s = (Thread2*)threadstate;
  (s->cache)->unpin(s->H1);
  return NULL;
}

void* threads_UnPin3(void* threadstate) {
  Thread3* s = (Thread3*)threadstate;
  (s->cache)->unpin(s->H1);
  return NULL;
}


//Sequential: Checking whether pin file is evicted;
TEST(PinningSequential, PinnedFileNotEvicted) {
  FileCache my_cache((5 * 10<<10)/10);  //5Kb
  Buffer *buf1,*buf2;
  int error;
  my_cache.pin("a.html", &buf1, &error);
  EXPECT_EQ(my_cache.failed(), 0);
  my_cache.pin("b.html", &buf2, &error);
  EXPECT_EQ(my_cache.failed(), 0);
  
  for(int i=0; i<10; i++) {
    my_cache.pin("a.html", &buf1, &error);
    my_cache.pin("b.html", &buf2, &error);
  }
  EXPECT_EQ(my_cache.bytesUsed(), 5000);
  EXPECT_EQ(my_cache.hits(), 20);  
  EXPECT_EQ(my_cache.pins(), 22);
}

void* threads_Pinning1(void* cache);
void* threads_Pinning2(void* cache);
void* threads_Pinning3(void* cache);
void* threads_Pinning4(void* cache);

//Concurrent: Multiple threads hit same/different files
TEST(PinningConcurrent, PinnedFileNotEvicted) {
  FileCache my_cache((5 * 10<<10)/10);  //11kb
  Buffer* buf;
  int error;
  pthread_t thread1, thread2, thread3, thread4;
  my_cache.pin("a.html", &buf, &error);
  EXPECT_EQ(my_cache.failed(), 0);
  my_cache.pin("b.html", &buf, &error);
  EXPECT_EQ(my_cache.failed(), 0);

  pthread_create(&thread1, NULL, &threads_Pinning1, (void*)&my_cache);
  pthread_create(&thread2, NULL, &threads_Pinning2, (void*)&my_cache);
  pthread_create(&thread3, NULL, &threads_Pinning3, (void*)&my_cache);
  pthread_create(&thread4, NULL, &threads_Pinning4, (void*)&my_cache);

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
  pthread_join(thread3, NULL);
  pthread_join(thread4, NULL);

  EXPECT_EQ(my_cache.bytesUsed(), 5000);
  EXPECT_EQ(my_cache.hits(), 40);
  EXPECT_EQ(my_cache.pins(), 42);
}

void* threads_Pinning1(void* cache) {
  FileCache* s=(FileCache*)cache;
  for(int i=0; i<10; i++) {
    Buffer* buf;
    int error;
    s->pin("a.html", &buf, &error);
  }
  return NULL;
}

void* threads_Pinning2(void* cache) {
  FileCache* s=(FileCache*)cache;
  for(int i=0; i<10; i++) {
    Buffer* buf;
    int error;
    s->pin("a.html", &buf, &error);
  }
  return NULL;
}

void* threads_Pinning3(void* cache) {
  FileCache* s=(FileCache*)cache;
  for(int i=0; i<10; i++) {
    Buffer* buf;
    int error;
    s->pin("b.html", &buf, &error);
  }
  return NULL;
}

void* threads_Pinning4(void* cache) {
  FileCache* s=(FileCache*)cache;
  for(int i=0; i<10; i++) {
    Buffer* buf;
    int error;
    s->pin("b.html", &buf, &error);
  }
  return NULL;
}

}  // unnamed namespace

int main(int argc, char *argv[]) {
  // Creates tests throught the fixtures on the local directory.  The
  // test cases can refer to these files.  The fixture will clean up
  // at when it is tore town.
  const char* files[] =  { "a.html", "2500", "x",
                           "b.html", "2500", "y",
                           "c.html", "5000", "z",
                           "1.html", "2048", "1",
                           "2.html", "2048", "2",
                           "3.html", "2048", "3",
                           "4.html", "2048", "4",
                           "5.html", "2048", "5" };
  size_t size = sizeof(files)/sizeof(files[0]);
  FileFixture ff(files, size);
  ff.startUp();
  int res = RUN_TESTS(argc, argv);
  ff.tearDown();
  return res;
}

