#pragma once
#include <vulkan/vulkan.hpp>
#include <memory>
#include <optional>
#include <functional>
#include <swap_chain.h>
#include <RenderProcess.h>
#include <renderer.h>
#include <command_manager.h>

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	//将结构体的实例转换成一个 bool 类型的值
	explicit operator bool() const {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

namespace toy2d {
	using GetSurfaceCallback = std::function<vk::SurfaceKHR(vk::Instance)>;
class Context final {
public:
	static void Init(const std::vector<const char*>& extensions, GetSurfaceCallback func);
	static void Quit();
	static Context& Getinstance();
	void CreateSwapchain(int w, int h);
	void InitcommandManager();
	void InitImageInfo();
	void getSurface();
	~Context();

	vk::Instance instance;
	vk::PhysicalDevice physicaldevice = VK_NULL_HANDLE;			//物理设备
	vk::Device device;			//逻辑设备
	vk::Queue graphcisQueue;		//存储逻辑设备的队列
	vk::Queue presentQueue;
	vk::SurfaceKHR surface;
	QueueFamilyIndices queueFamilyIndices;
	std::unique_ptr<Swapchain> swapchain;
	std::unique_ptr<RenderProcess> renderProcess;
	std::unique_ptr<CommandManager> commandManager;
	std::unique_ptr<ImageInfo> imageInfo;
private:
	Context(const std::vector<const char*>& extension, GetSurfaceCallback func);
	static Context* _instance;

	GetSurfaceCallback getSurfaceCb_ = nullptr;

	void createInstanceInfo(const std::vector<const char*>& extensions);
	void pickupPhysicalDevice();
	void createDevice();		//创建逻辑设备
	void queryQueueFamilyIndices();
	void getQueue();
};

}