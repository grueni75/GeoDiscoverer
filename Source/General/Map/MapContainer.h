//============================================================================
// Name        : MapContainer.h
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


#ifndef MAPCONTAINER_H_
#define MAPCONTAINER_H_

namespace GEODISCOVERER {

typedef enum { GeographicBorderLatNorth, GeographicBorderLatSouth, GeographicBorderLngEast, GeographicBorderLngWest } GeographicBorder;
typedef enum { ImageTypeJPEG, ImageTypePNG } ImageType;

class MapContainer {

protected:

  bool                doNotDelete;          // Indicates if the object has been alloacted by an own memory handler
  std::string         mapFileFolder;        // Folder in which the picture and calibration data of the map is stored
  std::string         imageFileName;        // Filename of the picture of the map
  std::string         imageFilePath;        // Complete path to the picture of the map
  ImageType           imageType;            // File format of the image
  std::string         calibrationFileName;  // Filename of the calibration data of the map
  std::string         calibrationFilePath;  // Complete path to the calibration data of the map
  Int                 zoomLevel;            // Zoom level of the map
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

  // Lists of map tiles sorted by their boundaries
  std::vector<Int> mapTilesIndexByMapTop;
  std::vector<Int> mapTilesIndexByMapBottom;
  std::vector<Int> mapTilesIndexByMapLeft;
  std::vector<Int> mapTilesIndexByMapRight;

  // Root node of the kd tree
  MapTile *searchTree;

  // Adds a tile to the map
  void addTile(MapTile *tile);

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
  MapTile *findMapTileByPictureCoordinates(MapPosition pos, MapTile *currentTile, PictureBorder currentDimension);

  // Returns the map tile that lies in a given area and that is closest to the given neigbor
  MapTile *findMapTileByPictureArea(MapArea area, MapTile *preferredNeigbor, MapTile *currentTile, PictureBorder currentDimension, double &bestDistance, bool &betterTileFound, std::list<MapTile*> *foundMapTiles=NULL);

  // Stores the contents of the search tree in a binary file
  void storeSearchTree(std::ofstream *ofs, MapTile *node, Int &memorySize);

  // Reads the contents of the search tree from a binary file
  static MapTile *retrieveSearchTree(MapContainer *mapContainer, Int &nodeNumber, char *&cacheData, Int &cacheSize, char *&objectData, Int &objectSize);

public:

  // Constructor
  MapContainer(bool doNotDelete=false);

  // Destructor
  virtual ~MapContainer();

  // Destructs the objects correctly (i.e., if memory has not been allocated by new)
  static void destruct(MapContainer *object);

  // Returns the file extension of supported calibration files
  static std::string getCalibrationFileExtension(Int i);

  // Checks if the extension is supported as a calibration file
  static bool calibrationFileIsSupported(std::string extension);

  // Returns the map tile in which the position lies
  MapTile *findMapTileByPictureCoordinates(MapPosition pos);

  // Returns the map tile that lies in a given area and that is closest to the given neigbor
  MapTile *findMapTileByPictureArea(MapArea area, MapTile *preferredNeigbor);

  // Returns all map tiles that lies in a given area
  std::list<MapTile*> findMapTilesByPictureArea(MapArea area);

  // Reads a calibration file
  bool readCalibrationFile(std::string fileFolder, std::string fileBasename, std::string fileExtension);

  // Writes a calibration file
  void writeCalibrationFile();

  // Store the contents of the object in a binary file
  void store(std::ofstream *ofs, Int &memorySize);

  // Reads the contents of the object from a binary file
  static MapContainer *retrieve(char *&cacheData, Int &cacheSize, char *&objectData, Int &objectSize);

  // Getters and setters
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
      return imageFilePath;
  }

  MapCalibrator *getMapCalibrator() const
  {
      return mapCalibrator;
  }

  std::string getImageFileFolder() const
  {
      return mapFileFolder;
  }

  std::string getImageFileName() const
  {
      return imageFileName;
  }

  std::vector<MapTile*> *getMapTiles()
  {
  return (std::vector<MapTile*>*)(((((&mapTiles)))));
  }

  Int getHeight() const
  {
      return height;
  }

  Int getWidth() const
  {
      return width;
  }

  double getLatNorth() const
  {
      return latNorth;
  }

  double getLatSouth() const
  {
      return latSouth;
  }

  double getLngEast() const
  {
      return lngEast;
  }

  double getLngWest() const
  {
      return lngWest;
  }

  double getBorder(GeographicBorder border);

  Int getZoomLevel() const
  {
      return zoomLevel;
  }

  void setZoomLevel(Int zoomLevel)
  {
      this->zoomLevel = zoomLevel;
  }

  ImageType getImageType() const
  {
      return imageType;
  }

  MapContainer *getLeftChild() const
{
    return leftChild;
}

  MapContainer *getRightChild() const
  {
      return rightChild;
  }

  void setLeftChild(MapContainer *leftChild)
  {
      this->leftChild = leftChild;
  }

  void setRightChild(MapContainer *rightChild)
  {
      this->rightChild = rightChild;
  }

  double getLatCenter() const
  {
    return latSouth+(latNorth-latSouth)/2;
  }

  double getLngCenter() const
  {
    return lngWest+(lngEast-lngWest)/2;
  }

  Int getTileCount() const
  {
    return mapTiles.size();
  }
};

}

#endif /* MAPCONTAINER_H_ */
