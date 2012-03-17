//============================================================================
// Name        : MapSource.h
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


#ifndef MAPSOURCE_H_
#define MAPSOURCE_H_

namespace GEODISCOVERER {

class MapSource {

protected:

  std::string folder;                       // Folder that contains calibrated maps
  double neighborDegreeTolerance;           // Maximum allowed difference in degrees to classify a tile as a neighbor
  std::vector<MapContainer*> mapContainers; // Vector of all maps
  MapPosition centerPosition;               // Center position of the map
  MapPosition currentPosition;              // Current position in the map
  bool isInitialized;                       // Indicates if the object is initialized
  bool isScratchOnly;                       // Indicates that the object should not be deinited
  bool doNotDelete;                         // Indicates if the object has been alloacted by an own memory handler
  char *objectData;                         // Memory that holds all the sub objects if retrieve was used
  Int mapTileLength;                        // Default length of a tile
  Int progressValue;                        // Current progress value of the retrieve
  DialogKey progressDialog;                 // Handle to progress dialog
  std::string progressDialogTitle;          // Title of the progress dialog
  Int progressUpdateValue;                  // Value when to update the progress dialog
  Int progressValueMax;                     // Maximum progress value
  Int progressIndex;                        // State of progress dialog

  // Lists of map containers sorted by their boundaries
  std::vector<Int> mapsIndexByLatNorth;
  std::vector<Int> mapsIndexByLatSouth;
  std::vector<Int> mapsIndexByLngWest;
  std::vector<Int> mapsIndexByLngEast;

  // Root node of the kd tree
  std::vector<MapContainerTreeNode*> zoomLevelSearchTrees;

  // Inserts a new tile into the sorted list associated with the given border
  void insertMapContainerToSortedList(std::vector<Int> *list, MapContainer *newMapContainer, Int newMapContainerIndex, GeographicBorder border);

  // Creates a sorted index vector from a given sorted index vector masked by range on an other index vector
  void createMaskedIndexVector(std::vector<Int> *currentIndexVector,
                               std::vector<Int> *allowedIndexVector,
                               std::vector<Int> &sortedIndexVector);

  // Creates the kd tree recursively
  MapContainerTreeNode *createSearchTree(MapContainerTreeNode *parentNode, bool leftBranch, GeographicBorder dimension, std::vector<Int> remainingMapContainersIndex);

  // Stores the contents of the search tree in a binary file
  void storeSearchTree(std::ofstream *ofs, MapContainerTreeNode *node, Int &memorySize);

  // Reads the contents of the search tree from a binary file
  static MapContainerTreeNode *retrieveSearchTree(MapSource *mapSource, char *&cacheData, Int &cacheSize, char *&objectData, Int &objectSize);

  // Returns the map container that lies in a given area
  MapContainer *findMapContainerByGeographicArea(MapArea area, MapTile *preferredNeigbor, MapContainerTreeNode* currentMapContainerTreeNode, GeographicBorder currentDimension, double &bestDistance, MapArea &bestTranslatedArea, bool &betterMapContainerFound, std::list<MapContainer*> *foundMapContainers=NULL);

  // Returns the map tile in which the position lies
  MapContainer *findMapContainerByGeographicCoordinates(MapPosition pos, MapContainer *preferredMapContainer, MapContainerTreeNode* currentMapContainerTreeNode, GeographicBorder currentDimension, MapPosition &bestPos, double &distToNearestLngScale, double &distToNearestLatScale, bool &betterMapContainerFound);

public:

  // Constructurs and destructor
  MapSource(bool isScratchOnly=false, bool doNotDelete=false);
  virtual ~MapSource();

  // Destructs the objects correctly (i.e., if memory has not been allocated by new)
  static void destruct(MapSource *object);

  // Initialzes the source
  bool init();

  // Clears the source
  void deinit();

  // Returns the map tile in which the position lies
  MapTile *findMapTileByGeographicCoordinates(MapPosition pos, Int zoomLevel, bool lockZoomLevel, MapContainer *preferredMapContainer=NULL);

  // Returns the map tile that lies in a given area
  MapTile *findMapTileByGeographicArea(MapArea area, MapTile *preferredNeigbor, MapContainer* &usedMapContainer);

  // Returns a list of map containers that overlap the given area
  std::list<MapContainer*> findMapContainersByGeographicArea(MapArea area);

  // Store the contents of the object in a binary file
  void store(std::ofstream *ofs, Int &memorySize);

  // Reads the contents of the object from a binary file
  static MapSource *retrieve(char *&cacheData, Int &cacheSize, char *&objectData, Int &objectSize, std::string folder);

  // Increases the progress by one tep
  void increaseProgress();

  // Getters and setters
  Int getMapTileLength() const
  {
      return mapTileLength;
  }

  Int getMapTileWidth() const
  {
      return mapTileLength;
  }

  Int getMapTileHeight() const
  {
      return mapTileLength;
  }

  double getNeighborDegreeTolerance() const
  {
      return neighborDegreeTolerance;
  }

  MapPosition getCenterPosition()
  {
      return centerPosition;
  }

  std::vector<MapContainer*> *getMapContainers()
  {
      return (std::vector<MapContainer*>*)((((((&mapContainers))))));
  }

  void setIsInitialized(bool value)
  {
      isInitialized=value;
  }

  bool getIsInitialized() const
  {
      return isInitialized;
  }

  std::string getFolder() const {
    return folder;
  }

  std::vector<MapContainerTreeNode*> *getZoomLevelSearchTrees()
  {
      return &zoomLevelSearchTrees;
  }
};

}

#endif /* MAPSOURCE_H_ */
