//============================================================================
// Name        : Image.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
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

#include <png.h>
#include <Core.h>

namespace GEODISCOVERER {

// Ints the png part
void Image::initPNG() {
}

// Queries the dimension of the png without loading it
bool Image::queryPNG(std::string filepath, Int &width, Int &height) {
  png_byte header[8]; // 8 is the maximum size that can be checked
  ImagePixel **image=NULL;
  png_structp png_ptr=NULL;
  png_infop info_ptr=NULL;
  bool result=true;

  // Open file and test for it being a PNG
  FILE *fp = fopen(filepath.c_str(), "rb");
  if (!fp) {
    ERROR("can not open <%s> for reading",filepath.c_str());
    goto cleanup;
  }
  fread(header, 1, 8, fp);
  if (png_sig_cmp(header, 0, 8)) {
    result=false;
    goto cleanup;
  }

  // Initialize the library for reading this file
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    FATAL("can not create png read struct",NULL);
    goto cleanup;
  }
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    FATAL("can not create png info struct",NULL);
    goto cleanup;
  }
  if (setjmp(png_jmpbuf(png_ptr))) {
    ERROR("can not set position in png image <%s>",filepath.c_str());
    goto cleanup;
  }
  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);

  // Read the header
  png_read_info(png_ptr, info_ptr);
  width = info_ptr->width;
  height = info_ptr->height;

cleanup:

  // Deinit
  if (png_ptr) {
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
  }
  if (fp)
    fclose(fp);
  return result;
}


// Loads a png
ImagePixel *Image::loadPNG(std::string filepath, Int &width, Int &height, UInt &pixelSize) {

  png_byte header[8]; // 8 is the maximum size that can be checked
  ImagePixel *image=NULL;
  png_structp png_ptr=NULL;
  png_infop info_ptr=NULL;
  Int number_of_passes;

  // Open file and test for it being a PNG
  abortLoad=false;
  FILE *fp = fopen(filepath.c_str(), "rb");
  if (!fp) {
    ERROR("can not open <%s> for reading",filepath.c_str());
    goto cleanup;
  }
  fread(header, 1, 8, fp);
  if (png_sig_cmp(header, 0, 8)) {
    ERROR("file <%s> is not a PNG file",filepath.c_str());
    goto cleanup;
  }

  // Initialize the library for reading this file
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    FATAL("can not create png read struct",NULL);
    goto cleanup;
  }
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    FATAL("can not create png info struct",NULL);
    goto cleanup;
  }
  if (setjmp(png_jmpbuf(png_ptr))) {
    ERROR("can not set position in png image <%s>",filepath.c_str());
    goto cleanup;
  }
  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);

  // Read the header
  png_read_info(png_ptr, info_ptr);

  // Convert palette to RGB
  if (info_ptr->color_type==PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(png_ptr);
  }

  // Set number of passes
  number_of_passes = png_set_interlace_handling(png_ptr);
  png_read_update_info(png_ptr, info_ptr);

  // Get information about the picture
  width = info_ptr->width;
  height = info_ptr->height;

  // Do some sanity check
  if (info_ptr->color_type==PNG_COLOR_TYPE_RGB_ALPHA) {
    pixelSize=getRGBAPixelSize();
  } else if (info_ptr->color_type==PNG_COLOR_TYPE_RGB) {
    pixelSize=getRGBPixelSize();
  } else {
    ERROR("the image <%s> does not use the RGBA, RGB or palette color space",filepath.c_str());
    goto cleanup;
  }
  if (info_ptr->bit_depth!=8) {
    ERROR("the image <%s> does not use 8-bit depth per color channel",filepath.c_str());
    goto cleanup;
  }
  if (info_ptr->rowbytes!=width*pixelSize) {
    FATAL("the row bytes of image <%s> do not have the expected length",filepath.c_str());
    goto cleanup;
  }

  // Reserve memory for the image
  image=(ImagePixel*)malloc(pixelSize*width*height);
  if (!image) {
    FATAL("can not reserve memory for the image <%s>",filepath.c_str());
    goto cleanup;
  }

  // Read the image
  if (setjmp(png_jmpbuf(png_ptr))) {
    ERROR("png image <%s> can not be read",filepath.c_str());
    goto cleanup;
  }
  for (Int i=0;(i<number_of_passes)&&(!abortLoad);i++) {
    for (Int y=0;(y<height)&&(!abortLoad);y++) {
      png_read_row(png_ptr, &image[pixelSize*width*y], NULL);
      core->interruptAllowedHere();
    }
  }
  if (!abortLoad)
    png_read_end(png_ptr, info_ptr);

cleanup:

  // Deinit
  if (abortLoad) {
    if (image) free(image);
    image=NULL;
    abortLoad=false;
  }
  if (png_ptr) {
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
  }
  if (fp)
    fclose(fp);
  return image;
}

// Loads a png-based icon by considering the DPI of the screen
ImagePixel *Image::loadPNGIcon(std::string filename, Int &imageWidth, Int &imageHeight, double &dpiScale, UInt &pixelSize) {

  FILE *in;
  std::string bestIconPath="";
  Int bestDPI;

  // First check if the icon in the device's native screen dpi is available
  std::stringstream s;
  bestDPI=core->getScreen()->getDPI();
  s << iconFolder << "/" << bestDPI << "dpi/" << filename;
  //DEBUG("checking existance of icon <%s>",s.str().c_str());
  if (access(s.str().c_str(),F_OK)==0) {
    bestIconPath=s.str();
  } else {

    // Native dpi icon not available, search for one whose dpi is closest to the
    // screen dpi
    DIR *dp;
    struct dirent *dirp;
    struct stat filestat;
    dp = opendir( iconFolder.c_str() );
    if (dp == NULL){
      FATAL("can not read available icons",NULL);
      return NULL;
    }
    Int bestDPIDistance=std::numeric_limits<Int>::max();
    while ((dirp = readdir( dp )))
    {
      // Only look at directories that end with dpi
      std::string dirpath = dirp->d_name;
      if (dirpath.find_first_of("dpi")!=std::string::npos) {
        dirpath=iconFolder+"/"+dirpath;
        if (stat( dirpath.c_str(), &filestat )) continue;
        if (S_ISDIR( filestat.st_mode )) {

          // Check if the icon is available in this directory
          std::string iconPath=dirpath+"/"+filename;
          //DEBUG("checking existance of icon <%s>",iconPath.c_str());
          if (access(iconPath.c_str(),F_OK)==0) {

            // Remember the found icon if it is nearer to the screen dpi
            std::string t=dirp->d_name;
            t=t.substr(0,dirpath.find_first_of("dpi"));
            Int dpi=atoi(t.c_str());
            Int dpiDistance=abs(core->getScreen()->getDPI()-dpi);
            if (dpiDistance<bestDPIDistance) {
              bestIconPath=iconPath;
              bestDPIDistance=dpiDistance;
              bestDPI=dpi;
            }
          }
        }
      }
    }
  }

  // Did we find an icon?
  //DEBUG("found the icon <%s> at the path <%s>",filename.c_str(),bestIconPath.c_str());
  if (bestIconPath=="") {
    FATAL("can not find icon <%s>",filename.c_str());
    return NULL;
  }

  // Load the icon
  ImagePixel *icon=loadPNG(bestIconPath,imageWidth,imageHeight,pixelSize);
  if (!icon)
    return NULL;

  // Compute the width and height of the scaled icon
  dpiScale=(double)core->getScreen()->getDPI()/(double)bestDPI;

  // That's it
  return icon;
}


}


