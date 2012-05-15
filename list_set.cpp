#include "list_set.hpp"
#include<stdlib.h>

namespace base {

RWMutex m;

ListBasedSet::ListBasedSet() {
  head = NULL;
}

ListBasedSet::~ListBasedSet() {
  clear();
}

bool ListBasedSet::insert(int value) {
  m.wLock();
  struct node* current = head;
  struct node* previous;
  bool found = false;

  while(current!=NULL) {
    if(current->data > value ) {
     break;
    }
    if(current->data == value) {
     found = true;
     break;
    }
    previous = current;
    current = current->next;
  }

  if(found == true) {
    m.unlock();
    return false;
  }

  if(current == head) {
    struct node* newnode = (struct node*)malloc(sizeof(struct node*));
    newnode->data = value;
    newnode->next = head;
    head =  newnode;
  }else if (current !=NULL) {
     struct node* newnode = (struct node*)malloc(sizeof(struct node*));
     newnode->data = value;
     newnode->next = previous->next;
     previous->next = newnode;
  } else {
     struct node* newnode = (struct node*)malloc(sizeof(struct node*));
     newnode->data = value;
     newnode->next = NULL;
     previous->next = newnode;
  }

  m.unlock();
  return true;  
}

bool ListBasedSet::remove(int value) { 
  m.wLock();
  struct node* current = head;
  struct node* previous;
  bool found = false;
  while(current!=NULL) {
   if(current->data == value) {
    if(current == head) {
      head = NULL;
    } else {
    previous->next = current->next;
    }
    free(current);
    found = true;
    break;
   }
   previous = current;
   current = current->next;
  }

  m.unlock();
  if(found == true) {
   return true;
  } else {
   return false;
  }  
}

bool ListBasedSet::lookup(int value) const {
  m.rLock();
  struct node* current = head;
  while(current!=NULL) {
   if(current->data == value) {
      break;
   }
   current = current->next;
  }

  m.unlock();
  if(current == NULL) {
    return false;
  } else {
    return true;
  }  
}

void ListBasedSet::clear() {
  struct node* current = head;
  struct node* previous = current;
  while(current!=NULL) {
   previous = current;
   current = current ->next;
   free(previous);
  }
  head = NULL;
}

bool ListBasedSet::checkIntegrity() const {
  m.rLock();
  struct node* current = head;
  struct node* previous = current;
  bool sorted=true;

  if((head == NULL) || (head->next == NULL)) {
    m.unlock();
    return sorted;
  } else {
     current = current->next;
     while(current != NULL) {
      if(previous->data >= current->data) {
       sorted = false;
       break;
      }
      previous = current;
      current = current->next;
     }
  }
  m.unlock();
  return sorted;  
}

} // namespace base
