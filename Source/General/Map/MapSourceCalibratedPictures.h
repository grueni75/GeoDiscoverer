//============================================================================
// Name        : MapSourceCalibratedPictures.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef MAPSOURCECALIBRATEDPICTURES_H_
#define MAPSOURCECALIBRATEDPICTURES_H_

namespace GEODISCOVERER {

class MapSourceCalibratedPictures  : public MapSource {

protected:

  // Path to the map archive
  std::list<std::string> mapArchivePaths;

  // Memory that holds all the sub objects if retrieve was used
  char *cacheData;

  // Stores the contents of the search tree in a binary file
  void storeSearchTree(std::ofstream *ofs, MapContainerTreeNode *node);

  // Reads the contents of the search tree from a binary file
  static MapContainerTreeNode *retrieveSearchTree(MapSourceCalibratedPictures *mapSource, char *&cacheData, Int &cacheSize);

  // Loads all calibrated pictures in the given directory
  bool collectMapTiles(std::string directory, std::list<std::string> &mapFilebases);

public:

  // Constructurs and destructor
  MapSourceCalibratedPictures(std::list<std::string> mapArchivePaths);
  virtual ~MapSourceCalibratedPictures();

  // Operators
  MapSourceCalibratedPictures &operator=(const MapSourceCalibratedPictures &rhs);

  // Initialzes the source
  virtual bool init();

  // Clears the source
  virtual void deinit();

  // Store the contents of the object in a binary file
  void store(std::ofstream *ofs);

  // Reads the contents of the object from a binary file
  static bool retrieve(MapSourceCalibratedPictures *mapSource, char *&cacheData, Int &cacheSize, std::string folder);

  // Finds the calibrator for the given position
  virtual MapCalibrator *findMapCalibrator(Int zoomLevel, MapPosition pos, bool &deleteCalibrator);

  // Returns the scale values for the given zoom level
  virtual void getScales(Int zoomLevel, double &latScale, double &lngScale);

  // Getters and setters

};

}

#endif /* MAPSOURCECALIBRATEDPICTURES_H_ */
