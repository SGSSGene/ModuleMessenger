#include "moduleMessenger/moduleMessenger.h"
#include <iostream>

class TestClass {
public:
	TestClass() {
		std::cout<<"C'Tor"<<std::endl;
	}
	TestClass(TestClass const&) {
		std::cout<<"Copy'Tor"<<std::endl;
	}

};

class Module {
public:
	moduleMessenger::Registrator msreg;

	Module() {
		msreg.addListener(this, &Module::callbackString);
		msreg.addListener(this, &Module::callbackInt);
		msreg.addListener(this, &Module::callbackBoth);

	}
	void callbackString(std::string const& s) {
		std::cout<<s<<std::endl;
	}

	void callbackInt(int const& i) {
		std::cout<<i<<std::endl;
	}

	void callbackBoth(std::string const& s, int const& i) {
		std::cout<<"received both"<<std::endl;
	}
};
namespace {
	Module m;
}

int main() {
	moduleMessenger::changeThreadCount(1); // Work with 1 thread

	// Lock for ever
	std::mutex mutex;
	std::unique_lock<std::mutex> lock(mutex);
	std::condition_variable blockForEver;

	moduleMessenger::postMessage(std::string("Hallo Welt!"));
	moduleMessenger::postMessage(10);
	moduleMessenger::postMessage(std::make_shared<TestClass>());
	moduleMessenger::postMessage(TestClass());

	blockForEver.wait(lock);

	return 0;
}
