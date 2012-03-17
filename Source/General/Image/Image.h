//============================================================================
// Name        : Image.h
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

#ifndef IMAGE_H_
#define IMAGE_H_

namespace GEODISCOVERER {

// Image type
typedef UByte ImagePixel;

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

public:

  // Constructor and destructor
  Image();
  virtual ~Image();

  // Query the dimensions of a jpeg
  bool queryJPEG(std::string filepath, Int &width, Int &height);

  // Loads a jpeg
  ImagePixel *loadJPEG(std::string filepath, Int &width, Int &height, UInt &pixelSize);

  // Queries the dimension of the png without loading it
  bool queryPNG(std::string filepath, Int &width, Int &height);

  // Loads a png
  ImagePixel *loadPNG(std::string filepath, Int &width, Int &height,UInt &pixelSize);

  // Loads an PNG based icon
  // The correct file is determined from the screen dpi
  ImagePixel *loadPNGIcon(std::string filename, Int &imageWidth, Int &imageHeight, double &dpiScale, UInt &pixelSize);

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
