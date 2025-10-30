#include <toy2d.h>
#include <filesystem>

namespace toy2d {
	std::unique_ptr<Renderer> renderer;
	void Init(const std::vector<const char*>& extensions, CreateSurfaceFunc func, int w, int h)
	{
		Context::Init(extensions, func);
		Context::Getinstance().CreateSwapchain(w, h);		
		Shader::Init(ReadShaderFile("./shader.vert.spv"), ReadShaderFile("./shader.frag.spv"));

		//Context::Getinstance().CreateRenderProcess();
		Context::Getinstance().renderProcess->InitPipelineLayout();
		Context::Getinstance().renderProcess->InitRenderPass();
		Context::Getinstance().swapchain->createFramerbuffers(w, h);		//这里才真正使用了renderpass
		Context::Getinstance().renderProcess->InitPipeline(w, h);
		Context::Getinstance().InitcommandManager();
		renderer=std::make_unique<Renderer>();
	}

	void Quit() {
		Context::Getinstance().device.waitIdle();			//cpu等待GPU所有操作完成之后再执行
		renderer.reset();
		Shader::Quit();
		Context::Quit();
	}
	Renderer* Getrender()
	{
		return renderer.get();
	}
}

