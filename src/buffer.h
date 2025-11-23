#pragma once
#ifndef BUFFER_H
#define BUFFER_H
#include <vulkan/vulkan.hpp>

namespace toy2d {
	class Buffer final {
	public:
		vk::Buffer buffer;
		vk::DeviceMemory memory;
		size_t size;
		void* map;
		//注意这里的第二个参数为BufferUsageFlags=Flags<BufferUsageFlagBits>，调用时才可以使用BufferUsageFlagBits的位运算
		Buffer(size_t size_, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags property);
		~Buffer();
	private:
		struct MemoryInfo final {
			size_t size;
			uint32_t index;
		};

		void createBuffer(vk::BufferUsageFlags usage);
		void allocMemory(MemoryInfo info);
		void bindingMem2Buf();
		MemoryInfo queeryBufferInfo(vk::MemoryPropertyFlags property);
	};
}
#endif