#include <renderer.h>
#include <iostream>
#include <context.h>
#include <buffer.h>
#include <Vertex.h>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

//namespace toy2d {
//	const std::array<Vertex, 3> vertexs = { {
//		{{0.0f, -0.5f}},
//		{{0.5f, 0.5f}},
//		{{-0.5f, 0.5f}}
//	} };
//	const Uniform uniform = { Color{1,0,0} };
//}

namespace toy2d {
	const std::array<Vertex, 3> vertexs = { {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
	} };
	Uniform uniform{};
}

toy2d::Renderer::Renderer(int maxFrames):maxFlightCount_(maxFrames),curFrame_(0)
{
	createCommandBuffer();
	createSems();
	creteFence();
	createVertexBuffer();
	bufferVertexData();
	createUniformBuffers();
	//bufferUniformData();
	createDescriptorPool();
	allocateSets();
	updateSets();
}

toy2d::Renderer::~Renderer()
{
	auto& device = Context::Getinstance().device;
	device.destroyDescriptorPool(descriptorPool);
	
	for (auto& buffer : hostUniformBuffer)
	{
		buffer.reset();
	}
	hostUniformBuffer.clear();
	//mapMemoryPtr.clear();
	//deviceUniformBuffer.clear();
	hostVertexBuffer.reset();
	deviceVertexBuffer.reset();

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
	updateUniformBuffer(curFrame_);
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
		cmdBuffer_[curFrame_].beginRenderPass(renderPassbegin, vk::SubpassContents::eInline);
		{
			cmdBuffer_[curFrame_].bindPipeline(vk::PipelineBindPoint::eGraphics, renderProcess->pipline);			//先绑定图形管线
			cmdBuffer_[curFrame_].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, renderProcess->piplineLayout, 0, desSets[curFrame_], {});
			vk::DeviceSize offset = { 0 };
			cmdBuffer_[curFrame_].bindVertexBuffers(0, deviceVertexBuffer->buffer, offset);
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

void toy2d::Renderer::createVertexBuffer()
{
	hostVertexBuffer.reset(new Buffer(sizeof(vertexs[0]) * vertexs.size(),
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));			//eHostVisible用于从CPU写入数据
																										//eHostCoherent保证映射的内存的内容和缓冲关联的内存的内容一致

	deviceVertexBuffer.reset(new Buffer(sizeof(vertexs[0]) * vertexs.size(),
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal));
}

void toy2d::Renderer::bufferVertexData()
{
	void* ptr = Context::Getinstance().device.mapMemory(hostVertexBuffer->memory, 0, hostVertexBuffer->size);		//将缓冲关联的内存映射到CPU可以访问的内存
	memcpy(ptr, vertexs.data(), (size_t)hostVertexBuffer->size);			//顶点数据复制到映射后的内存
	Context::Getinstance().device.unmapMemory(hostVertexBuffer->memory);

	auto cmdBuf = Context::Getinstance().commandManager->CreateOneCommandBuffer();
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	cmdBuf.begin(beginInfo);
	{
		vk::BufferCopy region;
		region.setSize(hostVertexBuffer->size)
			.setDstOffset(0)
			.setSrcOffset(0);
		cmdBuf.copyBuffer(hostVertexBuffer->buffer, deviceVertexBuffer->buffer, region);			//把cpu缓冲中的数据传输到GPU中
	}
	cmdBuf.end();

	vk::SubmitInfo submit;
	submit.setCommandBuffers(cmdBuf);
	Context::Getinstance().graphcisQueue.submit(submit);
	Context::Getinstance().device.waitIdle();			//等待传输完成
	Context::Getinstance().commandManager->FreeCmd(cmdBuf);
}

void toy2d::Renderer::copyBuffer(vk::Buffer& src, vk::Buffer& dst, size_t size, size_t srcOffset, size_t dstOffset)
{
	auto cmdBuf = Context::Getinstance().commandManager->CreateOneCommandBuffer();
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	cmdBuf.begin(beginInfo);
	{
		vk::BufferCopy region;
		region.setSize(size)
			.setDstOffset(dstOffset)
			.setSrcOffset(srcOffset);
		cmdBuf.copyBuffer(src, dst, region);
	}
	cmdBuf.end();

	vk::SubmitInfo submit;
	submit.setCommandBuffers(cmdBuf);
	Context::Getinstance().graphcisQueue.submit(submit);
	Context::Getinstance().device.waitIdle();			//等待传输完成
	Context::Getinstance().commandManager->FreeCmd(cmdBuf);
}

void toy2d::Renderer::createUniformBuffers()
{
	hostUniformBuffer.resize(maxFlightCount_);
	deviceUniformBuffer.resize(maxFlightCount_);
	for (int i = 0; i < hostUniformBuffer.size(); i++)
	{
		auto& buffer = hostUniformBuffer[i];
		buffer.reset(new Buffer(sizeof(Uniform),
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));			//eHostVisible用于从CPU写入数据
		//eHostCoherent保证映射的内存的内容和缓冲关联的内存的内容一致
	}
	
	for (auto& buffer : deviceUniformBuffer)
	{
		buffer.reset(new Buffer(sizeof(Uniform),
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal));
	}
}

//void toy2d::Renderer::bufferUniformData()
//{
//	for (int i = 0; i < hostUniformBuffer.size(); i++)
//	{
//		auto& buffer = hostUniformBuffer[i];
//		void* ptr = Context::Getinstance().device.mapMemory(buffer->memory, 0, buffer->size);		//将缓冲关联的内存映射到CPU可以访问的内存
//		mapMemoryPtr[i] = ptr;
//		memcpy(mapMemoryPtr[i], &uniform, sizeof(uniform));			//uniform数据复制到映射后的内存
//		//Context::Getinstance().device.unmapMemory(buffer->memory);
//		//copyBuffer(buffer->buffer, deviceUniformBuffer[i]->buffer, buffer->size, 0, 0);
//	}
//}

void toy2d::Renderer::createDescriptorPool()
{
	vk::DescriptorPoolCreateInfo creatInfo;
	vk::DescriptorPoolSize poolsize;
	poolsize.setType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(maxFlightCount_);		//总的描述符数量
	creatInfo.setMaxSets(maxFlightCount_)			//几帧创建几个描述符集，一个描述符集对应一个uniform（shader中）即一个描述符
		.setPoolSizes({ poolsize });
	descriptorPool = Context::Getinstance().device.createDescriptorPool(creatInfo);
}

void toy2d::Renderer::allocateSets()
{
	std::vector<vk::DescriptorSetLayout> layouts(maxFlightCount_, Context::Getinstance().renderProcess->setLayout);
	vk::DescriptorSetAllocateInfo allocInfo;
	allocInfo.setDescriptorPool(descriptorPool)
		.setDescriptorSetCount(maxFlightCount_)
		.setSetLayouts(layouts);			//一个描述符集对象使用一个描述符布局，需要多个相同的描述符布局

	desSets = Context::Getinstance().device.allocateDescriptorSets(allocInfo);
}

void toy2d::Renderer::updateSets()
{
	for (int i = 0; i < desSets.size(); i++)
	{
		vk::DescriptorBufferInfo bufferInfo;
		bufferInfo.setBuffer(deviceUniformBuffer[i]->buffer)
			.setOffset(0)
			.setRange(deviceUniformBuffer[i]->size);
		vk::WriteDescriptorSet write;
		write.setBufferInfo(bufferInfo)
			.setDescriptorCount(1)				//更新的描述符数组元素的数量
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setDstArrayElement(0)				//描述符可以是数组，需要指定数组的第一个元素的索引
			.setDstBinding(0)
			.setDstSet(desSets[i]);				//每一帧有一个描述符集，需要更新
			
		Context::Getinstance().device.updateDescriptorSets(write, {});
	}
}

//1.使用“持久映射” ：一个缓冲区要HostCoherent，否则需要flush  2.这里不使用 ：
void toy2d::Renderer::updateUniformBuffer(int curImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	uniform.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	uniform.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	auto extent = Context::Getinstance().swapchain->info.imageExtent;
	uniform.proj = glm::perspective(glm::radians(45.0f), (float)extent.width / (float)extent.height, 0.1f, 10.0f);
	uniform.proj[1][1] *= -1;			//GLM 最初是为 OpenGL 设计的，其中裁剪坐标的 Y 坐标是反转的

	auto& buffer = hostUniformBuffer[curImage];
	void* ptr = Context::Getinstance().device.mapMemory(buffer->memory, 0, buffer->size);
	memcpy(ptr, &uniform, sizeof(uniform));
	Context::Getinstance().device.unmapMemory(buffer->memory);
	copyBuffer(buffer->buffer, deviceUniformBuffer[curImage]->buffer, buffer->size, 0, 0);
}

