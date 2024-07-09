#include<iostream>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<thread>
#include<string>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

bool Initialize() {
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
}

void SendMsg(SOCKET s) {
    cout << "Enter chat name: ";
    string name;
    getline(cin, name);
    string message;

    while (1) {
        getline(cin, message);
        string msg = name + " : " + message;
        int bytesSent = send(s, msg.c_str(), msg.length(), 0);
        if (bytesSent == SOCKET_ERROR) {
            cout << "Error sending message! " << endl;
            break;
        }
        if (message == "SYSQUIT") {
            cout << "stopping!" << endl;
            break;
        }
    }
    closesocket(s);
    WSACleanup();
}

void ReceiveMsg(SOCKET s) {
    char buffer[4096];
    int recvlength;
    string receivedMessage;
    while (1) {
        recvlength = recv(s, buffer, sizeof(buffer), 0);
        if (recvlength <= 0) {
            cout << "disconnected from the server" << endl;
            break;
        }
        else {
            receivedMessage = string(buffer, recvlength);
            cout << receivedMessage << endl;
        }
    }
}

int main() {
    if (!Initialize()) {
        cout << "(-) Initialization Failed" << endl;
        return 1;
    }

    string serverAdd = "127.0.0.1";
    int PORT = 1234;
    SOCKET s;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        cout << "(-) Failed to create socket" << endl;
        return 1;
    }

    sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(PORT);
    inet_pton(AF_INET, serverAdd.c_str(), &(serveraddr.sin_addr));

    if (connect(s, reinterpret_cast<sockaddr*>(&serveraddr), sizeof(serveraddr)) == SOCKET_ERROR) {
        cout << "(-)Unable to connect to the server" << endl;
        closesocket(s);
        WSACleanup();
        return 1;
    }

    cout << "(+) Connected to the server" << endl;

    thread sender(SendMsg, s);
    thread receiver(ReceiveMsg, s);

    sender.join();
    receiver.join();

    return 0;
}
