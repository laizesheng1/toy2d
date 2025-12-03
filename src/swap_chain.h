#pragma once
#include <vulkan/vulkan.hpp>

namespace toy2d {
	class Swapchain final{
	public:
		Swapchain(vk::SurfaceKHR* surface, int w, int h);
		~Swapchain();
		void queryInfo(int w, int h);
		void getImages();
		void createImageViews();
		void createFramerbuffers(int w, int h);

		const auto& GetExtent() const { return info.imageExtent; }
		vk::SurfaceKHR* surface_ = nullptr;

		vk::SwapchainKHR swapchain;
		struct SwapchainInfo {
			vk::Extent2D imageExtent;		//图像大小
			uint32_t imageCount;		//数量
			vk::SurfaceFormatKHR format;			//用于指定颜色通道和存储类型
			vk::SurfaceTransformFlagsKHR transform;			//呈现模式
			vk::PresentModeKHR present;
		};
		SwapchainInfo info;

		std::vector<vk::Image> images;		//进行绘制操作的交换链图像
		std::vector<vk::ImageView> swapChainImageViews;
		std::vector<vk::Framebuffer> framebuffers;
	};
}