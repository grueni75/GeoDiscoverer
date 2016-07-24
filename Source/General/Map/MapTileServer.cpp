//============================================================================
// Name        : MapTileServer.cpp
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

MapTileServer::MapTileServer(MapSourceMercatorTiles *mapSource, std::string layerGroupName, UInt orderNr, std::string serverURL, double overlayAlpha, ImageType imageType, Int minZoomLevel, Int maxZoomLevel) {
  this->mapSource = mapSource;
  this->layerGroupName = layerGroupName;
  this->serverURL = serverURL;
  this->overlayAlpha = overlayAlpha;
  this->imageType = imageType;
  this->orderNr = orderNr;
  std::stringstream imagePathStream;
  imagePathStream << mapSource->getFolderPath() << "/download." << orderNr;
  this->imagePath = imagePathStream.str();
  this->minZoomLevelServer=minZoomLevel;
  this->maxZoomLevelServer=maxZoomLevel;
  this->minZoomLevelMap=-1;
  this->maxZoomLevelMap=-1;
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
DownloadResult MapTileServer::downloadTileImage(MapContainer *mapContainer, Int threadNr, std::string &url) {

  Int imageWidth, imageHeight;

  // Prepare the directory
  std::stringstream t;
  std::stringstream z; z << mapContainer->getZoomLevelServer();
  std::stringstream x; x << mapContainer->getX();
  std::stringstream y; y << mapContainer->getY();

  // Prepare the url
  url=serverURL;
  replaceVariableInServerURL(url,"${z}",z.str());
  replaceVariableInServerURL(url,"${x}",x.str());
  replaceVariableInServerURL(url,"${y}",y.str());

  // Download the file
  std::stringstream threadImagePath;
  threadImagePath << imagePath << "." << threadNr << ".bin";
  DownloadResult result = core->downloadURL(url,threadImagePath.str(),!mapSource->getDownloadWarningOccured(),true);
  switch (result) {
    case DownloadResultSuccess:

      // Check if the image can be loaded
      if ((!core->getImage()->queryPNG(threadImagePath.str(),imageWidth,imageHeight))&&
         (!core->getImage()->queryJPEG(threadImagePath.str(),imageWidth,imageHeight))) {
        WARNING("downloaded map from <%s> is not a valid image",url.c_str());
        return DownloadResultOtherFail;
      } else {
        return DownloadResultSuccess;
      }

    // Some tile servers do not have tiles for every location, so ignore file not found
    case DownloadResultFileNotFound:
      return DownloadResultFileNotFound;

    case DownloadResultOtherFail:
      mapSource->lockAccess(__FILE__, __LINE__);
      mapSource->setDownloadWarningOccured(true);
      mapSource->unlockAccess();
      break;
  }
  return DownloadResultOtherFail;
}

// Loads a map image in RGBA format
bool MapTileServer::composeTileImage(std::string url, ImagePixel* &composedTileImage, Int &composedImageWidth, Int &composedImageHeight, Int threadNr) {

  ImagePixel *tileImage;
  UInt pixelSize;
  Int width,height;
  struct stat stat_buffer;
  std::stringstream threadImagePath;
  threadImagePath << imagePath << "." << threadNr << ".bin";

  // Check if the path exist
  if (core->statFile(threadImagePath.str(),&stat_buffer)!=0)
    return true;  // download was aborted but not flagged as error, so continue with image processing

  // Read in the image
  switch(imageType) {
    case ImageTypeJPEG:
      tileImage=core->getImage()->loadJPEG(threadImagePath.str(),width,height,pixelSize,false);
      break;
    case ImageTypePNG:
      tileImage=core->getImage()->loadPNG(threadImagePath.str(),width,height,pixelSize,false);
      break;
    case ImageTypeUnknown:
      FATAL("image type not known",NULL);
  }
  if (!tileImage)
    return false;
  if ((pixelSize!=Image::getRGBPixelSize())&&(pixelSize!=Image::getRGBAPixelSize())) {
    WARNING("pixel size %d in downloaded map from <%s> is not supported",pixelSize,url.c_str());
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
