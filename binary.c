#include <iostream>

using namespace std;

void binary(int);

int main() {
	int number;

	cout << "Please enter a positive integer: ";
	cin >> number;
	if (number < 0) 
		cout << "That is not a positive integer.\n";
	else {
		cout << number << " converted to binary is: ";
		binary(number);
		cout << endl;
	}
}

void binary(int number) {
	int remainder;
	cout << "Number:" << number << endl;
	if(number <= 1) {
		cout << number;
		return;
	}

	remainder = number%2;
	binary(number >> 1);    
	cout << remainder;
}

