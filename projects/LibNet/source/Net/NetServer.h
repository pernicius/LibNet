#pragma once

#include "NetCommon.h"

#include "Net/NetConnection.h"
#include "Net/NetMessage.h"
#include "Net/NetMsgQueue.h"


namespace Net {


	class Server
	{
	public:
		Server();
		~Server();

		// Starts the server, listening on the specified port
		bool Start(uint16_t port);

		void Stop();

		// Send a message to a single client
		void Send(std::shared_ptr<Connection> client, Message& msg);
		// Send message to all clients
		void Broadcast(Message& msg, std::shared_ptr<Connection> clientIgnore = nullptr);

		void DisconnectClient(std::shared_ptr<Connection> client);

		// Process incoming messages
		void Update(bool wait);
		// Process incoming messages
		void Update(size_t nMaxMessages = -1, bool wait = false);
		// Search for and remove dead clients
		void UpdateDeadClients();

	protected:
		// The server class should override thse functions
		// to implement customised functionality

		// Called when a client connects, you can veto the connection by returning false
		virtual bool OnClientConnect(std::shared_ptr<Connection> client) { return true; }

		// Called when a client appears to have disconnected
		virtual void OnClientDisconnect(std::shared_ptr<Connection> client) {}

		// Called when a message arrives
		virtual void OnMessage(Message& msg) { }

	private:
		// ASYNC - Instruct asio to wait for incomming connection
		void ASYNC_WaitForConnection();

	private:
		// Thread safe queue for incoming message packets
		MsgQueue m_MessagesIn;

		// Status of asio acceptor
		bool m_IsListening = false;

		// Container of active connections
		std::deque<std::shared_ptr<Connection>> m_Connections;

		// asio context handles the data transfer...
		asio::io_context m_asioContext;
		// ...but needs a thread of its own to execute its work commands
		std::thread m_threadContext;

		// Handles new incoming connection attempts
		asio::ip::tcp::acceptor m_asioAcceptor;
	};


} // namespace Net
