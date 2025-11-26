#pragma once
#ifndef TOY2D_H
#define TOY2D_H

#include <vulkan/vulkan.h>
#include <context.h>
#include <shader.h>
#include <utils.h>
#include "image.h"

namespace toy2d {
	void Init(const std::vector<const char*>& extension, CreateSurfaceFunc func, int w, int h);
	void Quit();
	Renderer* Getrender();
	Image* loadTextureImage(std::string filename);
	void DestroyTexture(Image* texture);
}
#endif // TOY2D_H
