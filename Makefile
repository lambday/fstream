all:
	mkdir -p build
	g++ -O3 -std=c++14 tests/main.cpp -Isrc -o build/test
check:
	build/test
clean:
	rm build/test
