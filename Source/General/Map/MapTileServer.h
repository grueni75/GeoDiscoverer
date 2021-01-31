//============================================================================
// Name        : MapTileServer.h
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

#include <Image.h>
#include <MapContainerTreeNode.h>
#include <MapContainer.h>

#ifndef MAPTILESERVER_H_
#define MAPTILESERVER_H_

namespace GEODISCOVERER {

class MapTileServer {

protected:

  MapSourceMercatorTiles *mapSource;    // The map source this server belongs to
  std::string layerGroupName;           // The name of the layer group this server belongs to
  UInt orderNr;                         // Order number of the tile server
  std::string serverURL;                // URL of the tile server to use
  double overlayAlpha;                  // Alpha to use when creating the complete image
  ImageType imageType;                  // Type of the image
  std::vector<Memory*> images;            // Memory that contains the image
  Int minZoomLevelServer;               // Minimum usable zoom level of this server
  Int maxZoomLevelServer;               // Maximum usable zoom level of this server
  Int minZoomLevelMap;                  // Minimum zoom level in map
  Int maxZoomLevelMap;                  // Maximum zoom level in map
  std::list<std::string> httpHeader;    // HTTP header to use

  // Replaces a variable in a string
  bool replaceVariableInServerURL(std::string &url, std::string variableName, std::string variableValue);

public:

  // Constructor
  MapTileServer(MapSourceMercatorTiles *mapSource, std::string layerGroupName, UInt orderNr, std::string serverURL, double overlayAlpha, ImageType imageType, Int minZoomLevel, Int maxZoomLevel, std::list<std::string> httpHeader, UInt threadCount);

  // Destructor
  virtual ~MapTileServer();

  // Downloads a map image from the server
  DownloadResult downloadTileImage(MapContainer *mapContainer, Int threadNr, std::string &url);

  // Loads a map image and overlays it atop of imagePixel
  bool composeTileImage(std::string url, ImagePixel* &composedTileImage, Int &composedImageWidth, Int &composedImageHeight, Int threadNr);

  // Getters and setters
  Int getMaxZoomLevelMap() const {
    return maxZoomLevelMap;
  }

  void setMaxZoomLevelMap(Int maxZoomLevelMap) {
    this->maxZoomLevelMap = maxZoomLevelMap;
  }

  Int getMaxZoomLevelServer() const {
    return maxZoomLevelServer;
  }

  void setMaxZoomLevelServer(Int maxZoomLevelServer) {
    this->maxZoomLevelServer = maxZoomLevelServer;
  }

  Int getMinZoomLevelMap() const {
    return minZoomLevelMap;
  }

  void setMinZoomLevelMap(Int minZoomLevelMap) {
    this->minZoomLevelMap = minZoomLevelMap;
  }

  Int getMinZoomLevelServer() const {
    return minZoomLevelServer;
  }

  void setMinZoomLevelServer(Int minZoomLevelServer) {
    this->minZoomLevelServer = minZoomLevelServer;
  }

  const std::string& getLayerGroupName() const {
    return layerGroupName;
  }
};

} /* namespace GEODISCOVERER */
#endif /* MAPTILESERVER_H_ */
