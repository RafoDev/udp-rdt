// Server side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>

#include <iostream> // std::cout
#include <thread>		// std::thread, std::this_thread::sleep_for
#include <chrono>		// std::chrono::seconds
#include <string>
#include <cstdlib>
#include <map>

#include "include/util.h"
#include "include/test.h"
// #include "include/File.h"
#include "include/checksum.h"
#include "include/Timer.h"
using namespace std;

class connection
{
	int sockfd;
	struct hostent *host;
	in_port_t port;
	size_t seqNumber;
	char mode;
	struct sockaddr_in servaddr, cliaddr;
	uint fix_len, msg_len;
	string filename;
	string ip;
	bool timeout, loss;

	string attach(uint *flag, uint *numMsg, string *data, uint *padding, uint *chs);
	int receiveAndDetach(uint *_seqNumber, uint *flag, uint *numMsg, string *data, uint *padding, uint *chs);
	void detach(string msg, uint *currSeqNumber, uint *currFlowNumber, uint *numMsg, string *data, uint *padding, uint *chs);
	bool checkIntegrity(string &msg, uint &msgChs);

public:
	bool failed, success;
	Timer t1;
	connection()
	{
		msg_len = 1000;
		fix_len = msg_len + 15;
		seqNumber = 1;
		mode = 'S';
		port = 45041;
		ip = "127.0.0.1";
		failed = false;
		success = false;
	}
	void setMode(char m) { mode = m; };
	void sendData(string data, uint numMsg, uint flag);
	void sendFile(string _filename);
	void setIniMode();
	void serverThread();
	void receiveFromServerThread();
};

int main(int argc, char *argv[])
{
	// fflush(stdout);
	connection cnn;
	if (argv[1][0] == 'S')
	{
		cnn.setMode('S');
		cnn.setIniMode();
	}
	else if (argv[1][0] == 'C')
	{
		cnn.setMode('C');
		cnn.setIniMode();
		vector<string> filenames = getFilenames("client");

		map<string, vector<double>> times;
		for (int k = 0; k < 40; k++)
			for (auto file : filenames)
			{
				cnn.t1.start();
				cnn.success = false;
				while (!cnn.success)
					cnn.sendFile(file);
				cnn.t1.stop();
				times[file].push_back(cnn.t1.elapsedMilliseconds());
			}
		for (auto i : times)
		{
			cout << i.first << '\n';
			for (auto j : i.second)
			{
				cout << j << "\n";
			}
			cout << '\n';
		}
	}
	else
	{
		cout << "[CONNECTION:ERROR] C/S undefined." << endl;
	}
	while (1)
	{
	};
	return 0;
}

void connection::setIniMode()
{
	if (mode == 'S')
	{
		if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		{
			perror("socket creation failed");
			exit(EXIT_FAILURE);
		}

		memset(&servaddr, 0, sizeof(servaddr));
		memset(&cliaddr, 0, sizeof(cliaddr));

		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = INADDR_ANY;
		servaddr.sin_port = htons(port);

		if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
		{
			perror("bind failed");
			exit(EXIT_FAILURE);
		}

		thread(&connection::serverThread, this).detach();
	}
	if (mode == 'C')
	{
		host = (struct hostent *)gethostbyname((char *)ip.c_str());
		if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		{
			perror("socket creation failed");
			exit(EXIT_FAILURE);
		}

		memset(&servaddr, 0, sizeof(servaddr));

		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(port);
		servaddr.sin_addr = *((struct in_addr *)host->h_addr);

		thread(&connection::receiveFromServerThread, this).detach();
	}
}

void connection::sendFile(string filename)
{
	string buffData;
	File f(msg_len);
	f.setReadFilename("client/" + filename);
	uint numMsg = f.size / msg_len + (f.size % msg_len != 0);

	sendData(filename, numMsg, 1);

	while (!f.endReached() && !failed)
	{
		buffData = f.read();
		sendData(buffData, numMsg--, 2);
	}

	this_thread::sleep_for(std::chrono::milliseconds(100));

	success = !failed;
	failed = false;
}

string connection::attach(uint *flag, uint *numMsg, string *data, uint *padding, uint *chs)
{
	return normalize(seqNumber, 5) + intToString(*flag) + normalize(*numMsg, 5) + normalizeMessage(*data, msg_len) + normalize(*padding, 3) + intToString(*chs);
}

int connection::receiveAndDetach(uint *_seqNumber, uint *flag, uint *numMsg, string *data, uint *padding, uint *chs)
{
	uint len = sizeof(servaddr);
	char data_c[1100];

	int n = recvfrom(sockfd, data_c, fix_len, MSG_WAITALL, (struct sockaddr *)&servaddr, &len);
	detach(string(data_c), _seqNumber, flag, numMsg, data, padding, chs);
	return n;
}

void connection::detach(string msg, uint *_seqNumber, uint *flag, uint *numMsg, string *data, uint *padding, uint *chs)
{
	int i = 0;
	*_seqNumber = stringToInt(msg.substr(i, 5));
	i += 5;
	*flag = stringToInt(msg.substr(i, 1));
	i += 1;
	*numMsg = stringToInt(msg.substr(i, 5));
	i += 5;
	*data = msg.substr(i, msg_len);
	i += msg_len;
	*padding = stringToInt(msg.substr(i, 3));
	i += 3;
	*chs = stringToInt(msg.substr(i, 1));
	*data = data->substr(0, msg_len - *padding);
}

void connection::sendData(string data, uint numMsg, uint flag = 0)
{
	int dataSize = data.size();
	seqNumber = (dataSize + seqNumber) % 100000;
	uint padding = msg_len - dataSize;
	uint chs = checkSum(data);
	string msg = attach(&flag, &numMsg, &data, &padding, &chs);
	// cout<<msg<<'\n';
	sendto(sockfd, msg.c_str(), msg.size(), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));
}

bool connection::checkIntegrity(string &msg, uint &msgChs)
{
	// bool intentionalError = (rand() % 10) == 5;
	// if (intentionalError)
	// {
	// 	cout << "(intentional error)\n";
	// 	return false;
	// }
	// else
	// {
	return checkSum(msg) == msgChs;
	// }
}

void connection::serverThread()
{
	uint len, n, currSeqNumber, flag, numMsg, padding, chs;
	string msg, data, _filename;

	cout << "[CONNECTION:INFO] Waiting..." << endl;
	do
	{
		receiveAndDetach(&currSeqNumber, &flag, &numMsg, &data, &padding, &chs);
		switch (flag)
		{
		case 1:
		{
			string fileData;
			uint i = 1, newFlag, newNumMsg, tmpNumMsg, j = numMsg;
			tmpNumMsg = numMsg;

			filename = data;
			cout << "[CLIENT:INFO] A new file <" << filename << "> is being uploaded\n";

			failed = false;
			loss = false;

			while (numMsg > 1)
			{
				receiveAndDetach(&currSeqNumber, &newFlag, &numMsg, &data, &padding, &chs);
				loss = j != numMsg;
				if (loss)
				{
					cout << "[CONNECTION:ERROR] "
							 << "Error while sending " << i << "/" << tmpNumMsg << '\n';
					sendData("Packet Loss in <" + filename + "> file.", 1, 3);
					failed = true;
					numMsg = 0;
				}
				if (!failed)
				{
					if (checkIntegrity(data, chs))
					{
						fileData += data;
						// cout << "[CLIENT:MESSAGE] " << i++ << "/" << tmpNumMsg << " " << data << '\n';
					}
					else
					{
						cout << "[CONNECTION:ERROR] "
								 << "Error while sending " << i << "/" << tmpNumMsg << '\n';
						sendData("Integrity Error uploading <" + filename + "> file.", 1, 3);
						failed = true;
						numMsg = 0;
					}
				}
				j--;
			}
			if (!failed)
			{
				File f(msg_len);
				f.setWriteFilename("server/" + filename);
				f.write(fileData);
				sendData("File <" + filename + "> was uploaded successfully.", 2, 4);
				cout << "[CLIENT:INFO] File <" << filename << "> was uploaded successfully\n";
			}
			break;
		}
		default:
			break;
		}
	} while (1);
}

void connection::receiveFromServerThread()
{
	uint _seqNumber, flag, numMsg, padding, chs;
	string data;
	do
	{
		receiveAndDetach(&_seqNumber, &flag, &numMsg, &data, &padding, &chs);
		if (flag == 3)
		{
			cout << "ERROR#connection -> "
					 << data << '\n';
			failed = true;
		}
		else if (flag == 4)
		{
			cout << "SUCCESS#connection -> "
					 << data << '\n';
		}

	} while (1);
}