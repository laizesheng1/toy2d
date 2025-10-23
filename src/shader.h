#pragma once
#include <vulkan/vulkan.hpp>

namespace toy2d {
	class Shader final {
	public:
		Shader(const std::string& vertexSource, const std::string& fragSource);
		~Shader();
		static void Init(const std::string& vertexSource, const std::string& fragSource);
		static void Quit();
		
		static Shader& GetInstance();
		std::vector<vk::PipelineShaderStageCreateInfo> GetStages();
		vk::ShaderModule vertexModule;
		vk::ShaderModule fragmentModule;
		
	private:
		static std::unique_ptr<Shader> instance_;
		std::vector<vk::PipelineShaderStageCreateInfo> stages;

		void InitStages();
	};
}