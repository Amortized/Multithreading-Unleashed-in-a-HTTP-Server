#include "circular_buffer.hpp"
#include "test_unit.hpp"

namespace {

using base::CircularBuffer;

TEST(Simple, ReadWrite) {
  CircularBuffer b(1);
  b.write(0);
  EXPECT_EQ(0, b.read());
}


TEST(Read, ReadInOrder) {
  CircularBuffer b(2);
  b.write(0);
  b.write(1);
  EXPECT_EQ(0,b.read());
  EXPECT_EQ(1,b.read());
}


TEST(Read, ReadNotAvailaible) {
  CircularBuffer b(2);
  b.write(0);
  b.write(1);
  EXPECT_EQ(0,b.read());
  EXPECT_EQ(1,b.read());
  EXPECT_EQ(-1,b.read()); 
  EXPECT_EQ(-1,b.read()); 
}


TEST(Clear, ReadAfterClear) {
  CircularBuffer b(2);
  b.write(0);
  b.write(1);
  b.clear();
  EXPECT_EQ(-1,b.read());  
}

TEST(Clear, WriteAfterClear) {
  CircularBuffer b(2);
  b.write(0);
  b.write(1);
  b.clear();
  b.write(2);
  EXPECT_EQ(2,b.read());
}

TEST(Write, WriteOnEmptyBuffer) {
  CircularBuffer b(0);
  b.write(1);
  b.write(2);
  EXPECT_EQ(1,b.read());
}

TEST(Write, OverWriteValue_NotReadOut) {
  CircularBuffer b(2);
  b.write(0);
  b.write(1);
  b.write(2);
  EXPECT_EQ(1,b.read());
}

TEST(Write, OverWriteValue_ReadOut) {
  CircularBuffer b(3);
  b.write(0);
  b.write(1);
  b.write(2);
  EXPECT_EQ(0,b.read());
  b.write(3);
  EXPECT_EQ(1,b.read());
  b.write(4);	    
  b.write(5);
  EXPECT_EQ(3,b.read());
}
} // unnamed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}
