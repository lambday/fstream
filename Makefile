flags=-O3 -std=c++14 -fno-omit-frame-pointer
all:
	mkdir -p build
	g++ $(flags) tests/main.cpp -Isrc -o build/test -lbenchmark -lpthread
check:
	build/test
clean:
	rm build/test
