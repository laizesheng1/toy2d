#ifndef IMAGE_H
#define IMAGE_H
#include <buffer.h>


namespace toy2d {
	class Image final {
	public:
		Image(std::string filename);
		~Image();
		vk::Image image;
		vk::ImageView imageView;
		vk::Sampler TextureSampler;
	private:
		int w, h;
		std::unique_ptr<Buffer> ImageBuffer;
		vk::DeviceMemory memory;

		void createImage(vk::MemoryPropertyFlagBits property);			//创建由设备内存支持的图像对象
		vk::MemoryAllocateInfo queryImageInfo(vk::MemoryPropertyFlagBits property);
		void transitionImageLayoutFromUndefine2Dst();
		void transitionImageLayoutFromDst2Optimal();
		void transformData2Image(Buffer& buffer);
		void createImageView();
		void createTextureSampler();
		//void updateDescriptorSet();
	};
}
#endif
