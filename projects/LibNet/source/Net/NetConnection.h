#pragma once

#include "Net/NetCommon.h"

#include "Net/NetMsgQueue.h"
//#include "Net/NetServer.h"


namespace Net {


	// Forward declaration because Server depends itself on Connection
	class Server;


	class Connection : public std::enable_shared_from_this<Connection>
	{
	public:
		Connection(bool server, asio::io_context& asioContext, asio::ip::tcp::socket socket, MsgQueue& qIn, uint16_t port);
		virtual ~Connection() {}
	public:
		uint16_t GetID() const;
		std::string GetAddress() const;
		uint16_t GetPort() const;

		void ConnectToClient(uint16_t port = 0);
		void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints);
		void Disconnect();
		bool IsConnected() const;
		// Send a message
		void Send(Message& msg);

	private:
		// ASYNC - Send a message, connections are one-to-one so no need to specifiy
		// the target, for a client, the target is the server and vice versa
		void ASYNC_Send(const Message& msg);

		// ASYNC - Prime context to write a message header
		void ASYNC_WriteHeader();

		// ASYNC - Prime context to write a message body
		void ASYNC_WriteBody();

		// ASYNC - Prime context ready to read a message header
		void ASYNC_ReadHeader();

		// ASYNC - Prime context ready to read a message body
		void ASYNC_ReadBody();

		// Once a full message is received, add it to the incoming queue
		void AddToIncomingMessageQueue();

	protected:
		// Each connection has a unique socket to a remote 
		asio::ip::tcp::socket m_socket;

		// This context is shared with the whole asio instance
		asio::io_context& m_asioContext;

		// This queue holds all messages to be sent to the remote side
		// of this connection
		MsgQueue m_MessagesOut;

		// This references the incoming queue of the parent object
		MsgQueue& m_MessagesIn;

		// Incoming messages are constructed asynchronously, so we will
		// store the part assembled message here, until it is ready
		Message m_msgTemporaryIn;

		// Decides how some of the connection behaves
		bool m_IsServer;

		uint16_t m_port = 0;
	};


} // namespace Net
