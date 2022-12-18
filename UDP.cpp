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
#include <thread>   // std::thread, std::this_thread::sleep_for
#include <chrono>   // std::chrono::seconds
#include <string>
#include "util.h"
#include "File.h"
#include "checksum.h"
using namespace std;

class connection
{
    int sockfd;
    struct hostent *host;
    in_port_t puerto;
    char ip[17];
    in_port_t NumFlujo;
    size_t NumSec;
    char mode;
    struct sockaddr_in servaddr, cliaddr;
    unsigned int fix_len;
    bool error;
    char data_fj1[1100];
    int fj1;
    int lastCorrectSqNumber;
    string attach(int &seqNumber, int &currFlowNumber, int &numMsg, string &data, int &padding, int &chs);
    void detach(string msg, int &seqNumber, int &currFlowNumber, int &numMsg, string &data, int &padding, int &chs);
    bool checkIntegrity(string &msg, int &msgChs);
    void sendFileName(string filename);

public:
    connection()
    {
        fix_len = 25;
        NumSec = 777;
        NumFlujo = 5;
        mode = 'S';
        puerto = 45000;
        lastCorrectSqNumber = NumSec;
        error = false;
    }

    void setPort(int p) { puerto = 45000; };
    void setMode(char m) { mode = m; };
    void setIP(char *p_ip) { strcpy(ip, p_ip); };
    ssize_t enviarData(char *buf, size_t count);
    ssize_t sendData(string data, int numMsg, int &seqNumber, int flag);
    ssize_t sendFile(string filename);
    int recivirData(char *buf);
    void setServer(in_port_t puerto, char *buf);
    void setIniMode();
    void serverThread();
    void ReciveFromServerThread();
};

// Driver code
int main(int argc, char *argv[])
{
    if (argv[1][0] == 'S')
    {
        connection cnn;
        cnn.setPort(45000);
        cnn.setMode('S');
        cnn.setIniMode();
    }
    else if (argv[1][0] == 'C')
    {
        int n;
        char buffer[1100];
        connection cnn;
        cnn.setPort(45000);
        cnn.setMode('C');
        cnn.setIP("127.0.0.1");
        cnn.setIniMode();
        // cnn.enviarData("A234567890B234567890C234567890D234567890F234567890", 50);
        // cnn.enviarData("AA34567890BB34567890CC34567890DD345", 35);
        // cnn.enviarData("X2345", 5);
        // ccn.sendData("X2345");
        // n = cnn.recivirData(buffer);
        vector<string> filenames = getFilenames("client");
        for (auto i : filenames)
        {
            cnn.sendFile(i);
        }
    }
    else
    {
        cout << "C/S no definido." << endl;
    }
    connection cnn;

    /*  n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                   MSG_WAITALL, (struct sockaddr *)&cliaddr,
                   &len);

  */
    while (1)
    {
    };
    return 0;
} // end main
/***********************************************/
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
        servaddr.sin_port = htons(puerto);

        if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }

        thread(&connection::serverThread, this).detach();
    }
    if (mode == 'C')
    {
        host = (struct hostent *)gethostbyname((char *)ip);
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            perror("socket creation failed");
            exit(EXIT_FAILURE);
        }

        memset(&servaddr, 0, sizeof(servaddr));

        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(puerto);
        servaddr.sin_addr = *((struct in_addr *)host->h_addr);

        thread(&connection::ReciveFromServerThread, this).detach();
    }
}
/***********************************************/

ssize_t connection::sendFile(string filename)
{
    File f(10);
    f.setReadFilename("client/" + filename);
    int numMsg = f.size / 10 + (f.size % 10 != 0);
    int seqNumber = NumSec;

    sendData(filename, numMsg, seqNumber, 1);

    string buffData;
    while (!f.endReached())
    {
        buffData = f.read();
        sendData(buffData, numMsg, seqNumber, 2);
    }
    return 0;
}

string connection::attach(int &seqNumber, int &currFlowNumber, int &numMsg, string &data, int &padding, int &chs)
{
    return normalize(seqNumber, 5) + intToString(currFlowNumber) + normalize(numMsg, 5) + normalizeMessage(data, 10) + normalize(padding, 3) + intToString(chs);
}
void connection::detach(string msg, int &seqNumber, int &currFlowNumber, int &numMsg, string &data, int &padding, int &chs)
{
    // cout << msg << " " << msg.size();
    // contruir msg 5B(#sec)1B(#Flujo)5B(#msg)1000B(data)3B(padding)1B(CheckSum)
    int i = 0;
    seqNumber = stringToInt(msg.substr(i, 5));
    i += 5;
    currFlowNumber = stringToInt(msg.substr(i, 1));
    i += 1;
    numMsg = stringToInt(msg.substr(i, 5));
    i += 5;
    data = msg.substr(i, 10);
    i += 10;
    padding = stringToInt(msg.substr(i, 3));
    i += 3;
    chs = stringToInt(msg.substr(i, 1));
    data = data.substr(0, 10 - padding);
}

ssize_t connection::sendData(string data, int numMsg, int &seqNumber, int flag = 0)
{
    int dataSize = data.size();
    seqNumber += dataSize;
    int padding = 10 - dataSize;
    int chs = checkSum(data);

    string msg = attach(seqNumber, flag, numMsg, data, padding, chs);
    cout << "->" << msg << '\n';
    sendto(sockfd, msg.c_str(), msg.size(), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));

    return 0;
}
ssize_t connection::enviarData(char *buf, size_t count)
{
    // contruir msg 5B(#sec)1B(#Flujo)5B(#msg)1000B(data)3B(padding)1B(CheckSum)

    /*
    5B #Secc
    1B #Flujo
    V Data - 1000
    1B CS
    */
    unsigned int len, n;
    // calcular numero de flujo
    in_port_t l_NumFlujo = NumFlujo++;
    // calcular numero de msg
    int NumMsg = 0;
    NumMsg = (int)((count + 9) / 10);

    // split data in blocks of 1000
    char buffer[1100], buffTmp[1001];
    char *p = buf;
    int pi = 0;
    int pf = 0;
    int ii, i, c = 0;
    for (i = 0; i < count; i = i + 10)
    {
        if (count < 10)
        {
            pi = 0;
            pf = count;
        }
        else if ((count - pi - (10 * c)) > 10)
        {
            pi = pi + (10 * c);
            pf = 10;
        }
        else if ((count - pi - (10 * c)) < 10)
        {
            pi = pi + (10 * c);
            pf = count - pi;
        }
        if (c == 0)
            c++;
        p = buf;
        p = p + pi;
        strncpy(buffTmp, p, pf);
        buffTmp[pf] = '\0';

        // Calcular padding
        int padding = 10 - pf;
        for (ii = 0; ii < padding; ii++)
        {
            buffTmp[pf + ii] = '0';
            buffTmp[pf + ii + 1] = '\0';
        }
        // printf("%d:%d:%s\n",pi,pf,buffTmp);

        sprintf(buffer, "%05d%1d%05d%s%03d%1d", (int)NumSec, (int)l_NumFlujo, NumMsg, buffTmp, padding, 1);
        // sprintf(buffer, "%05d %1d %05d %s %03d %1d", (int)NumSec, (int)l_NumFlujo, NumMsg, buffTmp, padding, 1);
        printf(">>%s<<\n", buffer);
        // re-calcular numero de secuencia
        NumSec = NumSec + pf;
        // send data over UDP
        len = sizeof(servaddr);
        n = sendto(sockfd, (const char *)buffer, strlen(buffer),
                   MSG_CONFIRM, (const struct sockaddr *)&servaddr,
                   len);
        // cout << n << endl;
    }
    fflush(stdout);
    return 0;
}

/***********************************************/
int connection::recivirData(char *buf)
{
    int n;
    int numeroDeTO = 0;
    while (fj1 == 0)
    {
        this_thread::sleep_for(std::chrono::milliseconds(100));
        if (fj1 == 1)
        {
            // data se recivio al 100% durante los 100ms
            strcpy(buf, data_fj1);
            n = strlen(data_fj1);
        }
        else
        {
            // data no se recivio al 100% durante los 100ms
            if (numeroDeTO == 3)
            {
                cout << "ERROR, time out reached for 3 times" << endl;
                buf[0] = '\0';
                n = 0;
                fj1 = 1;
            }
            else
            {
                numeroDeTO++;
                cout << "Timeout reached for fj1" << endl;
            }
        }
    }
}
/***********************************************/
bool connection::checkIntegrity(string &msg, int &msgChs)
{
    return checkSum(msg) == msgChs;
}
void connection::serverThread()
{
    unsigned int len, n;
    char buffer[1100];
    string msg;
    int seqNumber;
    int flag;
    int numMsg;
    string data;
    int padding;
    int chs;
    string filename;

    cout << "[CONNECTION:INFO] Waiting..." << endl;
    do
    {
        len = sizeof(servaddr); // len is value/resuslt

        n = recvfrom(sockfd, (char *)buffer, fix_len,
                     MSG_WAITALL, (struct sockaddr *)&servaddr,
                     &len);
        buffer[n] = '\0';
        detach(string(buffer), seqNumber, flag, numMsg, data, padding, chs);
        if (flag == 1 || flag == 2)
        {
            if (flag == 1)
            {
                filename = data;
                cout << "[CLIENT:INFO] A new file <" << filename << "> is being uploaded\n";
            }
            string fileData;
            int i = 1;
            while (numMsg)
            {
                n = recvfrom(sockfd, (char *)buffer, fix_len,
                             MSG_WAITALL, (struct sockaddr *)&servaddr,
                             &len);
                buffer[n] = '\0';
                int newFlag, newNumMsg;
                detach(string(buffer), seqNumber, newFlag, newNumMsg, data, padding, chs);
                fileData += data;
                cout << "[CLIENT:MESSAGE] "<<i++<<"/"<<newNumMsg<<" "<<data<<'\n';
                numMsg--;
            }
            File f;
            f.setWriteFilename("server/" + filename);
            f.write(fileData);
            cout << "[CLIENT:INFO] File <" << filename << "> was uploaded successfully\n";
        }
        // if (checkIntegrity(data, chs))
        // {
        //     // Biz logic ...  read a file .. process data ...
        //     // std::this_thread::sleep_for(std::chrono::seconds(1));
        //     cout << "[CLIENT:MESSAGE] " << data << '\n';
        // }
        // else
        // {
        //     cout << "[CLIENT:ERROR] "
        //          << "Error en envío en mensaje <" << seqNumber << ">" << '\n';
        // }
        // n = sendto(sockfd, (const char *)buffer, fix_len,
        //            MSG_CONFIRM, (const struct sockaddr *)&servaddr,
        //            len);
    } while (1);
}
/**********************************/
void connection::ReciveFromServerThread()
{
    unsigned int len, n;
    char buffer[1100];
    do
    {
        len = sizeof(servaddr); // len is value/resuslt
        n = recvfrom(sockfd, (char *)buffer, fix_len,
                     MSG_WAITALL, (struct sockaddr *)&servaddr,
                     &len);
        // cout << n << endl;
        buffer[n] = '\0';
        cout << "INFO -> " << buffer << '\n';
        // procesar msg recivodo, como parte de un flujo
        // ejercicio de 1 msg de regreso del servidor
        strcpy(data_fj1, buffer);
        fj1 = 1;

    } while (1);
}