#include "Net/NetServer.h"


namespace Net {


	Server::Server()
		: m_asioAcceptor(m_asioContext)
	{
	}


	Server::~Server()
	{
		if(m_IsListening)
			Stop();
	}


	bool Server::Start(uint16_t port)
	{
		try
		{
			// Setup asio acceptor
			asio::ip::tcp::endpoint ep(asio::ip::tcp::v6(), port);
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
		std::cout << "[SERVER] Started!" << std::endl;
		return true;
	}


	void Server::Stop()
	{
		// Request the context to close...
		m_asioContext.stop();
		// ...adnd wait for its thread to exit
		if (m_threadContext.joinable())
			m_threadContext.join();

		// Log
		std::cout << "[SERVER] Stopped!" << std::endl;
	}


	void Server::Send(std::shared_ptr<Connection> client, Message& msg)
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


	void Server::Broadcast(Message& msg, std::shared_ptr<Connection> clientIgnore)
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


	void Server::DisconnectClient(std::shared_ptr<Connection> client)
	{
		// Inform app and reset pointer
		OnClientDisconnect(client);
		client.reset();
		// Remove the client
		m_Connections.erase(
			std::remove(m_Connections.begin(), m_Connections.end(), nullptr),
			m_Connections.end());
	}


	void Server::Update(bool wait)
	{
		Update(-1, wait);
	}
	void Server::Update(size_t nMaxMessages, bool wait)
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


	void Server::UpdateDeadClients()
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


	void Server::ASYNC_WaitForConnection()
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


} // netspace Net
