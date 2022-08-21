#pragma once

#include "Net/NetCommon.h"

#include "Net/NetConnection.h"
#include "Net/NetMessage.h"
#include "Net/NetMsgQueue.h"


namespace Net {


	class Client
	{
	public:
		Client() {}
		~Client();

	public:
		// Connect to server with hostname/ip-address and port
		bool Connect(const std::string& host, const uint16_t port);

		// Disconnect from server
		void Disconnect();

		// Check if client is actually connected to a server
		bool IsConnected();

		// Send message to server
		void Send(Message& msg);

//		virtual bool OnConnect() {}
//		virtual void OnDisconnect() {}
		virtual void OnMessage(Message& msg) {}

		// Process incoming messages
		void Update(size_t nMaxMessages = -1, bool wait = false);

	private:
		// Thread safe queue for incoming message packets
		MsgQueue m_MessagesIn;

		// The single connection instance
		std::unique_ptr<Connection> m_Connection;

		// asio context handles the data transfer...
		asio::io_context m_asioContext;
		// ...but needs a thread of its own to execute its work commands
		std::thread threadContext;
	};


} // namespace Net
