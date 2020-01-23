#include "Client.h"

int Client::init(uint16_t port, char* address)
{

	comSocket;
	comSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (comSocket == INVALID_SOCKET)
	{
		return SETUP_ERROR;
	}
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr(address);
	serverAddr.sin_port = htons(port);
	int result =connect(comSocket,(SOCKADDR*)&serverAddr,sizeof(serverAddr));
	if (result == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		return CONNECT_ERROR;
	}
	else if (result == 0&& shutDown)
	{
		return SHUTDOWN;
	}
	else
	{
		return SUCCESS;
	}

}
int Client::readMessage(char* buffer, int32_t size)
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

int Client::sendMessage(char* data, int32_t length)
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

void Client::stop()
{
	shutDown = true;
	shutdown(comSocket, SD_BOTH);
	closesocket(comSocket);
}