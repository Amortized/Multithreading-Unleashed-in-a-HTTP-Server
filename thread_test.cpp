#include "test_unit.hpp"
#include "thread.hpp" 
#include "test_util.hpp"
#include "callback.hpp"

namespace {

using base::Callback;
using base::makeCallableOnce;
using base::makeCallableMany;
using test::Counter;

//Checking if thread was created and task was performed
TEST(Once, Simple) {  
  Counter c;
  Callback<void>* cb = makeCallableOnce(&Counter::inc, &c);
  pthread_t id = makeThread(cb);
  pthread_join(id, NULL);
  EXPECT_EQ(c.count(), 1);
}

//Binding attributes to callbacks
TEST(Once, Binding) {
  Counter c;
  Callback<void>* cb = makeCallableOnce(&Counter::incBy, &c, 3);
  pthread_t id = makeThread(cb);
  pthread_join(id, NULL);
  EXPECT_EQ(c.count(), 3);

  // reset
  Callback<void>* cb1 = makeCallableOnce(&Counter::reset, &c);
  pthread_t id1= makeThread(cb1);
  pthread_join(id1, NULL);
  EXPECT_EQ(c.count(), 0);

  //Bind Again
  Callback<void>* cb2 = makeCallableOnce(&Counter::incBy, &c, 1);
  pthread_t id2 = makeThread(cb2);
  pthread_join(id2, NULL);
  EXPECT_EQ(c.count(), 1);
}

// Currying Callbacks
TEST(Once, Currying) {
  Counter c;
  Callback<void,int>* cb = makeCallableOnce(&Counter::incBy, &c);
  Callback<void>* cb1 = makeCallableOnce(&Callback<void,int>::operator(), cb, 4);
  pthread_t id = makeThread(cb1);
  pthread_join(id, NULL);
  EXPECT_EQ(c.count(), 4);
}

//Checking if threads were created and tasks were performed
TEST(Many, Simple) {
  Counter c;
  Callback<void>* cb = makeCallableMany(&Counter::inc, &c);
  pthread_t id = makeThread(cb);
  pthread_t id1 = makeThread(cb);
  pthread_t id2 = makeThread(cb);
  pthread_join(id, NULL);
  pthread_join(id1, NULL);
  pthread_join(id2, NULL);
  EXPECT_TRUE(c.between(0, 3));
  delete cb;
}

//Binding attributes to callbacks
TEST(Many, Binding) {
  Counter c;
  Callback<void>* cb = makeCallableMany(&Counter::incBy, &c, 4);
  pthread_t id = makeThread(cb);
  pthread_t id1 = makeThread(cb);
  pthread_t id2 = makeThread(cb);
  pthread_join(id, NULL);
  pthread_join(id1, NULL);
  pthread_join(id2, NULL);
  EXPECT_TRUE(c.between(0, 12));
  delete cb;
}

//Currying callbacks
TEST(Many, Currying) {
  Counter c;
  Callback<void,int>* cb = makeCallableMany(&Counter::incBy, &c);
  Callback<void>* cb1 = makeCallableMany(&Callback<void,int>::operator(), cb, 4);
  pthread_t id  = makeThread(cb1);
  pthread_t id1 = makeThread(cb1);
  pthread_t id2 = makeThread(cb1);
  pthread_join(id,  NULL);
  pthread_join(id1, NULL);
  pthread_join(id2, NULL); 
  EXPECT_TRUE(c.between(0,12));
  delete cb;
  delete cb1;
}

} // unnammed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}
