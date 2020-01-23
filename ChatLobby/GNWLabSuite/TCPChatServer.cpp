#include "TCPChatServer.h"

TCPChatServer::TCPChatServer(ChatLobby& chat_int) : chat_interface(chat_int)
{

}
TCPChatServer::~TCPChatServer(void)
{

}
bool TCPChatServer::init(uint16_t port)
{
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
	{
		chat_interface.DisplayString("SERVER: LISTEN SOCKET CREATION FAILED");
		return false;
	}
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);
	int result = bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (result == INVALID_SOCKET)
	{
		chat_interface.DisplayString("SERVER: BINDING FAILED");
		return false;
	}
	result = listen(listenSocket, 5);
	if (result == INVALID_SOCKET)
	{
		chat_interface.DisplayString("SERVER: LISTENING FAILED");
		return false;
	}
	chat_interface.DisplayString("SERVER: NOW HOSTING SERVER");
	FD_ZERO(&masterSet);
	FD_SET(listenSocket, &masterSet);
	timeout.tv_sec = 3;
	timeout.tv_usec = 500;
	
	return true;
}
bool TCPChatServer::run(void)
{
	FD_ZERO(&readSet);
	readSet = masterSet;
	numReady = select(0, &readSet, NULL, NULL, &timeout);
	if (FD_ISSET(listenSocket, &readSet))
	{

		for (size_t i = 0; i <= MAX_CLIENTS; i++)
		{
			if (comSocket[i] == NULL)
			{
				comSocket[i] = accept(listenSocket, NULL, NULL);
				if (comSocket[i] == SOCKET_ERROR)
				{
					chat_interface.DisplayString("SERVER: ACCEPTING ERROR");
					return false;
				}
				
				FD_SET(comSocket[i], &masterSet);
				break;
			}
		}
	}
	for (uint8_t i = 0; i <= MAX_CLIENTS; i++)
	{
		if (FD_ISSET(comSocket[i], &readSet))
		{
			uint16_t msgSize = 0;

			int  result = tcp_recv_whole(comSocket[i], (char*)&msgSize, 2, 0);
			if (result <= 0)
			{
				chat_interface.DisplayString("SERVER: BAD STARTING MESSAGE");
				return false;
			}
			char* buffer = new char[msgSize];

			result = tcp_recv_whole(comSocket[i], buffer, msgSize, 0);
			if (result <= 0)
			{
				chat_interface.DisplayString("SERVER: BAD MESSAGE RECEIVED");
				return false;
			}
			switch (buffer[0])
			{
			case cl_reg:
			{

				if (i == MAX_CLIENTS)
				{
					char bufferfull[3];
					uint16_t len = 1;
					memcpy(bufferfull, &len, sizeof(uint16_t));
					bufferfull[2] = sv_full;
					result = send(comSocket[i], bufferfull, 3, 0);
					if (result <= 0)
					{
						chat_interface.DisplayString("SERVER: BAD SENDING MESSAGE");
					}
					comSocket[i] = NULL;
					FD_CLR(comSocket[i],&masterSet);
					
				}
				else
				{
				char name[17];
				strcpy(name, &buffer[1]);
				myNameList[i]=name;
				//chat_interface.AddNameToUserList(name, index);

				char buff[4];
				uint16_t len = 2;
				memcpy(buff,&len,sizeof(uint16_t));
				buff[2] = sv_cnt;
				buff[3] = i;
				result = send(comSocket[i], buff, 4, 0);
				if (result <= 0)
				{
					chat_interface.DisplayString("SERVER: BAD SV_CNT MESSAGE");
				}
					for (size_t j = 0; j < MAX_CLIENTS; ++j)
					{
					if ((j != i)&&(comSocket[j]!=NULL))
					{
						//Notify other clients
						char add[21];
						uint16_t length = 19;
						memcpy(add, &length, sizeof(uint16_t));
						add[2] = sv_add;
						add[3] = i;
						strcpy(&add[4],name);
						result = send(comSocket[j], add, 21, 0);
						if (result == SOCKET_ERROR)
						{
							chat_interface.DisplayString("SERVER: BAD SV_ADD MESSAGE");
						}
					}
					}
				}
			}break;

			case cl_get:
			{
				uint8_t numInList = myNameList.size();
				uint16_t len = numInList * 18 + 2;
				char* listBuff = new char[len + 2];
				memcpy(listBuff, &len, sizeof(uint16_t));
				listBuff[2] = sv_list;
				listBuff[3] = numInList;
				for (uint8_t j = 0; j < numInList; j++)
				{
					if(comSocket[j]!=NULL) //still connected
					{
						memcpy(&listBuff[4+j*18],&j,1);//ID
						memcpy(&listBuff[5+j*18],myNameList[j].c_str(),17);
					}
				}
				result = send(comSocket[i],listBuff,len+2,0);
				if (result == SOCKET_ERROR)
				{
					chat_interface.DisplayString("SERVER: BAD LIST");
				}
				delete[] listBuff;
			}break;

			case sv_cl_msg:
			{
				char* msgBuffer = new char[msgSize+2];
				memcpy(msgBuffer, &msgSize, 2);
				memcpy(msgBuffer+2,buffer,msgSize);

				
				for (size_t j = 0; j < MAX_CLIENTS; j++)
				{
					if (comSocket[j]!=NULL)
					{
						result = send(comSocket[j], msgBuffer, msgSize + 2, 0);
						if (result == SOCKET_ERROR)
						{
							chat_interface.DisplayString("SERVER: CAN'T SEND THE MESSAGE");
							//return false;
						}
					}
				}
				delete[] msgBuffer;
			}break;
			case sv_cl_close:
			{
				uint16_t length = 2;
				char closeBuff[4];
				memcpy(closeBuff,&length,sizeof(uint16_t));
				closeBuff[2] = sv_remove;
				closeBuff[3] = buffer[1]; ///Get the ID
				//chat_interface.RemoveNameFromUserList(buffer[1]);
				for (size_t j = 0; j < MAX_CLIENTS; j++)
				{
					if ((i != j) && (comSocket[j] != NULL))
						result = send(comSocket[j], closeBuff, 4, 0);
					if (result == SOCKET_ERROR)
					{
						chat_interface.DisplayString("SERVER: BAD REMOVE MESSAGE");
						return false;
					}
				}
				FD_CLR(comSocket[i],&masterSet);
				FD_CLR(comSocket[i], &readSet);
				comSocket[i] = NULL;

			}break;

			default:
				break;
			}

			delete[] buffer;
		}
	}
	return true;
}
bool TCPChatServer::stop(void)
{
	char buffer[3];
	uint16_t length = 1;
	memcpy(buffer, (char*)&length, sizeof(uint16_t));

	buffer[2] = sv_cl_close;

	for (size_t i = 0; i <= MAX_CLIENTS; i++)
	{
		if (comSocket[i]!=NULL)
		{
			if (send(comSocket[i], buffer, 3, 0)<=0)
			{
				chat_interface.DisplayString("SERVER: FAILED SENDING CLOSE_MSG TO CLIENTS");
				return false;
			}
			
			shutdown(comSocket[i], SD_BOTH);
			closesocket(comSocket[i]);
			comSocket[i] = NULL;
		}
	}
	
	FD_ZERO(&masterSet);
	shutdown(listenSocket, SD_BOTH);
	closesocket(listenSocket);
	listenSocket = NULL;
	myNameList.clear();
	return true;
}