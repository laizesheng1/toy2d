#pragma once
#include <vulkan/vulkan.hpp>
#include <Uniform.h>

namespace toy2d {
	class Buffer;
	class Renderer final{
	public:
		Renderer(int maxFrames=2);
		~Renderer();

		void render();

	private:
		vk::CommandPool cmdPool_;
		std::vector<vk::CommandBuffer> cmdBuffer_;
		std::vector<vk::Semaphore> imageAvaliables;			//发出图像已经被获取，可以开始渲染的信号
		std::vector<vk::Semaphore> imageDrawFinishs;			//渲染已经结果，可以开始呈现的信号，访问命令队列
		std::vector<vk::Fence> cmdAvaliableFences;			//用于cpu、gpu之间的同步

		std::unique_ptr<Buffer> hostVertexBuffer;
		std::unique_ptr<Buffer> deviceVertexBuffer;
		std::vector<std::unique_ptr<Buffer>> hostUniformBuffer;
		std::vector<std::unique_ptr<Buffer>> deviceUniformBuffer;

		vk::DescriptorPool descriptorPool;
		std::vector<vk::DescriptorSet> desSets;

		int maxFlightCount_;
		int curFrame_;

		void createCommandBuffer();
		void createSems();
		void creteFence();
		void createVertexBuffer();
		void createUniformBuffers();
		void bufferVertexData();
		//void bufferUniformData();
		void createDescriptorPool();
		void allocateSets();
		void updateSets();
		void updateUniformBuffer(int curImage);

		void copyBuffer(vk::Buffer& src, vk::Buffer& dst, size_t size, size_t srcOffset, size_t dstOffset);
	};
}