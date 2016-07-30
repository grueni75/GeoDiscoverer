//============================================================================
// Name        : Image.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#include <Core.h>

namespace GEODISCOVERER {

// Writes PNG data to a device
void pngWriteDataToDevice(png_structp png_ptr, png_bytep data, png_size_t length) {
  Device *device = (Device*)png_get_io_ptr(png_ptr);
  if (!device->send(data,length)) {
    png_error(png_ptr, "pngWriteDataToDevice: could not write to device");
  }
}

// Writes PNG data to memory
void pngWriteDataToMemory(png_structp png_ptr, png_bytep data, png_size_t length) {
  Memory *mem = (Memory*)png_get_io_ptr(png_ptr);
  mem->data = (UByte*)realloc((UByte*)mem->data, mem->size + length + 1);
  if(mem->data == NULL) {
    FATAL("not enough memory",NULL);
    return;
  }
  memcpy(&(mem->data[mem->size]), data, length);
  mem->size += length;
  mem->data[mem->size] = 0;
}

// Dummy flush function for png library
void pngFlush(png_structp png_ptr) {
}

// Reads PNG data from memory
void pngReadDataFromMemory(png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead) {

  Memory *image = (Memory*)png_get_io_ptr(png_ptr);
  if (image == NULL) {
    FATAL("image data missing",NULL);
    return;
  }
  if (image->pos+byteCountToRead>image->size) {
    DEBUG("png tries to read more memory than available",NULL);
    return;
  }
  memcpy(outBytes,&image->data[image->pos],byteCountToRead);
  image->pos+=byteCountToRead;
}

// Inits the png part
void Image::initPNG() {
}

// Inits the reading of a PNG file
bool Image::initPNGRead(png_structp &png_ptr, png_infop &info_ptr) {

  // Initialize the library for reading this file
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    FATAL("can not create png read struct",NULL);
    return false;
  }
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    FATAL("can not create png info struct",NULL);
    return false;
  }
  png_set_sig_bytes(png_ptr, 8);
  return true;
}

// Queries the dimension of the png from memory without loading it
bool Image::queryPNG(UByte *imageData, UInt imageSize, Int &width, Int &height) {
  png_structp png_ptr=NULL;
  png_infop info_ptr=NULL;
  bool result=true;
  struct Memory image;

  // Test memory for being a PNG
  if (imageSize<8)
    return false;
  if (png_sig_cmp(imageData, 0, 8)) {
    return false;
  }

  // Initialize the library for reading this file
  if (!initPNGRead(png_ptr,info_ptr)) {
    goto cleanup;
  }
  if (setjmp(png_jmpbuf(png_ptr))) {
    DEBUG("can not set position in png image",NULL);
    goto cleanup;
  }
  image.data=imageData;
  image.pos=8;
  image.size=imageSize;
  png_set_read_fn(png_ptr, (png_voidp)&image, pngReadDataFromMemory);

  // Read the header
  png_read_info(png_ptr, info_ptr);
  width = info_ptr->width;
  height = info_ptr->height;

cleanup:

  // Deinit
  if (png_ptr) {
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
  }
  return result;

}

// Queries the dimension of the png from a file without loading it
bool Image::queryPNG(std::string filepath, Int &width, Int &height) {
  png_byte header[8]; // 8 is the maximum size that can be checked
  ImagePixel **image=NULL;
  png_structp png_ptr=NULL;
  png_infop info_ptr=NULL;
  bool result=true;

  // Open file and test for it being a PNG
  FILE *fp = fopen(filepath.c_str(), "rb");
  if (!fp) {
    DEBUG("can not open <%s> for reading",filepath.c_str());
    goto cleanup;
  }
  fread(header, 1, 8, fp);
  if (png_sig_cmp(header, 0, 8)) {
    result=false;
    goto cleanup;
  }

  // Initialize the library for reading this file
  if (!initPNGRead(png_ptr,info_ptr)) {
    goto cleanup;
  }
  if (setjmp(png_jmpbuf(png_ptr))) {
    DEBUG("can not set position in png image <%s>",filepath.c_str());
    goto cleanup;
  }
  png_init_io(png_ptr, fp);

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

// Loads a png after the library has been prepared
ImagePixel *Image::loadPNG(png_structp png_ptr, png_infop info_ptr, std::string imageDescription, Int &width, Int &height, UInt &pixelSize, bool calledByMapUpdateThread) {

  ImagePixel *image=NULL;
  Int number_of_passes;

  // Read the header
  png_read_info(png_ptr, info_ptr);

  // Convert palette to RGB
  if (info_ptr->color_type==PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(png_ptr);
  }

  // Convert gray to RGB
  if (info_ptr->color_type==PNG_COLOR_TYPE_GRAY) {
    png_set_gray_to_rgb(png_ptr);
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
    DEBUG("the %s does not use the RGBA, RGB or palette color space",imageDescription.c_str());
    return NULL;
  }
  if (info_ptr->bit_depth!=8) {
    DEBUG("the %s does not use 8-bit depth per color channel",imageDescription.c_str());
    return NULL;
  }
  if (info_ptr->rowbytes!=width*pixelSize) {
    FATAL("the row bytes of the %s do not have the expected length",imageDescription.c_str());
    return NULL;
  }

  // Reserve memory for the image
  image=(ImagePixel*)malloc(pixelSize*width*height);
  if (!image) {
    FATAL("can not reserve memory for %s",imageDescription.c_str());
    return NULL;
  }

  // Read the image
  for (Int i=0;(i<number_of_passes)&&((!calledByMapUpdateThread)||(!abortLoad));i++) {
    for (Int y=0;(y<height)&&((!calledByMapUpdateThread)||(!abortLoad));y++) {
      png_read_row(png_ptr, &image[pixelSize*width*y], NULL);
      //if (calledByMapUpdateThread)
      //  core->interruptAllowedHere(__FILE__, __LINE__);
    }
  }
  if (((!calledByMapUpdateThread)||(!abortLoad)))
    png_read_end(png_ptr, info_ptr);

  return image;
}

// Loads a png from a file
ImagePixel *Image::loadPNG(std::string filepath, Int &width, Int &height, UInt &pixelSize, bool calledByMapUpdateThread) {

  png_byte header[8]; // 8 is the maximum size that can be checked
  ImagePixel *image=NULL;
  png_structp png_ptr=NULL;
  png_infop info_ptr=NULL;
  Int number_of_passes;

  // Open file and test for it being a PNG
  abortLoad=false;
  FILE *fp = fopen(filepath.c_str(), "rb");
  if (!fp) {
    DEBUG("can not open <%s> for reading",filepath.c_str());
    goto cleanup;
  }
  fread(header, 1, 8, fp);
  if (png_sig_cmp(header, 0, 8)) {
    DEBUG("file <%s> is not a PNG file",filepath.c_str());
    goto cleanup;
  }

  // Initialize the library for reading this file
  if (!initPNGRead(png_ptr,info_ptr)) {
    goto cleanup;
  }
  if (setjmp(png_jmpbuf(png_ptr))) {
    DEBUG("can not set position in png image <%s>",filepath.c_str());
    goto cleanup;
  }
  png_init_io(png_ptr, fp);

  // Read the pixel data
  image = loadPNG(png_ptr,info_ptr,"image <"+filepath+">",width,height,pixelSize,calledByMapUpdateThread);

cleanup:

  // Deinit
  if (calledByMapUpdateThread&&abortLoad) {
    if (image) free(image);
    image=NULL;
    abortLoad=false;
  }
  if ((png_ptr)||(info_ptr)) {
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
  }
  if (fp)
    fclose(fp);
  return image;
}


// Loads a png from memory
ImagePixel *Image::loadPNG(UByte *imageData, UInt imageSize, Int &width, Int &height, UInt &pixelSize, bool calledByMapUpdateThread) {

  png_byte header[8]; // 8 is the maximum size that can be checked
  ImagePixel *image=NULL;
  png_structp png_ptr=NULL;
  png_infop info_ptr=NULL;
  Int number_of_passes;
  Memory imageMemory;

  // Test memory for being a PNG
  abortLoad=false;
  if (imageSize<8)
    return NULL;
  if (png_sig_cmp(imageData, 0, 8)) {
    return NULL;
  }

  // Initialize the library for reading this file
  if (!initPNGRead(png_ptr,info_ptr)) {
    goto cleanup;
  }
  if (setjmp(png_jmpbuf(png_ptr))) {
    DEBUG("can not set position in png image",NULL);
    goto cleanup;
  }
  imageMemory.data=imageData;
  imageMemory.pos=8;
  imageMemory.size=imageSize;
  png_set_read_fn(png_ptr, (png_voidp)&imageMemory, pngReadDataFromMemory);

  // Read the pixel data
  image = loadPNG(png_ptr,info_ptr,"image",width,height,pixelSize,calledByMapUpdateThread);

cleanup:

  // Deinit
  if (calledByMapUpdateThread&&abortLoad) {
    if (image) free(image);
    image=NULL;
    abortLoad=false;
  }
  if ((png_ptr)||(info_ptr)) {
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
  }
  return image;
}

// Loads a png-based icon by considering the DPI of the screen
ImagePixel *Image::loadPNGIcon(Screen *screen, std::string filename, Int &imageWidth, Int &imageHeight, double &dpiScale, UInt &pixelSize) {

  FILE *in;
  std::string bestIconPath="";
  Int bestDPI;

  // First check if the icon in the device's native screen dpi is available
  std::stringstream s;
  bestDPI=screen->getDPI();
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
    dp = core->openDir(iconFolder);
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
        if (core->statFile( dirpath, &filestat )) continue;
        if (S_ISDIR( filestat.st_mode )) {

          // Check if the icon is available in this directory
          std::string iconPath=dirpath+"/"+filename;
          //DEBUG("checking existance of icon <%s>",iconPath.c_str());
          if (access(iconPath.c_str(),F_OK)==0) {

            // Remember the found icon if it is nearer to the screen dpi
            std::string t=dirp->d_name;
            t=t.substr(0,dirpath.find_first_of("dpi"));
            Int dpi=atoi(t.c_str());
            Int dpiDistance=abs(screen->getDPI()-dpi);
            if ((dpiDistance<bestDPIDistance)||((dpiDistance==bestDPIDistance)&&(dpi>bestDPI))) {
              bestIconPath=iconPath;
              bestDPIDistance=dpiDistance;
              bestDPI=dpi;
            }
          }
        }
      }
    }
    closedir(dp);
  }

  // Did we find an icon?
  //DEBUG("found the icon <%s> at the path <%s>",filename.c_str(),bestIconPath.c_str());
  if (bestIconPath=="") {
    FATAL("can not find icon <%s>",filename.c_str());
    return NULL;
  }

  // Load the icon
  ImagePixel *icon=loadPNG(bestIconPath,imageWidth,imageHeight,pixelSize,false);
  if (!icon)
    return NULL;

  // Compute the width and height of the scaled icon
  dpiScale=(double)screen->getDPI()/(double)bestDPI;

  // That's it
  return icon;
}

// Writes a png to memory
UByte *Image::writePNG(ImagePixel *image, Int width, Int height, UInt pixelSize, UInt &imageSize, bool inverseRows) {

  // Call the internal function
  Memory imageMemory;
  imageMemory.data = (UByte*)malloc(1);
  imageMemory.size = 0;
  bool result = writePNG(image,ImageOutputTargetTypeMemory,&imageMemory,width,height,pixelSize,inverseRows);
  imageSize = imageMemory.size;

  // Return result
  return imageMemory.data;
}

// Writes a png to a file
bool Image::writePNG(ImagePixel *image, std::string filepath, Int width, Int height, UInt pixelSize, bool inverseRows) {

  // Open the file
  FILE *fp = fopen(filepath.c_str(), "wb");
  if (!fp) {
    DEBUG("can not open <%s> for writing",filepath.c_str());
    return false;
  }

  // Call the internal function
  bool result = writePNG(image,ImageOutputTargetTypeFile,(void*)fp,width,height,pixelSize,inverseRows);

  // Clean up
  fclose(fp);

  // Return result
  return result;
}

// Writes a png to a device
bool Image::writePNG(ImagePixel *image, Device *device, Int width, Int height, UInt pixelSize, bool inverseRows) {
  if (!device->announcePNGImage())
    return false;
  return writePNG(image,ImageOutputTargetTypeDevice,(void*)device,width,height,pixelSize,inverseRows);
}

// Writes a png to either a C file pointer or to Linux file descriptor
bool Image::writePNG(ImagePixel *image, ImageOutputTargetType targetType, void *target, Int width, Int height, UInt pixelSize, bool inverseRows=false) {

  png_structp png_ptr=NULL;
  png_infop info_ptr=NULL;
  bool result=false;
  Int color_type = -1;
  Int y;
  png_bytepp rows;

  // Initialize the library for writing this image
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    FATAL("can not create png write struct",NULL);
    goto cleanup;
  }
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    FATAL("can not create png info struct",NULL);
    goto cleanup;
  }
  if (setjmp(png_jmpbuf(png_ptr))) {
    DEBUG("error while writing png image",NULL);
    goto cleanup;
  }
  switch (targetType) {
    case ImageOutputTargetTypeFile:
      png_init_io(png_ptr, (FILE*)target);
      break;
    case ImageOutputTargetTypeDevice:
      png_set_write_fn(png_ptr, target, pngWriteDataToDevice, pngFlush);
      break;
    case ImageOutputTargetTypeMemory:
      png_set_write_fn(png_ptr, target, pngWriteDataToMemory, pngFlush);
      break;
    default:
      FATAL("unsupported target type",NULL);
  }

  // Tell the library what to write
  if (pixelSize==getRGBPixelSize())
    color_type=PNG_COLOR_TYPE_RGB;
  if (pixelSize==getRGBAPixelSize())
    color_type=PNG_COLOR_TYPE_RGB_ALPHA;
  png_set_IHDR(png_ptr,info_ptr,width,height,8,color_type,PNG_INTERLACE_NONE,
  PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT);

  // Set the image data
  if (!(rows=(png_bytepp)malloc(sizeof(*rows)*height))) {
    FATAL("can not reserve memory for the row pointers",NULL);
    goto cleanup;
  }
  for (y=0;y<height;y++) {
    Int y2=inverseRows ? height-1-y : y;
    rows[y]=&image[y2*width*pixelSize];
  }
  png_set_rows(png_ptr,info_ptr,rows);

  // Write the image
  png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
  free(rows);
  result=true;

cleanup:

  // Deinit
  if ((png_ptr)||(info_ptr)) {
    png_destroy_write_struct(&png_ptr, &info_ptr);
  }
  return result;
}

}


