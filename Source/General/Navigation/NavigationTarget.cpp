//============================================================================
// Name        : NavigationTarget.cpp
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

// Constructor
NavigationTarget::NavigationTarget(MapPosition pos) {

  // Set the variables
  this->pos=pos;

  // Add the visualization to all tiles
  updateTileVisualization(NULL);

}

// Destructor
NavigationTarget::~NavigationTarget() {

  // Remove the visualization from all tiles
  for(NavigationTargetTileInfoMap::iterator i=tileInfoMap.begin();i!=tileInfoMap.end();i++) {
    MapTile *tile = i->first;
    GraphicObject *tileVisualization = tile->getVisualization();
    tileVisualization->lockAccess(__FILE__, __LINE__);
    tileVisualization->removePrimitive(i->second,true);
    tileVisualization->unlockAccess();
  }
  tileInfoMap.clear();
}

// Updates the visualization of the tile
void NavigationTarget::updateTileVisualization(std::list<MapContainer*> *mapContainers) {

  // Find all map tiles that overlap with this segment
  core->getMapSource()->lockAccess();
  std::list<MapContainer*> foundContainers;
  if (!mapContainers) {
    foundContainers=core->getMapSource()->findMapContainersByGeographicCoordinate(pos);
  } else {
    for (std::list<MapContainer*>::iterator i=mapContainers->begin();i!=mapContainers->end();i++) {
      if (  ((*i)->getLatNorth()>=pos.getLat())&&((*i)->getLatSouth()<=pos.getLat())
          &&((*i)->getLngEast() >=pos.getLng())&&((*i)->getLngWest() <=pos.getLng())) {
        foundContainers.push_back(*i);
      }
    }
  }

  // Go through all of them
  for(std::list<MapContainer*>::iterator j=foundContainers.begin();j!=foundContainers.end();j++) {

    // Compute the coordinates of the segment within the container
    MapContainer *mapContainer=*j;
    bool overflowOccured=false;
    if (!mapContainer->getMapCalibrator()->setPictureCoordinates(pos)) {
      overflowOccured=true;
    }
    if (!overflowOccured) {

      // Find the map tile in which the position lies
      MapTile *tile=mapContainer->findMapTileByPictureCoordinate(pos);

      // Add the visualization to the tile
      GraphicObject *tileVisualization=tile->getVisualization();
      tileVisualization->lockAccess(__FILE__, __LINE__);
      GraphicRectangle *icon = new GraphicRectangle(*core->getGraphicEngine()->getTargetIcon());
      if (!icon) {
        FATAL("can not create graphic rectangle object",NULL);
        return;
      }
      icon->setX(pos.getX()-tile->getMapX(0)-icon->getIconWidth()/2);
      icon->setY(tile->getHeight()-(pos.getY()-tile->getMapY(0)-icon->getIconHeight()/2));
      icon->setZ(3); // ensure that rectangle is drawn over pathes
      icon->setDestroyTexture(false);
      tileInfoMap.insert(NavigationTargetTileInfoPair(tile,tileVisualization->addPrimitive(icon)));
      tileVisualization->unlockAccess();
    }
  }
  core->getMapSource()->unlockAccess();
}


// Indicates that textures and buffers have been invalidated
void NavigationTarget::recreateGraphic() {

  // Recreate all icons
  for(NavigationTargetTileInfoMap::iterator i=tileInfoMap.begin();i!=tileInfoMap.end();i++) {
    MapTile *tile = i->first;
    GraphicObject *tileVisualization = tile->getVisualization();
    GraphicRectangle *icon=(GraphicRectangle*)tileVisualization->getPrimitive(i->second);
    icon->setTexture(core->getGraphicEngine()->getTargetIcon()->getTexture());
  }
}

// Updates the visualization of the given containers
void NavigationTarget::addVisualization(std::list<MapContainer*> *mapContainers) {
  updateTileVisualization(mapContainers);
}

// Removes the visualization of the given container
void NavigationTarget::removeVisualization(MapContainer* mapContainer) {

  // Remove the tile from the visualization
  std::vector<MapTile*> *mapTiles=mapContainer->getMapTiles();
  for(std::vector<MapTile*>::iterator i=mapTiles->begin();i!=mapTiles->end();i++) {
    MapTile *tile = *i;
    NavigationTargetTileInfoMap::iterator j;
    j=tileInfoMap.find(tile);
    if (j!=tileInfoMap.end()) {
      GraphicObject *tileVisualization=tile->getVisualization();
      tileVisualization->lockAccess(__FILE__, __LINE__);
      tile->getVisualization()->removePrimitive(j->second);
      tileVisualization->unlockAccess();
      tileInfoMap.erase(j);
    }
  }
}

} /* namespace GEODISCOVERER */
