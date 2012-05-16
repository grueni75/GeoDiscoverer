//============================================================================
// Name        : NavigationTarget.h
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

#ifndef NAVIGATIONTARGET_H_
#define NAVIGATIONTARGET_H_

namespace GEODISCOVERER {

// Types for storing the tile info
typedef std::map<MapTile*, GraphicPrimitiveKey> NavigationTargetTileInfoMap;
typedef std::pair<MapTile*, GraphicPrimitiveKey> NavigationTargetTileInfoPair;

class NavigationTarget {

  MapPosition pos;                          // Position to navigate to
  NavigationTargetTileInfoMap tileInfoMap;  // Map to find the primitive key of the visualization for a given tile

  // Updates the visualization of the tile
  void updateTileVisualization(std::list<MapContainer*> *mapContainers);

public:

  // Constructor
  NavigationTarget(MapPosition pos);

  // Destructor
  virtual ~NavigationTarget();

  // Indicates that textures and buffers have been invalidated
  void recreateGraphic();

  // Adds the visualization for the given containers
  void addVisualization(std::list<MapContainer*> *containers);

  // Remove the visualization for the given container
  void removeVisualization(MapContainer *container);

};

} /* namespace GEODISCOVERER */
#endif /* NAVIGATIONTARGET_H_ */
