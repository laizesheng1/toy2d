#include <context.h>
#include <iostream>

namespace toy2d {

	std::unique_ptr<Context> Context::_instance = nullptr;
void Context::Init(const std::vector<const char*>& extensions, CreateSurfaceFunc func)
{
	_instance.reset(new Context(extensions, func));
}

void Context::Quit()
{
	_instance.reset();
}

Context& Context::Getinstance()
{
	return *_instance;
}

void Context::CreateSwapchain(int w, int h)
{
	swapchain.reset(new Swapchain(w, h));		//在Context构造函数中，不存在其单例，在Swapchain构造函数不能使用其单例
}

void Context::InitcommandManager()
{
	commandManager = std::make_unique<CommandManager>();
}

Context::Context(const std::vector<const char*>& extension, CreateSurfaceFunc func) {
	
	createInstanceInfo(extension);
	pickupPhysicalDevice();
	surface = func(instance);		
	queryQueueFamilyIndices();		//物理设备需要先获取surface support
	createDevice();
	getQueue();
	renderProcess.reset(new RenderProcess());
}

void Context::createInstanceInfo(const std::vector<const char*>& extensions)
{
	vk::InstanceCreateInfo vkinfo;
	vk::ApplicationInfo appinfo;
	appinfo.setApiVersion(VK_API_VERSION_1_4);

	std::vector<const char*> layers = { "VK_LAYER_KHRONOS_validation" };

	vkinfo.setPApplicationInfo(&appinfo)
		.setPEnabledLayerNames(layers)
		.setPEnabledExtensionNames(extensions);
		

	instance = vk::createInstance(vkinfo);


	vk::Result result = vk::createInstance(&vkinfo, nullptr, &instance);

	if (result != vk::Result::eSuccess) {
		throw std::runtime_error("failed to create instance!");
	}
}

void Context::pickupPhysicalDevice()
{
	auto devices = instance.enumeratePhysicalDevices();
	if (devices.size() == 0)
		throw::std::runtime_error("failed to find GPUs with Vulkan support");

	physicaldevice = devices[0];
	//std::cout << physicaldevice.getProperties().deviceName << std::endl;

	if (physicaldevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}
	
}

void Context::createDevice()
{
	std::array extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	vk::DeviceCreateInfo createInfo;
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	float priorities = 1.0;			//队列的优先级
	if (queueFamilyIndices.graphicsFamily.value() == queueFamilyIndices.presentFamily.value())
	{
		vk::DeviceQueueCreateInfo queueCreateInfo;
		queueCreateInfo.setPQueuePriorities(&priorities)
			.setQueueCount(1)
			.setQueueFamilyIndex(queueFamilyIndices.graphicsFamily.value());
		queueCreateInfos.push_back(std::move(queueCreateInfo));
	}
	else {
		vk::DeviceQueueCreateInfo queueCreateInfo;
		queueCreateInfo.setPQueuePriorities(&priorities)
			.setQueueCount(1)
			.setQueueFamilyIndex(queueFamilyIndices.graphicsFamily.value());
		queueCreateInfos.push_back(queueCreateInfo);
		queueCreateInfo.setPQueuePriorities(&priorities)
			.setQueueCount(1)
			.setQueueFamilyIndex(queueFamilyIndices.presentFamily.value());
		queueCreateInfos.push_back(queueCreateInfo);
	}
	vk::PhysicalDeviceFeatures features;
	features.setSamplerAnisotropy(true);
	createInfo.setQueueCreateInfos(queueCreateInfos)
		.setPEnabledFeatures(&features)					//开启各项异性过滤
		.setPEnabledExtensionNames(extensions);

	device = physicaldevice.createDevice(createInfo);

	if (physicaldevice.createDevice(&createInfo, nullptr, &device) != vk::Result::eSuccess) {
		throw std::runtime_error("failed to create logical device!");
	}
}

void Context::queryQueueFamilyIndices()
{
	//查找带有图形能力的队列族对应的下标
	auto priorities = physicaldevice.getQueueFamilyProperties();
	int i = 0;
	for (auto& p : priorities)
	{
		if (p.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			queueFamilyIndices.graphicsFamily = i;
		}
		if (physicaldevice.getSurfaceSupportKHR(i, surface))
		{
			queueFamilyIndices.presentFamily = i;
		}
		i++;
		if (queueFamilyIndices)
		{
			break;
		}
	}
}

void Context::getQueue()
{
	graphcisQueue = device.getQueue(queueFamilyIndices.graphicsFamily.value(), 0);
	presentQueue = device.getQueue(queueFamilyIndices.presentFamily.value(), 0);
}

Context::~Context() {
	commandManager.reset();
	renderProcess.reset();
	swapchain.reset();				//潜在的问题：调用instance_.reset()时，会调用~Context()，此时会调用~Swapchain():使用到了instance_这个指针，但此时Context内容还是完整的？
	instance.destroySurfaceKHR(surface);
	device.destroy();		//先销毁逻辑设备（与instance有关
	instance.destroy();
}
		
}