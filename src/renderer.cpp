#include <renderer.h>
#include <iostream>
#include <context.h>
#include <buffer.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <chrono>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

toy2d::Renderer::Renderer(std::string model, int maxFrames) :maxFlightCount_(maxFrames), curFrame_(0), filename(model)
{
	createCommandBuffer();
	createSems();
	creteFence();
	loadModel();
	createBuffers();
	bufferData();
	createUniformBuffers();
	//uniformBufferData();
	desSets = DescriptorSetManager::Instance().AllocBufferSets(maxFlightCount_);
	updateSets();

	//projectMat_ = glm::identity<glm::mat4>();
	//viewMat_ = glm::identity<glm::mat4>();
	//SetDrawColor(Color{ 1.0, 1.0, 1.0 });
}

toy2d::Renderer::~Renderer()
{
	auto& device = Context::Getinstance().device;
	desSets.clear();
	for (auto& buffer : UniformBuffer)
	{
		buffer.reset();
	}
	UniformBuffer.clear();
	VertexBuffer.reset();
	IndicesBuffer.reset();

	for(auto& finish:imageDrawFinishs)
		device.destroySemaphore(finish);
	for(auto& avia:imageAvaliables)
		device.destroySemaphore(avia);
	for(auto& fence:cmdAvaliableFences)
		device.destroyFence(fence);
}

void toy2d::Renderer::endRender()
{
	auto& swapchain = Context::Getinstance().swapchain;
	auto& cmdBuf = cmdBuffer_[curFrame_];
	cmdBuf.endRenderPass();
	cmdBuf.end();

	vk::SubmitInfo submit;
	vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	submit.setCommandBuffers(cmdBuf)
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

void toy2d::Renderer::startRender()
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
	auto result = device.acquireNextImageKHR(swapchain->swapchain, std::numeric_limits<uint64_t>::max(), imageAvaliables[curFrame_], nullptr);
	if (result.result != vk::Result::eSuccess)
	{
		std::cout << "acquire next image failed!" << std::endl;
	}
	updateUniformBuffer(curFrame_);			//uniform对象：mvp、sampler 
	imageIdx = result.value;
	auto& cmdBuf = cmdBuffer_[curFrame_];
	cmdBuf.reset();

	vk::CommandBufferBeginInfo begininfo;
	begininfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);			//指令缓冲在执行一次后，就被用来记录新的指令（只使用一次
	cmdBuf.begin(begininfo);			//记录指令到指令缓冲

	vk::Rect2D area;
	area.setOffset({ 0,0 })
		.setExtent(swapchain->info.imageExtent);
	std::array<vk::ClearValue, 2> clearValues{};
	vk::ClearColorValue color = vk::ClearColorValue(std::array<float, 4>{0.1f, 0.1f, 0.1f, 1.0f});
	vk::ClearDepthStencilValue depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
	clearValues[0].setColor(color);
	clearValues[1].setDepthStencil(depthStencil);
	vk::RenderPassBeginInfo renderPassbegin;
	renderPassbegin.setFramebuffer(swapchain->framebuffers[imageIdx])		//设置对应的 帧缓存
		.setRenderArea(area)				//指定用于渲染的区域
		.setRenderPass(renderProcess->renderPass)			//设置渲染流程
		.setClearValues(clearValues);
	cmdBuf.beginRenderPass(renderPassbegin, vk::SubpassContents::eInline);
}

void toy2d::Renderer::DrawTexture()
{
	auto& renderProcess = Context::Getinstance().renderProcess;
	cmdBuffer_[curFrame_].bindPipeline(vk::PipelineBindPoint::eGraphics, renderProcess->pipline);			//先绑定图形管线
	cmdBuffer_[curFrame_].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, renderProcess->piplineLayout, 0, desSets[curFrame_].set, {});
	vk::DeviceSize offset = { 0 };
	cmdBuffer_[curFrame_].bindVertexBuffers(0, VertexBuffer->buffer, offset);
	cmdBuffer_[curFrame_].bindIndexBuffer(IndicesBuffer->buffer, 0, vk::IndexType::eUint32);
	cmdBuffer_[curFrame_].drawIndexed((uint32_t)indices.size(), 1, 0, 0, 0);
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

void toy2d::Renderer::createBuffers()
{
	VertexBuffer.reset(new Buffer(sizeof(vertices[0]) * vertices.size(),
		vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));			//eHostVisible用于从CPU写入数据
																										//eHostCoherent保证映射的内存的内容和缓冲关联的内存的内容一致
	IndicesBuffer.reset(new Buffer(sizeof(indices[0]) * indices.size(),
		vk::BufferUsageFlagBits::eIndexBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
}

void toy2d::Renderer::bufferData()
{
	memcpy(VertexBuffer->map, vertices.data(), (size_t)VertexBuffer->size);			//顶点数据复制到映射后的内存
	memcpy(IndicesBuffer->map, indices.data(), (size_t)IndicesBuffer->size);
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
	UniformBuffer.resize(maxFlightCount_);
	for (int i = 0; i < UniformBuffer.size(); i++)
	{
		auto& buffer = UniformBuffer[i];
		//在Buffer中已经map和unamp
		buffer.reset(new Buffer(sizeof(Uniform),
			vk::BufferUsageFlagBits::eUniformBuffer,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));			//eHostVisible用于从CPU写入数据
		//eHostCoherent保证映射的内存的内容和缓冲关联的内存的内容一致
	}
}

void toy2d::Renderer::updateSets()
{
	auto image = ImageManager::GetInstance().Get(0);
	for (int i = 0; i < desSets.size(); i++)
	{
		std::vector<vk::DescriptorBufferInfo> bufferInfo(1);
		std::vector<vk::WriteDescriptorSet> writes(2);
		//mvp
		bufferInfo[0].setBuffer(UniformBuffer[i]->buffer)
			.setOffset(0)
			.setRange(UniformBuffer[i]->size);
		
		writes[0].setBufferInfo(bufferInfo[0])
			.setDescriptorCount(1)				//更新的描述符数组元素的数量
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setDstArrayElement(0)				//描述符可以是数组，需要指定数组的第一个元素的索引
			.setDstBinding(0)
			.setDstSet(desSets[i].set);				//每一帧有一个描述符集，需要更新
		//sampler
		vk::DescriptorImageInfo imageInfo;
		imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setImageView(image->TextureImageView)
			.setSampler(image->TextureSampler);
		writes[1].setImageInfo(imageInfo)
			.setDstBinding(1)
			.setDstArrayElement(0)
			.setDstSet(desSets[i].set)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
		Context::Getinstance().device.updateDescriptorSets(writes, {});
	}
}

//1.使用“持久映射” ：一个缓冲区要HostCoherent，否则需要flush  2.这里使用 ：
void toy2d::Renderer::updateUniformBuffer(int curImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	Uniform uniform{};
	uniform.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	uniform.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	auto extent = Context::Getinstance().swapchain->info.imageExtent;
	uniform.proj = glm::perspective(glm::radians(45.0f), (float)extent.width / (float)extent.height, 0.1f, 10.0f);
	uniform.proj[1][1] *= -1;			//GLM 最初是为 OpenGL 设计的，其中裁剪坐标的 Y 坐标是反转的

	memcpy(UniformBuffer[curFrame_]->map, &uniform, sizeof(uniform));
}

void toy2d::recreateSwapChain(int width,int height)
{
	auto& device = Context::Getinstance().device;
	auto& old_swapchain = Context::Getinstance().swapchain;
	device.waitIdle();

	old_swapchain.reset();
	Context::Getinstance().getSurface();
	Context::Getinstance().CreateSwapchain(width, height);
	auto& swapchain = Context::Getinstance().swapchain;
	Context::Getinstance().InitImageInfo();
	swapchain->createFramerbuffers(width, height);
}

void toy2d::Renderer::loadModel()
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.data()))
	{
		std::cout << warn << err << std::endl;
		throw std::runtime_error(warn + err);
	}
	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};
			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};
			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};
			vertex.color = { 1.0f,1.0f,1.0f };
			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}
			indices.push_back(uniqueVertices[vertex]);
		}
	}
}