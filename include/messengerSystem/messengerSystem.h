#ifndef MESSENGERSYSTEM_MESSENGERSYSTEM_H
#define MESSENGERSYSTEM_MESSENGERSYSTEM_H


#include <array>
#include <functional>
#include <memory>
#include <map>
#include <tuple>
#include <vector>

#include <threadPool/threadPool.h>


namespace messengerSystem {


/**
 * Holds all callbacks that listen to a certain DataType
 */
template<typename P>
class Listener {
private:
	std::mutex mutex;
	std::map<int, std::function<void(P const&)>> l;
	std::map<int, std::function<void(std::shared_ptr<P>&)>> lSync;

	int ct;
	int ctSync;
private:
	Listener()
		: ct(0)
		, ctSync(0)
	{}
	Listener(Listener const&)            = delete;
	Listener& operator=(Listener const&) = delete;

public:

	static Listener<P>& getInstance() {
		static Listener<P> t;
		return t;
	}
	int add(std::function<void(P const&)> _func) {
		std::unique_lock<std::mutex> lock(mutex);

		l[ct] = _func;
		return ct++;
	}
	int addSync(std::function<void(std::shared_ptr<P>&)> _func) {
		std::unique_lock<std::mutex> lock(mutex);

		lSync[ctSync] = _func;
		return ctSync++;
	}

	void remove(int id) {
		std::unique_lock<std::mutex> lock(mutex);
		l.erase(l.find(id));
	}
	void removeSync(int id) {
		std::unique_lock<std::mutex> lock(mutex);
		lSync.erase(lSync.find(id));
	}

	void call(std::shared_ptr<P> p) {
		std::unique_lock<std::mutex> lock(mutex);

		for (auto const& f : l) {
			f.second(*p);
		}
		for (auto const& f : lSync) {
			f.second(p);
		}
	}

};

/** Main loop
 * it registers generic Listener
 */
class ModuleMessenger {
private:
	using ThreadPool = threadPool::ThreadPool<std::function<void()>>;
	std::unique_ptr<ThreadPool> mThreadPool;
	std::mutex mutex;

	ModuleMessenger()
		: mThreadPool(new ThreadPool())
	{
		mThreadPool->spawnThread([](std::function<void()> _f) {
			_f();
		}, 1);
	}
	ModuleMessenger(ModuleMessenger const&)            = delete;
	ModuleMessenger& operator=(ModuleMessenger const&) = delete;
public:
	static ModuleMessenger& getInstance() {
		static ModuleMessenger m;
		return m;
	}
	template<typename P>
	int registerCallback(std::function<void(P const&)> _func) {
		return Listener<P>::getInstance().add(_func);
	}
	template<typename P>
	int registerCallbackSync(std::function<void(std::shared_ptr<P>&)> _func) {
		return Listener<P>::getInstance().addSync(_func);
	}
	template<typename P>
	void unregisterCallback(int id) {
		Listener<P>::getInstance().remove(id);
	}
	template<typename P>
	void unregisterCallbackSync(int id) {
		Listener<P>::getInstance().removeSync(id);
	}


	template<typename T>
	void postMessage(T const& t) {
		std::unique_lock<std::mutex> lock(mutex);
		std::shared_ptr<T> tPtr = std::make_shared<T>(t);

		auto b = [tPtr](){
			Listener<T>::getInstance().call(tPtr);
		};
		mThreadPool->queue(std::move(b));
	}
	template<typename T>
	void postMessage(std::shared_ptr<T>& t) {
		std::unique_lock<std::mutex> lock(mutex);
		std::shared_ptr<T> tPtr = t;

		auto b = [tPtr](){
			Listener<T>::getInstance().call(tPtr);
		};
		mThreadPool->queue(std::move(b));
	}

	void changeThreadCt(int ct) {
		std::unique_ptr<ThreadPool> oldThreadPool(new ThreadPool);
		{
			std::unique_lock<std::mutex> lock(mutex);
			mThreadPool.swap(oldThreadPool);
			mThreadPool->spawnThread([](std::function<void()> _f) {
				_f();
			}, ct);

		}
		oldThreadPool->wait();
	}
};


template <typename T, int I, typename... Args>
struct MessageSyncRegImpl;

template <typename T, int I, typename P, typename... Args>
struct MessageSyncRegImpl<T, I, P, Args...> {
	static std::vector<int> recReg(T* t) {
		std::function<void(std::shared_ptr<P>&)> f = [t](std::shared_ptr<P>& _p) {
			t->template callbackFunc<I>(_p);
		};
		auto id = ModuleMessenger::getInstance().registerCallbackSync(f);
		auto idList = MessageSyncRegImpl<T, I+1, Args...>::recReg(t);
		idList.push_back(id);
		return idList;
	}
	static void unRecReg(std::vector<int> idList) {
		ModuleMessenger::getInstance().unregisterCallbackSync<P>(idList.back());
		idList.pop_back();
		MessageSyncRegImpl<T, I+1, Args...>::unRecReg(idList);
	}

};
template <typename T, int I>
struct MessageSyncRegImpl<T, I> {
	static std::vector<int> recReg(T* t) { return {}; }
	static void unRecReg(std::vector<int> idList) {}
};



template<int ...>
struct seq { };

template<int N, int ...S>
struct gens : gens<N-1, N-1, S...> { };

template<int ...S>
struct gens<0, S...> {
  typedef seq<S...> type;
};


class MessageSyncBase {
public:
	virtual ~MessageSyncBase() {}
};

/** MessageSyncronizer
 *
 * callbacks can listen to several events and will be called if both events have occured
 */
template <typename T, typename... Args>
class MessageSync : public MessageSyncBase {
private:
	std::mutex mutex;
	std::vector<int> idList;

public:
	MessageSync(T* t, void (T::*_func) (Args const&...)) {
		funcPtr = [t, _func](Args const&... args) {
			(t->*_func)(args...);
		};
		for (auto& b : paraValid) {
			b = false;
		}

		idList = MessageSyncRegImpl<MessageSync, 0, Args...>::recReg(this);
	}
	virtual ~MessageSync() override {
		 MessageSyncRegImpl<MessageSync, 0, Args...>::unRecReg(idList);

	}
	template<int I, typename T2>
	void callbackFunc(std::shared_ptr<T2>& _t) {
		std::unique_lock<std::mutex> lock(mutex);

		std::get<I>(para) = _t;
		paraValid[I]      = true;
		sync();
	}
private:
	void sync() {

		for (auto const& b : paraValid) {
			if (not b) return;
		}
		runFuncPtr(typename gens<sizeof...(Args)>::type());

		for (auto& b : paraValid) {
			b = false;
		}
	}
	template<int ...S>
	void runFuncPtr(seq<S...>) {
		funcPtr(*std::get<S>(para)...);
	}
private:
	std::tuple<std::shared_ptr<Args>...> para;
	std::array<bool, sizeof...(Args)> paraValid;
	std::function<void(Args const&...)> funcPtr;
};


class Registrator {
private:
	std::vector<std::function<void()>> messageList;
	std::vector<MessageSyncBase*>      messageSyncList;
public:
	~Registrator() {
		for (auto const& m : messageList) {
			m();
		}
		for (auto m : messageSyncList) {
			delete m;
		}
	}
	template<typename T, typename R, typename P>
	void addListener(T* t, R (T::*_func)(P const&)) {
		std::function<void(P const&)> f = [t, _func](P const& _p) {
			(t->*_func)(_p);
		};
		auto id = ModuleMessenger::getInstance().registerCallback(f);
		messageList.push_back([id]() {
			ModuleMessenger::getInstance().unregisterCallback<P>(id);
		});
	}

	template<typename T, typename R, typename Arg1, typename Arg2, typename... Args>
	void addListener(T* t, R (T::*_func)(Arg1 const&, Arg2 const&, Args const&...)) {
		messageSyncList.push_back(new MessageSync<T, Arg1, Arg2, Args...>(t, _func));
	}

};

template<typename T>
void postMessage(T const& t) {
	ModuleMessenger::getInstance().postMessage(t);
}

template<typename T>
void postMessage(std::shared_ptr<T>& t) {
	ModuleMessenger::getInstance().postMessage(t);
}

inline void changeThreadCount(int ct) {
	ModuleMessenger::getInstance().changeThreadCt(ct);
}


}

#endif
