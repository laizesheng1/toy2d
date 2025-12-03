#ifndef IMAGE_H
#define IMAGE_H
#include <buffer.h>
#include <descriptor_manager.h>

namespace toy2d {
	class ImageManager;
	void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlagBits property, vk::Image& image, vk::DeviceMemory& imageMemory);			//创建由设备内存支持的图像对象
	vk::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags);
	vk::Format findSupportFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
	vk::MemoryAllocateInfo queryImageInfo(vk::Image image, vk::MemoryPropertyFlagBits property);
	class Image final {
	public:
		friend class ImageManager;
		~Image();
		vk::Image TextureImage;
		vk::ImageView TextureImageView;
		vk::Sampler TextureSampler;
		DescriptorSetManager::SetInfo setInfo;
	private:
		int w, h;
		std::unique_ptr<Buffer> ImageBuffer;
		vk::DeviceMemory TextureMemory;

		Image(std::string filename);
		
		void transitionImageLayoutFromUndefine2Dst();
		void transitionImageLayoutFromDst2Optimal();
		void transformData2Image(Buffer& buffer);
		void createTextureSampler();
		void updateDescriptorSet();
	};

	class DepthImageInfo final{
	public:
		DepthImageInfo();
		void destroyDepthImage();

		vk::ImageView DepthImageView;				//befor createFramerbuffers
		vk::Format Depthformat;				//before InitRenderPass
	private:
		vk::Image DepthImage;
		vk::DeviceMemory DepthMemory;
		void createDepthResources();
	};

	class ImageManager final {
	public:
		static ImageManager& GetInstance()
		{
			if (!instance)
			{
				instance.reset(new ImageManager);
			}
			return *instance;
		}
		void load(std::string filename);
		void Destroy(Image* textureImgae);
		void Clear();
		Image* Get(size_t i);

	private:
		static std::unique_ptr<ImageManager> instance;
		std::vector<std::unique_ptr<Image>> data;
	};
}
#endif