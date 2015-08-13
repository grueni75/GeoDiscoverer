//============================================================================
// Name        : MapTileServer.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

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
  std::string imagePath;                // Path to the downloaded image
  Int minZoomLevelServer;               // Minimum usable zoom level of this server
  Int maxZoomLevelServer;               // Maximum usable zoom level of this server
  Int minZoomLevelMap;                  // Minimum zoom level in map
  Int maxZoomLevelMap;                  // Maximum zoom level in map

  // Replaces a variable in a string
  bool replaceVariableInServerURL(std::string &url, std::string variableName, std::string variableValue);

public:

  // Constructor
  MapTileServer(MapSourceMercatorTiles *mapSource, std::string layerGroupName, UInt orderNr, std::string serverURL, double overlayAlpha, ImageType imageType, Int minZoomLevel, Int maxZoomLevel);

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
