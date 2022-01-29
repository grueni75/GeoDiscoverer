//============================================================================
// Name        : Image.h
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

#ifndef IMAGE_H_
#define IMAGE_H_

namespace GEODISCOVERER {

// Image type
typedef UByte ImagePixel;

// Transfer types
enum ImageOutputTargetType { ImageOutputTargetTypeFile, ImageOutputTargetTypeDevice, ImageOutputTargetTypeMemory };

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

  // Inits the jpeg part
  void initJPEG();

  // Intis the png part
  void initPNG();

  // Writes a png
  bool writePNG(ImagePixel *image, ImageOutputTargetType targetType, void *target, Int width, Int height, UInt pixelSize, bool inverseRows);

  // Inits the reading of a PNG file
  bool initPNGRead(png_structp &png_ptr, png_infop &info_ptr);

  // Loads a png after the library has been prepared
  ImagePixel *loadPNG(png_structp png_ptr, png_infop info_ptr, std::string imageDescription, Int &width, Int &height, UInt &pixelSize, bool calledByMapUpdateThread);

  // Loads a jpg after the library has been prepared
  ImagePixel *loadJPEG(struct jpeg_decompress_struct *cinfo, std::string imageDescription, Int &width, Int &height, UInt &pixelSize, bool calledByMapUpdateThread);

public:

  // Constructor and destructor
  Image();
  virtual ~Image();

  // Query the dimensions of a jpeg
  bool queryJPEG(UByte *imageData, UInt imageSize, Int &width, Int &height);
  bool queryJPEG(std::string filepath, Int &width, Int &height);

  // Loads a jpeg
  ImagePixel *loadJPEG(UByte *imageData, UInt imageSize, Int &width, Int &height, UInt &pixelSize, bool calledByMapUpdateThread);
  ImagePixel *loadJPEG(std::string filepath, Int &width, Int &height, UInt &pixelSize, bool calledByMapUpdateThread);

  // Queries the dimension of the png without loading it
  bool queryPNG(std::string filepath, Int &width, Int &height);
  bool queryPNG(UByte *imageData, UInt imageSize, Int &width, Int &height);

  // Loads a png
  ImagePixel *loadPNG(std::string filepath, Int &width, Int &height,UInt &pixelSize, bool calledByMapUpdateThread);
  ImagePixel *loadPNG(UByte *imageData, UInt imageSize, Int &width, Int &height, UInt &pixelSize, bool calledByMapUpdateThread);

  // Loads an PNG based icon
  // The correct file is determined from the screen dpi
  ImagePixel *loadPNGIcon(Screen *screen, std::string filename, Int &imageWidth, Int &imageHeight, double &dpiScale, UInt &pixelSize);

  // Writes a png to a file
  bool writePNG(ImagePixel *image, std::string filepath, Int width, Int height, UInt pixelSize, bool inverseRows=false);

  // Writes a png to a device
  bool writePNG(ImagePixel *image, Device *device, Int width, Int height, UInt pixelSize, bool inverseRows=false);

  // Writes a png to memory
  UByte *writePNG(ImagePixel *image, Int width, Int height, UInt pixelSize, UInt &imageSize, bool inverseRows=false);

  // Aborts the current jpeg image loading
  void setAbortLoad()
  {
      this->abortLoad = true;
  }

  // Computes a gaussion blur
  bool iirGaussFilter(ImagePixel *image, Int width, Int height, UInt pixelSize, float sigma);

  // Changes the brightness by the given value
  void brightnessFilter(ImagePixel *image, Int width, Int height, UInt pixelSize, Int brightnessOffset);

  // Changes the brightness of the given PNG file
  bool brightnessFilter(std::string pngFilename, Int brightnessOffset);

  // Converts RGB pixel into HSL
  void rgb2hsv(ImagePixel *pixel, double &h, double &s, double &v);
  
  // Converts RGB pixel into HSL
  void hsv2rgb(double h, double s, double v, ImagePixel *pixel);

  // Changes the hsv components by the given value
  void hsvFilter(ImagePixel *image, Int width, Int height, UInt pixelSize, double hOffset, double sOffset, double vOffset);

  // Changes the hsv components of the given PNG file by the given values
  UByte *hsvFilter(UByte *pngData, UInt &pngSize, double hOffset, double sOffset, double vOffset);

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
