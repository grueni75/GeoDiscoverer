//============================================================================
// Name        : MapCalibrator.h
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


#ifndef MAPCALIBRATOR_H_
#define MAPCALIBRATOR_H_

namespace GEODISCOVERER {

class MapCalibrator {

protected:

  bool doNotDelete; // Indicates if the object has been alloacted by an own memory handler
  std::list<MapPosition *> calibrationPoints; // List of calibration points
  ThreadMutexInfo *accessMutex; // Mutex for using the set*Coordinates methods

public:

  // Constructors and desctructor
  MapCalibrator(bool doNotDelete=false);
  virtual ~MapCalibrator();

  // Destructs the objects correctly (i.e., if memory has not been allocated by new)
  static void destruct(MapCalibrator *object);

  // Adds a calibration point
  void addCalibrationPoint(MapPosition pos);

  // Updates the geographic coordinates (longitude and latitude) from the given picture coordinates
  void setGeographicCoordinates(MapPosition &pos);

  // Updates the picture coordinates from the given geographic coordinates
  bool setPictureCoordinates(MapPosition &pos);

  // Returns the number of calibration points
  Int numberOfCalibrationPoints() const
  {
    return calibrationPoints.size();
  }

  // Store the contents of the object in a binary file
  void store(std::ofstream *ofs, Int &memorySize);

  // Reads the contents of the object from a binary file
  static MapCalibrator *retrieve(char *&cacheData, Int &cacheSize, char *&objectData, Int &objectSize);

  // Getters and setters
  std::list<MapPosition*> *getCalibrationPoints()
  {
      return &calibrationPoints;
  }

};

}

#endif /* MAPCALIBRATOR_H_ */
