#include "image.h"
#include "context.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

toy2d::Image::Image(std::string filename)
{
	int textureWidth, textureHeight, textureChannel;
	stbi_uc* pixels = stbi_load(filename.data(), &textureWidth, &textureHeight, &textureChannel, STBI_rgb_alpha);
	w = textureWidth;
	h = textureHeight;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}
	ImageBuffer.reset(new Buffer(size_t(w * h * 4),
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
	memcpy(ImageBuffer->map, pixels, ImageBuffer->size);				//把图像数据存到 暂存缓冲区

	stbi_image_free(pixels);
	createImage(w, h, vk::Format::eR8G8B8A8Srgb,
		vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		vk::MemoryPropertyFlagBits::eDeviceLocal, TextureImage, TextureMemory);
	transitionImageLayoutFromUndefine2Dst();
	transformData2Image(*ImageBuffer);
	transitionImageLayoutFromDst2Optimal();

	TextureImageView = createImageView(TextureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
	createTextureSampler();
	setInfo = DescriptorSetManager::Instance().AllocImageSet();
	updateDescriptorSet();
}

toy2d::Image::~Image()
{
	auto& device = Context::Getinstance().device;
	DescriptorSetManager::Instance().FreeImageSet(setInfo);
	device.destroySampler(TextureSampler);
	device.destroyImageView(TextureImageView);
	device.destroyImage(TextureImage);
	device.freeMemory(TextureMemory);
	ImageBuffer.reset();
}

void toy2d::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlagBits property, vk::Image& image, vk::DeviceMemory& imageMemory)
{
	auto& device = Context::Getinstance().device;
	vk::ImageCreateInfo createInfo;
	vk::Extent3D extent;
	extent.setWidth(width)
		.setHeight(height)
		.setDepth(1);

	createInfo.setArrayLayers(1)
		.setExtent(extent)
		.setFormat(format)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setMipLevels(1)
		.setTiling(tiling)
		.setUsage(usage)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setImageType(vk::ImageType::e2D);
	if (device.createImage(&createInfo, nullptr, &image) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create image!");
	}

	auto info = queryImageInfo(image, property);
	imageMemory= device.allocateMemory(info);
	device.bindImageMemory(image, imageMemory, 0);
}

vk::MemoryAllocateInfo toy2d::queryImageInfo(vk::Image image, vk::MemoryPropertyFlagBits property)
{
	vk::MemoryRequirements requirements;
	Context::Getinstance().device.getImageMemoryRequirements(image, &requirements);
	vk::MemoryAllocateInfo allocateInfo;
	allocateInfo.setAllocationSize(requirements.size);
	allocateInfo.memoryTypeIndex = QueryBufferMemTypeIndex(requirements.memoryTypeBits, property);
	return allocateInfo;
}

void toy2d::Image::transitionImageLayoutFromUndefine2Dst()
{
	Context::Getinstance().commandManager->ExecuteCmd(Context::Getinstance().graphcisQueue,
		[&](vk::CommandBuffer cmdBuf) {
			vk::ImageMemoryBarrier barrier;
			vk::ImageSubresourceRange range;
			range.setAspectMask(vk::ImageAspectFlagBits::eColor)			//像素复制到图像的哪一部分
				.setLayerCount(1)
				.setBaseArrayLayer(0)
				.setBaseMipLevel(0)
				.setLevelCount(1);
			barrier.setImage(TextureImage)
				.setOldLayout(vk::ImageLayout::eUndefined)
				.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
				.setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setSubresourceRange(range);
			//eTopOfPipe:最早可能的管线阶段
			cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
				{}, {}, nullptr, barrier);
		});
}

void toy2d::Image::transitionImageLayoutFromDst2Optimal()
{
	Context::Getinstance().commandManager->ExecuteCmd(Context::Getinstance().graphcisQueue,
		[&](vk::CommandBuffer cmdBuf) {
			vk::ImageMemoryBarrier barrier;
			vk::ImageSubresourceRange range;
			//像素复制到图像的哪一部分
			range.setAspectMask(vk::ImageAspectFlagBits::eColor)			
				.setLayerCount(1)
				.setBaseArrayLayer(0)
				.setBaseMipLevel(0)
				.setLevelCount(1);
			barrier.setImage(TextureImage)
				.setOldLayout(vk::ImageLayout::eTransferDstOptimal)			//传输目标 → 着色器读取
				.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
				.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setSubresourceRange(range);
			//需要等待传输完成 ，fragment shader才能读取
			//第一个参数指定在哪个管线阶段发生应在屏障之前发生的操作。第二个参数指定在哪个管线阶段操作将等待屏障
			cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
				{}, {}, nullptr, barrier);
		});
}

void toy2d::Image::transformData2Image(Buffer& buffer)
{
	Context::Getinstance().commandManager->ExecuteCmd(Context::Getinstance().graphcisQueue,
		[&](vk::CommandBuffer cmdBuf) {
			vk::ImageSubresourceLayers range;
			//像素复制到图像的哪一部分
			range.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setLayerCount(1)
				.setBaseArrayLayer(0)
				.setMipLevel(0);
			vk::BufferImageCopy region;
			region.setBufferOffset(0)
				.setBufferRowLength(0)			//像素只是紧密排列的,无padding
				.setBufferImageHeight(0)
				.setImageExtent({ (uint32_t)w,(uint32_t)h,1 })
				.setImageOffset(0)
				.setImageSubresource(range);
			//把 缓冲区的数据填充 image对象
			cmdBuf.copyBufferToImage(buffer.buffer, TextureImage, vk::ImageLayout::eTransferDstOptimal, region);
		});
	
}

vk::ImageView toy2d::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags)
{
	vk::ImageViewCreateInfo createInfo;
	vk::ComponentMapping mapping;
	vk::ImageSubresourceRange range;
	vk::ImageView res;
	range.setAspectMask(aspectFlags)
		.setBaseArrayLayer(0)
		.setBaseMipLevel(0)
		.setLayerCount(1)
		.setLevelCount(1);
	createInfo.setImage(image)
		.setViewType(vk::ImageViewType::e2D)
		.setComponents(mapping)
		.setFormat(format)
		.setSubresourceRange(range);
	res = Context::Getinstance().device.createImageView(createInfo);
	return res;
}

void toy2d::Image::createTextureSampler()
{
	vk::PhysicalDeviceProperties properties = Context::Getinstance().physicaldevice.getProperties();
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.setAddressModeU(vk::SamplerAddressMode::eRepeat)			//超出图像尺寸时重复纹理，例如纹理坐标设置为2.0超出[0,1)
		.setAddressModeV(vk::SamplerAddressMode::eRepeat)			
		.setAddressModeW(vk::SamplerAddressMode::eRepeat)
		.setAnisotropyEnable(true)								//使用各向异性过滤
		.setBorderColor(vk::BorderColor::eIntOpaqueBlack)		//采样超出图像范围时返回的颜色
		.setMaxAnisotropy(properties.limits.maxSamplerAnisotropy)		
		.setMagFilter(vk::Filter::eLinear)			////如何插值放大或缩小的纹素
		.setMinFilter(vk::Filter::eLinear)			
		.setUnnormalizedCoordinates(false)		//使用轴上的 [0, 1) 坐标系来寻址图像中的纹素
		.setCompareEnable(false)				//启用了比较函数，则纹素将首先与一个值进行比较，并且该比较的结果用于过滤操作
		.setCompareOp(vk::CompareOp::eAlways)
		.setMipmapMode(vk::SamplerMipmapMode::eLinear);
	TextureSampler = Context::Getinstance().device.createSampler(samplerInfo);
}

void toy2d::Image::updateDescriptorSet()
{
	vk::WriteDescriptorSet writer;
	vk::DescriptorImageInfo imageInfo;
	imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
		.setImageView(TextureImageView)
		.setSampler(TextureSampler);
	writer.setImageInfo(imageInfo)
		.setDstBinding(0)
		.setDstArrayElement(0)
		.setDstSet(setInfo.set)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
	Context::Getinstance().device.updateDescriptorSets(writer, {});
}

toy2d::DepthImageInfo::DepthImageInfo()
{
	createDepthResources();
}

void toy2d::DepthImageInfo::destroyDepthImage()
{
	auto& device = Context::Getinstance().device;
	device.destroyImageView(DepthImageView);
	device.destroyImage(DepthImage);
	device.freeMemory(DepthMemory);
}

void toy2d::DepthImageInfo::createDepthResources()
{
	std::vector<vk::Format> candidates = { vk::Format::eD32Sfloat,vk::Format::eD32SfloatS8Uint,vk::Format::eD24UnormS8Uint };
	Depthformat = findSupportFormat(candidates, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
	auto extent = Context::Getinstance().swapchain->info.imageExtent;
	createImage(extent.width, extent.height, Depthformat, vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal , DepthImage, DepthMemory);
	DepthImageView = createImageView(DepthImage, Depthformat, vk::ImageAspectFlagBits::eDepth);
}

vk::Format toy2d::findSupportFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
	for (vk::Format format : candidates) {
		vk::FormatProperties props = Context::Getinstance().physicaldevice.getFormatProperties(format);
		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

namespace toy2d {
	std::unique_ptr<ImageManager> ImageManager::instance = nullptr;
	void ImageManager::load(std::string filename)
	{
		data.push_back(std::unique_ptr<Image>(new Image(filename)));
		return;
	}
	void ImageManager::Destroy(Image* textureImgae)
	{
		auto it = std::find_if(data.begin(), data.end(),
			[&](const std::unique_ptr<Image>& t)
			{
				return t.get() == textureImgae;
			});
		if (it != data.end())
		{
			Context::Getinstance().device.waitIdle();
			data.erase(it);
			return;
		}
	}
	void ImageManager::Clear() {
		data.clear();
	}
	Image* ImageManager::Get(size_t i)
	{
		return data.at(i).get();
	}
}
