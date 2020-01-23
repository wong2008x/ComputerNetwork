#pragma once
#include "../platform.h"
#include "../definitions.h"

class Server
{
	public:

	int init(uint16_t port);
	int readMessage(char* buffer, int32_t size);
	int sendMessage(char* data, int32_t length);
	void stop();

	SOCKET listenSocket;
	SOCKET comSocket;
	bool shutDown=false;

	int tcp_recv_whole(SOCKET s, char* buf, int len)
	{
		int total = 0;

		do
		{
			int ret = recv(s, buf + total, len - total, 0);
			if (ret < 1)
				return ret;
			else
				total += ret;

		} while (total < len);

		return total;
	}
	int tcp_send_whole(SOCKET skSocket, const char* data, uint16_t length)
	{
		int result;
		int bytesSent = 0;

		while (bytesSent < length)
		{
			result = send(skSocket, (const char*)data + bytesSent, length - bytesSent, 0);

			if (result <= 0)
				return result;

			bytesSent += result;
		}

		return bytesSent;
	}
};