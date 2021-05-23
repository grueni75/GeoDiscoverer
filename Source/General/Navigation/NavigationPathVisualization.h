//============================================================================
// Name        : NavigationPathVisualization.h
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

#include <NavigationPathTileInfo.h>
#include <NavigationEngine.h>
#include <MapCalibrator.h>
#include <MapPosition.h>

#ifndef NAVIGATIONPATHVISUALIZATION_H_
#define NAVIGATIONPATHVISUALIZATION_H_

namespace GEODISCOVERER {

// Types for storing the tile info
typedef std::map<MapTile*, NavigationPathTileInfo*> NavigationPathTileInfoMap;
typedef std::pair<MapTile*, NavigationPathTileInfo*> NavigationPathTileInfoPair;

class NavigationPathVisualization {

protected:

  Int zoomLevel;                                  // Zoom level that is represented by this visualization
  double colorOffset;                             // Current offset for animating the color of the arrow
  MapPosition prevLinePoint;                      // Last position used for creating the graphic line
  MapPosition prevArrowPoint;                     // Last position used for creating the direction arrow
  std::vector<MapPosition> points;                // All used points
  NavigationPathTileInfoMap tileInfoMap;          // Hash that holds information for each map tile
  double latScale;                                // Approximated latitude scale
  double lngScale;                                // Approximated longitude scale

public:

  // Constructor
  NavigationPathVisualization();

  // Destructor
  virtual ~NavigationPathVisualization();

  // Indicates that textures and buffers have been cleared
  void destroyGraphic();

  // Indicates that textures and buffers shall be created
  void createGraphic();

  // Returns the tile info for the given tile
  NavigationPathTileInfo *findTileInfo(MapTile *tile);

  // Removes the tile from the visualization
  void removeTileInfo(MapTile *tile);

  // Recreate the graphic objects to reduce the number of graphic point buffers
  void optimizeGraphic();

  // Compute the distance in pixels between two points
  double computePixelDistance(MapPosition a, MapPosition b);

  // Get the calibrator for the given position
  MapCalibrator *findCalibrator(MapPosition pos, bool &deleteCalibrator);

  // Adds a new point to the visualization
  void addPoint(MapPosition pos);

  // Resets the overlay graphic hash for all map containers
  void resetOverlayGraphicHash();

  // Stores the content into a file
  void store(std::ofstream *ofs);

  // Recreates the content from a binary file
  void retrieve(char *&data, Int &size);

  // Getters and setters
  MapPosition getPoint(Int index) {
    return points[index];
  }

  Int getPointsSize() const {
    return points.size();
  }

  MapPosition getPrevArrowPoint() const
  {
      return prevArrowPoint;
  }

  MapPosition getPrevLinePoint() const
  {
      return prevLinePoint;
  }

  Int getZoomLevel() const
  {
      return zoomLevel;
  }

  void setZoomLevel(Int zoomLevel)
  {
      this->zoomLevel = zoomLevel;
  }

  std::vector<MapPosition> *getPoints() {
    return &points;
  }

  double getLatScale() const {
    return latScale;
  }

  void setLatScale(double latScale) {
    this->latScale = latScale;
  }

  double getLngScale() const {
    return lngScale;
  }

  void setLngScale(double lngScale) {
    this->lngScale = lngScale;
  }

  double getColorOffset() {
    return colorOffset;
  }

  void updateColorOffset(bool reverse) {
    double t=core->getNavigationEngine()->getColorOffsetDelta();
    colorOffset += reverse ? -t : +t;
    if (colorOffset>=1.0)
      colorOffset=0;
    if (colorOffset<0.0)
      colorOffset=1.0;
  }

};

}

#endif /* NAVIGATIONPATHVISUALIZATION_H_ */
