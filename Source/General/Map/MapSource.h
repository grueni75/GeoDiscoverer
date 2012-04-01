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

typedef enum { MapSourceTypeCalibratedPictures, MapSourceTypeMercatorTiles } MapSourceType;

class MapSource {

protected:

  MapSourceType type;                      // Type of source
  std::string folder;                       // Folder that contains calibrated maps
  double neighborPixelTolerance ;           // Maximum allowed difference in pixels to classify a tile as a neighbor
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
  bool contentsChanged;                     // Indicates if the users of the map source need to update their data structures

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

  // Returns the map container that lies in a given area
  MapContainer *findMapContainerByGeographicArea(MapArea area, MapTile *preferredNeigbor, MapContainerTreeNode* currentMapContainerTreeNode, GeographicBorder currentDimension, double &bestDistance, MapArea &bestTranslatedArea, bool &betterMapContainerFound, std::list<MapContainer*> *foundMapContainers=NULL);

  // Returns the map tile in which the position lies
  MapContainer *findMapContainerByGeographicCoordinates(MapPosition pos, MapContainer *preferredMapContainer, MapContainerTreeNode* currentMapContainerTreeNode, GeographicBorder currentDimension, MapPosition &bestPos, double &distToNearestLngScale, double &distToNearestLatScale, bool &betterMapContainerFound);

  // Inserts a new map container in the search tree
  void insertNodeIntoSearchTree(MapContainer *newMapContainer, Int zoomLevel, MapContainerTreeNode* prevMapContainerTreeNode, bool useRightChild, GeographicBorder currentDimension);

  // Recreates the search data structures
  void createSearchDataStructures(bool showProgressDialog=false);

public:

  // Constructurs and destructor
  MapSource();
  virtual ~MapSource();

  // Initialzes the source
  virtual bool init() = 0;

  // Clears the source
  virtual void deinit();

  // Returns the map tile in which the position lies
  virtual MapTile *findMapTileByGeographicCoordinate(MapPosition pos, Int zoomLevel, bool lockZoomLevel, MapContainer *preferredMapContainer=NULL);

  // Returns the map tile that lies in a given area
  virtual MapTile *findMapTileByGeographicArea(MapArea area, MapTile *preferredNeigbor, MapContainer* &usedMapContainer);

  // Returns a list of map containers that overlap the given area
  std::list<MapContainer*> findMapContainersByGeographicArea(MapArea area);

  // Initializes the progress bar
  void openProgress(std::string title, Int valueMax);

  // Increases the progress by one tep
  void increaseProgress();

  // Closes the progress bar
  void closeProgress();

  // Creates the required type of map source object
  static MapSource *newMapSource();

  // Performs maintenance (e.g., recreate degraded search tree)
  virtual void maintenance();

  // Returns the scale values for the given zoom level
  virtual void getScales(Int zoomLevel, double &latScale, double &lngScale) = 0;

  // Finds the calibrator for the given position
  virtual MapCalibrator *findMapCalibrator(Int zoomLevel, MapPosition pos, bool &deleteCalibrator) = 0;

  // Getters and setters
  Int getMapTileLength() const {
    return mapTileLength;
  }

  Int getMapTileWidth() const {
    return mapTileLength;
  }

  Int getMapTileHeight() const {
    return mapTileLength;
  }

  double getNeighborPixelTolerance() const {
    return neighborPixelTolerance;
  }

  MapPosition getCenterPosition() {
    return centerPosition;
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

  std::string getFolderPath() const {
    return core->getHomePath() + "/Map/" + folder;
  }

  virtual void lockAccess() {
  }

  virtual void unlockAccess() {
  }

  std::vector<MapContainer*>* getMapContainers() {
    return (std::vector<MapContainer*>*) (((((((&mapContainers)))))));
  }

  Int getZoomLevelCount()
  {
      return zoomLevelSearchTrees.size();
  }

};

}

#endif /* MAPSOURCE_H_ */
