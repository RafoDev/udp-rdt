all: clean udp.exe
udp.exe:
	g++ -std=c++17 -o udp.exe UDP.cpp -pthread 
clean:
	rm -f *.exe