#include <swap_chain.h>
#include <context.h>
#include <algorithm>

toy2d::Swapchain::Swapchain(int w, int h)
{
	queryInfo(w, h);

	vk::SwapchainCreateInfoKHR createInfo;
	createInfo.setClipped(true)
		.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)			//窗口内容被视为完全不透明,用于指定alpha通道是否被用来和窗口系统中的其它窗口进行混合操作
		.setImageArrayLayers(1)				//用于指定每个图像所包含的层次
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)			//在图像上进行绘制操作，也就是将图像作为一个颜色附着来使用
		.setSurface(Context::Getinstance().surface)
		.setImageColorSpace(info.format.colorSpace)
		.setImageExtent(info.imageExtent)
		.setImageFormat(info.format.format)
		.setMinImageCount(info.imageCount)
		.setPresentMode(vk::PresentModeKHR::eMailbox);
		//.setPresentMode(info.present);
	auto& queueIndices = Context::Getinstance().queueFamilyIndices;
	//显示的命令队列和绘图的命令队列是同一个
	if (queueIndices.graphicsFamily.value() == queueIndices.presentFamily.value())
	{
		createInfo.setQueueFamilyIndices(queueIndices.graphicsFamily.value());
		createInfo.setImageSharingMode(vk::SharingMode::eExclusive);		
	}
	else {
		std::array indices = { queueIndices.graphicsFamily.value() ,queueIndices.presentFamily.value() };
		createInfo.setQueueFamilyIndices(indices);
		createInfo.setImageSharingMode(vk::SharingMode::eConcurrent);		//共存
	}

	swapchain = Context::Getinstance().device.createSwapchainKHR(createInfo);
	getImages();
	createImageViews();
}

toy2d::Swapchain::~Swapchain()
{
	auto& device = Context::Getinstance().device;
	for (auto& framebuffer : framebuffers)
	{
		device.destroyFramebuffer(framebuffer);
	}
	for (auto& view : swapChainImageViews)
	{
		device.destroyImageView(view);
	}
	device.destroySwapchainKHR(swapchain);
}

void toy2d::Swapchain::queryInfo(int w, int h)
{
	auto& phyDevice = Context::Getinstance().physicaldevice;
	auto& surface = Context::Getinstance().surface;
	auto& formats = phyDevice.getSurfaceFormatsKHR(surface);
	for (const auto& format : formats)
	{
		if (format.format == vk::Format::eR8G8B8A8Srgb &&
			format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)			//颜色空间为非线性sgrb
		{
			info.format = format;
			break;
		}
	}
	auto capabilities = phyDevice.getSurfaceCapabilitiesKHR(surface);			//查询表面特性
	//info.imageExtent.width = std::max(std::min(capabilities.currentExtent.width,capabilities.maxImageExtent.width), capabilities.minImageExtent.width);
	info.imageCount = std::clamp<uint32_t>(2, capabilities.minImageCount, capabilities.maxImageCount);			//maxImageCount的值为0表明，只要内存可以满足，我们可以使用任意数量的图像
	info.imageExtent.width = std::clamp<uint32_t>(w, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	info.imageExtent.height = std::clamp<uint32_t>(h, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	info.transform = capabilities.currentTransform;
	
	auto presents = phyDevice.getSurfacePresentModesKHR(surface);
	info.present = vk::PresentModeKHR::eFifo;			//先进先出模式
	for (auto& p : presents)
	{
		if (p == vk::PresentModeKHR::eMailbox)		//信箱模式
		{
			info.present = p;
			break;
		}
	}


}

void toy2d::Swapchain::getImages()
{
	images = Context::Getinstance().device.getSwapchainImagesKHR(swapchain);
}

void toy2d::Swapchain::createImageViews()
{
	swapChainImageViews.resize(images.size());
	for (int i = 0; i < swapChainImageViews.size(); i++)
	{
		vk::ImageViewCreateInfo createInfo;
		vk::ComponentMapping mapping(vk::ComponentSwizzle::eIdentity);			//不改变原本的值,进行图像颜色通道的映射
		vk::ImageSubresourceRange range;
		range.setBaseArrayLayer(0)
			.setBaseMipLevel(0)
			.setLayerCount(1)
			.setLevelCount(1)
			.setAspectMask(vk::ImageAspectFlagBits::eColor);			//作渲染目标，并且没有细分级别，只存在一个图层

		createInfo.setImage(images[i])
			.setViewType(vk::ImageViewType::e2D)
			.setComponents(mapping)
			.setFormat(info.format.format)
			.setSubresourceRange(range);
		swapChainImageViews[i] = Context::Getinstance().device.createImageView(createInfo);
	}
}

void toy2d::Swapchain::createFramerbuffers(int w, int h)
{
	framebuffers.resize(images.size());
	for (int i = 0; i < framebuffers.size(); i++)
	{
		vk::FramebufferCreateInfo createinfo;
		createinfo.setAttachments(swapChainImageViews[i])
			.setWidth(w)
			.setHeight(h)
			.setLayers(1)
			.setRenderPass(Context::Getinstance().renderProcess->renderPass);
		framebuffers[i] = Context::Getinstance().device.createFramebuffer(createinfo);
	}
}
