#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <string>
#include <functional>
#include <cerrno>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define FDR_UNUSED(expr){ (void)(expr); } 
#define FDR_ON_ERROR std::function<void(int, std::string)> onError = [](int errorCode, std::string errorMessage){FDR_UNUSED(errorCode); FDR_UNUSED(errorMessage)}

class BaseSocket
{
// Definitions
public:
    sockaddr_in address;
    bool isClosed = false;

protected:
    SOCKET sock = INVALID_SOCKET;

    static std::string ipToString(sockaddr_in addr) {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN);

        return std::string(ip);
    } 

    BaseSocket(FDR_ON_ERROR, int socketId = -1) {
        // Initialize Winsock
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            onError(errno, "WSAStartup failed with error: " + std::to_string(iResult));
            return;
        }

        if (socketId < 0) {
            // Create a SOCKET to connect to server or to listen for client connections
            this->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (this->sock == INVALID_SOCKET) {
                onError(errno, "socket failed with error: " + std::to_string(WSAGetLastError()));
                WSACleanup();
                return;
            }
        } else {
            this->sock = (SOCKET) socketId;
        }
    }

// Methods
public:
    virtual void Close() {
        if (isClosed) return;

        isClosed = true;
        closesocket(this->sock);
        WSACleanup();
    }

    std::string remoteAddress() { return ipToString(this->address); }
    int remotePort() { return ntohs(this->address.sin_port); }
};