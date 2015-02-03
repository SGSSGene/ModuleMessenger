# ModuleMessenger

This library makes it very simple sending messages between classes.
Any datatype can be send. It will only be copied once.
Messege will be placed in a queue, and processed in the order the are inserted.
Multiple threads will process the queue (default 1)

## Dependency
	This Library depence on sgssgene/threadpool

## Example
### How to send messages:
```c
	moduleMessenger::postMessage(std::string("Hello World!"));
	moduleMessenger::postMessage(10);
```

### How to register a for a message
```c
class Module {
private:
	moduleMessenger::Registrator msreg;
public:
	Module() {
		msreg.addListener(this, &Module::callbackString);
		msreg.addListener(this, &Module::callbackInt);
		msreg.addListener(this, &Module::callbackBoth);
	}

	void callbackString(std::string const& s) {
		std::cout<<"Received a string: "<<s<<std::endl;
	}
	void callbackInt(int const& i) {
		std::cout<<"Received an int: "<<i<<std::endl;
	}
	void callbackBoth(std::string const& s, int const& i) {
		std::cout<<"Received a string and an int: "<<s<<" "<<i<<std::endl;
	}
};
```

### How to change thread count
```c
	moduleMessenger::changeThreadCount(10);
```

