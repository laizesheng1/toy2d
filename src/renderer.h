#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>
#include <Uniform.h>
#include <utils.h>
#include "image.h"
#include <swap_chain.h>
#include <Vertex.h>

namespace toy2d {
	class Buffer;
	class Renderer final{
	public:
		Renderer(std::string model, int maxFrames = 2);
		~Renderer();
		void startRender();
		void DrawTexture();
		void endRender();
	private:
		std::string filename;
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		std::vector<vk::CommandBuffer> cmdBuffer_;
		std::vector<vk::Semaphore> imageAvaliables;			//发出图像已经被获取，可以开始渲染的信号
		std::vector<vk::Semaphore> imageDrawFinishs;			//渲染已经结果，可以开始呈现的信号，访问命令队列
		std::vector<vk::Fence> cmdAvaliableFences;			//用于cpu、gpu之间的同步

		std::unique_ptr<Buffer> VertexBuffer;
		std::unique_ptr<Buffer> IndicesBuffer;
		std::vector<std::unique_ptr<Buffer>> UniformBuffer;
		std::vector<DescriptorSetManager::SetInfo> desSets;

		int maxFlightCount_;
		int curFrame_;
		uint32_t imageIdx;

		void createCommandBuffer();
		void createSems();
		void creteFence();
		void createBuffers();
		void bufferData();
		void createUniformBuffers();
		void updateSets();
		void updateUniformBuffer(int curImage);
		//void uniformBufferData();

		void copyBuffer(vk::Buffer& src, vk::Buffer& dst, size_t size, size_t srcOffset, size_t dstOffset);
		void loadModel();
	};
	void recreateSwapChain(int width, int height);
}