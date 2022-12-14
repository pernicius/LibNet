#pragma once

#include "NetCommon.h"

#include "NetConnection.h"
#include "NetMessage.h"
#include "NetMsgQueue.h"


namespace NETLIB_NAMESPACE {


	class Server
	{
	public:
		Server()
			: m_asioAcceptor(m_asioContext)
		{
		}


		~Server()
		{
			if (m_IsListening)
				Stop();
		}


		// Starts the server, listening on the specified port and optional address
		bool Start(uint16_t port, const std::string& ip = {})
		{
			std::string addr;
			try
			{
				// Setup asio acceptor
				asio::ip::tcp::endpoint ep(ip.empty() ? asio::ip::address_v6::any() : asio::ip::address::from_string(ip), port);
				addr = ep.address().to_string();
				m_asioAcceptor.open(ep.protocol());
				m_asioAcceptor.bind(ep);
				m_asioAcceptor.listen();

				// Issue a task to the asio context - This is important as it will
				// prime the context with "work", and stop it from exiting immediately.
				// Since this is a server, we want it primed ready to handle clients
				// trying to connect.
				ASYNC_WaitForConnection();

				// Launch the asio context in its own thread
				m_threadContext = std::thread([this]() { m_asioContext.run(); });
			}
			catch (std::exception& e)
			{
				// Error
				std::cerr << "[SERVER] Exception: " << e.what() << std::endl;
				return false;
			}
			m_IsListening = true;
			// Log
			std::cout << "[SERVER] Started, listening on: " << addr << " : " << port << std::endl;
			return true;
		}


		void Stop()
		{
			// Request the context to close...
			m_asioContext.stop();
			// ...adnd wait for its thread to exit
			if (m_threadContext.joinable())
				m_threadContext.join();

			// Log
			std::cout << "[SERVER] Stopped!" << std::endl;
		}


		// Send a message to a single client
		void Send(std::shared_ptr<Connection> client, Message& msg)
		{
			// Check client is legitimate...
			if (client && client->IsConnected())
			{
				// ...and post the message via the connection
				client->Send(msg);
			}
			else
			{
				// If we cant communicate with client then we may as 
				// well remove the client - let the server know, it may
				// be tracking it somehow
				OnClientDisconnect(client);

				// Off you go now, bye bye!
				client.reset();

				// Then physically remove it from the container
				m_Connections.erase(
					std::remove(m_Connections.begin(), m_Connections.end(), client),
					m_Connections.end());
			}
		}


		// Send message to all clients
		void Broadcast(Message& msg, std::shared_ptr<Connection> clientIgnore = nullptr)
		{
			bool bInvalidClientExists = false;

			// Iterate through all clients in container
			for (auto& client : m_Connections)
			{
				// Check client is connected...
				if (client && client->IsConnected())
				{
					// ..it is!
					if (client != clientIgnore)
						client->Send(msg);
				}
				else
				{
					// The client couldnt be contacted, so assume it has
					// disconnected.
					OnClientDisconnect(client);
					client.reset();

					// Set this flag to then remove dead clients from container
					bInvalidClientExists = true;
				}
			}

			// Remove dead clients, all in one go - this way, we dont invalidate the
			// container as we iterated through it.
			if (bInvalidClientExists)
				m_Connections.erase(
					std::remove(m_Connections.begin(), m_Connections.end(), nullptr),
					m_Connections.end());
		}


		void DisconnectClient(std::shared_ptr<Connection> client)
		{
			// Inform app and reset pointer
			OnClientDisconnect(client);
			client.reset();
			// Remove the client
			m_Connections.erase(
				std::remove(m_Connections.begin(), m_Connections.end(), nullptr),
				m_Connections.end());
		}


		// Process incoming messages
		void Update(bool wait)
		{
			Update(-1, wait);
		}


		// Process incoming messages
		void Update(size_t nMaxMessages = -1, bool wait = false)
		{
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

			// Search for dead clients...
			UpdateDeadClients();
		}


		// Search for and remove dead clients
		void UpdateDeadClients()
		{
			// Search for dead clients...
			bool bInvalidClientExists = false;
			for (auto& client : m_Connections)
			{
				// Check client is disconnected
				if (!client || !client->IsConnected())
				{
					// Inform app and reset pointer
					OnClientDisconnect(client);
					client.reset();
					// Set this flag to then remove dead clients from container
					bInvalidClientExists = true;
				}
			}
			// Remove dead clients, all in one go
			// This way, we dont invalidate the container as we iterated through it.
			if (bInvalidClientExists)
				m_Connections.erase(
					std::remove(m_Connections.begin(), m_Connections.end(), nullptr),
					m_Connections.end());
		}


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
		void ASYNC_WaitForConnection()
		{
			// Prime context with an instruction to wait until a socket connects. This
			// is the purpose of an "acceptor" object. It will provide a unique socket
			// for each incoming connection attempt
			m_asioAcceptor.async_accept(
				[this](std::error_code ec, asio::ip::tcp::socket socket)
				{
					// Triggered by incoming connection request
					if (!ec)
					{
						// Display some useful(?) information
						std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << std::endl;
						uint16_t port = socket.remote_endpoint().port();

						// Create a new connection to handle this client 
						std::shared_ptr<Connection> newconn =
							std::make_shared<Connection>(true, m_asioContext, std::move(socket), m_MessagesIn, port);

						// Give the user server a chance to deny connection
						if (OnClientConnect(newconn))
						{
							// Connection allowed, so add to container of new connections
							m_Connections.push_back(std::move(newconn));

							// And very important! Issue a task to the connection's
							// asio context to sit and wait for bytes to arrive!
							m_Connections.back()->ConnectToClient(port);

							std::cout << "[" << port << "] Connection Approved" << std::endl;
						}
						else
						{
							std::cout << "[" << port << "] Connection Denied" << std::endl;

							// Connection will go out of scope with no pending tasks, so will
							// get destroyed automagically due to the wonder of smart pointers
						}
					}
					else
					{
						// Error has occurred during acceptance
						std::cout << "[SERVER] New Connection Error: " << ec.message() << std::endl;
					}

					// Prime the asio context with more work
					// again simply wait for another connection...
					ASYNC_WaitForConnection();
				});
		}


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
