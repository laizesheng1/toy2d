#include <renderer.h>
#include <iostream>
#include <context.h>

toy2d::Renderer::Renderer()
{
	initCmdPool();
	allocCmdBuffer();
	createSems();
	creteFence();
}

toy2d::Renderer::~Renderer()
{
	auto& device = Context::Getinstance().device;
	device.freeCommandBuffers(cmdPool_, cmdBuffer_);			//free
	device.destroyCommandPool(cmdPool_);
	device.destroySemaphore(imageDrawFinish_);
	device.destroySemaphore(imageAvaliable_);
	device.destroyFence(cmdAvaliableFence_);
}

void toy2d::Renderer::initCmdPool()
{
	vk::CommandPoolCreateInfo createinfo;
	createinfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
		//.setQueueFamilyIndex(Context::Getinstance().queueFamilyIndices.graphicsFamily.value());

	cmdPool_ = Context::Getinstance().device.createCommandPool(createinfo);
}

void toy2d::Renderer::allocCmdBuffer()
{
	vk::CommandBufferAllocateInfo allcInfo;
	allcInfo.setCommandPool(cmdPool_)
		.setCommandBufferCount(1)
		.setLevel(vk::CommandBufferLevel::ePrimary);		//可以被提交到队列进行执行，但不能被其它指令缓冲对象调用
	cmdBuffer_ = Context::Getinstance().device.allocateCommandBuffers(allcInfo)[0];			//allocateCommandBuffers分配的数组
}

void toy2d::Renderer::render()
{
	auto& device = Context::Getinstance().device;
	auto& renderProcess = Context::Getinstance().renderProcess;
	auto& swapchain = Context::Getinstance().swapchain;

	Context::Getinstance().presentQueue.waitIdle();		//在每一帧开头等待前一帧的呈现操作完全结束，gpu浪费

	//从交换链获取一张图片
	auto result = device.acquireNextImageKHR(swapchain->swapchain, std::numeric_limits<uint64_t>::max(), imageAvaliable_);
	if (result.result != vk::Result::eSuccess)
	{
		std::cout << "acquire next image failed!" << std::endl;
	}

	auto imageIdx = result.value;
	cmdBuffer_.reset();

	vk::CommandBufferBeginInfo begin;
	begin.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);			//指令缓冲在执行一次后，就被用来记录新的指令（只使用一次
	cmdBuffer_.begin(begin);			//记录指令到指令缓冲
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
		cmdBuffer_.beginRenderPass(renderPassbegin, {});
		{
			cmdBuffer_.bindPipeline(vk::PipelineBindPoint::eGraphics, renderProcess->pipline);			//先绑定图形管线
			cmdBuffer_.draw(3, 1, 0, 0);			//再进行绘制，（顶点数量，图元（实例）数量，要绘制的第一个顶点索引，要绘制的第一个图元索引）
		}
		cmdBuffer_.endRenderPass();
	}
	cmdBuffer_.end();

	vk::SubmitInfo submit;
	vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	submit.setCommandBuffers(cmdBuffer_)
		.setWaitSemaphores(imageAvaliable_)
		.setWaitDstStageMask(waitStages)
		.setSignalSemaphores(imageDrawFinish_);				//发出渲染已经结果，可以开始呈现的信号
	Context::Getinstance().graphcisQueue.submit(submit, cmdAvaliableFence_);			//提交指令缓冲给图形指令队列

	vk::PresentInfoKHR present;
	present.setImageIndices(imageIdx)
		.setSwapchains(swapchain->swapchain)
		.setWaitSemaphores(imageDrawFinish_);
	if (Context::Getinstance().presentQueue.presentKHR(present) != vk::Result::eSuccess) {		//请求交换链进行图像呈现操作
		std::cout << "image present failed" << std::endl;
	}

	if (device.waitForFences(cmdAvaliableFence_, true, std::numeric_limits<uint64_t>::max())
		!= vk::Result::eSuccess)
	{
		std::cout << "wait for fence failed" << std::endl;
	}
	device.resetFences(cmdAvaliableFence_);
}

void toy2d::Renderer::createSems()
{
	vk::SemaphoreCreateInfo createinfo;
	imageAvaliable_ = Context::Getinstance().device.createSemaphore(createinfo);
	imageDrawFinish_ = Context::Getinstance().device.createSemaphore(createinfo);
}

void toy2d::Renderer::creteFence()
{
	vk::FenceCreateInfo createinfo;
	//createinfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
	cmdAvaliableFence_ = Context::Getinstance().device.createFence(createinfo);
}