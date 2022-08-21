#pragma once

#include "Net/NetCommon.h"

#include "Net/NetMessage.h"


namespace Net {


	class MsgQueue
	{
	public:
		MsgQueue() = default;
		MsgQueue(const MsgQueue&) = delete; // no copy constructor
		~MsgQueue() { Clear(); }

	public:


		// Returns and maintains item at front of queue
		const Message& GetFront()
		{
			std::scoped_lock scoped_lock(m_mutexQueue);
			return m_deque.front();
		}


		// Returns and maintains item at back of queue
		const Message& GetBack()
		{
			std::scoped_lock scoped_lock(m_mutexQueue);
			return m_deque.back();
		}


		// Removes and returns item from front of queue
		Message PopFront()
		{
			std::scoped_lock scoped_lock(m_mutexQueue);
			auto t = std::move(m_deque.front());
			m_deque.pop_front();
			return t;
		}


		// Removes and returns item from back of queue
		Message PopBack()
		{
			std::scoped_lock scoped_lock(m_mutexQueue);
			auto t = std::move(m_deque.back());
			m_deque.pop_back();
			return t;
		}


		// Adds an item to front of queue
		void PushFront(const Message& msg)
		{
			std::scoped_lock scoped_lock(m_mutexQueue);
			m_deque.emplace_front(std::move(msg));

			std::unique_lock<std::mutex> unique_lock(m_mutexBlocking);
			m_condBlocking.notify_one();
		}


		// Adds an item to back of queue
		void PushBack(const Message& msg)
		{
			std::scoped_lock scoped_lock(m_mutexQueue);
			m_deque.emplace_back(std::move(msg));

			std::unique_lock<std::mutex> unique_lock(m_mutexBlocking);
			m_condBlocking.notify_one();
		}


		// Returns true if queue has no items
		bool IsEmpty()
		{
			std::scoped_lock scoped_lock(m_mutexQueue);
			return m_deque.empty();
		}


		// Returns number of items in queue
		size_t GetCount()
		{
			std::scoped_lock scoped_lock(m_mutexQueue);
			return m_deque.size();
		}


		// Clears queue
		void Clear()
		{
			std::scoped_lock scoped_lock(m_mutexQueue);
			m_deque.clear();
		}


		// Waits until queue has messages
		void Wait()
		{
			while (IsEmpty())
			{
				std::unique_lock<std::mutex> unique_lock(m_mutexBlocking);
				m_condBlocking.wait(unique_lock);
			}
		}


	protected:
		std::mutex m_mutexQueue;
		std::deque<Message> m_deque;
		std::condition_variable m_condBlocking;
		std::mutex m_mutexBlocking;
	};


} // namespace Net
