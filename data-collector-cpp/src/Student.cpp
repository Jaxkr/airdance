#include "Student.h"
#include <string>
#include <iostream>


// THIS FILE DOES NOTHING AND SIMPLY SERVES AS AN EXAMPLE FOR USING AN EXTERNAL HEADER FILE IN include/

Student::Student(std::string _name, int _age) {
		name = _name;
		age = _age;
	}

void Student::tellInfo() {
	std::cout << name << std::endl;
	std::cout << age << std::endl;
}
