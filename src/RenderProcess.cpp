#include "RenderProcess.h"
#include <context.h>
#include <Vertex.h>
#include <Uniform.h>
#include <iostream>
#include <swap_chain.h>

toy2d::RenderProcess::~RenderProcess()
{
	auto& device = Context::Getinstance().device;
	for(auto& setLayout:setLayouts)
		device.destroyDescriptorSetLayout(setLayout);
	device.destroyRenderPass(renderPass);
	device.destroyPipelineLayout(piplineLayout);
	device.destroyPipeline(pipline);
}

void toy2d::RenderProcess::InitPipeline()
{
	auto& swapchain = Context::Getinstance().swapchain;
	vk::GraphicsPipelineCreateInfo createInfo;
	//1. Vertex Input
	vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo;
	auto attribute = Vertex::GetAttributeDescriptions();
	auto binding = Vertex::GetBinding();
	vertexInputCreateInfo.setVertexBindingDescriptions(binding)
		.setVertexAttributeDescriptions(attribute);
	createInfo.setPVertexInputState(&vertexInputCreateInfo);

	//2. Vertex Assembly
	vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
	inputAssembly.setPrimitiveRestartEnable(false)			//PrimitiveTopology带有_strip后缀可以 复用顶点缓冲中的顶点数据
		.setTopology(vk::PrimitiveTopology::eTriangleList);
	createInfo.setPInputAssemblyState(&inputAssembly);

	//3.Shader
	auto stages = Shader::GetInstance().GetStages();
	createInfo.setStages(stages);

	//4. viewport
	vk::PipelineViewportStateCreateInfo StateCreateInfo;
	vk::Viewport viewport(0.0, 0.0, swapchain->GetExtent().width, swapchain->GetExtent().height, 0.0, 1.0);
	vk::Rect2D rect({ 0,0 }, swapchain->GetExtent());
	StateCreateInfo.setViewports(viewport)
		.setScissors(rect);				//裁剪大小
	createInfo.setPViewportState(&StateCreateInfo);

	//5.Rasterization
	vk::PipelineRasterizationStateCreateInfo RasterInfo;
	RasterInfo.setRasterizerDiscardEnable(false)		//几何图元都会通过光栅化阶段
		//.setCullMode(vk::CullModeFlagBits::eBack)			//背面剔除
		.setFrontFace(vk::FrontFace::eCounterClockwise)			//逆时针为正面，在投影矩阵中进行了 Y 翻转，顶点现在是以逆时针顺序而不是顺时针顺序绘制的
		.setFrontFace(vk::FrontFace::eClockwise)
		.setPolygonMode(vk::PolygonMode::eFill)			//指定几何图元生成片段，eFill：整个多边形，包括多边形内部都产生片段
		.setLineWidth(1);
	createInfo.setPRasterizationState(&RasterInfo);

	// 6. multisample
	vk::PipelineMultisampleStateCreateInfo multisampleInfo;
	multisampleInfo.setMinSampleShading(false)
		.setRasterizationSamples(Context::Getinstance().imageInfo->msaaSamples);		//VK_SAMPLE_COUNT_1_BIT
	createInfo.setPMultisampleState(&multisampleInfo);

	//7.test -> tenil test , depth test
	vk::PipelineDepthStencilStateCreateInfo depthStencilInfo;
	depthStencilInfo.setDepthTestEnable(true)
		.setDepthWriteEnable(true)
		.setDepthCompareOp(vk::CompareOp::eLess)			//较低深度 = 更近
		.setDepthBoundsTestEnable(false)			//通过深度测试的片段的新深度实际写入深度缓冲区
		.setStencilTestEnable(false);
	createInfo.setPDepthStencilState(&depthStencilInfo);

	//8.blending
	vk::PipelineColorBlendStateCreateInfo blendInfo;
	vk::PipelineColorBlendAttachmentState attach;
	attach.setBlendEnable(true)			//进行混合，对单个颜色附件
		.setColorWriteMask(vk::ColorComponentFlagBits::eA |
						   vk::ColorComponentFlagBits::eB |
						   vk::ColorComponentFlagBits::eG |
						   vk::ColorComponentFlagBits::eR)
		.setSrcColorBlendFactor(vk::BlendFactor::eOne)
		.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
		.setColorBlendOp(vk::BlendOp::eAdd)
		.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
		.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
		.setAlphaBlendOp(vk::BlendOp::eAdd);

	blendInfo.setLogicOpEnable(false)			//所有颜色附件（全局设置），实现特殊的位运算视觉效果
		.setAttachments(attach);
	createInfo.setPColorBlendState(&blendInfo);
	//9. renderPass and layout
	createInfo.setRenderPass(renderPass)			//只是记录了renderPass的配置（上下文）
		.setLayout(piplineLayout);

	auto result=Context::Getinstance().device.createGraphicsPipeline(nullptr, createInfo);
	if (result.result != vk::Result::eSuccess)
	{
		throw std::runtime_error("Create Pipeline failed");
	}
	else {
		pipline = result.value;
	}
}

void toy2d::RenderProcess::InitPipelineLayout()
{
	CreateSetLayout();
	vk::PipelineLayoutCreateInfo createInfo;			//PipelineLayout:[setLayout1, setLayout2]-> DescriptorSetLayout:-[binding1 binding2 ]  -set ->DescriptorSetLayoutBinding: binding 
	createInfo.setSetLayouts(setLayouts);
	piplineLayout = Context::Getinstance().device.createPipelineLayout(createInfo);
}

void toy2d::RenderProcess::InitRenderPass()
{
	vk::RenderPassCreateInfo createInfo;
	vk::AttachmentDescription ColorattachDes;
	ColorattachDes.setFormat(Context::Getinstance().swapchain->info.format.format)			//颜色缓冲附着的格式
		.setInitialLayout(vk::ImageLayout::eUndefined)					//指定渲染流程开始前的图像布局方式
		.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)			//图像被用作颜色附着, 多重采样的图像不能直接呈现
		.setLoadOp(vk::AttachmentLoadOp::eClear)			//在渲染之前清除附着的内容
		.setStoreOp(vk::AttachmentStoreOp::eStore)			//渲染的内容会被存储起来，以便之后读取
		.setSamples(Context::Getinstance().imageInfo->msaaSamples)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)			//未使用模板缓冲
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

	vk::AttachmentDescription depthAttachment;
	vk::Format format = Context::Getinstance().imageInfo->depthImage->format;
	depthAttachment.setFormat(format)
		.setSamples(Context::Getinstance().imageInfo->msaaSamples)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)				//不关心之前的深度内容
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::AttachmentDescription colorAttachmentResolve;
	colorAttachmentResolve.setFormat(Context::Getinstance().swapchain->info.format.format)			//颜色缓冲附着的格式
		.setInitialLayout(vk::ImageLayout::eUndefined)					//指定渲染流程开始前的图像布局方式
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR)			//图像被用在交换链中进行呈现操作
		.setLoadOp(vk::AttachmentLoadOp::eDontCare)			//在渲染之前清除附着的内容
		.setStoreOp(vk::AttachmentStoreOp::eStore)			//渲染的内容会被存储起来，以便之后读取
		.setSamples(vk::SampleCountFlagBits::e1)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)			//未使用模板缓冲
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

	vk::AttachmentReference colorReference;
	colorReference.setAttachment(0)			//引用的附着在附着描述结构体数组(AttachmentDescription数组）中的索引
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	vk::AttachmentReference depthReference;
	depthReference.setAttachment(1)
		.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	vk::AttachmentReference colorResolveReference;
	colorResolveReference.setAttachment(2)
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	vk::SubpassDescription subpassDes;
	subpassDes.setColorAttachments(colorReference)			//设置的颜色附着在数组中的索引会被片段着色器使用
		.setColorAttachmentCount(1)
		.setPDepthStencilAttachment(&depthReference)
		.setResolveAttachments(colorResolveReference)
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

	//设置渲染流程依赖
	vk::SubpassDependency dependency;
	dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)			//默认的隐含的init renderPass
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests)
		.setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite| vk::AccessFlagBits::eColorAttachmentWrite)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite)
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests);
	std::vector<vk::AttachmentDescription> attachs = { ColorattachDes,depthAttachment,colorAttachmentResolve };
	createInfo.setSubpasses(subpassDes)
		.setAttachments(attachs)
		.setDependencies(dependency);

	renderPass = Context::Getinstance().device.createRenderPass(createInfo);
}

void toy2d::RenderProcess::CreateSetLayout()
{
	vk::DescriptorSetLayoutCreateInfo createInfo;
	auto UniformBindings = Uniform::getUniformBindings();
	auto SamplerBindings = Uniform::getSamplerBindings();			//两个uniform都是属于同一个set，在每一个shader中不能把set从0直接到1
	std::vector<vk::DescriptorSetLayoutBinding> bindings = { UniformBindings[0],SamplerBindings[0] };			//同一个set，不同的binding
	createInfo.setBindings(bindings);								
	setLayouts.push_back(Context::Getinstance().device.createDescriptorSetLayout(createInfo));
}


