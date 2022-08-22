#include "Net/NetServer.h"


enum MsgTypes : uint16_t
{
	None = 0,      // not allowed!

	ServerAccept,  // server -> client
	ServerDeny,    // server -> client
	ServerPing,    // bidirectional
	MessageAll,    // client -> server
	ServerMessage, // server -> client

	Uninitialized = ((uint16_t)~((uint16_t)0))
};


class MyServer : public Net::Server
{
public:

	bool OnClientConnect(std::shared_ptr<Net::Connection> client) override
	{
		std::cout << "[" << client->GetID() << "] OnClientConnect()" << std::endl;

		// std::string id   = client->GetID()       // same value as GetPort()
		// std::string ip   = client->GetAddress()  // remote adress
		// uint16_t    port = client->GetPort()     // connected on port

		Net::Message msg;
		msg.header.type = MsgTypes::ServerAccept;
		Send(client, msg);

		return true;
	}


	void OnClientDisconnect(std::shared_ptr<Net::Connection> client) override
	{
		std::cout << "[" << client->GetID() << "] OnClientDisconnect()" << std::endl;
	}


	void OnServerPing(Net::Message& msg)
	{
		std::cout << "[" << msg.remote->GetID() << "] OnServerPing()" << std::endl;
		// Simply bounce message back
		Send(msg.remote, msg);
	}


	void OnMessageAll(Net::Message& msg)
	{
		std::cout << "[" << msg.remote->GetID() << "] OnMessageAll()" << std::endl;
		Net::Message msgOut;
		msgOut.header.type = ServerMessage;
		msgOut << msg.remote->GetID();
		Broadcast(msgOut, msg.remote);
	}


	void OnMessage(Net::Message& msg) override
	{
		switch (msg.header.type)
		{
		case ServerPing: OnServerPing(msg); return;
		case MessageAll: OnMessageAll(msg); return;
		}
		std::cout << "[" << msg.remote->GetID() << "] OnMessage() type: " << msg.header.type << " - Unknown!!!" << std::endl;
	}

};


int main()
{
	MyServer myServer;

	if (!myServer.Start(60000, "127.0.0.1"))
		return -1;

	bool bRun = true;
	while (bRun)
	{
		myServer.Update(true);
	}
}
