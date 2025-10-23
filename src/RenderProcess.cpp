#include "RenderProcess.h"
#include <context.h>

toy2d::RenderProcess::~RenderProcess()
{
	auto& device = Context::Getinstance().device;
	device.destroyRenderPass(renderPass);
	device.destroyPipelineLayout(piplineLayout);
	device.destroyPipeline(pipline);
}

void toy2d::RenderProcess::InitPipeline(int width, int height)
{
	vk::GraphicsPipelineCreateInfo createInfo;
	//1. Vertex Input
	vk::PipelineVertexInputStateCreateInfo inputState;			//在shder固定顶点输入，故没有传递具体参数
	createInfo.setPVertexInputState(&inputState);

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
	vk::Viewport viewport(0.0, 0.0, width, height, 0.0, 1.0);
	vk::Rect2D rect({ 0,0 }, { static_cast<uint32_t>(width), static_cast<uint32_t>(height) });
	StateCreateInfo.setViewports(viewport)
		.setScissors(rect);				//裁剪大小
	createInfo.setPViewportState(&StateCreateInfo);

	//5.Rasterization
	vk::PipelineRasterizationStateCreateInfo RasterInfo;
	RasterInfo.setRasterizerDiscardEnable(false)		//几何图元都会通过光栅化阶段
		.setCullMode(vk::CullModeFlagBits::eBack)			//背面剔除
		//.setFrontFace(vk::FrontFace::eCounterClockwise)			//逆时针为正面
		.setFrontFace(vk::FrontFace::eClockwise)
		.setPolygonMode(vk::PolygonMode::eFill)			//指定几何图元生成片段，eFill：整个多边形，包括多边形内部都产生片段
		.setLineWidth(1);
	createInfo.setPRasterizationState(&RasterInfo);

	// 6. multisample
	vk::PipelineMultisampleStateCreateInfo multisampleInfo;
	multisampleInfo.setMinSampleShading(false)
		.setRasterizationSamples(vk::SampleCountFlagBits::e1);		//VK_SAMPLE_COUNT_1_BIT
	createInfo.setPMultisampleState(&multisampleInfo);

	//7.test -> tenil test , depth test

	//8.blending
	vk::PipelineColorBlendStateCreateInfo blendInfo;
	vk::PipelineColorBlendAttachmentState attach;
	attach.setBlendEnable(false)			//不进行混合，对单个颜色附件
		.setColorWriteMask(vk::ColorComponentFlagBits::eA |
						   vk::ColorComponentFlagBits::eB |
					       vk::ColorComponentFlagBits::eG |
						   vk::ColorComponentFlagBits::eR);

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
	vk::PipelineLayoutCreateInfo createInfo;
	piplineLayout = Context::Getinstance().device.createPipelineLayout(createInfo);
}

void toy2d::RenderProcess::InitRenderPass()
{
	vk::RenderPassCreateInfo createInfo;
	vk::AttachmentDescription attachDes;
	attachDes.setFormat(Context::Getinstance().swapchain->info.format.format)			//颜色缓冲附着的格式
		.setInitialLayout(vk::ImageLayout::eUndefined)					//指定渲染流程开始前的图像布局方式
		//.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)			//图像被用作颜色附着
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR)		// 图像被用在交换链中进行呈现操作
		.setLoadOp(vk::AttachmentLoadOp::eClear)			//在渲染之前清除附着的内容
		.setStoreOp(vk::AttachmentStoreOp::eStore)			//渲染的内容会被存储起来，以便之后读取
		.setSamples(vk::SampleCountFlagBits::e1)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)			//未使用模板缓冲
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
	createInfo.setAttachments(attachDes);

	vk::AttachmentReference reference;
	reference.setAttachment(0)			//引用的附着在附着描述结构体数组(AttachmentDescription数组）中的索引
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	vk::SubpassDescription subpassDes;
	subpassDes.setColorAttachments(reference)			//设置的颜色附着在数组中的索引会被片段着色器使用
		.setColorAttachmentCount(1)
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	createInfo.setSubpasses(subpassDes);

	//设置渲染流程依赖
	vk::SubpassDependency dependency;
	dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)			//默认的隐含的init renderPass
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead)	
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
	createInfo.setDependencies(dependency);

	renderPass = Context::Getinstance().device.createRenderPass(createInfo);
}


