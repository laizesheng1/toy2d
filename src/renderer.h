#pragma once
#include <vulkan/vulkan.hpp>
#include <Uniform.h>
#include <utils.h>
#include "image.h"

namespace toy2d {
	class Buffer;
	class Renderer final{
	public:
		Renderer(int maxFrames=2);
		~Renderer();
		void SetDrawColor(const Color& color);
		void SetVPMat(int w, int h);
		void StartRender();
		void DrawTexture(Rec2D rec, Image* textureImage);
		void EndRender();

	private:
		vk::CommandPool cmdPool_;
		std::vector<vk::CommandBuffer> cmdBuffer_;
		std::vector<vk::Semaphore> imageAvaliables;			//发出图像已经被获取，可以开始渲染的信号
		std::vector<vk::Semaphore> imageDrawFinishs;			//渲染已经结果，可以开始呈现的信号，访问命令队列
		std::vector<vk::Fence> cmdAvaliableFences;			//用于cpu、gpu之间的同步

		std::unique_ptr<Buffer> VertexBuffer;
		std::unique_ptr<Buffer> IndicesBuffer;
		std::vector<std::unique_ptr<Buffer>> hostUniformBuffer;
		std::vector<std::unique_ptr<Buffer>> deviceUniformBuffer;
		std::vector<std::unique_ptr<Buffer>> ColorBuffer;
		std::vector<std::unique_ptr<Buffer>> deviceColorBuffer;
		std::unique_ptr<Buffer> hostImageBuffer;
		std::unique_ptr<Buffer> deviceImageBuffer;
		std::vector<DescriptorSetManager::SetInfo> desSets;

		int maxFlightCount_;
		int curFrame_;
		uint32_t imageIdx;
		glm::mat4 projectMat_;
		glm::mat4 viewMat_;

		void createCommandBuffer();
		void createSems();
		void creteFence();
		void createBuffers();
		void bufferData();
		void createUniformBuffers();
		void updateSets();
		void updateUniformBuffer(int curImage);
		void uniformBufferData();

		void copyBuffer(vk::Buffer& src, vk::Buffer& dst, size_t size, size_t srcOffset, size_t dstOffset);
		
	};
}