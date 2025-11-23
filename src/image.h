#pragma once
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <buffer.h>

namespace toy2d {
	class Image final {
	public:
		Image(std::string filename);
		~Image();
		std::unique_ptr<Buffer> ImageBuffer;
		vk::Image image;
		//std::unique_ptr<Buffer> deviceImageBuffer;
	private:
		int w, h;
		stbi_uc* pixels;
		VkImage textureImage;
		VkDeviceMemory textureImageMemory;
		void createImageBuffer();
		void bufferImageData();
		void createImage(vk::MemoryPropertyFlagBits property);
		vk::MemoryAllocateInfo queryImageInfo(vk::MemoryPropertyFlagBits property);

	};
}

