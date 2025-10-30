#include "buffer.h"
#include <context.h>

toy2d::Buffer::Buffer(size_t size_, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags property) :size(size_)
{
	createBuffer(usage);
	MemoryInfo info = queeryBufferInfo(property);
	allocMemory(info);
	bindingMem2Buf();
}

toy2d::Buffer::~Buffer()
{
	Context::Getinstance().device.freeMemory(memory);
	Context::Getinstance().device.destroyBuffer(buffer);
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

toy2d::Buffer::MemoryInfo toy2d::Buffer::queeryBufferInfo(vk::MemoryPropertyFlags property)
{
	MemoryInfo info;
	auto requirements = Context::Getinstance().device.getBufferMemoryRequirements(buffer);
	info.size = requirements.size;

	auto properties = Context::Getinstance().physicaldevice.getMemoryProperties();
	for (int i = 0; i < properties.memoryTypeCount; ++i)
	{
		if ((1 << i) & requirements.memoryTypeBits &&							//memoryTypeBits指示适合该缓冲使用的内存类型的位域
			properties.memoryTypes[i].propertyFlags & property)
		{
			info.index = i;
			break;
		}
	}
	return info;
}