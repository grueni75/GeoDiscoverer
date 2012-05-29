//============================================================================
// Name        : NavigationPathVisualization.h
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


#ifndef NAVIGATIONPATHVISUALIZATION_H_
#define NAVIGATIONPATHVISUALIZATION_H_

namespace GEODISCOVERER {

// Types for storing the tile info
typedef std::map<MapTile*, NavigationPathTileInfo*> NavigationPathTileInfoMap;
typedef std::pair<MapTile*, NavigationPathTileInfo*> NavigationPathTileInfoPair;

class NavigationPathVisualization {

protected:

  Int zoomLevel;                                  // Zoom level that is represented by this visualization
  MapPosition prevLinePoint;                      // Last position used for creating the graphic line
  MapPosition prevArrowPoint;                     // Last position used for creating the direction arrow
  std::list<MapPosition> points;                  // All used points
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

  // Getters and setters
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

  std::list<MapPosition> *getPoints() {
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
};

}

#endif /* NAVIGATIONPATHVISUALIZATION_H_ */
