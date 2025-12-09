#pragma once
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

namespace toy2d {
	using Color = glm::vec3;
	
	struct Uniform final {
		alignas(16) glm::mat4 model;				
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
		//Color color;

		static std::vector<vk::DescriptorSetLayoutBinding> getUniformBindings()
		{
			std::vector<vk::DescriptorSetLayoutBinding> bindings(1);
			//mvp
			bindings[0].setBinding(0)
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setStageFlags(vk::ShaderStageFlagBits::eVertex)				//描述符被使用的着色器阶段
				.setDescriptorCount(1);			//对应shader variable中的vbo可以是vbo[n]这里只有一个变量表示
			////Color
			//bindings[1].setBinding(1)
			//	.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			//	.setStageFlags(vk::ShaderStageFlagBits::eFragment)				//描述符被使用的着色器阶段
			//	.setDescriptorCount(1);
			return bindings;
		}
		static std::vector<vk::DescriptorSetLayoutBinding> getSamplerBindings()
		{
			std::vector<vk::DescriptorSetLayoutBinding> bindings(1);
			//sample
			bindings[0].setBinding(1)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setStageFlags(vk::ShaderStageFlagBits::eFragment)				//描述符被使用的着色器阶段
				.setDescriptorCount(1);
			return bindings;
		}

		static vk::PushConstantRange GetPushConstantRange()
		{
			vk::PushConstantRange range;
			range.setOffset(0)
				.setSize(sizeof(glm::mat4))
				.setStageFlags(vk::ShaderStageFlagBits::eVertex);
			return range;
		}
	};
}
