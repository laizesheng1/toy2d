#pragma once
#include <vulkan/vulkan.hpp>
#include <shader.h>

namespace toy2d {
	class RenderProcess final{
	public:
		vk::Pipeline pipline;
		vk::PipelineLayout piplineLayout;
		vk::RenderPass renderPass;

		~RenderProcess();

		void InitPipeline(int width, int height);
		void InitPipelineLayout();
		void InitRenderPass();
	};
}