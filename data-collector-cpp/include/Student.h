#include <string>

// THIS FILE DOES NOTHING AND SIMPLY SERVES AS AN EXAMPLE FOR USING AN EXTERNAL HEADER FILE IN include/

class Student {
	public:
		Student(std::string name, int age);
		void tellInfo();
	private:
		std::string name;
		int age;
};
