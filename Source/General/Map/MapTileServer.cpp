//============================================================================
// Name        : MapTileServer.cpp
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

namespace GEODISCOVERER {

MapTileServer::MapTileServer(MapSourceMercatorTiles *mapSource, UInt orderNr, std::string serverURL, double overlayAlpha, ImageType imageType) {
  this->mapSource = mapSource;
  this->serverURL = serverURL;
  this->overlayAlpha = overlayAlpha;
  this->imageType = imageType;
  this->orderNr = orderNr;
  std::stringstream imagePathStream;
  imagePathStream << mapSource->getFolderPath() << "/download." << orderNr << ".bin";
  this->imagePath = imagePathStream.str();
}

MapTileServer::~MapTileServer() {
}

// Replaces a variable in a string
bool MapTileServer::replaceVariableInServerURL(std::string &url, std::string variableName, std::string variableValue) {
  size_t pos;
  pos=url.find(variableName);
  if (pos==std::string::npos) {
    ERROR("variable %s not found in tile server URL <%s>",variableName.c_str(),serverURL.c_str());
    mapSource->setErrorOccured(true);
    return false;
  }
  url.replace(pos,variableName.size(),variableValue);
  return true;
}

// Downloads a map image from the server
DownloadResult MapTileServer::downloadTileImage(MapContainer *mapContainer, UInt nr) {

  Int imageWidth, imageHeight;

  // Prepare the directory
  std::stringstream t;
  std::stringstream z; z << mapContainer->getZoomLevel()+mapSource->getMinZoomLevel()-1;
  std::stringstream x; x << mapContainer->getX();
  std::stringstream y; y << mapContainer->getY();

  // Prepare the url
  std::string url=serverURL;
  replaceVariableInServerURL(url,"${z}",z.str());
  replaceVariableInServerURL(url,"${x}",x.str());
  replaceVariableInServerURL(url,"${y}",y.str());
  lastDownloadURL=url;

  // Download the file
  DownloadResult result = core->downloadURL(url,imagePath,!mapSource->getDownloadWarningOccured(),true);
  switch (result) {
    case DownloadResultSuccess:

      // Check if the image can be loaded
      if ((!core->getImage()->queryPNG(imagePath,imageWidth,imageHeight))&&
         (!core->getImage()->queryJPEG(imagePath,imageWidth,imageHeight))) {
        WARNING("downloaded map from <%s> is not a valid image",url.c_str());
        return DownloadResultOtherFail;
      } else {
        return DownloadResultSuccess;
      }

    // Some tile servers do not have tiles for every location, so ignore file not found
    case DownloadResultFileNotFound:
      return DownloadResultFileNotFound;

    case DownloadResultOtherFail:
      mapSource->lockAccess();
      mapSource->setDownloadWarningOccured(true);
      mapSource->unlockAccess();
      break;
  }
  return DownloadResultOtherFail;
}

// Loads a map image in RGBA format
bool MapTileServer::composeTileImage(ImagePixel* &composedTileImage, Int &composedImageWidth, Int &composedImageHeight) {

  ImagePixel *tileImage;
  UInt pixelSize;
  Int width,height;
  struct stat stat_buffer;

  // Check if the path exist
  if (stat(imagePath.c_str(),&stat_buffer)!=0)
    return true;  // download was aborted but not flagged as error, so continue with image processing

  // Read in the image
  switch(imageType) {
    case ImageTypeJPEG:
      tileImage=core->getImage()->loadJPEG(imagePath,width,height,pixelSize,false);
      break;
    case ImageTypePNG:
      tileImage=core->getImage()->loadPNG(imagePath,width,height,pixelSize,false);
      break;
  }
  if (!tileImage)
    return false;
  if ((pixelSize!=Image::getRGBPixelSize())&&(pixelSize!=Image::getRGBAPixelSize())) {
    WARNING("pixel size %d in downloaded map from <%s> is not supported",pixelSize,lastDownloadURL.c_str());
    free(tileImage);
    return false;
  }

  // Create a new composed image if it is not already set
  if (composedTileImage==NULL) {
    if (!(composedTileImage=(ImagePixel*)malloc(width*height*Image::getRGBPixelSize()))) {
      FATAL("can not reserve memory for composed tile image",NULL);
      free(tileImage);
      return false;
    }
    memset(composedTileImage,0xFF,width*height*Image::getRGBPixelSize());
    composedImageWidth=width;
    composedImageHeight=height;
  }

  // Convert the image in RGBA format if necessary
  double alpha;
  for (Int i=0;i<width*height;i++) {
    if (pixelSize==Image::getRGBAPixelSize()) {
      alpha = ((double)tileImage[pixelSize*i+3])/255.0*overlayAlpha;
    } else {
      alpha = overlayAlpha;
    }
    for (Int j=0;j<core->getImage()->getRGBPixelSize();j++)
      composedTileImage[Image::getRGBPixelSize()*i+j] = (ImagePixel)
        (double)composedTileImage[Image::getRGBPixelSize()*i+j] * (1.0-alpha) +
        (double)tileImage[pixelSize*i+j] * alpha;
  }
  free(tileImage);
  return true;
}

} /* namespace GEODISCOVERER */
