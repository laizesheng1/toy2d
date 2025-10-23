#include <shader.h>
#include <context.h>
#include <iostream>

namespace toy2d {
	std::unique_ptr<Shader> Shader::instance_ = nullptr;

	void Shader::Init(const std::string& vertexSource, const std::string& fragSource)
	{
		instance_.reset(new Shader(vertexSource, fragSource));
	}

	void Shader::Quit()
	{
		instance_.reset();
	}

	Shader& Shader::GetInstance()
	{
		return *instance_;
	}

	std::vector<vk::PipelineShaderStageCreateInfo> Shader::GetStages()
	{
		return stages;
	}

	void Shader::InitStages()
	{
		stages.resize(2);
		stages[0].setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(vertexModule)
			.setPName("main");
		stages[1].setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(fragmentModule)
			.setPName("main");
	}

	Shader::Shader(const std::string& vertexSource, const std::string& fragSource)
	{
		if (vertexSource.empty() || fragSource.empty())
		{
			//std::cout << "vertexSource file and fragSource file is empty" << std::endl;
			throw std::runtime_error("vertexSource file and fragSource file is empty");
		}
		vk::ShaderModuleCreateInfo createInfo;
		createInfo.codeSize = vertexSource.size();
		createInfo.pCode = (uint32_t*)vertexSource.data();
		vertexModule = Context::Getinstance().device.createShaderModule(createInfo);

		createInfo.codeSize = fragSource.size();
		createInfo.pCode = (uint32_t*)fragSource.data();
		fragmentModule = Context::Getinstance().device.createShaderModule(createInfo);

		InitStages();
	}

	Shader::~Shader()
	{
		auto& device = Context::Getinstance().device;
		device.destroyShaderModule(vertexModule);
		device.destroyShaderModule(fragmentModule);
	}
}
