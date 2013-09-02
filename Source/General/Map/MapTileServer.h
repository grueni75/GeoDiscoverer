//============================================================================
// Name        : MapTileServer.h
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
  DownloadResult downloadTileImage(MapContainer *mapContainer, UInt nr);

  // Loads a map image and overlays it atop of imagePixel
  bool composeTileImage(ImagePixel* &composedTileImage, Int &composedImageWidth, Int &composedImageHeight);

};

} /* namespace GEODISCOVERER */
#endif /* MAPTILESERVER_H_ */
