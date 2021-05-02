// Winsock Server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <tchar.h>
#include <process.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "5050"
#define mymin(a, b) (((a) < (b)) ? (a) : (b))



bool senddata(SOCKET sock, void* buf, int buflen)
{
    char* pbuf = (char*)buf;

    while (buflen > 0)
    {
        int num = send(sock, pbuf, buflen, 0);
        if (num == SOCKET_ERROR)
        {
            if (WSAGetLastError() == WSAEWOULDBLOCK)
            {
                // optional: use select() to check for timeout to fail the send
                continue;
            }
            return false;
        }

        pbuf += num;
        buflen -= num;
    }

    return true;
}

bool sendlong(SOCKET sock, long value)
{
    value = htonl(value);
    return senddata(sock, &value, sizeof(value));
}
bool sendChar(SOCKET sock, char value)
{
    value = htonl(value);
    return senddata(sock, &value, sizeof(value));
}
bool sendInt(SOCKET sock, int value)
{
    value = htonl(value);
    return senddata(sock, &value, sizeof(value));
}

bool readdata(SOCKET sock, void* buf, int buflen)
{
    char* pbuf = (char*)buf;

    while (buflen > 0)
    {
        int num = recv(sock, pbuf, buflen, 0);
        if (num == SOCKET_ERROR)
        {
            if (WSAGetLastError() == WSAEWOULDBLOCK)
            {
                // optional: use select() to check for timeout to fail the read
                continue;
            }
            return false;
        }
        else if (num == 0)
            return false;

        pbuf += num;
        buflen -= num;
    }

    return true;
}

bool readlong(SOCKET sock, long long* value)
{
    if (!readdata(sock, value, sizeof(value)))
        return false;
    *value = ntohl(*value);
    return true;
}

bool readfile(SOCKET sock, FILE* f)
{
    long long filesize;
    if (!readlong(sock, &filesize))
        return false;
    std::cout << "Filesize: " << filesize << std::endl;
    //filesize -= 4;
    if (filesize > 0)
    {
        char buffer[1024];
        do
        {
            size_t num = mymin(filesize, sizeof(buffer));
            if (filesize < 1024)
                std::cout << "almost" << filesize;
            if (!readdata(sock, buffer, num))
                return false;
            int offset = 0;
            do
            {
                size_t written = fwrite(&buffer[offset], 1, num - offset, f);
                if (written < 1)
                    return false;
                offset += written;
            } while (offset < num);
            filesize -= num;
            std::cout << "filesize: " << filesize << std::endl;
        } while (filesize > 0);
    }
    return true;
}

unsigned __stdcall ClientSession(void* data)
{
    SOCKET ClientSocket = (SOCKET)data;
    // Process the client.
    std::cout << "Accepted a connection.\n";
    int iResult = 0;

    //// No longer need server socket
    //closesocket(ListenSocket);

    // Receive until the peer shuts down the connection




    FILE* filehandle = fopen("output.jpg", "wb");
    if (filehandle != NULL)
    {
        bool ok = readfile(ClientSocket, filehandle);
        fclose(filehandle);

        if (ok)
        {
            // use file as needed...
            std::cout << "Received Good File" << std::endl;

            ////send ACK back
            char buffer[1024] = "";
            char* sendbuf;
            std::string msg = "hello";// processImage("output.jpg");
            //system("start processImg.exe");
            STARTUPINFO si = {};
            si.cb = sizeof si;

            PROCESS_INFORMATION pi = {};
            const TCHAR* target = _T("processImg.exe");

            if (!CreateProcess(target, 0, 0, FALSE, 0, 0, 0, 0, &si, &pi))
            {
                std::cerr << "CreateProcess failed (" << GetLastError() << ").\n";
            }
            else
            {
                std::cout << "Waiting on process until finished" << std::endl;
                WaitForSingleObject(pi.hProcess, INFINITE);
                /*
                if ( TerminateProcess(pi.hProcess, 0) ) // Evil
                    cout << "Process terminated!";
                */
                //if (PostThreadMessage(pi.dwThreadId, WM_QUIT, 0, 0)) // Good
                //    std::cout << "Request to terminate process has been sent!";

                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }

            std::ifstream infile("out.txt");
            std::getline(infile, msg); // To get you all the lines.
            infile.close();
            std::cout << msg;
            msg.copy(buffer, msg.length());
            sendbuf = buffer;

            //char* buf = new char[msg.length()];// buffer = msg.c_str();
            //for (int i = 0; i < msg.length(); i++) {
            //    buf[i] = msg[i];
            //}
/*
//
//*/
            if(msg.length() <= 1 || msg == ""){
                msg = "Could not detect an object with great enough certainty. Try Again.";
                msg.copy(buffer, msg.length());

            }
            iResult = send(ClientSocket, buffer, msg.length(), 0);
            if (iResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(ClientSocket);
                WSACleanup();
                return 1;
            }

            /*if (!senddata(ClientSocket, buffer, msg.length())) {
                std::cout << "Error sending datatype" << std::endl;
            }*/

            // iResult = 1;
        }
        else
        {
            //socket disconnected
            remove("output.jpg");
            iResult = 0;
        }
    }





    //// shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }
}


int __cdecl main(void)
{
   


    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Listening for connections" << std::endl;
    while(ClientSocket = accept(ListenSocket, NULL, NULL))
    {
        // Accept a client socket
        
        if (ClientSocket == INVALID_SOCKET) {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        unsigned threadID;
        HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, &ClientSession, (void*)ClientSocket, 0, &threadID);

       
    }
    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}

//#define WIN32_LEAN_AND_MEAN
//
//#include <windows.h>
//#include <winsock2.h>
//#include <ws2tcpip.h>
//#include <stdlib.h>
//#include <stdio.h>
//
//
//// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
//#pragma comment (lib, "Ws2_32.lib")
//#pragma comment (lib, "Mswsock.lib")
//#pragma comment (lib, "AdvApi32.lib")
//
//
//#define DEFAULT_BUFLEN 512
//#define DEFAULT_PORT "5032"
//
//
//
//bool senddata(SOCKET sock, void* buf, int buflen)
//{
//    char* pbuf = (char*)buf;
//
//    while (buflen > 0)
//    {
//        int num = send(sock, pbuf, buflen, 0);
//        if (num == SOCKET_ERROR)
//        {
//            if (WSAGetLastError() == WSAEWOULDBLOCK)
//            {
//                // optional: use select() to check for timeout to fail the send
//                continue;
//            }
//            return false;
//        }
//
//        pbuf += num;
//        buflen -= num;
//    }
//
//    return true;
//}
//
//bool sendlong(SOCKET sock, long long value)
//{
//    value = htonl(value);
//    return senddata(sock, &value, sizeof(value));
//}
//
//bool sendfile(SOCKET sock, FILE* f)
//{
//    fseek(f, 0, SEEK_END);
//    long long filesize = ftell(f);
//    rewind(f);
//    if (filesize == EOF)
//        return false;
//    if (!sendlong(sock, filesize))
//        return false;
//    if (filesize > 0)
//    {
//        char buffer[1024];
//        do
//        {
//            size_t num = min(filesize, sizeof(buffer));
//            num = fread(buffer, 1, num, f);
//            if (num < 1)
//                return false;
//            if (!senddata(sock, buffer, num))
//                return false;
//            filesize -= num;
//        } while (filesize > 0);
//    }
//    return true;
//}
//int __cdecl main(int argc, char** argv)
//{
//    WSADATA wsaData;
//    SOCKET ConnectSocket = INVALID_SOCKET;
//    struct addrinfo* result = NULL,
//        * ptr = NULL,
//        hints;
//    const char* sendbuf = "this is a test";
//    char recvbuf[DEFAULT_BUFLEN];
//    int iResult;
//    int recvbuflen = DEFAULT_BUFLEN;
//
//    // Validate the parameters
//    if (argc != 2) {
//        printf("usage: %s server-name\n", argv[0]);
//        return 1;
//    }
//
//    // Initialize Winsock
//    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
//    if (iResult != 0) {
//        printf("WSAStartup failed with error: %d\n", iResult);
//        return 1;
//    }
//
//    ZeroMemory(&hints, sizeof(hints));
//    hints.ai_family = AF_UNSPEC;
//    hints.ai_socktype = SOCK_STREAM;
//    hints.ai_protocol = IPPROTO_TCP;
//
//    // Resolve the server address and port
//    iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
//    if (iResult != 0) {
//        printf("getaddrinfo failed with error: %d\n", iResult);
//        WSACleanup();
//        return 1;
//    }
//
//    // Attempt to connect to an address until one succeeds
//    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
//
//        // Create a SOCKET for connecting to server
//        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
//            ptr->ai_protocol);
//        if (ConnectSocket == INVALID_SOCKET) {
//            printf("socket failed with error: %ld\n", WSAGetLastError());
//            WSACleanup();
//            return 1;
//        }
//
//        // Connect to server.
//        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
//        if (iResult == SOCKET_ERROR) {
//            closesocket(ConnectSocket);
//            ConnectSocket = INVALID_SOCKET;
//            continue;
//        }
//        break;
//    }
//
//    freeaddrinfo(result);
//
//    if (ConnectSocket == INVALID_SOCKET) {
//        printf("Unable to connect to server!\n");
//        WSACleanup();
//        return 1;
//    }
//
//    // Send an initial buffer
//  /*  iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
//    if (iResult == SOCKET_ERROR) {
//        printf("send failed with error: %d\n", WSAGetLastError());
//        closesocket(ConnectSocket);
//        WSACleanup();
//        return 1;
//    }
//
//    printf("Bytes Sent: %ld\n", iResult);*/
//
//
//    //send image
//    FILE* filehandle = fopen("p.jpg", "rb");
//    if (filehandle != NULL)
//    {
//        sendfile(ConnectSocket, filehandle);
//        fclose(filehandle);
//    }
//
//
//
//
//    // shutdown the connection since no more data will be sent
//    iResult = shutdown(ConnectSocket, SD_SEND);
//    if (iResult == SOCKET_ERROR) {
//        printf("shutdown failed with error: %d\n", WSAGetLastError());
//        closesocket(ConnectSocket);
//        WSACleanup();
//        return 1;
//    }
//
//    // Receive until the peer closes the connection
//    do {
//
//        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
//        if (iResult > 0)
//            printf("Bytes received: %d\n", iResult);
//        else if (iResult == 0)
//            printf("Connection closed\n");
//        else
//            printf("recv failed with error: %d\n", WSAGetLastError());
//
//    } while (iResult > 0);
//
//    // cleanup
//    closesocket(ConnectSocket);
//    WSACleanup();
//
//    return 0;
//}