#pragma once

#include "Net/NetCommon.h"

//#include "Net/NetConnection.h"


namespace Net {


	class Connection;


	struct message_header
	{
		// App defined type of message
		uint32_t type = ((uint32_t)~((uint32_t)0));
		// Size of body storage (0 if header only)
		uint32_t size = 0;
		uint32_t crc_header = 0;
		uint32_t crc_body = 0;
	};


	class Message
	{
	public:
		Message() {}
		~Message() {}

	public:


		void UpdateCRC()
		{
			header.crc_header = 0;
			header.crc_header += header.type;
			header.crc_header += header.size;

			header.crc_body = 0;
			for (auto x : body)
			{
				header.crc_body += x;
			}
		}


		bool IsHeaderValid()
		{
			uint32_t check = 0;
			check += header.type;
			check += header.size;

			return (check == header.crc_header);
		}


		bool IsBodyValid()
		{
			if (!IsHeaderValid())
				return false;

			uint32_t check = 0;
			for (auto x : body)
			{
				check += x;
			}

			return (check == header.crc_body);
		}


		// Should be implemented by the application to push data onto the body storage
//		virtual void Serialize() {}
		// Should be implemented by the application to pop data from the body storage
//		virtual void Unserialize() {}

	public:
		// Convenience Operator overloads - These allow to add and remove stuff from
		// the body vector as if it were a stack, so First-In, Last-Out. These are a 
		// template in itself, because the data type the app is pushing or 
		// popping is unknown, so allow them all.
		//
		// NOTE: It assumes the data type is fundamentally Plain Old Data (POD).
		// TLDR: Serialise & Unserialise onto/from a vector
		//
		// Dataformat:
		//   On push: First pushes the data byten then pushes the count of data bytes
		//   On pull: First pull the cont of data bytes then the data bytes itself
		// 
		// NOTE: The count of data bytes is stored to allow a basic type validation


		static void PushSize(Message& msg, const size_t size)
		{
			// Cache current size of vector, as this will be the point we insert the data
			size_t i = msg.body.size();
			// Resize the vector by the size of sizevalue being pushed
			msg.body.resize(msg.body.size() + sizeof(uint32_t));
			// Physically copy the data into the newly allocated vector space
			std::memcpy(msg.body.data() + i, &size, sizeof(uint32_t));
			// Recalculate the message size
			msg.header.size = (uint32_t)msg.body.size();
		}


		static uint32_t PullSize(Message& msg)
		{
			uint32_t data_size;

			// Cache the location towards the end of the vector where the pulled data starts
			size_t i = msg.body.size() - sizeof(uint32_t);
			// Physically copy the data from the vector into the user variable
			std::memcpy(&data_size, msg.body.data() + i, sizeof(uint32_t));
			// Shrink the vector to remove read bytes, and reset end position
			msg.body.resize(i);
			// Recalculate the message size
			msg.header.size = (uint32_t)msg.body.size();

			return data_size;
		}


		// Pushes any POD-like data onto the message buffer
		template<typename DataType>
		friend Message& operator << (Message& msg, const DataType& data)
		{
			// Check that the type of the data being pushed is trivially copyable
			static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

			// Push data onto the message buffer
			{
				// Cache current size of vector, as this will be the point we insert the data
				size_t i = msg.body.size();
				// Resize the vector by the size of the data being pushed
				msg.body.resize(msg.body.size() + sizeof(DataType));
				// Physically copy the data into the newly allocated vector space
				std::memcpy(msg.body.data() + i, &data, sizeof(DataType));
				// Recalculate the message size
				msg.header.size = (uint32_t)msg.body.size();
			}

			// Push size of data onto the message buffer
			PushSize(msg, sizeof(DataType));

			// Return the target message so it can be "chained"
			return msg;
		}


		// Pulls any POD-like data from the message buffer
		template<typename DataType>
		friend Message& operator >> (Message& msg, DataType& data)
		{
			// Check that the type of the data being pulled is trivially copyable
			static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pulled from vector");

			// Pull size of data from message buffer
			uint32_t data_size = PullSize(msg);

			// assert() if stored data size != expected size
			assert(data_size == sizeof(DataType));

			// Pull data from message buffer
			{
				// Cache the location towards the end of the vector where the pulled data starts
				size_t i = msg.body.size() - sizeof(DataType);
				// Physically copy the data from the vector into the user variable
				std::memcpy(&data, msg.body.data() + i, sizeof(DataType));
				// Shrink the vector to remove read bytes, and reset end position
				msg.body.resize(i);
				// Recalculate the message size
				msg.header.size = (uint32_t)msg.body.size();
			}

			// Return the target message so it can be "chained"
			return msg;
		}


		// Pushes std::string data onto the message buffer
		friend Message& operator << (Message& msg, const std::string& data)
		{
			// Push data onto the message buffer
			{
				// Cache current size of vector, as this will be the point we insert the data
				size_t i = msg.body.size();
				// Resize the vector by the size of the data being pushed
				msg.body.resize(msg.body.size() + data.size());
				// Physically copy the data into the newly allocated vector space
				std::memcpy(msg.body.data() + i, data.c_str(), data.size());
				// Recalculate the message size
				msg.header.size = (uint32_t)msg.body.size();
			}

			// Push size of data onto the message buffer
			PushSize(msg, data.size());

			// Return the target message so it can be "chained"
			return msg;
		}


		// Pulls std::string data from the message buffer
		friend Message& operator >> (Message& msg, std::string& data)
		{
			// Pull size of data from message buffer
			uint32_t data_size = PullSize(msg);

			// Pull data from message buffer
			{
				// Resize std:string to required size
				data.resize(data_size);
				// Cache the location towards the end of the vector where the pulled data starts
				size_t i = msg.body.size() - data_size;
				// Physically copy the data from the vector into the user variable (char by char)
				for (size_t n = 0; n < data_size; n++)
				{
					data[n] = msg.body.data()[i + n];
				}
				// Shrink the vector to remove read bytes, and reset end position
				msg.body.resize(i);
				// Recalculate the message size
				msg.header.size = (uint32_t)msg.body.size();
			}

			// Return the target message so it can be "chained"
			return msg;
		}


	public:
		// Storage for header & body
		message_header header{};
		std::vector<uint8_t> body;

		// Remote-Connection of the message
		//   On a server, remote would be the client that sent the message
		//   On a client remote would be the server
		std::shared_ptr<Connection> remote = nullptr;
	};


} // namespace Net
