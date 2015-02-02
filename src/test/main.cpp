
#include "messengerSystem/messengerSystem.h"
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
	messengerSystem::Registrator msreg;

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
	messengerSystem::changeThreadCount(1); // Work with 1 thread

	// Lock for ever
	std::mutex mutex;
	std::unique_lock<std::mutex> lock(mutex);
	std::condition_variable blockForEver;

	messengerSystem::postMessage(std::string("Hallo Welt!"));
	messengerSystem::postMessage(10);
	messengerSystem::postMessage(std::make_shared<TestClass>());
	messengerSystem::postMessage(TestClass());

	blockForEver.wait(lock);

	return 0;
}
