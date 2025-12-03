#pragma once
#include <vulkan/vulkan.hpp>
#include <shader.h>

namespace toy2d {
	class RenderProcess final{
	public:
		vk::Pipeline pipline;
		vk::PipelineLayout piplineLayout;
		vk::RenderPass renderPass;
		std::vector<vk::DescriptorSetLayout> setLayouts;

		~RenderProcess();

		void InitPipeline();
		void InitPipelineLayout();
		void InitRenderPass();
		void CreateSetLayout();
	};
}