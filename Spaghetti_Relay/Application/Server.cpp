#include "Server.h"

int Server::init(uint16_t port)
{
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
	{
		return SETUP_ERROR;
	}
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);
	int result = bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (result == SOCKET_ERROR)
	{
		return BIND_ERROR;
	}
	
	result = listen(listenSocket, 1);
	if (result == SOCKET_ERROR)
	{
		return SETUP_ERROR;
	}

	comSocket = accept(listenSocket, NULL, NULL);
	if (comSocket == INVALID_SOCKET)
	{
		return CONNECT_ERROR;

	}
	return SUCCESS;
}
int Server::readMessage(char* buffer, int32_t size)
{
	uint8_t msgSize = 0;

	int  result = tcp_recv_whole(comSocket, (char*)&msgSize, 1);
	if (size < msgSize)
	{
		return PARAMETER_ERROR;
	}
	if (result == 0 && shutDown)
	{
		return SHUTDOWN;
	}
	if ((result == SOCKET_ERROR) || (result == 0))
	{
		int error = WSAGetLastError();
		return DISCONNECT;
	}

	result = tcp_recv_whole(comSocket, buffer, msgSize);
	if (result == 0 && shutDown)
	{
		return SHUTDOWN;
	}
	if ((result == SOCKET_ERROR) || (result == 0))
	{
		int error = WSAGetLastError();
		return DISCONNECT;
	}

	return SUCCESS;
}
int Server::sendMessage(char* data, int32_t length)
{
	if (length < 0 || length>255)
	{
		return PARAMETER_ERROR;
	}

	int result = tcp_send_whole(comSocket, (char*)&length, 1);
	if (result == 0 && shutDown)
	{
		return SHUTDOWN;
	}
	if ((result == SOCKET_ERROR) || (result == 0))
	{
		int error = WSAGetLastError();
		return DISCONNECT;
	}

	result = tcp_send_whole(comSocket, data, length);
	if (result == 0 && shutDown)
	{
		return SHUTDOWN;
	}
	if ((result == SOCKET_ERROR) || (result == 0))
	{
		int error = WSAGetLastError();
		return DISCONNECT;
	}

	return SUCCESS;
}
void Server::stop()
{
	shutDown = true;
	shutdown(listenSocket, SD_BOTH);
	closesocket(listenSocket);

	shutdown(comSocket, SD_BOTH);
	closesocket(comSocket);
}