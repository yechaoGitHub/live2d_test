#pragma once

#include <atomic>
#include <vector>

namespace D3D
{
	class CopyTaskRingBuffer
	{
	public:
		CopyTaskRingBuffer();
		~CopyTaskRingBuffer();

		void Emplace(uint64_t task_id, void* data);
		uint64_t Pop(void** data);
		uint64_t Size();
		uint64_t Capacity();

	private:
		void Resize(uint64_t new_capacity);

		std::atomic<void*>*					m_data{};
		std::atomic<int64_t>				m_capacity;
		std::atomic<int64_t>				m_size;
		std::atomic<uint64_t>				m_dequeue_count_;
		std::atomic<uint64_t>				m_enqueue;
		std::atomic<uint64_t>				m_dequeue;
		std::atomic<uint64_t>				m_offset;
		std::atomic<bool>					m_resize;
	};
};

