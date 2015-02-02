#ifndef MESSANGERSYSTEM_MESSANGERSYSTEM_H
#define MESSANGERSYSTEM_MESSANGERSYSTEM_H


#include <array>
#include <functional>
#include <memory>
#include <map>
#include <tuple>
#include <vector>

#include <threadPool/threadPool.h>


namespace messangerSystem {


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

class ModuleMessanger {
private:
	using ThreadPool = threadPool::ThreadPool<std::function<void()>>;
	std::unique_ptr<ThreadPool> mThreadPool;
	std::mutex mutex;

	ModuleMessanger()
		: mThreadPool(new ThreadPool())
	{
		mThreadPool->spawnThread([](std::function<void()> _f) {
			_f();
		}, 1);
	}
	ModuleMessanger(ModuleMessanger const&)            = delete;
	ModuleMessanger& operator=(ModuleMessanger const&) = delete;
public:
	static ModuleMessanger& getInstance() {
		static ModuleMessanger m;
		return m;
	}
	template<typename P>
	void registerCallback(std::function<void(P const&)> _func) {
		Listener<P>::getInstance().add(_func);
	}
	template<typename P>
	void registerCallbackSync(std::function<void(std::shared_ptr<P>&)> _func) {
		Listener<P>::getInstance().addSync(_func);
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
	static void recReg(T* t) {
		std::function<void(std::shared_ptr<P>&)> f = [t](std::shared_ptr<P>& _p) {
			t->template callbackFunc<I>(_p);
		};
		ModuleMessanger::getInstance().registerCallbackSync(f);
		MessageSyncRegImpl<T, I+1, Args...>::recReg(t);
	}
};
template <typename T, int I>
struct MessageSyncRegImpl<T, I> {
	static void recReg(T* t) {}
};

template<int ...>
struct seq { };

template<int N, int ...S>
struct gens : gens<N-1, N-1, S...> { };

template<int ...S>
struct gens<0, S...> {
  typedef seq<S...> type;
};


template <typename T, typename... Args>
class MessageSync {
private:
	std::mutex mutex;

public:
	MessageSync(T* t, void (T::*_func) (Args const&...)) {
		funcPtr = [t, _func](Args const&... args) {
			(t->*_func)(args...);
		};
		for (auto& b : paraValid) {
			b = false;
		}

		MessageSyncRegImpl<MessageSync, 0, Args...>::recReg(this);
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
public:
	template<typename T, typename R, typename P>
	void addListener(T* t, R (T::*_func)(P const&)) {
		std::function<void(P const&)> f = [t, _func](P const& _p) {
			(t->*_func)(_p);
		};
		ModuleMessanger::getInstance().registerCallback(f);
	}

	template<typename T, typename R, typename Arg1, typename Arg2, typename... Args>
	void addListener(T* t, R (T::*_func)(Arg1 const&, Arg2 const&, Args const&...)) {
		new MessageSync<T, Arg1, Arg2, Args...>(t, _func);
	}

};

template<typename T>
void postMessage(T const& t) {
	ModuleMessanger::getInstance().postMessage(t);
}

template<typename T>
void postMessage(std::shared_ptr<T>& t) {
	ModuleMessanger::getInstance().postMessage(t);
}

void changeThreadCount(int ct) {
	ModuleMessanger::getInstance().changeThreadCt(ct);
}


}

#endif
