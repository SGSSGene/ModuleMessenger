all:
	clang++ -std=c++11 -isystem../threadpool/include -Iinclude -pthread src/test/main.cpp src/test/main2.cpp -o test
