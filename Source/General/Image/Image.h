//============================================================================
// Name        : Image.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#ifndef IMAGE_H_
#define IMAGE_H_

namespace GEODISCOVERER {

// Image type
typedef UByte ImagePixel;

// Transfer types
enum ImageOutputTargetType { ImageOutputTargetTypeFile, ImageOutputTargetTypeDevice };

class Image {

protected:

  // Path to the icon directory
  std::string iconFolder;

  // Size of a RGB pixel
  static const UInt RGBPixelSize = 3;

  // Size of a RGBA pixel
  static const UInt RGBAPixelSize = 4;

  // Indicates that the current load operation shall be aborted
  bool abortLoad;

  // Ints the jpeg part
  void initJPEG();

  // Ints the png part
  void initPNG();

  // Writes a png
  bool writePNG(ImagePixel *image, ImageOutputTargetType targetType, void *target, Int width, Int height, UInt pixelSize, bool inverseRows);

public:

  // Constructor and destructor
  Image();
  virtual ~Image();

  // Query the dimensions of a jpeg
  bool queryJPEG(std::string filepath, Int &width, Int &height);

  // Loads a jpeg
  ImagePixel *loadJPEG(std::string filepath, Int &width, Int &height, UInt &pixelSize, bool calledByMapUpdateThread);

  // Queries the dimension of the png without loading it
  bool queryPNG(std::string filepath, Int &width, Int &height);

  // Loads a png
  ImagePixel *loadPNG(std::string filepath, Int &width, Int &height,UInt &pixelSize, bool calledByMapUpdateThread);

  // Loads an PNG based icon
  // The correct file is determined from the screen dpi
  ImagePixel *loadPNGIcon(Screen *screen, std::string filename, Int &imageWidth, Int &imageHeight, double &dpiScale, UInt &pixelSize);

  // Writes a png to a file
  bool writePNG(ImagePixel *image, std::string filepath, Int width, Int height, UInt pixelSize, bool inverseRows=false);

  // Writes a png to a device
  bool writePNG(ImagePixel *image, Device *device, Int width, Int height, UInt pixelSize, bool inverseRows=false);

  // Aborts the current jpeg image loading
  void setAbortLoad()
  {
      this->abortLoad = true;
  }

  // Getters and setters
  static UInt getRGBPixelSize()
  {
      return RGBPixelSize;
  }
  static UInt getRGBAPixelSize()
  {
      return RGBAPixelSize;
  }
};

}

#endif /* IMAGE_H_ */
