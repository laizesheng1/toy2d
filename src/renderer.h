#pragma once
#include <vulkan/vulkan.hpp>


namespace toy2d {
	class Renderer final{
	public:
		Renderer();
		~Renderer();

		void render();

	private:
		vk::CommandPool cmdPool_;
		vk::CommandBuffer cmdBuffer_;
		vk::Semaphore imageAvaliable_;			//发出图像已经被获取，可以开始渲染的信号
		vk::Semaphore imageDrawFinish_;			//渲染已经结果，可以开始呈现的信号，访问命令队列
		vk::Fence cmdAvaliableFence_;			//用于cpu、gpu之间的同步

		void initCmdPool();
		void allocCmdBuffer();
		void createSems();
		void creteFence();
	};
}