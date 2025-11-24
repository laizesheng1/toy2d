#include "buffer.h"
#include <context.h>

toy2d::Buffer::Buffer(size_t size_, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags property) :size(size_)
{
	auto device = Context::Getinstance().device;
	createBuffer(usage);
	MemoryInfo info;
	auto requirements = Context::Getinstance().device.getBufferMemoryRequirements(buffer);
	info.size = requirements.size;
	info.index = QueryBufferMemTypeIndex(requirements.memoryTypeBits, property);
	allocMemory(info);
	bindingMem2Buf();
	if (property & vk::MemoryPropertyFlagBits::eHostVisible) {
		map = device.mapMemory(memory, 0, size);
	}
	else {
		map = nullptr;
	}
}

toy2d::Buffer::~Buffer()
{
	auto device = Context::Getinstance().device;
	if (map)
		device.unmapMemory(memory);
	device.freeMemory(memory);
	device.destroyBuffer(buffer);
}

void toy2d::Buffer::createBuffer(vk::BufferUsageFlags usage)
{
	vk::BufferCreateInfo createinfo;
	createinfo.setSize(size)
		.setUsage(usage)		//缓冲中的数据的使用目的
		.setSharingMode(vk::SharingMode::eExclusive);

	buffer = Context::Getinstance().device.createBuffer(createinfo);
}

void toy2d::Buffer::allocMemory(MemoryInfo info)
{
	vk::MemoryAllocateInfo alloInfo;
	alloInfo.setAllocationSize(info.size)			//requirements size：缓冲需要的内存的字节大小，它可能和size的值不同
		.setMemoryTypeIndex(info.index);
	memory = Context::Getinstance().device.allocateMemory(alloInfo);
}

void toy2d::Buffer::bindingMem2Buf()
{
	Context::Getinstance().device.bindBufferMemory(buffer, memory, 0);			//第三个参数为偏移值：需要满足能够被requirements.alignment整除
}

uint32_t toy2d::QueryBufferMemTypeIndex(uint32_t type, vk::MemoryPropertyFlags flag)
{
	auto property = Context::Getinstance().physicaldevice.getMemoryProperties();

	for (std::uint32_t i = 0; i < property.memoryTypeCount; i++) {
		if ((1 << i) & type &&					
			property.memoryTypes[i].propertyFlags & flag) {				//memoryTypeBits:type指示适合该缓冲使用的内存类型的位域
			return i;
		}
	}

	return 0;
}
