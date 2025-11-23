#include "image.h"
#include "context.h"

toy2d::Image::Image(std::string filename)
{
	int textureWidth, textureHeight, textureChannel;
	pixels = stbi_load(&filename[0], &textureWidth, &textureHeight, &textureChannel, STBI_rgb_alpha);
	w = textureWidth;
	h = textureHeight;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}
}

toy2d::Image::~Image()
{

}

void toy2d::Image::createImageBuffer()
{
	ImageBuffer.reset(new Buffer(size_t(w * h * 4),
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
}

void toy2d::Image::bufferImageData()
{
	if (pixels)
	{
		void* ptr = Context::Getinstance().device.mapMemory(ImageBuffer->memory, 0, ImageBuffer->size);
		memcpy(ptr, pixels, ImageBuffer->size);
		Context::Getinstance().device.unmapMemory(ImageBuffer->memory);
		stbi_image_free(pixels);
		createImage(vk::MemoryPropertyFlagBits::eDeviceLocal);
	}
	
}

void toy2d::Image::createImage(vk::MemoryPropertyFlagBits property)
{
	auto device = Context::Getinstance().device;
	vk::ImageCreateInfo createInfo;
	vk::Extent3D extent;
	extent.setWidth(w)
		.setHeight(h)
		.setDepth(1);

	createInfo.setArrayLayers(1)
		.setExtent(extent)
		.setFormat(vk::Format::eR8G8B8A8Srgb)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setMipLevels(1)
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setImageType(vk::ImageType::e2D);
	if (device.createImage(&createInfo, nullptr, &image) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create image!");
	}

	auto info = queryImageInfo(property);
	textureImageMemory = device.allocateMemory(info);
	device.bindImageMemory(image, textureImageMemory, 0);
}

vk::MemoryAllocateInfo toy2d::Image::queryImageInfo(vk::MemoryPropertyFlagBits property)
{
	vk::MemoryRequirements requirements;
	Context::Getinstance().device.getImageMemoryRequirements(image, &requirements);
	vk::MemoryAllocateInfo allocateInfo;
	allocateInfo.setAllocationSize(requirements.size);

	auto properties = Context::Getinstance().physicaldevice.getMemoryProperties();
	for (int i = 0; i < properties.memoryTypeCount; ++i)
	{
		if ((1 << i) & requirements.memoryTypeBits &&							//memoryTypeBits指示适合该缓冲使用的内存类型的位域
			properties.memoryTypes[i].propertyFlags & property)
		{
			allocateInfo.memoryTypeIndex = i;
			break;
		}
	}
	return allocateInfo;
}
