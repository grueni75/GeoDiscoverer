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

#include <Core.h>
#include <jpeglib.h>

namespace GEODISCOVERER {

// Structure for recovering from jpeg lib error
struct jpegErrorHandlerInfo {
  struct jpeg_error_mgr mgr;
  jmp_buf setjmpBuffer;
};

// In case of error: output message and jump back to calling routine
void jpegErrorHandler(j_common_ptr cinfo) {
  jpegErrorHandlerInfo *info=(jpegErrorHandlerInfo*)cinfo->err;
  (*cinfo->err->output_message) (cinfo);
  longjmp(info->setjmpBuffer, 1);
}

// Ints the jpeg part
void Image::initJPEG() {

  // Sanity checks
  if (BITS_IN_JSAMPLE!=8) {
    FATAL("a JPEG library with 8 bits per sample is required",NULL);
    return;
  }
  if (sizeof(JSAMPLE)!=sizeof(ImagePixel)) {
    FATAL("size of UByte does not match size of JSAMPLE",NULL);
    return;
  }
}

// Queries the dimension of the jpeg without loading it
bool Image::queryJPEG(std::string filepath, Int &width, Int &height) {

  FILE *file;
  struct jpeg_decompress_struct cinfo;
  struct jpegErrorHandlerInfo jerr;
  bool result=true;

  if ((file = fopen(filepath.c_str(), "rb")) == NULL) {
    ERROR("can not open <%s> for reading",filepath.c_str());
    return false;
  }
  cinfo.err = jpeg_std_error(&jerr.mgr);
  jerr.mgr.error_exit=jpegErrorHandler;
  if (setjmp(jerr.setjmpBuffer)) {
    result=false;
    goto cleanup;
  }
  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, file);
  jpeg_read_header(&cinfo, TRUE);
  width=cinfo.image_width;
  height=cinfo.image_height;

cleanup:
  jpeg_destroy_decompress(&cinfo);
  fclose(file);
  return result;
}

// Loads a jpeg
ImagePixel *Image::loadJPEG(std::string filepath, Int &width, Int &height, UInt &pixelSize, bool calledByMapUpdateThread) {

  FILE *file;
  struct jpeg_decompress_struct cinfo;
  ImagePixel *image=NULL,*scanline=NULL;
  UInt scanlineSize,imageSize;
  Int y=0;
  struct jpegErrorHandlerInfo jerr;

  // Prepare the decompression
  abortLoad=false;
  if ((file = fopen(filepath.c_str(), "rb")) == NULL) {
    ERROR("can not open <%s> for reading",filepath.c_str());
    return NULL;
  }
  cinfo.err = jpeg_std_error(&jerr.mgr);
  jerr.mgr.error_exit=jpegErrorHandler;
  if (setjmp(jerr.setjmpBuffer)) {
    ERROR("jpeg image <%s> can not be read",NULL);
    goto cleanup;
  }
  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, file);
  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  // Check type of image
  pixelSize=sizeof(JSAMPLE)*cinfo.output_components;
  if (cinfo.out_color_space != JCS_RGB) {
    ERROR("the image <%s> does not use the RGB color space",filepath.c_str());
    goto cleanup;
  }
  if (cinfo.out_color_components != 3) {
    ERROR("the image <%s> does not use three samples per pixel",filepath.c_str());
    goto cleanup;
  }
  if (cinfo.output_components!=cinfo.out_color_components) {
    FATAL("the color components used in the image <%s> do not match the output color components",filepath.c_str());
    goto cleanup;
  }

  // Copy properties
  width=cinfo.output_width;
  height=cinfo.output_height;

  // Reserve memory for the image
  scanlineSize=pixelSize*cinfo.output_width;
  if (!(scanline=(ImagePixel *)malloc(scanlineSize))) {
    FATAL("can not reserve memory for the image <%s>",filepath.c_str());
    goto cleanup;
  }
  imageSize=scanlineSize*cinfo.output_height;
  if (!(image=(ImagePixel *)malloc(imageSize))) {
    FATAL("can not reserve memory for the image <%s>",filepath.c_str());
    goto cleanup;
  }

  // Do the decompression
  y=0;
  while( cinfo.output_scanline < cinfo.output_height && ((!calledByMapUpdateThread)||(!abortLoad)) ) {
    //DEBUG("output_scanline: %d output_height: %d",cinfo.output_scanline,cinfo.output_height);
    jpeg_read_scanlines(&cinfo, &scanline, 1);
    memcpy(&image[y*scanlineSize], scanline, scanlineSize);
    y++;
    //if (calledByMapUpdateThread)
    //  core->interruptAllowedHere(__FILE__, __LINE__);
  }

cleanup:
  if (calledByMapUpdateThread&&abortLoad) {
    jpeg_abort_decompress(&cinfo);
    if (image) free(image);
    image=NULL;
    abortLoad=false;
  } else {
    jpeg_finish_decompress(&cinfo);
  }
  jpeg_destroy_decompress(&cinfo);
  fclose(file);
  if (scanline) free(scanline);
  return image;

}

}

