all:
	clang++ -std=c++11 -isystem../threadpool/include -Iinclude -pthread src/test/main.cpp -o test
