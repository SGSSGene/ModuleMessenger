#include <iostream>

#include "moduleMessenger/moduleMessenger.h"

class TestClass {
public:
	TestClass() {
		std::cout<<"C'Tor"<<std::endl;
	}
	TestClass(TestClass const&) {
		std::cout<<"Copy'Tor"<<std::endl;
	}

};

class CountClass {
public:
	CountClass() {
		std::cout << "CC: C'Tor" << std::endl;
	}
	CountClass(CountClass const&) {
		std::cout << "CC: Copy-C'Tor" << std::endl;
	}
	CountClass(CountClass&&) {
		std::cout << "CC: Move'Tor" << std::endl;
	}
	CountClass& operator=(CountClass const&) {
		std::cout << "CC: assign" << std::endl;
		return *this;
	}
	CountClass& operator=(CountClass&&) {
		std::cout << "CC: move" << std::endl;
		return *this;
	}
	~CountClass() {
		std::cout << "CC: D'tor" << std::endl;
	}


};

class Module {
public:
	moduleMessenger::Registrator               msreg;
	moduleMessenger::TRegistrator<std::string> mstreg;

	Module()
		: mstreg([](std::string const& s) {
			std::cout<<"registered in c'tor"<<std::endl;
		})
	{
		msreg.addListener(this, &Module::callbackString);
		msreg.addListener(this, &Module::callbackInt);
		msreg.addListener(this, &Module::callbackBoth);
		msreg.addListener<std::string>([](std::string const& s) {
			std::cout<<"lambda register"<<std::endl;
		});

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

	moduleMessenger::Registrator reg;
	reg.addListener<CountClass>([](CountClass const&){});

	moduleMessenger::postMessage(CountClass());

	blockForEver.wait(lock);

	return 0;
}
