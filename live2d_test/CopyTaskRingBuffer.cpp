#include "CopyTaskRingBuffer.h"

#include <assert.h>
#include <thread>


namespace D3D
{
	CopyTaskRingBuffer::CopyTaskRingBuffer()
	{
		m_data = new std::atomic<void*>[100];
		m_capacity = 100;
		m_dequeue_count_ = 0;
		m_size = 0;
		m_enqueue = 0;
		m_dequeue = 0;
		m_offset = 0;
		m_resize = false;
	}

	CopyTaskRingBuffer::~CopyTaskRingBuffer()
	{
	}

	uint64_t CopyTaskRingBuffer::Pop(void** data)
	{
		int64_t old_size(0);
		int64_t new_size(0);
		uint64_t old_count(0);
		uint64_t de_pos(0);

	start:
		m_dequeue++;

		if (m_resize)
		{
			m_dequeue--;
			while (m_resize) 
			{
				std::this_thread::yield();
			}
			goto start;
		}

		do
		{
			old_size = m_size;
			new_size = std::max(old_size - 1, 0ll);

			if (new_size >= old_size)
			{
				m_dequeue--;
				return 0;
			}
		} 
		while (!m_size.compare_exchange_strong(old_size, new_size));

		do
		{
			old_count = m_dequeue_count_;
			de_pos = (old_count + m_offset) % m_capacity;
		} 
		while (!m_dequeue_count_.compare_exchange_strong(old_count, old_count + 1));

		auto& dest_data = m_data[de_pos];
		void* pop_data{ nullptr };
		do 
		{
			pop_data = dest_data;
		} 
		while (dest_data.compare_exchange_strong(pop_data, nullptr));
		m_dequeue--;

		return old_count;
	}

	void CopyTaskRingBuffer::Emplace(uint64_t task_id, void* data)
	{
		int64_t old_size{};
		int64_t new_size{};

	start:
		m_enqueue++;

		if (m_resize) 
		{
			m_enqueue--;
			while (m_resize)
			{
				std::this_thread::yield();
			}
			goto start;
		}

		do 
		{
			old_size = m_size;
			new_size = std::min(old_size + 1, static_cast<int64_t>(m_capacity));

			if (new_size <= old_size)
			{
				m_enqueue--;
				Resize(new_size * 1.5);
				goto start;
			}
		} 
		while (!m_size.compare_exchange_strong(old_size, new_size));

		uint64_t en_pos = (task_id - m_dequeue_count_) % m_capacity;
		auto& dest_data = m_data[en_pos];

		void* exception{ nullptr };
		while (!dest_data.compare_exchange_strong(exception, data))
		{
			exception = nullptr;
		}
		m_enqueue--;
	}

	void CopyTaskRingBuffer::Resize(uint64_t new_capacity)
	{
		if (new_capacity <= m_capacity) 
		{
			return;
		}

		if (m_resize.exchange(true))
		{
			while (m_resize)
			{
				std::this_thread::yield();
			}
			return;
		}

		while (m_dequeue | m_enqueue)
		{
			std::this_thread::yield();
		}

		assert(new_capacity > m_capacity);

		std::atomic<void*>*	new_data = new std::atomic<void*>[new_capacity];
		uint64_t head_pos((m_dequeue_count_ + m_offset) % m_capacity);
		for (uint64_t i = 0; i < m_size; i++)
		{
			new_data[i].store(m_data[head_pos]);
			head_pos = (head_pos + 1) % m_capacity;
		}

		delete[] m_data;
		m_data = new_data;
		m_offset = new_capacity - (m_dequeue_count_ % new_capacity);

		m_resize = false;
	}

	uint64_t CopyTaskRingBuffer::Size()
	{
		return m_size;
	}

	uint64_t CopyTaskRingBuffer::Capacity()
	{
		return m_capacity;
	}
};

