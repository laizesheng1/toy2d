#ifndef IMAGE_H
#define IMAGE_H
#include <buffer.h>
#include <descriptor_manager.h>

namespace toy2d {
	class ImageManager;
	class ImageBase {
	public:
		virtual ~ImageBase() = default;
		virtual void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlagBits property, vk::Image& image, vk::DeviceMemory& imageMemory, uint32_t mipLevels=1, vk::SampleCountFlagBits numSamples=vk::SampleCountFlagBits::e1);			//创建由设备内存支持的图像对象
		virtual vk::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels=1);
		virtual vk::Format findSupportFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
		virtual vk::MemoryAllocateInfo queryImageInfo(vk::Image image, vk::MemoryPropertyFlagBits property);
	};

	class Image :ImageBase {
	public:
		friend class ImageManager;
		~Image();
		vk::Image TextureImage;
		vk::ImageView TextureImageView;
		vk::Sampler TextureSampler;
		DescriptorSetManager::SetInfo setInfo;
	private:
		uint32_t mipLevels;
		int w, h;
		std::unique_ptr<Buffer> ImageBuffer;
		vk::DeviceMemory TextureMemory;

		Image(std::string filename);
		//image: eUndefined->Dst->shader read only
		void transitionImageLayoutFromUndefine2Dst();
		void transitionImageLayoutFromDst2Optimal();
		//behind eUndefined->Dst, Dst->src ->need src blit(copy) different level mipmap      ->shader read only
		void generateMipmaps();
		void transformData2Image(Buffer& buffer);
		void createTextureSampler();
		void updateDescriptorSet();
	};

	class ImageInfo :public ImageBase {
	public:
		struct ImageResources{
			vk::ImageView view;				//befor createFramerbuffers
			vk::Image Image;
			vk::DeviceMemory Memory;
			vk::Format format;				//before InitRenderPass
		};
		ImageInfo();
		void destroyDepthImage();
		vk::SampleCountFlagBits msaaSamples;
		ImageResources* depthImage=nullptr;
		ImageResources* colorImage=nullptr;			
		
	private:
		vk::SampleCountFlagBits getMaxUsableSampleCount();
		void DestroyResources(ImageResources* image);
		void createDepthResources();
		void createColorResources();
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