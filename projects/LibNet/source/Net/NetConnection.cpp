#include "Net/NetConnection.h"


namespace Net {


	Connection::Connection(bool server, asio::io_context& asioContext, asio::ip::tcp::socket socket, MsgQueue& qIn, uint16_t port)
		: m_asioContext(asioContext), m_socket(std::move(socket)), m_MessagesIn(qIn), m_IsServer(server), m_port(port)
	{
	}


	uint16_t Connection::GetID() const
	{
		return GetPort();
	}


	std::string Connection::GetAddress() const
	{
		return m_socket.remote_endpoint().address().to_string();
	}


	uint16_t Connection::GetPort() const
	{
		return m_port;
	}


	void Connection::ConnectToClient(uint16_t port)
	{
		// Only servers can connect to clients
		if (m_IsServer)
		{
			if (m_socket.is_open())
			{
				ASYNC_ReadHeader();
			}
		}
	}


	void Connection::ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints)
	{
		// Only clients can connect to servers
		if (!m_IsServer)
		{
			// Request asio attempts to connect to an endpoint
			asio::async_connect(m_socket, endpoints,
				[this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
				{
					if (!ec)
					{
						std::cout << "Connect to server succesfully!" << std::endl;
						ASYNC_ReadHeader();
					}
					else
					{
						std::cout << "Connection error: " << ec.message() << std::endl;
						m_socket.close();
					}
				});
		}
	}


	void Connection::Disconnect()
	{
		if (IsConnected())
			asio::post(m_asioContext, [this]() { m_socket.close(); });
	}


	bool Connection::IsConnected() const
	{
		return m_socket.is_open();
	}


	void Connection::Send(Message& msg)
	{
		msg.UpdateCRC();
		ASYNC_Send(msg);
	}


	void Connection::ASYNC_Send(const Message& msg)
	{
		asio::post(m_asioContext,
			[this, msg]()
			{
				// If the queue has a message in it, then we must assume that it is in the
				// process of asynchronously being written. Either way add the message to
				// the queue to be output. If no messages were available to be written,
				// then start the process of writing the message at the front of the queue.
				bool bWritingMessage = !m_MessagesOut.IsEmpty();
				m_MessagesOut.PushBack(msg);
				if (!bWritingMessage)
				{
					ASYNC_WriteHeader();
				}
			});
	}


	void Connection::ASYNC_WriteHeader()
	{
		// If this function is called, we know the outgoing message queue must have 
		// at least one message to send. So allocate a transmission buffer to hold
		// the message, and issue the work - asio, send these bytes
		asio::async_write(m_socket, asio::buffer(&m_MessagesOut.GetFront().header, sizeof(message_header)),
			[this](std::error_code ec, std::size_t length)
			{
				// asio has now sent the bytes - if there was a problem
				// an error would be available...
				if (!ec)
				{
					// ... no error, so check if the message header just sent
					// also has a message body...
					if (m_MessagesOut.GetFront().body.size() > 0)
					{
						// ...it does, so issue the task to write the body bytes
						ASYNC_WriteBody();
					}
					else
					{
						// ...it didnt, so we are done with this message. Remove it from 
						// the outgoing message queue
						m_MessagesOut.PopFront();

						// If the queue is not empty, there are more messages to send, so
						// make this happen by issuing the task to send the next header.
						if (!m_MessagesOut.IsEmpty())
						{
							ASYNC_WriteHeader();
						}
					}
				}
				else
				{
					// ...asio failed to write the message, we could analyse why but 
					// for now simply assume the connection has died by closing the
					// socket. When a future attempt to write to this client fails due
					// to the closed socket, it will be tidied up.
					std::cout << "[" << GetID() << "] WriteHeader() Failed: " << ec.message() << std::endl;
					m_socket.close();
				}
			});
	}


	void Connection::ASYNC_WriteBody()
	{
		// If this function is called, a header has just been sent, and that header
		// indicated a body existed for this message. Fill a transmission buffer
		// with the body data, and send it!
		asio::async_write(m_socket, asio::buffer(m_MessagesOut.GetFront().body.data(), m_MessagesOut.GetFront().body.size()),
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					// Sending was successful, so we are done with the message
					// and remove it from the queue
					m_MessagesOut.PopFront();

					// If the queue still has messages in it, then issue the task to 
					// send the next messages' header.
					if (!m_MessagesOut.IsEmpty())
					{
						ASYNC_WriteHeader();
					}
				}
				else
				{
					// Sending failed, see WriteHeader() equivalent for description :P
					std::cout << "[" << GetID() << "] WriteBody() Failed: " << ec.message() << std::endl;
					m_socket.close();
				}
			});
	}


	void Connection::ASYNC_ReadHeader()
	{
		// If this function is called, we are expecting asio to wait until it receives
		// enough bytes to form a header of a message. We know the headers are a fixed
		// size, so allocate a transmission buffer large enough to store it. In fact, 
		// we will construct the message in a "temporary" message object as it's 
		// convenient to work with.
		asio::async_read(m_socket, asio::buffer(&m_msgTemporaryIn.header, sizeof(message_header)),
			[this](std::error_code ec, std::size_t length)
			{
				// check error-flag and checksum
				if (!ec && m_msgTemporaryIn.IsHeaderValid())
				{
					// Check if this message has a body to follow...
					if (m_msgTemporaryIn.header.size > 0)
					{
						// it does, so allocate enough space in the messages' body
						// vector, and issue asio with the task to read the body.
						m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size);
						ASYNC_ReadBody();
					}
					else
					{
						// it doesn't, so add this bodyless message to
						// the connections incoming message queue
						AddToIncomingMessageQueue();
					}
				}
				else
				{
					// Reading form the client went wrong, most likely a disconnect
					// has occurred. Close the socket and let the system tidy it up later.
					if (ec)
						std::cout << "[" << GetID() << "] ReadHeader() Failed: " << ec.message() << std::endl;
					else
						std::cout << "[" << GetID() << "] ReadHeader() Failed: Incorrect checksum." << std::endl;
					m_socket.close();
				}
			});
	}


	void Connection::ASYNC_ReadBody()
	{
		// If this function is called, a header has already been read, and that header
		// request we read a body. The space for that body has already been allocated
		// in the temporary message object, so just wait for the bytes to arrive...
		asio::async_read(m_socket, asio::buffer(m_msgTemporaryIn.body.data(), m_msgTemporaryIn.body.size()),
			[this](std::error_code ec, std::size_t length)
			{
				// check error-flag and checksum
				if (!ec && m_msgTemporaryIn.IsBodyValid())
				{
					// The message is now complete, so add
					// the whole message to incoming queue
					AddToIncomingMessageQueue();
				}
				else
				{
					// Reading form the client went wrong, most likely a disconnect
					// has occurred. Close the socket and let the system tidy it up later.
					if (ec)
						std::cout << "[" << GetID() << "] ReadBody() Failed: " << ec.message() << std::endl;
					else
						std::cout << "[" << GetID() << "] ReadBody() Failed: Incorrect checksum." << std::endl;
					m_socket.close();
				}
			});
	}


	void Connection::AddToIncomingMessageQueue()
	{
		// Shove it in queue, converting it to an "owned message", by initialising
		// with the a shared pointer from this connection object
		if (m_IsServer)
			m_msgTemporaryIn.remote = this->shared_from_this();
		m_MessagesIn.PushBack(m_msgTemporaryIn);

		// We must now prime the asio context to receive the next message. It 
		// wil just sit and wait for bytes to arrive, and the message construction
		// process repeats itself. Clever huh?
		ASYNC_ReadHeader();
	}


} // namespace Net
