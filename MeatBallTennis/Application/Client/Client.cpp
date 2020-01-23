// Client.cpp : handles all client network functions.
#include "Client.h"
#include "../NetworkMessage.h"

static int snapShotCount = 0;

// Initializes the client; connects to the server.
int Client::init(char* address, uint16_t port, uint8_t _player)
{

	// TODO:
	//       1) Set the player.
	//       2) Set up the connection.
	//       3) Connect to the server.
	//       4) Get response from server.
	//       5) Make sure to mark the client as running.
	seqNum = 0;
	state.player0.keyUp = state.player0.keyDown = false;
	state.player1.keyUp = state.player1.keyDown = false;
	state.gamePhase = WAITING;
	player = static_cast<int>(_player);
	clSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (clSocket == INVALID_SOCKET)
	{
		return SETUP_ERROR;
	}
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr(address);
	serverAddr.sin_port = htons(port);
	if (serverAddr.sin_addr.S_un.S_addr == INADDR_NONE)
	{
		return ADDRESS_ERROR;
	}
	
	if (connect(clSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		return DISCONNECT;
	}
	NetworkMessage msg_Out = NetworkMessage(IO::_OUTPUT);
	NetworkMessage msg_In = NetworkMessage(IO::_INPUT);
	uint8_t type = CL_CONNECT;
	msg_Out.writeByte(type);
	msg_Out.writeByte(_player);
	sendNetMessage(clSocket, msg_Out);
	recvNetMessage(clSocket,msg_In);
	uint16_t Sequencing=msg_In.readShort();

	if (Sequencing>seqNum )
	{
		seqNum = Sequencing;
		uint8_t msg = msg_In.readByte();
		if (msg == SV_FULL)
		{
			active = false;
			shutdown(clSocket,SD_BOTH);
			closesocket(clSocket);
			return SHUTDOWN;
		}
		
		else if (msg == SV_OKAY)
		{
			active = true;
			state.gamePhase = RUNNING;
		}
	}


	return SUCCESS;
}

// Receive and process messages from the server.
int Client::run()
{	// TODO: Continuously process messages from the server aslong as the client running.
	// HINT: Set game phase to DISCONNECTED on SV_CL_CLOSE! (Try calling stop().)
	// HINT: You can keep track of the number of snapshots with a static variable...
	NetworkMessage msg_IN = NetworkMessage(IO::_INPUT);
	while (active==true&&state.gamePhase!=DISCONNECTED)
	{
		msg_IN.reset(IO::_INPUT);
		if (recvNetMessage(clSocket, msg_IN) <= 0)
		{
			//Bad Message Received
			if(active == true)
			return DISCONNECT;
			else
			return SHUTDOWN;
		}
		if (msg_IN.getLength() > 0)
		{
			uint16_t Sequencing = msg_IN.readShort();
			if (Sequencing > seqNum)
			{ 
				seqNum = Sequencing;

				uint8_t recv_Msg=msg_IN.readByte();
				if (recv_Msg == SV_CL_CLOSE)
				{
					stop();
				}
				else if (recv_Msg == SV_SNAPSHOT)
				{
					snapShotCount++;
					state.gamePhase = msg_IN.readByte();
					state.ballX = msg_IN.readShort();
					state.ballY = msg_IN.readShort();
					state.player0.y = msg_IN.readShort();
					state.player0.score = msg_IN.readShort();
					state.player1.y = msg_IN.readShort();
					state.player1.score = msg_IN.readShort();

					if (snapShotCount % 10 == 0)
					{
						sendAlive();
					}
				}
			}
			else
			{
				return MESSAGE_ERROR;
			}
		}
	}

	return DISCONNECT;
}

// Clean up and shut down the client.
void Client::stop()
{
	// TODO:
	//       1) Make sure to send a SV_CL_CLOSE message.
	//       2) Make sure to mark the client as shutting down and close socket.
	//       3) Set the game phase to DISCONNECTED.
	sendClose();
	active = false;
	shutdown(clSocket, SD_BOTH);
	closesocket(clSocket);
	state.gamePhase = DISCONNECTED;
}

// Send the player's input to the server.
int Client::sendInput(int8_t keyUp, int8_t keyDown, int8_t keyQuit)
{
	if (keyQuit)
	{
		stop();
		return SHUTDOWN;
	}

	cs.enter();
	if (player == 0)
	{
		state.player0.keyUp = keyUp;
		state.player0.keyDown = keyDown;
	}
	else
	{
		state.player1.keyUp = keyUp;
		state.player1.keyDown = keyDown;
	}
	cs.leave();

	//TODO:	Transmit the player's input status.
	NetworkMessage msg_Out = NetworkMessage(IO::_OUTPUT);
	uint8_t type = CL_KEYS;
	msg_Out.writeByte(type);
	if (player == 0)
	{
		msg_Out.writeByte(state.player0.keyUp);
		msg_Out.writeByte(state.player0.keyDown);
	}
	else
	{
		msg_Out.writeByte(state.player1.keyUp);
		msg_Out.writeByte(state.player1.keyDown);
	}
	sendNetMessage(clSocket,msg_Out);
	return SUCCESS;
}

// Copies the current state into the struct pointed to by target.
void Client::getState(GameState* target)
{
	// TODO: Copy state into target.
	memcpy(target,&state,sizeof(GameState));

}

// Sends a SV_CL_CLOSE message to the server (private, suggested)
void Client::sendClose()
{
	// TODO: Send a CL_CLOSE message to the server.
	NetworkMessage msg_Out = NetworkMessage(IO::_OUTPUT);
	uint8_t type = SV_CL_CLOSE;
	msg_Out.writeByte(type);
	sendNetMessage(clSocket, msg_Out);

}

// Sends a CL_ALIVE message to the server (private, suggested)
int Client::sendAlive()
{
	// TODO: Send a CL_ALIVE message to the server.
	NetworkMessage msg_Out = NetworkMessage(IO::_OUTPUT);
	uint8_t type = CL_ALIVE;
	msg_Out.writeByte(type);
	sendNetMessage(clSocket, msg_Out);
	return SUCCESS;
}
