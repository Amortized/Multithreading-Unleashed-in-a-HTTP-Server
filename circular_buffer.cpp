#include "circular_buffer.hpp"


namespace base {

CircularBuffer::CircularBuffer(int slots) {
  read_position = 0;
  write_position = 0;
  length = slots;
 
  if(slots == 0) {
    buffer = new circularBuffer[10];
    length = 10;
  }
  else {
    buffer = new circularBuffer[length];
  }

  for(int i=0;i<length;i++) {
    buffer[i].read = true;
  }
}   

CircularBuffer::~CircularBuffer() {
  delete []buffer; 
}

void CircularBuffer::write(int value) {
  if(buffer[write_position].read == false) {
    read_position++;
    if(read_position == length) {
    read_position = 0;
    }
  }

  buffer[write_position].value = value;
  buffer[write_position].read = false;
  write_position++;

  if(write_position == length) {
    write_position = 0;
  }
}

int CircularBuffer::read() {
  if(buffer[read_position].read == true) {
    return -1;
  }
  else {
    int temp = buffer[read_position].value;
    buffer[read_position].read = true;
    read_position++;
    if(read_position == length) {
     read_position = 0;
    }
    return temp;
  }
}

void CircularBuffer::clear() {
  for(int i=0;i<length;i++) {
   buffer[i].read = true;
  } 
  write_position = 0;
  read_position = 0;
}

}  // namespace base
