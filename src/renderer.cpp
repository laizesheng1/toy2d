#include <renderer.h>
#include <iostream>
#include <context.h>

toy2d::Renderer::Renderer(int maxFrames):maxFlightCount_(maxFrames),curFrame_(0)
{
	createCommandBuffer();
	createSems();
	creteFence();
}

toy2d::Renderer::~Renderer()
{
	auto& device = Context::Getinstance().device;

	for(auto& finish:imageDrawFinishs)
		device.destroySemaphore(finish);
	for(auto& avia:imageAvaliables)
		device.destroySemaphore(avia);
	for(auto& fence:cmdAvaliableFences)
		device.destroyFence(fence);
}

void toy2d::Renderer::render()
{
	auto& device = Context::Getinstance().device;
	auto& renderProcess = Context::Getinstance().renderProcess;
	auto& swapchain = Context::Getinstance().swapchain;

	if (device.waitForFences(cmdAvaliableFences[curFrame_], true, std::numeric_limits<uint64_t>::max())
		!= vk::Result::eSuccess)
	{
		std::cout << "wait for fence failed" << std::endl;
	}
	device.resetFences(cmdAvaliableFences[curFrame_]);
	//Context::Getinstance().presentQueue.waitIdle();		//在每一帧开头等待前一帧的呈现操作完全结束，gpu浪费

	//从交换链获取一张图片
	auto result = device.acquireNextImageKHR(swapchain->swapchain, std::numeric_limits<uint64_t>::max(), imageAvaliables[curFrame_],nullptr);
	if (result.result != vk::Result::eSuccess)
	{
		std::cout << "acquire next image failed!" << std::endl;
	}

	auto imageIdx = result.value;
	cmdBuffer_[curFrame_].reset();

	vk::CommandBufferBeginInfo begininfo;
	begininfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);			//指令缓冲在执行一次后，就被用来记录新的指令（只使用一次
	cmdBuffer_[curFrame_].begin(begininfo);			//记录指令到指令缓冲
	{
		vk::Rect2D area;
		area.setOffset({ 0,0 })
			.setExtent(swapchain->info.imageExtent);
		vk::ClearValue value;
		value.color = vk::ClearColorValue(std::array<float, 4>{0.1f, 0.1f, 0.1f, 1.0f});
		vk::RenderPassBeginInfo renderPassbegin;
		renderPassbegin.setFramebuffer(swapchain->framebuffers[imageIdx])		//设置对应的 帧缓存
			.setRenderArea(area)				//指定用于渲染的区域
			.setRenderPass(renderProcess->renderPass)			//设置渲染流程
			.setClearValues(value);
		cmdBuffer_[curFrame_].beginRenderPass(renderPassbegin, {});
		{
			cmdBuffer_[curFrame_].bindPipeline(vk::PipelineBindPoint::eGraphics, renderProcess->pipline);			//先绑定图形管线
			cmdBuffer_[curFrame_].draw(3, 1, 0, 0);			//再进行绘制，（顶点数量，图元（实例）数量，要绘制的第一个顶点索引，要绘制的第一个图元索引）
		}
		cmdBuffer_[curFrame_].endRenderPass();
	}
	cmdBuffer_[curFrame_].end();

	vk::SubmitInfo submit;
	vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	submit.setCommandBuffers(cmdBuffer_[curFrame_])
		.setWaitSemaphores(imageAvaliables[curFrame_])
		.setWaitDstStageMask(waitStages)
		.setSignalSemaphores(imageDrawFinishs[curFrame_]);				//发出渲染已经结果，可以开始呈现的信号
	Context::Getinstance().graphcisQueue.submit(submit, cmdAvaliableFences[curFrame_]);			//提交指令缓冲给图形指令队列

	vk::PresentInfoKHR present;
	present.setImageIndices(imageIdx)
		.setSwapchains(swapchain->swapchain)
		.setWaitSemaphores(imageDrawFinishs[curFrame_]);
	if (Context::Getinstance().presentQueue.presentKHR(present) != vk::Result::eSuccess) {		//请求交换链进行图像呈现操作
		std::cout << "image present failed" << std::endl;
	}

	curFrame_ = (curFrame_ + 1) % maxFlightCount_;
}

void toy2d::Renderer::createSems()
{
	auto& device = Context::Getinstance().device;
	vk::SemaphoreCreateInfo info;

	imageAvaliables.resize(maxFlightCount_);
	imageDrawFinishs.resize(maxFlightCount_);

	for (auto& sem : imageAvaliables) {
		sem = device.createSemaphore(info);
	}

	for (auto& sem : imageDrawFinishs) {
		sem = device.createSemaphore(info);
	}
}

void toy2d::Renderer::creteFence()
{
	cmdAvaliableFences.resize(maxFlightCount_,nullptr);

	for (auto& fence : cmdAvaliableFences)
	{
		vk::FenceCreateInfo createinfo;
		createinfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
		fence = Context::Getinstance().device.createFence(createinfo);
	}	
}

void toy2d::Renderer::createCommandBuffer()
{
	cmdBuffer_.resize(maxFlightCount_);

	for (auto& cmd : cmdBuffer_) {
		cmd = Context::Getinstance().commandManager->CreateOneCommandBuffer();
	}
}
