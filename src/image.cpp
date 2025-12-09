#include "image.h"
#include "context.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void toy2d::ImageBase::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, 
	vk::MemoryPropertyFlagBits property, vk::Image& image, vk::DeviceMemory& imageMemory, uint32_t mipLevels, vk::SampleCountFlagBits numSamples)
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
		.setMipLevels(mipLevels)
		.setTiling(tiling)
		.setUsage(usage)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setSamples(numSamples)
		.setImageType(vk::ImageType::e2D);
	if (device.createImage(&createInfo, nullptr, &image) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create image!");
	}

	auto info = queryImageInfo(image, property);
	imageMemory = device.allocateMemory(info);
	device.bindImageMemory(image, imageMemory, 0);
}

vk::ImageView toy2d::ImageBase::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels)
{
	vk::ImageViewCreateInfo createInfo;
	vk::ComponentMapping mapping;
	vk::ImageSubresourceRange range;
	vk::ImageView res;
	range.setAspectMask(aspectFlags)
		.setBaseArrayLayer(0)
		.setBaseMipLevel(0)
		.setLayerCount(1)
		.setLevelCount(mipLevels);
	createInfo.setImage(image)
		.setViewType(vk::ImageViewType::e2D)
		.setComponents(mapping)
		.setFormat(format)
		.setSubresourceRange(range);
	res = Context::Getinstance().device.createImageView(createInfo);
	return res;
}

toy2d::Image::Image(std::string filename)
{
	int textureWidth, textureHeight, textureChannel;
	stbi_uc* pixels = stbi_load(filename.data(), &textureWidth, &textureHeight, &textureChannel, STBI_rgb_alpha);
	w = textureWidth;
	h = textureHeight;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}
	mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(w, h)))) + 1;

	ImageBuffer.reset(new Buffer(size_t(w * h * 4),
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
	memcpy(ImageBuffer->map, pixels, ImageBuffer->size);				//把图像数据存到 暂存缓冲区

	stbi_image_free(pixels);
	createImage(w, h, vk::Format::eR8G8B8A8Srgb,
		vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		vk::MemoryPropertyFlagBits::eDeviceLocal, TextureImage, TextureMemory, mipLevels, vk::SampleCountFlagBits::e1);
	transitionImageLayoutFromUndefine2Dst();
	transformData2Image(*ImageBuffer);
	generateMipmaps();
	//transitionImageLayoutFromDst2Optimal();

	TextureImageView = createImageView(TextureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, mipLevels);
	createTextureSampler();
	//setInfo = DescriptorSetManager::Instance().AllocImageSet();
	//updateDescriptorSet();			//当sampler 使用不同的set
	
}

toy2d::Image::~Image()
{
	auto& device = Context::Getinstance().device;
	//DescriptorSetManager::Instance().FreeImageSet(setInfo);
	device.destroySampler(TextureSampler);
	device.destroyImageView(TextureImageView);
	device.destroyImage(TextureImage);
	device.freeMemory(TextureMemory);
	ImageBuffer.reset();
}

vk::MemoryAllocateInfo toy2d::ImageBase::queryImageInfo(vk::Image image, vk::MemoryPropertyFlagBits property)
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
				.setLevelCount(mipLevels);				//***include all mipLevel
			barrier.setImage(TextureImage)
				.setOldLayout(vk::ImageLayout::eUndefined)
				.setNewLayout(vk::ImageLayout::eTransferDstOptimal)			//在copyBufferToImage时作为传输目标，需要写入
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

void toy2d::Image::generateMipmaps()
{
	vk::FormatProperties formatProperties=Context::Getinstance().physicaldevice.getFormatProperties(vk::Format::eR8G8B8A8Srgb);
	if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
	{
		throw std::runtime_error("Texture image format does not support linear blitting");
	}
	Context::Getinstance().commandManager->ExecuteCmd(Context::Getinstance().graphcisQueue,
		[&](vk::CommandBuffer cmdBuf)
		{
			vk::ImageMemoryBarrier barrier;
			vk::ImageSubresourceRange range;
			//像素复制到图像的哪一部分
			range.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setLayerCount(1)
				.setBaseArrayLayer(0)
				.setLevelCount(1);
			barrier.setImage(TextureImage)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
			int32_t mipWidth = w, mipHeight = h;
			for (uint32_t i = 1; i < mipLevels; i++)
			{
				range.setBaseMipLevel(i - 1);
				barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)			//传输目标： 用于write
					.setNewLayout(vk::ImageLayout::eTransferSrcOptimal)				//from mip0(image)->mip1，image作为源图像，用于read
					.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
					.setDstAccessMask(vk::AccessFlagBits::eTransferRead)
					.setSubresourceRange(range);
				cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
					{}, {}, nullptr, barrier);
				vk::ImageBlit blit;
				std::array<vk::Offset3D, 2> srcOffset;
				srcOffset[0] = vk::Offset3D(0, 0, 0);
				srcOffset[1] = vk::Offset3D(mipWidth, mipHeight, 1);
				std::array<vk::Offset3D, 2> dstOffset;
				dstOffset[0] = vk::Offset3D(0, 0, 0);
				dstOffset[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1);
				vk::ImageSubresourceLayers SrcLayers;
				SrcLayers.setAspectMask(vk::ImageAspectFlagBits::eColor)
					.setLayerCount(1)
					.setBaseArrayLayer(0)
					.setMipLevel(i - 1);
				vk::ImageSubresourceLayers DstLayers;
				DstLayers.setAspectMask(vk::ImageAspectFlagBits::eColor)
					.setLayerCount(1)
					.setBaseArrayLayer(0)
					.setMipLevel(i);
				blit.setDstOffsets(dstOffset)
					.setSrcOffsets(srcOffset)
					.setDstSubresource(DstLayers)
					.setSrcSubresource(SrcLayers);
				cmdBuf.blitImage(TextureImage, vk::ImageLayout::eTransferSrcOptimal, TextureImage, vk::ImageLayout::eTransferDstOptimal,
					blit, vk::Filter::eLinear);
				barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal)			//传输源 → 着色器读取
					.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
					.setSrcAccessMask(vk::AccessFlagBits::eTransferRead)			//传输src -> 读取
					.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
				cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
					{}, {}, nullptr, barrier);
				if (mipWidth > 1) mipWidth /= 2;
				if (mipHeight > 1) mipHeight /= 2;
			}
			//last :dst->shader only
			range.setBaseMipLevel(mipLevels - 1);
			barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
				.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
				.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
				.setSubresourceRange(range);
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
		.setMipmapMode(vk::SamplerMipmapMode::eLinear)			//相邻两个级别的纹理图像进行线性混合采样
		.setMinLod(0.0f)
		.setMaxLod(static_cast<float>(mipLevels))
		.setMipLodBias(0.0f);
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

toy2d::ImageInfo::ImageInfo()
{
	msaaSamples = getMaxUsableSampleCount();
	createColorResources();
	createDepthResources();
}

void toy2d::ImageInfo::destroyDepthImage()
{
	DestroyResources(colorImage);
	DestroyResources(depthImage);
}

vk::SampleCountFlagBits toy2d::ImageInfo::getMaxUsableSampleCount()
{
	auto& physicaldevice = Context::Getinstance().physicaldevice;
	vk::PhysicalDeviceProperties physicalDeviceProperties= physicaldevice.getProperties();

	vk::SampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
	if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
	if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
	if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
	if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
	if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }

	return vk::SampleCountFlagBits::e1;
}

void toy2d::ImageInfo::DestroyResources(ImageResources* image)
{
	auto& device = Context::Getinstance().device;
	device.destroyImageView(image->view);
	device.destroyImage(image->Image);
	device.freeMemory(image->Memory);
	delete image;
}

void toy2d::ImageInfo::createDepthResources()
{
	if (depthImage == nullptr)
		depthImage = new ImageResources();
	std::vector<vk::Format> candidates = { vk::Format::eD32Sfloat,vk::Format::eD32SfloatS8Uint,vk::Format::eD24UnormS8Uint };
	depthImage->format = findSupportFormat(candidates, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
	auto extent = Context::Getinstance().swapchain->info.imageExtent;
	createImage(extent.width, extent.height, depthImage->format, vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, depthImage->Image, depthImage->Memory, 1, msaaSamples);
	depthImage->view = createImageView(depthImage->Image, depthImage->format, vk::ImageAspectFlagBits::eDepth);
}

void toy2d::ImageInfo::createColorResources()
{
	if (colorImage == nullptr)
		colorImage = new ImageResources();
	colorImage->format = Context::Getinstance().swapchain->info.format.format;
	auto extent = Context::Getinstance().swapchain->info.imageExtent;
	createImage(extent.width, extent.height, colorImage->format, vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, colorImage->Image, colorImage->Memory, 1, msaaSamples);
	colorImage->view = createImageView(colorImage->Image, colorImage->format, vk::ImageAspectFlagBits::eColor);
}

vk::Format toy2d::ImageBase::findSupportFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
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
