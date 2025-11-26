#ifndef IMAGE_H
#define IMAGE_H
#include <buffer.h>
#include <descriptor_manager.h>

namespace toy2d {
	class ImageManager;
	class Image final {
	public:
		friend class ImageManager;

		~Image();
		vk::Image image;
		vk::ImageView imageView;
		vk::Sampler TextureSampler;
		DescriptorSetManager::SetInfo setInfo;
	private:
		int w, h;
		std::unique_ptr<Buffer> ImageBuffer;
		vk::DeviceMemory memory;

		Image(std::string filename);
		void createImage(vk::MemoryPropertyFlagBits property);			//创建由设备内存支持的图像对象
		vk::MemoryAllocateInfo queryImageInfo(vk::MemoryPropertyFlagBits property);
		void transitionImageLayoutFromUndefine2Dst();
		void transitionImageLayoutFromDst2Optimal();
		void transformData2Image(Buffer& buffer);
		void createImageView();
		void createTextureSampler();
		void updateDescriptorSet();
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
		Image* load(std::string filename);
		void Destroy(Image* textureImgae);
		void Clear();

	private:
		static std::unique_ptr<ImageManager> instance;
		std::vector<std::unique_ptr<Image>> data;
	};
}
#endif