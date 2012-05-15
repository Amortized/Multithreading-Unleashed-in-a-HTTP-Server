#include <iostream>
#include <list>

using namespace std;

int main() {
  list<int> mylist;

  mylist.push_back(100);
  mylist.push_back(200);
  mylist.push_back(300);


  cout << mylist.front() << endl;
  mylist.pop_front(); 
  cout << mylist.front() << endl;
 
  return 0;
}

