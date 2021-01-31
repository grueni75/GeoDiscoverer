//============================================================================
// Name        : MapTileServer.cpp
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
#include <MapTileServer.h>
#include <MapSourceMercatorTiles.h>

namespace GEODISCOVERER {

MapTileServer::MapTileServer(MapSourceMercatorTiles *mapSource, std::string layerGroupName, UInt orderNr, std::string serverURL, double overlayAlpha, ImageType imageType, Int minZoomLevel, Int maxZoomLevel, std::list<std::string> httpHeader, UInt threadCount) {
  this->mapSource = mapSource;
  this->layerGroupName = layerGroupName;
  this->serverURL = serverURL;
  this->overlayAlpha = overlayAlpha;
  this->imageType = imageType;
  this->orderNr = orderNr;
  this->images.resize(threadCount);
  for (int i=0;i<threadCount;i++) {
    images[i]=(Memory*)malloc(sizeof(Memory));
    if (images[i]==NULL) {
      FATAL("can not create memory struct",NULL);
      return;
    }
    images[i]->data=NULL;
    images[i]->size=0;
    images[i]->pos=0;
  }
  this->minZoomLevelServer=minZoomLevel;
  this->maxZoomLevelServer=maxZoomLevel;
  this->minZoomLevelMap=-1;
  this->maxZoomLevelMap=-1;
  this->httpHeader=httpHeader;
}

MapTileServer::~MapTileServer() {
  for (std::vector<Memory*>::iterator i=images.begin();i!=images.end();i++) {
    if ((*i)->data)
      free((*i)->data);
    if (*i)
      free(*i);
  }
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
  DownloadResult result;
  if (images[threadNr]->data!=NULL)
    FATAL("previous image has not been freed",NULL);
  UInt size;
  images[threadNr]->data = core->downloadURL(url,result,images[threadNr]->size,!mapSource->getDownloadWarningOccured(),true,&httpHeader);
  switch (result) {
    case DownloadResultSuccess:

      // Check if the image can be loaded
      if ((!core->getImage()->queryPNG(images[threadNr]->data,images[threadNr]->size,imageWidth,imageHeight))&&
         (!core->getImage()->queryJPEG(images[threadNr]->data,images[threadNr]->size,imageWidth,imageHeight))) {
        WARNING("downloaded map from <%s> is not a valid image",url.c_str());
        free(images[threadNr]->data);
        images[threadNr]->data=NULL;
        return DownloadResultOtherFail;
      } else {
        return DownloadResultSuccess;
      }

    // Some tile servers do not have tiles for every location, so ignore file not found
    case DownloadResultFileNotFound:
      if (images[threadNr]->data)
        free(images[threadNr]->data);
      images[threadNr]->data=NULL;
      return DownloadResultFileNotFound;

    case DownloadResultOtherFail:
      mapSource->lockAccess(__FILE__, __LINE__);
      mapSource->setDownloadWarningOccured(true);
      mapSource->unlockAccess();
      break;
  }
  if (images[threadNr]->data)
    free(images[threadNr]->data);
  images[threadNr]->data=NULL;
  return DownloadResultOtherFail;
}

// Loads a map image in RGBA format
bool MapTileServer::composeTileImage(std::string url, ImagePixel* &composedTileImage, Int &composedImageWidth, Int &composedImageHeight, Int threadNr) {

  ImagePixel *tileImage;
  UInt pixelSize;
  Int width,height;

  // Check if the image exists
  if (images[threadNr]->data==NULL)
    return true;  // download was aborted but not flagged as error, so continue with image processing

  // Read in the image
  switch(imageType) {
    case ImageTypeJPEG:
      tileImage=core->getImage()->loadJPEG(images[threadNr]->data,images[threadNr]->size,width,height,pixelSize,false);
      break;
    case ImageTypePNG:
      tileImage=core->getImage()->loadPNG(images[threadNr]->data,images[threadNr]->size,width,height,pixelSize,false);
      break;
    case ImageTypeUnknown:
      FATAL("image type not known",NULL);
  }
  free(images[threadNr]->data);
  images[threadNr]->data=NULL;
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
