#pragma once

#include <pangolin/pangolin.h>
#include "../image.h"

// Pangolin uses HWC instead of CHW
pangolin::TypedImage toPangolin(const Image& im);
Image fromPangolin(const pangolin::Image<unsigned char>& a, const pangolin::PixelFormat& fmt);
Image feat_norm(const Image& a);
Image download_framebuffer(void);
