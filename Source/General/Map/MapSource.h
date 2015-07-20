//============================================================================
// Name        : MapSource.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef MAPSOURCE_H_
#define MAPSOURCE_H_

namespace GEODISCOVERER {

typedef enum { MapSourceTypeCalibratedPictures, MapSourceTypeMercatorTiles, MapSourceTypeEmpty } MapSourceType;
typedef std::map<std::string, Int> MapLayerNameMap;
typedef std::pair<std::string, Int> MapLayerNamePair;

class MapSource {

protected:

  MapSourceType type;                       // Type of source
  std::string folder;                       // Folder that contains the map data
  std::list<ZipArchive*> mapArchives;       // Zip archives that contain the calibrated maps
  ThreadMutexInfo *mapArchivesMutex;        // Mutex to access the map archives
  double neighborPixelTolerance ;           // Maximum allowed difference in pixels to classify a tile as a neighbor
  std::vector<MapContainer*> mapContainers; // Vector of all maps
  MapPosition *centerPosition;              // Center position of the map
  bool isInitialized;                       // Indicates if the object is initialized
  Int mapTileLength;                        // Default length of a tile
  Int progressValue;                        // Current progress value of the retrieve
  DialogKey progressDialog;                 // Handle to progress dialog
  std::string progressDialogTitle;          // Title of the progress dialog
  Int progressUpdateValue;                  // Value when to update the progress dialog
  Int progressValueMax;                     // Maximum progress value
  Int progressIndex;                        // State of progress dialog
  bool contentsChanged;                     // Indicates if the users of the map source need to update their data structures
  std::list<std::string> status;            // Status of the map source
  ThreadMutexInfo *statusMutex;             // Mutex for accessing the status
  MapDownloader *mapDownloader;             // Downlads missing tiles from the tileserver
  MapLayerNameMap mapLayerNameMap;          // Defines for each zoom level a name
  Int minZoomLevel;                         // Minimum zoom value
  Int maxZoomLevel;                         // Maximum zoom value

  // Lists of map containers sorted by their boundaries
  std::vector<Int> mapsIndexByLatNorth;
  std::vector<Int> mapsIndexByLatSouth;
  std::vector<Int> mapsIndexByLngWest;
  std::vector<Int> mapsIndexByLngEast;

  // Root node of the kd tree
  std::vector<MapContainerTreeNode*> zoomLevelSearchTrees;

  // Holds all attributes extracted from the gds file
  static std::list<ConfigSection> availableGDSInfos;
  static ConfigSection resolvedGDSInfo;

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
  MapContainer *findMapContainerByGeographicCoordinate(MapPosition pos, MapContainer *preferredMapContainer, MapContainerTreeNode* currentMapContainerTreeNode, GeographicBorder currentDimension, MapPosition &bestPos, double &distToNearestLngScale, double &distToNearestLatScale, bool &betterMapContainerFound, std::list<MapContainer*> *foundMapContainers=NULL);

  // Inserts a new map container in the search tree
  void insertNodeIntoSearchTree(MapContainer *newMapContainer, Int zoomLevel, MapContainerTreeNode* prevMapContainerTreeNode, bool useRightChild, GeographicBorder currentDimension);

  // Recreates the search data structures
  void createSearchDataStructures(bool showProgressDialog=false);

  // Reads all available map sources
  static void readAvailableGDSInfos();

  // Reads information about the map
  static bool resolveGDSInfo(std::string infoFilePath);

  // Renames the layers with the infos in the gds file
  void renameLayers();

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

  // Returns a list of map containers in which the given position lies
  std::list<MapContainer*> findMapContainersByGeographicCoordinate(MapPosition pos, Int zoomLevel=0);

  // Initializes the progress bar
  void openProgress(std::string title, Int valueMax);

  // Increases the progress by one tep
  bool increaseProgress();

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

  // Marks a map container as obsolete
  // Please note that other objects might still use this map container
  // Call unlinkMapContainer to solve this afterwards
  virtual void markMapContainerObsolete(MapContainer *c);

  // Removes all obsolete map containers
  virtual void removeObsoleteMapContainers(bool removeFromMapArchive);

  // Returns the names of each map layer
  std::list<std::string> getMapLayerNames();

  // Selects the given map layer
  void selectMapLayer(std::string name);

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

  MapPosition *getCenterPosition() {
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

  std::string getLegendPath() const {
    return core->getHomePath() + "/Map/" + folder + "/legend.png";
  }

  virtual void lockAccess(const char *file, int line) {
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

  std::list<std::string> getStatus(const char *file, int line) const {
    core->getThread()->lockMutex(statusMutex, file, line);
    std::list<std::string> status=this->status;
    core->getThread()->unlockMutex(statusMutex);
    return status;
  }

  void setStatus(std::list<std::string> status, const char *file, int line) {
    core->getThread()->lockMutex(statusMutex, file, line);
    this->status = status;
    core->getThread()->unlockMutex(statusMutex);
  }

  std::list<ZipArchive*> *lockMapArchives(const char *file, int line) {
    core->getThread()->lockMutex(mapArchivesMutex, file, line);
    return &mapArchives;
  }

  void unlockMapArchives() {
    core->getThread()->unlockMutex(mapArchivesMutex);
  }

  MapDownloader* getMapDownloader() {
    return mapDownloader;
  }

};

}

#endif /* MAPSOURCE_H_ */
