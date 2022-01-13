//============================================================================
// Name        : Image.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2016 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// GeoDiscoverer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GeoDiscoverer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GeoDiscoverer.  If not, see <http://www.gnu.org/licenses/>.
//
//============================================================================

#include <Core.h>
#include <Image.h>

namespace GEODISCOVERER {

Image::Image() {
  iconFolder=core->getHomePath() + "/Icon";
  abortLoad=false;
  initJPEG();
  initPNG();
}

Image::~Image() {
}

/**

IIR Gauss Filter v1.0
By Stephan Soller <stephan.soller@helionweb.de>
Based on the paper "Recursive implementation of the Gaussian filter" by Ian T. Young and Lucas J. van Vliet.
Licensed under the MIT license

QUICK START

	#include ...
	#include ...
	#define IIR_GAUSS_BLUR_IMPLEMENTATION
	#include "iir_gauss_blur.h"
	...
	int width = 0, height = 0, components = 1;
	uint8_t* image = stbi_load("foo.png", &width, &height, &components, 0);
	float sigma = 10;
	iir_gauss_blur(width, height, components, image, sigma);
	stbi_write_png("foo.blurred.png", width, height, components, image, 0);

This example uses stb_image.h to load the image, then blurs it and writes the result using stb_image_write.h.
`sigma` controls the strength of the blur. Higher values give you a blurrier image.

DOCUMENTATION

This is a single header file library. You'll have to define IIR_GAUSS_BLUR_IMPLEMENTATION before including this file to
get the implementation. Otherwise just the header will be included.

The library only has a single function: iir_gauss_blur(width, height, components, image, sigma).

- `width` and `height` are the dimensions of the image in pixels.
- `components` is the number of bytes per pixel. 1 for a grayscale image, 3 for RGB and 4 for RGBA.
  The function can handle an arbitrary number of channels, so 2 or 7 will work as well.
- `image` is a pointer to the image data with `width * height` pixels, each pixel having `components` bytes (interleaved
  8-bit components). There is no padding between the scanlines of the image.
  This is the format used by stb_image.h and stb_image_write.h and easy to work with.
- `sigma` is the strength of the blur. It's a number > 0.5 and most people seem to just eyeball it.
  Start with e.g. a sigma of 5 and go up or down until you have the blurriness you want.
  There are more informed ways to choose this parameter, see CHOOSING SIGMA below.

The function mallocs an internal float buffer with the same dimensions as the image. If that turns out to be a
bottleneck fell free to move that out of the function. The source code is quite short and straight forward (even if the
math isn't).

The function is an implementation of the paper "Recursive implementation of the Gaussian filter" by Ian T. Young and
Lucas J. van Vliet. It has nothing to do with recursive function calls, instead it's a special way to construct a
filter. Other (convolution based) gauss filters apply a kernel for each pixel and the kernel grows as sigma gets larger.
Meaning their performance degrades the more blurry you want your image to be.

Instead The algorithm in the paper gets it done in just a few passes: A horizontal forward and backward pass and a
vertical forward and backward pass. The work done is independent of the blur radius and so you can have ridiculously
large blur radii without any performance impact.

CHOOSING SIGMA

There seem to be several rules of thumb out there to get a sigma for a given "blur radius". Usually this is something
like `radius = 2 * sigma`. So if you want to have a blur radius of 10px you can use `sigma = (1.0 / 2.0) * radius` to
get the sigma for it (5.0). I'm not sure what that "radius" is supposed to mean though.

For my own projects I came up with two different kinds of blur radii and how to get a sigma for them: Given a big white
area on a black background, how far will the white "bleed out" into the surrounding black? How large is the distance
until the white (255) gets blurred down to something barely visible (smaller than 16) or even to nothing (smaller than
1)? There are to estimates to get the sigma for those radii:

	sigma = (1.0 / 1.42) * radius16;
	sigma = (1.0 / 3.66) * radius1;

Personally I use `radius16` to calculate the sigma when blurring normal images. Think: I want to blur a pixel across a
circle with the radius x so it's impact is barely visible at the edges.

When I need to calculate padding I use `radius1`: When I have a black border of 100px around the image I can use a
`radius1` of 100 and be reasonable sure that I still got black at the edges. So given a `radius1` blur strength I can
use it as a padding width as well.

I created those estimates by applying different sigmas (1 to 100) to a test image and measuring the effects with GIMP.
So take it with a grain of salt (or many). They're reasonable estimates but by no means exact. I tried to solve the
normal distribution to calculate the perfect sigma but gave up after a lot of confusion. If you know an exact solution
let me know. :)

VERSION HISTORY

v1.0  2018-08-30  Initial release

**/
bool Image::iirGaussFilter(ImagePixel *image, Int width, Int height, UInt pixelSize, float sigma) {

	// Create IDX macro but push any previous definition (and restore it later) so we don't overwrite a macro the user has possibly defined before us
	#pragma push_macro("IDX")
	#define IDX(x, y, n) ((y)*width*pixelSize + (x)*pixelSize + n)
	
	// Allocate buffers
	float* buffer = (float*)malloc(width * height * pixelSize * sizeof(buffer[0]));
  if (!buffer) {
    FATAL("can not reserve memory",NULL);
    return false;
  }
	
	// Calculate filter parameters for a specified sigma
	// Use Equation 11b to determine q, do nothing if sigma is to small (should have no effect) or negative (doesn't make sense)
	float q;
	if (sigma >= 2.5)
		q = 0.98711 * sigma - 0.96330;
	else if (sigma >= 0.5)
		q = 3.97156 - 4.14554 * sqrtf(1.0 - 0.26891 * sigma);
	else
		return false;
	
	// Use equation 8c to determine b0, b1, b2 and b3
	float b0 = 1.57825 + 2.44413*q + 1.4281*q*q + 0.422205*q*q*q;
	float b1 = 2.44413*q + 2.85619*q*q + 1.26661*q*q*q;
	float b2 = -( 1.4281*q*q + 1.26661*q*q*q );
	float b3 = 0.422205*q*q*q;
	// Use equation 10 to determine B
	float B = 1.0 - (b1 + b2 + b3) / b0;
	
	// Horizontal forward pass (from paper: Implement the forward filter with equation 9a)
	// The data is loaded from the byte image but stored in the float buffer
	for(unsigned int y = 0; y < height; y++) {
		float prev1[pixelSize], prev2[pixelSize], prev3[pixelSize];
		for(unsigned char n = 0; n < pixelSize; n++) {
			prev1[n] = image[IDX(0, y, n)];
			prev2[n] = prev1[n];
			prev3[n] = prev2[n];
		}
		
		for(unsigned int x = 0; x < width; x++) {
			for(unsigned char n = 0; n < pixelSize; n++) {
				float val = B * image[IDX(x, y, n)] + (b1 * prev1[n] + b2 * prev2[n] + b3 * prev3[n]) / b0;
				buffer[IDX(x, y, n)] = val;
				prev3[n] = prev2[n];
				prev2[n] = prev1[n];
				prev1[n] = val;
			}
		}
	}
	
	// Horizontal backward pass (from paper: Implement the backward filter with equation 9b)
	for(unsigned int y = height-1; y < height; y--) {
		float prev1[pixelSize], prev2[pixelSize], prev3[pixelSize];
		for(unsigned char n = 0; n < pixelSize; n++) {
			prev1[n] = buffer[IDX(width-1, y, n)];
			prev2[n] = prev1[n];
			prev3[n] = prev2[n];
		}
		
		for(unsigned int x = width-1; x < width; x--) {
			for(unsigned char n = 0; n < pixelSize; n++) {
				float val = B * buffer[IDX(x, y, n)] + (b1 * prev1[n] + b2 * prev2[n] + b3 * prev3[n]) / b0;
				buffer[IDX(x, y, n)] = val;
				prev3[n] = prev2[n];
				prev2[n] = prev1[n];
				prev1[n] = val;
			}
		}
	}
	
	// Vertical forward pass (from paper: Implement the forward filter with equation 9a)
	for(unsigned int x = 0; x < width; x++) {
		float prev1[pixelSize], prev2[pixelSize], prev3[pixelSize];
		for(unsigned char n = 0; n < pixelSize; n++) {
			prev1[n] = buffer[IDX(x, 0, n)];
			prev2[n] = prev1[n];
			prev3[n] = prev2[n];
		}
		
		for(unsigned int y = 0; y < height; y++) {
			for(unsigned char n = 0; n < pixelSize; n++) {
				float val = B * buffer[IDX(x, y, n)] + (b1 * prev1[n] + b2 * prev2[n] + b3 * prev3[n]) / b0;
				buffer[IDX(x, y, n)] = val;
				prev3[n] = prev2[n];
				prev2[n] = prev1[n];
				prev1[n] = val;
			}
		}
	}
	
	// Vertical backward pass (from paper: Implement the backward filter with equation 9b)
	// Also write the result back into the byte image
	for(unsigned int x = width-1; x < width; x--) {
		float prev1[pixelSize], prev2[pixelSize], prev3[pixelSize];
		for(unsigned char n = 0; n < pixelSize; n++) {
			prev1[n] = buffer[IDX(x, height-1, n)];
			prev2[n] = prev1[n];
			prev3[n] = prev2[n];
		}
		
		for(unsigned int y = height-1; y < height; y--) {
			for(unsigned char n = 0; n < pixelSize; n++) {
				float val = B * buffer[IDX(x, y, n)] + (b1 * prev1[n] + b2 * prev2[n] + b3 * prev3[n]) / b0;
				image[IDX(x, y, n)] = val;
				prev3[n] = prev2[n];
				prev2[n] = prev1[n];
				prev1[n] = val;
			}
		}
	}
	
	// Free temporary buffers and restore any potential IDX macro
	free(buffer);
	#pragma pop_macro("IDX")
  return true;
}

// Changes the brightness by the given value
void Image::brightnessFilter(ImagePixel *image, Int width, Int height, UInt pixelSize, Int brightnessOffset) {
	for (Int i=0;i<width*height*pixelSize;i++) {
		Int p=(Int)image[i]+brightnessOffset;
		if (p>255) p=255;
		if (p<0) p=0;
		image[i]=(ImagePixel)p;
	}
}

// Changes the hsv components by the given value
void Image::hsvFilter(ImagePixel *image, Int width, Int height, UInt pixelSize, double hOffset, double sOffset, double vOffset) {
	ImagePixel *curPixel=image;
	for (Int i=0;i<width*height;i++) {
		double h,s,v;
		rgb2hsv(curPixel,h,s,v);
		h+=hOffset; if (h<0) h=0; if (h>360.0) h=360.0;
		s+=sOffset; if (s<0) s=0; if (s>1.0) s=1.0;
		v+=vOffset; if (v<0) v=0; if (v>1.0) v=1.0;
		hsv2rgb(h,s,v,curPixel);
		curPixel+=pixelSize;
	}
}

// Changes the hsv components of the given PNG file by the given values
bool Image::hsvFilter(std::string pngFilename, double hOffset, double sOffset, double vOffset) {
	Int width,height;
	UInt pixelSize;
  ImagePixel *pixels=loadPNG(pngFilename,width,height,pixelSize,false);
  if (!pixels) {
    DEBUG("can not read <%s>",pngFilename.c_str());
    return false;		
  }
	//DEBUG("hOffset=%f sOffset=%f vOffset=%f",hOffset,sOffset,vOffset);
	hsvFilter(pixels,width,height,pixelSize,hOffset,sOffset,vOffset);
  writePNG(pixels,pngFilename,width,height,pixelSize,false);
  free(pixels);
	return true;
}

// Changes the brightness of the given PNG file
bool Image::brightnessFilter(std::string pngFilename, Int brightnessOffset) {
	Int width,height;
	UInt pixelSize;
  ImagePixel *pixels=loadPNG(pngFilename,width,height,pixelSize,false);
  if (!pixels) {
    DEBUG("can not read <%s>",pngFilename.c_str());
    return false;		
  }
	brightnessFilter(pixels,width,height,pixelSize,brightnessOffset);
  writePNG(pixels,pngFilename,width,height,pixelSize,false);
  free(pixels);
	return true;
} 

// Converts RGB pixel into HSL
void Image::rgb2hsv(ImagePixel *pixel, double &h, double &s, double &v) {
	double      min, max, delta;

	double r=pixel[0]/255.0;
	double g=pixel[1]/255.0;
	double b=pixel[2]/255.0;

	min = r < g ? r : g;
	min = min < b ? min : b;
	max = r > g ? r : g;
	max = max > b ? max : b;

	v = max;                        
	delta = max - min;
	if (delta < 0.00001)
	{
		s = 0;
		h = 0; 																// undefined, maybe nan?
		return;
	}
	if (max > 0.0 ) { 											// NOTE: if Max is == 0, this divide would cause a crash
		s = (delta / max);       
	} else {
		// if max is 0, then r = g = b = 0              
		// s = 0, h is undefined
		s = 0.0;
		h = NAN;                            	// its now undefined
		return;
	}
	if (r >= max)                           // > is bogus, just keeps compilor happy
		h = ( g - b ) / delta;        				// between yellow & magenta
	else
	if (g >= max)
		h = 2.0 + ( b - r ) / delta;  				// between cyan & yellow
	else
		h = 4.0 + ( r - g ) / delta;  				// between magenta & cyan

	h *= 60.0;                              // degrees

	if (h < 0.0 )
		h += 360.0;

	return;
}

// Converts RGB pixel into HSL
void Image::hsv2rgb(double h, double s, double v, ImagePixel *pixel) {
	double hh, p, q, t, ff;
	long i; 
	double r,g,b;

	if (s <= 0.0) {       	// < is bogus, just shuts up warnings
		r = v;
		g = v;
		b = v;

	} else {

		hh = h;
		if (hh >= 360.0) hh = 0.0;
		hh /= 60.0;
		i = (long)hh;
		ff = hh - i;
		p = v * (1.0 - s);
		q = v * (1.0 - (s * ff));
		t = v * (1.0 - (s * (1.0 - ff)));

		switch(i) {
		case 0:
			r = v;
			g = t;
			b = p;
			break;
		case 1:
			r = q;
			g = v;
			b = p;
			break;
		case 2:
			r = p;
			g = v;
			b = t;
			break;
		case 3:
			r = p;
			g = q;
			b = v;
			break;
		case 4:
			r = t;
			g = p;
			b = v;
			break;
		case 5:
		default:
			r = v;
			g = p;
			b = q;
			break;
		}
	}

	pixel[0]=r*255;
	pixel[1]=g*255;
	pixel[2]=b*255;
}

}
