all: clean udp.exe
udp.exe:
	g++ -std=c++17 -o udp.exe udp.cc -pthread 
clean:
	rm -f *.exe *.out server/*.html *.txt
test:
	g++ -std=c++17 -o test.exe test.cc
	./test.exe
clean_test:
	rm -f *.txt 