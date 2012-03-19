//============================================================================
// Name        : MapOfflineSource.h
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


#ifndef MAPOFFLINESOURCE_H_
#define MAPOFFLINESOURCE_H_

namespace GEODISCOVERER {

class MapOfflineSource  : public MapSource {

protected:

  bool isScratchOnly;                       // Indicates that the object should not be deinited
  bool doNotDelete;                         // Indicates if the object has been alloacted by an own memory handler
  char *objectData;                         // Memory that holds all the sub objects if retrieve was used

  // Stores the contents of the search tree in a binary file
  void storeSearchTree(std::ofstream *ofs, MapContainerTreeNode *node, Int &memorySize);

  // Reads the contents of the search tree from a binary file
  static MapContainerTreeNode *retrieveSearchTree(MapOfflineSource *mapSource, char *&cacheData, Int &cacheSize, char *&objectData, Int &objectSize);

  // Returns the map container that lies in a given area
  MapContainer *findMapContainerByGeographicArea(MapArea area, MapTile *preferredNeigbor, MapContainerTreeNode* currentMapContainerTreeNode, GeographicBorder currentDimension, double &bestDistance, MapArea &bestTranslatedArea, bool &betterMapContainerFound, std::list<MapContainer*> *foundMapContainers=NULL);

  // Returns the map tile in which the position lies
  MapContainer *findMapContainerByGeographicCoordinates(MapPosition pos, MapContainer *preferredMapContainer, MapContainerTreeNode* currentMapContainerTreeNode, GeographicBorder currentDimension, MapPosition &bestPos, double &distToNearestLngScale, double &distToNearestLatScale, bool &betterMapContainerFound);

public:

  // Constructurs and destructor
  MapOfflineSource(bool isScratchOnly=false, bool doNotDelete=false);
  virtual ~MapOfflineSource();

  // Destructs the objects correctly (i.e., if memory has not been allocated by new)
  static void destruct(MapOfflineSource *object);

  // Initialzes the source
  virtual bool init();

  // Clears the source
  virtual void deinit();

  // Returns the map tile in which the position lies
  virtual MapTile *findMapTileByGeographicCoordinates(MapPosition pos, Int zoomLevel, bool lockZoomLevel, MapContainer *preferredMapContainer=NULL);

  // Returns the map tile that lies in a given area
  virtual MapTile *findMapTileByGeographicArea(MapArea area, MapTile *preferredNeigbor, MapContainer* &usedMapContainer);

  // Returns a list of map containers that overlap the given area
  virtual std::list<MapContainer*> findMapContainersByGeographicArea(MapArea area);

  // Store the contents of the object in a binary file
  void store(std::ofstream *ofs, Int &memorySize);

  // Reads the contents of the object from a binary file
  static MapOfflineSource *retrieve(char *&cacheData, Int &cacheSize, char *&objectData, Int &objectSize, std::string folder);

  // Getters and setters

};

}

#endif /* MAPOFFLINESOURCE_H_ */
