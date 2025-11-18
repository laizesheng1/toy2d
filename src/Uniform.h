#pragma once
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

namespace toy2d {
	using Color = glm::vec3;

	struct Uniform final {
		//Color color;
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;

		static vk::DescriptorSetLayoutBinding getBinding()
		{
			vk::DescriptorSetLayoutBinding binding;
			binding.setBinding(0)
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setStageFlags(vk::ShaderStageFlagBits::eFragment| vk::ShaderStageFlagBits::eVertex)				//描述符被使用的着色器阶段
				.setDescriptorCount(1);			//对应shader variable中的vbo可以是vbo[n]这里只有一个变量表示
			return binding;
		}
	};
}
