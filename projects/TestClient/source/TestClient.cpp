#include "Net/NetClient.h"


enum MsgTypes : uint32_t
{
	None = 0,      // not allowed!

	ServerAccept,  // server -> client
	ServerDeny,    // server -> client
	ServerPing,    // bidirectional
	MessageAll,    // client -> server
	ServerMessage, // server -> client

	Uninitialized = ((uint32_t)~((uint32_t)0))
};


class MyClient : public Net::Client
{
public:
	void OnServerAccept(Net::Message& msg)
	{
		std::cout << "OnServerAccept() - Server accepted connection" << std::endl;
	}


	void OnServerDeny(Net::Message& msg)
	{
		std::cout << "OnServerDeny()" << std::endl;
	}


	void OnServerPing(Net::Message& msg)
	{
		std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
		std::chrono::system_clock::time_point timeThen;
		msg >> timeThen;
		std::cout << "OnServerPing() - Ping: " << std::chrono::duration_cast<std::chrono::microseconds>(timeNow - timeThen).count() * 0.001f << "ms" << std::endl;
	}


	void OnServerMessage(Net::Message& msg)
	{
		uint16_t clientID;
		msg >> clientID;
		std::cout << "OnServerMessage() - From: " << clientID << std::endl;
	}


	virtual void OnMessage(Net::Message& msg)
	{
		switch (msg.header.type)
		{
		case ServerAccept:  OnServerAccept(msg);  return;
		case ServerDeny:    OnServerDeny(msg);    return;
		case ServerPing:    OnServerPing(msg);    return;
		case ServerMessage: OnServerMessage(msg); return;
		}
		std::cout << "OnMessage() type: " << msg.header.type << " - Unknown!!!" << std::endl;
	}


	void PingServer()
	{
		Net::Message msg;
		msg.header.type = MsgTypes::ServerPing;
		std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
		msg << timeNow;
		Send(msg);
	}


	void MessageAll()
	{
		Net::Message msg;
		msg.header.type = MsgTypes::MessageAll;
		Send(msg);
	}
};


int main()
{
	MyClient myClient;
	if (!myClient.Connect("127.0.0.1", 60000))
	{
		std::cout << "Cant connect to server!" << std::endl;
		return -1;
	}

	bool key[3] = { false, false, false };
	bool old_key[3] = { false, false, false };

	bool bQuit = false;
	while (!bQuit)
	{
		if (!myClient.IsConnected())
		{
			std::cout << "Server down!" << std::endl;
			return -1;
		}
		myClient.Update();

		//     WINDOWS ONLY !!!
		//     WINDOWS ONLY !!!
		// vvv WINDOWS ONLY !!!
		if (GetForegroundWindow() == GetConsoleWindow())
		{
			key[0] = GetAsyncKeyState('1') & 0x8000;
			key[1] = GetAsyncKeyState('2') & 0x8000;
			key[2] = GetAsyncKeyState('3') & 0x8000;
		}
		// ^^^ WINDOWS ONLY !!!
		//     WINDOWS ONLY !!!
		//     WINDOWS ONLY !!!

		if (key[0] && !old_key[0]) myClient.PingServer();
		if (key[1] && !old_key[1]) myClient.MessageAll();
		if (key[2] && !old_key[2]) bQuit = true;
		for (int i = 0; i < 3; i++)
			old_key[i] = key[i];
	}
}
