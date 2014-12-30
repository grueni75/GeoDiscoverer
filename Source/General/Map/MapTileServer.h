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

  MapSourceMercatorTiles *mapSource;    // The map source this image belongs to
  UInt orderNr;                         // Order number of the tile server
  std::string serverURL;                // URL of the tile server to use
  std::string lastDownloadURL;          // URL of the last downloaded tile
  double overlayAlpha;                  // Alpha to use when creating the complete image
  ImageType imageType;                  // Type of the image
  std::string imagePath;                // Path to the downloaded image

  // Replaces a variable in a string
  bool replaceVariableInServerURL(std::string &url, std::string variableName, std::string variableValue);

public:

  // Constructor
  MapTileServer(MapSourceMercatorTiles *mapSource, UInt orderNr, std::string serverURL, double overlayAlpha, ImageType imageType);

  // Destructor
  virtual ~MapTileServer();

  // Downloads a map image from the server
  DownloadResult downloadTileImage(MapContainer *mapContainer, Int threadNr);

  // Loads a map image and overlays it atop of imagePixel
  bool composeTileImage(ImagePixel* &composedTileImage, Int &composedImageWidth, Int &composedImageHeight, Int threadNr);

};

} /* namespace GEODISCOVERER */
#endif /* MAPTILESERVER_H_ */
