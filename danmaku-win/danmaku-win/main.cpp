#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN

#include <cstdio>
#include <ctime>
#include <iostream>
#include <thread>
#include <string>
#include <queue>
#include <list>
#include <Windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <IPHlpApi.h>
#include "json\json.h"
#include "citron.h"
#include <sstream>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "lib_json.lib")

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

SOCKET SetUpSocket();
void work();
void deal(SOCKET);

SOCKET ListenSocket;
bool KeepWorking = true;
//Citron::queue<std::string> dmque;
Citron::list<SOCKET *> sockets;
std::string name;
time_t st;

int main() {
    std::cout << "请输入事件名称：";
    std::cin >> name;
    std::cout << "请输入开始时间（格式hh:mm:ss，以本机时间为准）：";
    std::string temp;
    std::cin >> temp;
    time_t now = time(nullptr);
    tm localNow;
    localtime_s(&localNow, &now);
    localNow.tm_hour = (temp[0] - '0') * 10 + temp[1] - '0';
    localNow.tm_min = (temp[3] - '0') * 10 + temp[4] - '0';
    localNow.tm_sec = (temp[6] - '0') * 10 + temp[7] - '0';
    st = mktime(&localNow);

    SetUpSocket();
    std::thread(work).detach();

    for (std::cin >> temp; temp != "q"; std::cin >> temp)
        continue;
    KeepWorking = false;

    closesocket(ListenSocket);
    WSACleanup();
	return 0;
}

SOCKET SetUpSocket() {
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult) {
        std::cout << "WSAStartup failed: " << iResult << std::endl;
        exit(1);
    }

    addrinfo *result = nullptr, *ptr = nullptr, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo(nullptr, DEFAULT_PORT, &hints, &result);
    if (iResult) {
        std::cout << "getaddrinfo failed: " << iResult << std::endl;
        WSACleanup();
        exit(1);
    }

    ListenSocket = socket(
        result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        exit(1);
    }

    iResult = bind(ListenSocket, result->ai_addr, result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        std::cout << "bind failed with error: " << WSAGetLastError()
            << std::endl;
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        exit(1);
    }
    freeaddrinfo(result);
		
    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "Listen failed with error: " << WSAGetLastError()
            << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        exit(1);
    }

    return ListenSocket;
}

void work() {
    SOCKET newClientSocket;
    while (KeepWorking) {
        newClientSocket = accept(ListenSocket, nullptr, nullptr);
        if (newClientSocket == INVALID_SOCKET)
            continue;
        std::thread(deal, newClientSocket).detach();
    }
}

void deal(SOCKET socket) {
    Citron::list<SOCKET *>::iterator it = sockets.push_back(&socket);

    time_t now = time(nullptr);
    tm localNow;
    localtime_s(&localNow, &now);
    int secs = difftime(st, mktime(&localNow));
    if (secs < 0)
        secs = 0;
    Json::Value value;
    value["Name"] = Json::Value(name);
    value["Time"] = Json::Value(secs);
	std::stringstream ss;
	ss << value;
	std::string temp;
	std::string subtemp;
	while (getline(ss, subtemp)) temp += subtemp;
    send(socket, temp.c_str(), temp.length(), 0);

    Json::Reader reader;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN, recvLen;
    recvLen = recv(socket, recvbuf, recvbuflen, 0);
    while (recvLen > 0) {
        reader.parse(recvbuf, recvbuf + recvLen, value, false);
        // dmque.push(value["dm"].asString());
        std::cout << value["dm"].asString() << std::endl;
        while (sockets.test_and_set())
            continue;
        for (auto & i : sockets)
            send(*i, recvbuf, recvLen, 0);
        sockets.clear_lock();
        recv(socket, recvbuf, recvbuflen, 0);
    }
    sockets.erase(it);
    closesocket(socket);
}
