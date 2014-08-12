//============================================================================
// Name        : MapSourceCalibratedPictures.h
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
  void getScales(Int zoomLevel, double &latScale, double &lngScale);

  // Getters and setters

};

}

#endif /* MAPOFFLINESOURCE_H_ */
