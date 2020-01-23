#include "TCPChatClient.h"

TCPChatClient::TCPChatClient(ChatLobby& chat_int) : chat_interface(chat_int)
{

}
TCPChatClient::~TCPChatClient(void)
{

}
bool TCPChatClient::init(std::string name, std::string ip_address, uint16_t port)
{
	comSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (comSocket == INVALID_SOCKET)
	{
		chat_interface.DisplayString("CLIENT: SETUP ERROR");
		return false;
	}
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr(ip_address.c_str());
	serverAddr.sin_port = htons(port);
	int result = connect(comSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (result ==SOCKET_ERROR)
	{
		chat_interface.DisplayString("CLIENT: CONNECTION ERROR");
		return false;
	}
	uint16_t len = 18;
	char buffer[20];
	memcpy(buffer, &len, 2);
	buffer[2] = NET_MESSAGE_TYPE::cl_reg;
	strcpy(&buffer[3],name.c_str());
	result = send(comSocket, buffer, 20, 0);

	if (result <=0)
	{
		chat_interface.DisplayString("CLIENT: SENDING ERROR");
		return false;
	}
	else {
		return true;
	}
}
bool TCPChatClient::run(void)
{
	uint16_t msgSize = 0;

	int  result = tcp_recv_whole(comSocket, (char*)&msgSize, 2,0);
	if (result <=0)
	{
		chat_interface.DisplayString("CLIENT: RECEIVING ERROR");
		return false;
	}
	
	char* buffer = new char[msgSize];
	result = tcp_recv_whole(comSocket, buffer, msgSize, 0);
	if (result <=0)
	{
		chat_interface.DisplayString("CLIENT: RECEIVING ERROR");
		return false;
	}

	switch (buffer[0])
	{
		case NET_MESSAGE_TYPE::sv_cnt:
		{
			chat_interface.DisplayString("CLIENT:  CONNECTED TO SERVER");

			myId = buffer[1];
			uint16_t len = 1;
			char getbuffer[3];
			memcpy(getbuffer, &len, 2);
			getbuffer[2] = NET_MESSAGE_TYPE::cl_get;
			result = send(comSocket, getbuffer, 3, 0);
			if (result <=0)
			{
				chat_interface.DisplayString("CLIENT:  REGISTERING ERROR");
				return false;
			}
			chat_interface.DisplayString("CLIENT:  REGISTERED IN SERVER");
		}break;

		case sv_list:
		{
			//no length now 0 slot---TAG
			uint8_t numInList = buffer[1];
			for (size_t i = 0; i < numInList; i++)
			{
				char name[17];
				strncpy(name, &buffer[3 + i * 18], 17);
				myNameList[buffer[2 + i * 18]] = name;
				chat_interface.AddNameToUserList(name, buffer[2+i*18]);
			}
				chat_interface.DisplayString("CLIENT:  RECEIVED USER LIST");
		}break;

		case sv_add:
		{
			char name[17];
			strncpy(name, &buffer[2], 17);
			myNameList[buffer[1]] = name;
			chat_interface.AddNameToUserList(name, buffer[1]);
			std::string info(name);
			info = "CLIENT:  " + info + " JOINED";
			chat_interface.DisplayString(info);
		}break;
		
		case sv_full:
		{
			chat_interface.DisplayString("CLIENT:  SERVER IS FULL");
			stop();
			return false;
		}break;

		case sv_remove:
		{
			std::string user(myNameList[buffer[1]]);
			user = "CLIENT:  " + user + " LEFT";
			chat_interface.DisplayString(user);
			chat_interface.RemoveNameFromUserList(buffer[1]);
			myNameList.erase(myNameList.find(buffer[1]));
		}break;

		case sv_cl_msg:
		{
			char* msgBuff = new char[msgSize - 2];
			strncpy(msgBuff, &buffer[2], msgSize - 2);
			chat_interface.AddChatMessage(msgBuff, buffer[1]);
			delete[] msgBuff;
		}break;

		case sv_cl_close:
		{
			chat_interface.DisplayString("CLIENT:  SERVER DISCONNECTED");
			for (unsigned int i = 0; i < 4; i++)
				chat_interface.RemoveNameFromUserList(i);
			return false;
		}break;

	default:
		break;
	}

	delete[] buffer;
	return true;
}
bool TCPChatClient::send_message(std::string message)
{
	uint16_t length = message.length()+3;
	char* buffer=new char[length+2];
	memcpy(buffer, &length, sizeof(uint16_t));
	buffer[2] = NET_MESSAGE_TYPE::sv_cl_msg;
	buffer[3] = myId;
	strcpy(&buffer[4], message.c_str());
	int result = send(comSocket, buffer, length+2, 0);
	delete[] buffer;
	if (result <= 0)
	{
		chat_interface.DisplayString("CLIENT:  MESSAGE SENDING ERROR");
		return false;
	}
	return true;
}
bool TCPChatClient::stop(void)
{
	uint16_t len = 2;
	char closebuffer[4];
	memcpy(closebuffer, &len, 2);
	closebuffer[2] = NET_MESSAGE_TYPE::sv_cl_close;
	closebuffer[3] = myId;
	int result = send(comSocket, closebuffer, 4, 0);
	if (result <= 0)
	{
		int error = WSAGetLastError();
		return false;
	}
	shutdown(comSocket, SD_BOTH);
	closesocket(comSocket);
	return true;
}
