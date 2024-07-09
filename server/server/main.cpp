#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <tchar.h>
#include <thread>
#include <vector>
#include <mutex>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

bool Initialize() {
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
}

mutex clientsMutex;

void InteractWithClient(SOCKET clientSocket, vector<SOCKET>& clients) {
    char buffer[4096];

    while (1) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) {
            cout << "Client disconnected!" << endl;
            break;
        }

        string message(buffer, bytesReceived);
        cout << "Client: " << message << endl;

        lock_guard<mutex> guard(clientsMutex);
        for (auto client : clients) {
            if (client != clientSocket) {
                send(client, message.c_str(), message.length(), 0);
            }
        }
    }

    lock_guard<mutex> guard(clientsMutex);
    auto it = find(clients.begin(), clients.end(), clientSocket);
    if (it != clients.end()) {
        clients.erase(it);
    }

    closesocket(clientSocket);
}

int main() {
    const int PORT = 1234;
    if (!Initialize()) {
        cout << "(-) Failed to initialize!" << endl;
        return 1;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET) {
        cout << "(-) Socket creation failed" << endl;
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    if (InetPton(AF_INET, _T("0.0.0.0"), &serverAddr.sin_addr) != 1) {
        cout << "(-) Setting address structure failed!" << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "(-) Failed to bind" << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        cout << "(-) Listen failed" << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    cout << "(+) Server started on port " << PORT << endl;

    vector<SOCKET> clients;
    while (1) {
        SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            cout << "(-) Invalid client socket" << endl;
            continue;
        }
        {
            lock_guard<mutex> guard(clientsMutex);
            clients.push_back(clientSocket);
        }
        thread t1(InteractWithClient, clientSocket, ref(clients));
        t1.detach();
    }

    closesocket(listenSocket);
    WSACleanup();

    return 0;
}
