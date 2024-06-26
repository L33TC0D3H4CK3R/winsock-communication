#include "server.h"
#include <WS2tcpip.h>
#include <iostream>
#include <thread>

int clientCount = 0;

SOCKET Server::AcceptClientConnection(const SOCKET &serverSocket, sockaddr_in &client, int &clientAddressSize)
{
    // accept a new connection
    const SOCKET clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&client), &clientAddressSize);

    if (clientSocket != INVALID_SOCKET)
    {
        clientCount++;
    }

    return clientSocket == INVALID_SOCKET ? INVALID_SOCKET : clientSocket;
}

char *Server::GetClientIP(const sockaddr_in &client)
{
    return inet_ntoa(client.sin_addr);
}

sockaddr_in Server::CreateServerAddress(const short &protocolFamily, const std::string &ipAddress, const int &port)
{
    sockaddr_in serverAddress{};

    serverAddress.sin_family = protocolFamily;
    serverAddress.sin_addr.s_addr = inet_addr(ipAddress.c_str());
    serverAddress.sin_port = htons(port);

    return serverAddress;
}

SOCKET Server::CreateServerSocket(const int &protocolFamily, const int &type, const bool &TCP)
{
    return socket(protocolFamily, type, TCP ? IPPROTO_TCP : IPPROTO_UDP);
}

void Server::ReadIncomingMessages(const SOCKET &clientSocket, const char *clientIp, Broadcaster &broadcaster, const sockaddr_in &clientAddress, const bool &closeServer, const SOCKET &serverSocket)
{

    struct hostent *host = gethostbyaddr((char*) &clientAddress.sin_addr, sizeof(clientAddress.sin_addr), AF_INET);

    const std::string name = host != nullptr ? std::string(host->h_name) : "";

    std::thread([=]() {
        while (clientSocket != INVALID_SOCKET)
        {
            // Read data from the client
            char buffer[1024];

            const int received = recv(clientSocket, buffer, sizeof(buffer), 0);

            if (received == INVALID_SOCKET)
            {

                // le nigga disconnected :cry:
                break;
            }

            const std::string message = std::string(buffer, received);

            // don't remove this lol
            if (message.empty())
            {
                break;
            }

            broadcaster.Broadcast("Client: " + name + " (" + std::string(clientIp) + ")" + " sent: " + message);

            broadcaster.Broadcast("Client Count: " + std::to_string(clientCount));
        }

        broadcaster.BroadcastError("Client: " + name + " (" + clientIp + ")" + " disconnected!");

        if (clientCount > 0)
        {
            clientCount--;
        }

        broadcaster.BroadcastError("Closing Client's socket!");
        closesocket(clientSocket);
        broadcaster.BroadcastError("Closed Client's socket!");



        if (closeServer && clientCount <= 0)
        {
            broadcaster.BroadcastError("Closing server socket!");
            closesocket(serverSocket);
            WSACleanup();

            broadcaster.BroadcastError("Closed server socket!");
        }
    }).detach();
}

int Server::GetClientCount() const
{
    return clientCount;
}