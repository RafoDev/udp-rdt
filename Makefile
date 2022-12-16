all: clean udp.exe
udp.exe:
	g++ -o udp.exe UDP.cpp -pthread 
clean:
	rm -f *.exe