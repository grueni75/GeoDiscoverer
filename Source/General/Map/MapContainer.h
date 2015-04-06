//============================================================================
// Name        : MapContainer.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef MAPCONTAINER_H_
#define MAPCONTAINER_H_

namespace GEODISCOVERER {

typedef enum { GeographicBorderLatNorth, GeographicBorderLatSouth, GeographicBorderLngEast, GeographicBorderLngWest } GeographicBorder;
typedef enum { ImageTypeJPEG=0, ImageTypePNG=1, ImageTypeUnknown=2 } ImageType;

class MapContainer {

protected:

  bool                doNotDelete;          // Indicates if the object has been alloacted by an own memory handler
  char                *mapFileFolder;       // Folder in which the picture and calibration data of the map is stored
  char                *imageFileName;       // Filename of the picture of the map
  char                *imageFilePath;       // Complete path to the picture of the map
  ImageType           imageType;            // File format of the image
  char                *calibrationFileName; // Filename of the calibration data of the map
  char                *calibrationFilePath; // Complete path to the calibration data of the map
  Int                 x;                    // X coordinate
  Int                 y;                    // Y coordinate
  Int                 zoomLevel;            // Zoom level of the map
  Int                 downloadRetries;      // Number of retries done so far for downloading the image
  MapCalibrator       *mapCalibrator;       // Calibration model of the map
  Int                 width;                // Width of the map
  Int                 height;               // Height of the map
  std::vector<MapTile*> mapTiles;           // All tiles the map consists of
  double latNorth;                          // Maximum north border
  double latSouth;                          // Minimum south border
  double lngEast;                           // Maximum east border
  double lngWest;                           // Minimum west border
  double lngScale;                          // Scale factor for longitude
  double latScale;                          // Scale factor for latitude
  MapContainer *leftChild;                  // Left child in the kd tree
  MapContainer *rightChild;                 // Right child in the kd tree
  bool downloadComplete;                    // Indicates that the image has been downloaded to disk
  bool overlayGraphicInvalid;               // Indicates that this tile is missing it's overlay graphics

  // Lists of map tiles sorted by their boundaries
  std::vector<Int> mapTilesIndexByMapTop;
  std::vector<Int> mapTilesIndexByMapBottom;
  std::vector<Int> mapTilesIndexByMapLeft;
  std::vector<Int> mapTilesIndexByMapRight;

  // Root node of the kd tree
  MapTile *searchTree;

  // Gets the next field in a semicolon seperated list
  std::string getNextSemicolonField(std::string list, Int &start);

  // Reads a gmi file
  //bool readGMICalibrationFile();

  // Reads a gdm file
  bool readGDMCalibrationFile();

  // Returns the text contained in a xml node
  std::string getNodeText(XMLNode node);

  // Inserts a new tile into the sorted list associated with the given border
  void insertTileToSortedList(std::vector<Int> *list, MapTile *newTile, Int newTileIndex, PictureBorder border);

  // Creates a sorted index vector from a given sorted index vector masked by range on an other index vector
  void createMaskedIndexVector(std::vector<Int> *currentIndexVector,
                               std::vector<Int> *allowedIndexVector,
                               std::vector<Int> &sortedIndexVector);

  // Creates the kd tree recursively
  void createSearchTree(MapTile *parentNode, bool leftBranch, PictureBorder dimension, std::vector<Int> remainingMapTilesIndex);

  // Returns the map tile in which the position lies
  MapTile *findMapTileByPictureCoordinate(MapPosition pos, MapTile *currentTile, PictureBorder currentDimension);

  // Returns the map tile that lies in a given area and that is closest to the given neigbor
  MapTile *findMapTileByPictureArea(MapArea area, MapTile *preferredNeigbor, MapTile *currentTile, PictureBorder currentDimension, double &bestDistance, bool &betterTileFound, std::list<MapTile*> *foundMapTiles=NULL);

  // Stores the contents of the search tree in a binary file
  void storeSearchTree(std::ofstream *ofs, MapTile *node);

  // Reads the contents of the search tree from a binary file
  static MapTile *retrieveSearchTree(MapContainer *mapContainer, Int &nodeNumber, char *&cacheData, Int &cacheSize, bool &abortRetrieve);

public:

  // Constructor
  MapContainer(bool doNotDelete=false);
  MapContainer(const MapContainer &pos);

  // Destructor
  virtual ~MapContainer();

  // Operators
  MapContainer &operator=(const MapContainer &rhs);

  // Destructs the objects correctly (i.e., if memory has not been allocated by new)
  static void destruct(MapContainer *object);

  // Returns the file extension of supported calibration files
  static std::string getCalibrationFileExtension(Int i);

  // Checks if the extension is supported as a calibration file
  static bool calibrationFileIsSupported(std::string extension);

  // Returns the map tile in which the position lies
  MapTile *findMapTileByPictureCoordinate(MapPosition pos);

  // Returns the map tile that lies in a given area and that is closest to the given neigbor
  MapTile *findMapTileByPictureArea(MapArea area, MapTile *preferredNeigbor);

  // Returns all map tiles that lies in a given area
  std::list<MapTile*> findMapTilesByPictureArea(MapArea area);

  // Reads a calibration file
  bool readCalibrationFile(std::string fileFolder, std::string fileBasename, std::string fileExtension);

  // Writes a calibration file
  void writeCalibrationFile();

  // Store the contents of the object in a binary file
  void store(std::ofstream *ofs);

  // Reads the contents of the object from a binary file
  static MapContainer *retrieve(char *&cacheData, Int &cacheSize);

  // Checks if the container contains tiles that are currently used for screen drawing
  bool isDrawn();

  // Adds a tile to the map
  void addTile(MapTile *tile);

  // Creates the search tree
  void createSearchTree();

  // Getters and setters
  bool getOverlayGraphicInvalid() const {
    return overlayGraphicInvalid;
  }

  void setOverlayGraphicInvalid(bool overlayGraphisInvalid) {
    this->overlayGraphicInvalid = overlayGraphicInvalid;
  }

  double getLatScale() const
  {
      return latScale;
  }

  double getLngScale() const
  {
      return lngScale;
  }

  std::string getImageFilePath() const
  {
      return std::string(imageFilePath);
  }

  MapCalibrator *getMapCalibrator() const
  {
      return mapCalibrator;
  }

  std::string getImageFileFolder() const
  {
      return std::string(mapFileFolder);
  }

  std::string getImageFileName() const
  {
      return std::string(imageFileName);
  }

  std::vector<MapTile*> *getMapTiles() {
    return (std::vector<MapTile*>*)(((((((((((((&mapTiles)))))))))))));
  }

  Int getHeight() const {
    return height;
  }

  Int getWidth() const {
    return width;
  }

  double getLatNorth() const {
    return latNorth;
  }

  double getLatSouth() const {
    return latSouth;
  }

  double getLngEast() const {
    return lngEast;
  }

  double getLngWest() const {
    return lngWest;
  }

  double getBorder(GeographicBorder border);

  Int getZoomLevel() const {
    return zoomLevel;
  }

  void setZoomLevel(Int zoomLevel) {
    this->zoomLevel = zoomLevel;
  }

  ImageType getImageType() const {
    return imageType;
  }

  MapContainer* getLeftChild() const {
    return leftChild;
  }

  MapContainer* getRightChild() const {
    return rightChild;
  }

  void setLeftChild(MapContainer* leftChild) {
    this->leftChild = leftChild;
  }

  void setRightChild(MapContainer* rightChild) {
    this->rightChild = rightChild;
  }

  double getLatCenter() const {
    return latSouth + (latNorth - latSouth) / 2;
  }

  double getLngCenter() const {
    return lngWest + (lngEast - lngWest) / 2;
  }

  Int getTileCount() const {
    return mapTiles.size();
  }

  TimestampInSeconds getLastAccess();

  Int getDownloadRetries() const {
    return downloadRetries;
  }

  void setDownloadRetries(Int downloadRetries) {
    this->downloadRetries = downloadRetries;
  }

  Int getX() const {
    return x;
  }

  void setX(Int x) {
    this->x = x;
  }

  Int getY() const {
    return y;
  }

  void setY(Int y) {
    this->y = y;
  }

  bool getDownloadComplete() const {
    return downloadComplete;
  }

  void setDownloadComplete(bool downloadComplete) {
    this->downloadComplete = downloadComplete;
    for(std::vector<MapTile *>::const_iterator i=mapTiles.begin(); i != mapTiles.end(); i++) {
      (*i)->setIsCached(false); // update the texture
    }
  }

  void setMapCalibrator(MapCalibrator* mapCalibrator) {
    this->mapCalibrator = mapCalibrator;
  }

  void setHeight(Int height) {
    this->height = height;
  }

  void setWidth(Int width) {
    this->width = width;
  }

  void setCalibrationFileName(std::string calibrationFileName) {
    if (doNotDelete) {
      FATAL("can not set new calibration file name because memory is statically allocated",NULL);
    } else {
      if (this->calibrationFileName) free(this->calibrationFileName);
      if (!(this->calibrationFileName=strdup(calibrationFileName.c_str()))) {
        FATAL("can not create string",NULL);
      }
      std::string path;
      if (strcmp(mapFileFolder,".")==0)
        path=calibrationFileName;
      else
        path=std::string(mapFileFolder) + "/" + calibrationFileName;
      if (this->calibrationFilePath) free(this->calibrationFilePath);
      if (!(this->calibrationFilePath=strdup(path.c_str()))) {
        FATAL("can not create string",NULL);
      }
    }
  }

  void setImageType(ImageType imageType) {
    this->imageType = imageType;
  }

  void setImageFileName(std::string imageFileName) {
    if (doNotDelete) {
      FATAL("can not set new image file name because memory is statically allocated",NULL);
    } else {
      if (this->imageFileName) free(this->imageFileName);
      if (!(this->imageFileName=strdup(imageFileName.c_str()))) {
        FATAL("can not create string",NULL);
      }
      std::string path;
      if (strcmp(mapFileFolder,".")==0)
        path = imageFileName;
      else
        path = std::string(mapFileFolder) + "/" + imageFileName;
      if (this->imageFilePath) free(this->imageFilePath);
      if (!(this->imageFilePath=strdup(path.c_str()))) {
        FATAL("can not create string",NULL);
      }
    }
  }

  void setMapFileFolder(std::string mapFileFolder) {
    if (doNotDelete) {
      FATAL("can not set new map file folder because memory is statically allocated",NULL);
    } else {
      if (this->mapFileFolder) free(this->mapFileFolder);
      if (!(this->mapFileFolder=strdup(mapFileFolder.c_str()))) {
        FATAL("can not create string",NULL);
      }
    }
  }

  char* getCalibrationFileName() const {
    return calibrationFileName;
  }

  char* getCalibrationFilePath() const {
    return calibrationFilePath;
  }
};

}

#endif /* MAPCONTAINER_H_ */
