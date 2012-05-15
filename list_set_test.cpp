#include "list_set.hpp"
#include "test_unit.hpp"

namespace {

using base::ListBasedSet;

TEST(Simple, Insertion) {
  ListBasedSet s;
  EXPECT_TRUE(s.insert(99));
}

TEST(Insert, ElementsDoesNotExists) {
  ListBasedSet s;
  EXPECT_TRUE(s.insert(100));
  EXPECT_TRUE(s.insert(-200));
}

TEST(Insert, ElementExists) {
  ListBasedSet s;
  EXPECT_TRUE(s.insert(100));
  EXPECT_FALSE(s.insert(100));  
  EXPECT_TRUE(s.insert(-200));
  EXPECT_FALSE(s.insert(-200));
}

TEST(Remove, ElementExists) {
  ListBasedSet s;
  EXPECT_TRUE(s.insert(100));
  EXPECT_TRUE(s.insert(-50));
  EXPECT_TRUE(s.remove(100));
  EXPECT_FALSE(s.remove(100));
  EXPECT_TRUE(s.remove(-50));
  EXPECT_FALSE(s.remove(-50));
}

TEST(Remove, ElementDoesNotExists) {
  ListBasedSet s;
  EXPECT_FALSE(s.remove(100));
  EXPECT_TRUE(s.insert(100));
  EXPECT_FALSE(s.remove(-50));
}

TEST(Lookup, ElementExists) {
  ListBasedSet s;
  EXPECT_TRUE(s.insert(100));
  EXPECT_TRUE(s.insert(-200));
  EXPECT_TRUE(s.lookup(-200));
  EXPECT_TRUE(s.lookup(100));
  EXPECT_TRUE(s.lookup(-200));
}

TEST(Lookup, ElementDoesNotExist) {
  ListBasedSet s;
  EXPECT_FALSE(s.lookup(-100));
  EXPECT_TRUE(s.insert(100));
  EXPECT_FALSE(s.lookup(-100));
}

TEST(Clear, Emptylist) {
  ListBasedSet s;
  s.clear();
  EXPECT_FALSE(s.remove(100));
  EXPECT_FALSE(s.lookup(200));
} 

TEST(Clear, Remove) {
  ListBasedSet s;
  EXPECT_TRUE(s.insert(100));
  EXPECT_TRUE(s.insert(-200));
  s.clear();
  EXPECT_FALSE(s.remove(100));
  EXPECT_FALSE(s.remove(-200));
}

TEST(Clear, Lookup) {
  ListBasedSet s;
  EXPECT_TRUE(s.insert(100));
  s.clear();
  EXPECT_FALSE(s.lookup(100));
}

TEST(CheckIntegrity, Insert) {
  ListBasedSet s;
  EXPECT_TRUE(s.insert(100));
  EXPECT_TRUE(s.insert(-200));
  EXPECT_TRUE(s.insert(50));
  EXPECT_TRUE(s.checkIntegrity());
}

TEST(CheckIntegrity, Remove) {
  ListBasedSet s;
  EXPECT_TRUE(s.insert(100));
  EXPECT_TRUE(s.insert(200));
  EXPECT_TRUE(s.insert(-50));
  EXPECT_TRUE(s.remove(100));
  EXPECT_TRUE(s.checkIntegrity());
  EXPECT_TRUE(s.remove(-50));
  EXPECT_TRUE(s.checkIntegrity());
}

TEST(CheckIntegrity, Lookup) {
  ListBasedSet s;
  EXPECT_TRUE(s.insert(-100));
  EXPECT_TRUE(s.insert(-200)); 
  EXPECT_TRUE(s.lookup(-100));
  EXPECT_TRUE(s.checkIntegrity());
  EXPECT_FALSE(s.lookup(300));
  EXPECT_TRUE(s.checkIntegrity());
}

TEST(CheckIntegrity, Clear) {
  ListBasedSet s;
  EXPECT_TRUE(s.checkIntegrity());
  EXPECT_TRUE(s.insert(100));
  EXPECT_TRUE(s.insert(-200));
  s.clear();
  EXPECT_TRUE(s.checkIntegrity());
}


void* threads_Insert1(void* list_set);
void* threads_Insert2(void* list_set);
void* threads_Insert3(void* list_set);
void* threads_Insert4(void* list_set);
void* threads_Insert5(void* list_set);

TEST(ThreadSafe, Inserts) {
  ListBasedSet s;
  pthread_t thread1, thread2, thread3, thread4, thread5;

  pthread_create(&thread1, NULL, &threads_Insert1, (void*)&s);
  pthread_create(&thread2, NULL, &threads_Insert2, (void*)&s);
  pthread_create(&thread3, NULL, &threads_Insert3, (void*)&s);
  pthread_create(&thread4, NULL, &threads_Insert4, (void*)&s);
  pthread_create(&thread5, NULL, &threads_Insert5, (void*)&s);

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
  pthread_join(thread3, NULL);
  pthread_join(thread4, NULL);
  pthread_join(thread5, NULL);

  EXPECT_TRUE(s.checkIntegrity());  
}

void* threads_Insert1(void* list_set) { 
  ListBasedSet* s=(ListBasedSet*)list_set;
  for(int i=0; i<10000; i+=2) {
    s->insert(i);	
  }
  return NULL;  
}
	
void* threads_Insert2(void* list_set) {
  ListBasedSet* s=(ListBasedSet*)list_set;
  for(int i=1; i<10000; i+=2) {
    s->insert(i);
  }
  return NULL;
}    

void* threads_Insert3(void* list_set) {
  ListBasedSet* s=(ListBasedSet*)list_set;
  for(int i=3; i<10000; i+=4) {
    s->insert(i);
  }
  return NULL;
}

void* threads_Insert4(void* list_set) {
  ListBasedSet* s=(ListBasedSet*)list_set;
  for(int i=5; i<10000; i+=6) {
    s->insert(i);
  }
  return NULL;
}

void* threads_Insert5(void* list_set) {
  ListBasedSet* s=(ListBasedSet*)list_set;
  for(int i=6; i<10000; i+=7) {
    s->insert(i);
  }
  return NULL;
}


void* threads_Remove1(void* list_set);
void* threads_Remove2(void* list_set);
void* threads_Remove3(void* list_set);
void* threads_Remove4(void* list_set);
void* threads_Remove5(void* list_set);

TEST(ThreadSafe, Removes) {
  ListBasedSet s; 
  pthread_t thread1, thread2, thread3, thread4, thread5;
  
  for(int z=0; z < 10000; z++) {
   EXPECT_TRUE(s.insert(z));
  }
  
  pthread_create(&thread1, NULL, &threads_Remove1, (void*)&s);
  pthread_create(&thread2, NULL, &threads_Remove2, (void*)&s);
  pthread_create(&thread3, NULL, &threads_Remove3, (void*)&s);
  pthread_create(&thread4, NULL, &threads_Remove4, (void*)&s);
  pthread_create(&thread5, NULL, &threads_Remove5, (void*)&s);

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
  pthread_join(thread3, NULL);
  pthread_join(thread4, NULL);
  pthread_join(thread5, NULL);

  EXPECT_TRUE(s.checkIntegrity());
}

void* threads_Remove1(void* list_set) {
  ListBasedSet* s=(ListBasedSet*)list_set;
  for(int i=1; i<10000; i+=2) {
    s->remove(i);
  }
  return NULL;
}

void* threads_Remove2(void* list_set) {
  ListBasedSet* s=(ListBasedSet*)list_set;
  for(int i=2; i<10000; i+=2) {
    s->remove(i);
  }
  return NULL;
}


void* threads_Remove3(void* list_set) {
  ListBasedSet* s=(ListBasedSet*)list_set;
  for(int i=5; i<10000; i+=2) {
    s->remove(i);
  }
  return NULL;
}


void* threads_Remove4(void* list_set) {
  ListBasedSet* s=(ListBasedSet*)list_set;
  for(int i=7; i<10000; i+=3) {
    s->remove(i);
  }
  return NULL;
}


void* threads_Remove5(void* list_set) {
  ListBasedSet* s=(ListBasedSet*)list_set;
  for(int i=9; i<100000; i+=3) {
    s->remove(i);
  }
  return NULL;
}


/* Checking Thread Safety for InsertRemove */

void* threads_Inserts1(void* list_set);
void* threads_Inserts2(void* list_set);
void* threads_Inserts3(void* list_set);
void* threads_Removes1(void* list_set);
void* threads_Removes2(void* list_set);

TEST(ThreadSafe, InsertRemove)
{
  ListBasedSet s;
  pthread_t thread1, thread2, thread3, thread4, thread5;

  for(int i=1; i<=10000; i+=2) {
   EXPECT_TRUE(s.insert(i));   
  }

  pthread_create(&thread1, NULL, &threads_Inserts1, (void*) &s);
  pthread_create(&thread2, NULL, &threads_Inserts2, (void*) &s);
  pthread_create(&thread3, NULL, &threads_Inserts3, (void*) &s);
  pthread_create(&thread4, NULL, &threads_Removes1, (void*) &s);
  pthread_create(&thread5, NULL, &threads_Removes2, (void*) &s);

  /* If not thread Safe,there will be a segmentation fault */

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
  pthread_join(thread3, NULL);
  pthread_join(thread4, NULL);
  pthread_join(thread5, NULL);

  EXPECT_TRUE(s.checkIntegrity());  

}
                                                
void* threads_Inserts1(void* list_set) {
  ListBasedSet* s=(ListBasedSet*)list_set;
  for(int k=2; k<=2000; k+=2) {
  s->insert(k);
  }
  return NULL;
}

void* threads_Inserts2(void* list_set) {
  ListBasedSet* s=(ListBasedSet*)list_set;
  for(int z=2100; z<=7000; z+=2) {
  s->insert(z);
  }
  return NULL;
}

void* threads_Inserts3(void* list_set) {
  ListBasedSet* s=(ListBasedSet*)list_set;
  for(int m=7000; m<=10000; m+=2) {
  s->insert(m);
  }
  return NULL;
}

void* threads_Removes1(void* list_set) {
  ListBasedSet* s=(ListBasedSet*)list_set;
  for(int i=1; i<=5000; i+=2) {
  s->remove(i);
  }
  return NULL;
}

void* threads_Removes2(void* list_set) {
  ListBasedSet* s=(ListBasedSet*)list_set;
  for(int j=5100; j<=10000; j+=2) {
  s->remove(j);
  }
  return NULL;
}
/* Checking ThreadSafety for InsertRemove Ends */                              
} // unnamed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}
