//============================================================================
// Name        : MapSource.h
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

#include <Dialog.h>
#include <MapDownloader.h>
#include <MapContainer.h>
#include <MapArchiveFile.h>

#ifndef MAPSOURCE_H_
#define MAPSOURCE_H_

namespace GEODISCOVERER {

typedef enum { MapSourceTypeCalibratedPictures, MapSourceTypeMercatorTiles, MapSourceTypeEmpty, MapSourceTypeRemote } MapSourceType;
typedef enum { OverlayArchiveTypeMapContainer=0, OverlayArchiveTypeNavigationEngine=1 } OverlayArchiveType;

class MapSource {

protected:

  MapSourceType type;                             // Type of source
  std::string folder;                             // Folder that contains the map data
  std::list<ZipArchive*> mapArchives;             // Zip archives that contain the calibrated maps
  ThreadMutexInfo *mapArchivesMutex;              // Mutex to access the map archives
  double neighborPixelTolerance ;                 // Maximum allowed difference in pixels to classify a tile as a neighbor
  std::vector<MapContainer*> mapContainers;       // Vector of all maps
  MapPosition *centerPosition;                    // Center position of the map
  bool isInitialized;                             // Indicates if the object is initialized
  Int mapTileLength;                              // Default length of a tile
  Int progressValue;                              // Current progress value of the retrieve
  DialogKey progressDialog;                       // Handle to progress dialog
  std::string progressDialogTitle;                // Title of the progress dialog
  Int progressUpdateValue;                        // Value when to update the progress dialog
  Int progressValueMax;                           // Maximum progress value
  Int progressIndex;                              // State of progress dialog
  bool contentsChanged;                           // Indicates if the users of the map source need to update their data structures
  std::list<std::string> status;                  // Status of the map source
  ThreadMutexInfo *statusMutex;                   // Mutex for accessing the status
  MapDownloader *mapDownloader;                   // Downloads missing tiles from the tileserver
  MapLayerNameMap mapLayerNameMap;                // Defines for each zoom level a name
  Int minZoomLevel;                               // Minimum zoom value
  Int maxZoomLevel;                               // Maximum zoom value
  static StringMap legendPaths;                   // List of available legends with their names
  bool quitRemoteServerThread;                    // Indicates that the remote server thread shall quit
  ThreadInfo *remoteServerThreadInfo;             // Thread that handles the serving of tiles for remote devices
  ThreadSignalInfo *remoteServerStartSignal;      // Signal for starting the remote server thread
  std::list<std::string> remoteServerCommandQueue; // Holds the commands that the remote server shall process
  bool resetRemoteServerThread;                   // Indicates if the remote server thread shall forget everything about the remote side
  std::list<MapArchiveFile> mapArchiveFiles;      // List of map archive files in the map folder
  bool recreateMapArchiveFiles;                   // Indicates that the map archive files shall be re-filled

  // Path animators loaded from overlay archives
  std::list<GraphicPrimitiveKey> retrievedPathAnimators;

  // Lists of map containers sorted by their boundaries
  std::vector<Int> mapsIndexByLatNorth;
  std::vector<Int> mapsIndexByLatSouth;
  std::vector<Int> mapsIndexByLngWest;
  std::vector<Int> mapsIndexByLngEast;

  // Root node of the kd tree
  std::vector<MapContainerTreeNode*> zoomLevelSearchTrees;

  // Holds all attributes extracted from the gds file
  static std::list<ConfigSection*> availableGDSInfos;
  static ConfigSection *resolvedGDSInfo;

  // Min and max zoom levels
  static const Int minZoomLevelDefault=0;
  static const Int maxZoomLevelDefault=19;

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

  // Replaces a variable in a string
  static bool replaceVariable(std::string &text, std::string variableName, std::string variableValue);

  // Reads information about the map
  static bool resolveGDSInfo(std::string infoFilePath, TimestampInSeconds *lastModification);

  // Renames the layers with the infos in the gds file
  void renameLayers();

  // Adds the given map container to the queue for sending to the remote server
  void queueRemoteMapContainer(MapContainer* c, std::vector<std::string> *alreadyKnownMapContainers, Int startIndex, std::list<std::vector<std::string> > *mapImagesToServe);

  // Updates the navigation engine overlay archive if necessary
  void queueRemoteNavigationEngineOverlayArchive(std::string remoteHash);

public:

  // Constructurs and destructor
  MapSource();
  virtual ~MapSource();

  // Initialzes the source
  virtual bool init() = 0;

  // Update any wear device about the map
  void remoteMapInit();

  // Clears the source
  virtual void deinit();

  // Handles request from remote devices for tiles
  void remoteServer();

  // Adds a new command for the remote server
  void queueRemoteServerCommand(std::string cmd);

  // Returns the map tile in which the position lies
  virtual MapTile *findMapTileByGeographicCoordinate(MapPosition pos, Int zoomLevel, bool lockZoomLevel, MapContainer *preferredMapContainer=NULL);

  // Returns the map tile that lies in a given area
  virtual MapTile *findMapTileByGeographicArea(MapArea area, MapTile *preferredNeigbor, MapContainer* &usedMapContainer);

  // Returns a list of map containers that overlap the given area
  std::list<MapContainer*> findMapContainersByGeographicArea(MapArea area);

  // Returns a list of map containers in which the given position lies
  std::list<MapContainer*> findMapContainersByGeographicCoordinate(MapPosition pos, Int zoomLevel=0);

  // Fills the given area with tiles
  virtual void fillGeographicAreaWithTiles(MapArea area, MapTile *preferredNeighbor, Int maxTiles, std::list<MapTile*> *tiles);

  // Initializes the progress bar
  void openProgress(std::string title, Int valueMax);

  // Increases the progress by one step
  bool increaseProgress();

  // Closes the progress bar
  void closeProgress();

  // Fetches a map tile and returns its image data
  virtual UByte *fetchMapTile(Int z, Int x, Int y, double saturationOffset, double brightnessOffset, UInt &imageSize);

  // Creates the required type of map source object
  static MapSource *newMapSource();

  // Performs maintenance (e.g., recreate degraded search tree)
  virtual void maintenance();

  // Returns the scale values for the given zoom level
  virtual void getScales(Int zoomLevel, double &latScale, double &lngScale);

  // Finds the calibrator for the given position
  virtual MapCalibrator *findMapCalibrator(Int zoomLevel, MapPosition pos, bool &deleteCalibrator);

  // Marks a map container as obsolete
  // Please note that other objects might still use this map container
  // Call unlinkMapContainer to solve this afterwards
  virtual void markMapContainerObsolete(MapContainer *c);

  // Removes all obsolete map containers
  virtual void removeObsoleteMapContainers(MapArea *displayArea, bool allZoomLevels=false);

  // Returns the names of each map layer
  std::list<std::string> getMapLayerNames();

  // Selects the given map layer
  void selectMapLayer(std::string name);

  // Returns the currently selected map layer
  std::string getSelectedMapLayer();

  // Adds a download job from the current visible map
  virtual void addDownloadJob(bool estimateOnly, std::string routeName, std::string zoomLevels);

  // Processes all pending download jobs
  virtual void processDownloadJobs();

  // Indicates if the map source has download jobs
  virtual bool hasDownloadJobs();

  // Returns the number of unqueued tiles
  virtual Int countUnqueuedDownloadTiles(bool peek);

  // Removes all download jobs
  virtual void clearDownloadJobs();

  // Ensures that all threads that download tiles are stopped
  virtual void stopDownloadThreads();

  // Trigger the download job processing
  virtual void triggerDownloadJobProcessing();

  // Returns the name of the given zoom level
  std::string getMapLayerName(int zoomLevel);

  // Returns a list of names of the maps that have a legend
  static std::list<std::string> getLegendNames();

  // Returns the file path to the legend with the given name
  static std::string getLegendPath(std::string name);

  // Adds a new map archive
  virtual bool addMapArchive(std::string path, std::string hash);

  // Adds a new overlay archive
  virtual bool addOverlayArchive(std::string path, std::string hash);

  // Creates all graphics
  void createGraphic();

  // Destroys all graphics
  void destroyGraphic();

  // Finds the path animator with the given name
  GraphicPrimitive *findPathAnimator(std::string name);

  // Finds the path animator with the given name
  void addPathAnimator(GraphicPrimitiveKey primitiveKey);

  // Inserts the new map archive file into the file list
  virtual void updateMapArchiveFiles(std::string filePath);

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

  virtual void lockAccess(const char *file, int line) {
  }

  virtual void unlockAccess() {
  }

  virtual void lockDownloadJobProcessing(const char *file, int line) {
  }

  virtual void unlockDownloadJobProcessing() {
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
    /*for (std::list<std::string>::iterator i=status.begin();i!=status.end();i++) {
      DEBUG("%s",i->c_str());
    }*/
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

  MapSourceType getType() {
    return type;
  }

  virtual Int getServerZoomLevel(Int mapZoomLevel) {
    return mapZoomLevel;
  }
};

}

#endif /* MAPSOURCE_H_ */
