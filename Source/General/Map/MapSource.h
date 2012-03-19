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

typedef enum { MapOfflineSourceType, MapOnlineSourceType } MapSourceTypes;

class MapSource {

protected:

  MapSourceTypes type;                      // Type of source
  std::string folder;                       // Folder that contains calibrated maps
  double neighborDegreeTolerance;           // Maximum allowed difference in degrees to classify a tile as a neighbor
  std::vector<MapContainer*> mapContainers; // Vector of all maps
  MapPosition centerPosition;               // Center position of the map
  MapPosition currentPosition;              // Current position in the map
  bool isInitialized;                       // Indicates if the object is initialized
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

public:

  // Constructurs and destructor
  MapSource();
  virtual ~MapSource();

  // Initialzes the source
  virtual bool init() = 0;

  // Clears the source
  virtual void deinit();

  // Returns the map tile in which the position lies
  virtual MapTile *findMapTileByGeographicCoordinates(MapPosition pos, Int zoomLevel, bool lockZoomLevel, MapContainer *preferredMapContainer=NULL) = 0;

  // Returns the map tile that lies in a given area
  virtual MapTile *findMapTileByGeographicArea(MapArea area, MapTile *preferredNeigbor, MapContainer* &usedMapContainer) = 0;

  // Returns a list of map containers that overlap the given area
  virtual std::list<MapContainer*> findMapContainersByGeographicArea(MapArea area) = 0;

  // Initializes the progress bar
  void openProgress(std::string title, Int valueMax);

  // Increases the progress by one tep
  void increaseProgress();

  // Closes the progress bar
  void closeProgress();

  // Finds out which type of source to create
  static MapSourceTypes determineType();

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
