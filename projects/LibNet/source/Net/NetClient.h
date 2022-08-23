#pragma once

#include "NetCommon.h"

#include "NetConnection.h"
#include "NetMessage.h"
#include "NetMsgQueue.h"


namespace NETLIB_NAMESPACE {


	class Client
	{
	public:
		Client() {}
		~Client()
		{
			// If the client is destroyed, always try and disconnect from server
			Disconnect();
		}


	public:
		// Connect to server with hostname/ip-address and port
		bool Connect(const std::string& host, const uint16_t port)
		{
			try
			{
				// Resolve hostname/ip-address into tangiable physical address
				asio::ip::tcp::resolver resolver(m_asioContext);
				asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));
				uint16_t port = endpoints->endpoint().port();

				// Create connection
				m_Connection = std::make_unique<Connection>(false, m_asioContext, asio::ip::tcp::socket(m_asioContext), m_MessagesIn, port);

				// Tell the connection object to connect to server
				m_Connection->ConnectToServer(endpoints);

				// Start context thread
				threadContext = std::thread([this]() { m_asioContext.run(); });
			}
			catch (std::exception& e)
			{
				std::cerr << "Client Exception: " << e.what() << std::endl;
				return false;
			}
			return true;
		}


		// Disconnect from server
		void Disconnect()
		{
			// If connection exists, and it's connected then...
			if (IsConnected())
			{
				// ...disconnect from server gracefully
				m_Connection->Disconnect();
			}

			// Either way, we're also done with the asio context...				
			m_asioContext.stop();
			// ...and its thread
			if (threadContext.joinable())
				threadContext.join();

			// Destroy the connection object
			m_Connection.release();
		}


		// Check if client is actually connected to a server
		bool IsConnected()
		{
			if (m_Connection)
				return m_Connection->IsConnected();
			else
				return false;
		}

		// Send message to server
		void Send(Message& msg)
		{
			if (IsConnected())
				m_Connection->Send(msg);
		}

//		virtual bool OnConnect() {}
//		virtual void OnDisconnect() {}
		virtual void OnMessage(Message& msg) {}

		// Process incoming messages
		void Update(size_t nMaxMessages = -1, bool wait = false)
		{
			if (!IsConnected())
				return;

			if (wait) m_MessagesIn.Wait();

			// Process as many messages as we can up to the value specified
			size_t nMessageCount = 0;
			while (nMessageCount < nMaxMessages && !m_MessagesIn.IsEmpty())
			{
				// Grab the front message
				auto msg = m_MessagesIn.PopFront();

				// Pass to message handler
				OnMessage(msg);

				nMessageCount++;
			}
		}

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
