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

typedef enum { MapCalibratorTypeLinear=0, MapCalibratorTypeMercator=1 } MapCalibratorType;

class MapCalibrator {

protected:

  MapCalibratorType type; // Type of calibrator
  bool doNotDelete; // Indicates if the object has been alloacted by an own memory handler
  std::list<MapPosition *> calibrationPoints; // List of calibration points
  ThreadMutexInfo *accessMutex; // Mutex for using the set*Coordinates methods

  // Finds the nearest n calibration points
  void sortCalibrationPoints(MapPosition &pos, bool usePictureCoordinates);

public:

  // Creates a new map calibrator of the given type by reserving new memory
  static MapCalibrator *newMapCalibrator(MapCalibratorType type);

  // Creates a new map calibrator of the given type by using given memory
  static MapCalibrator *newMapCalibrator(MapCalibratorType type, char *&cacheData, Int &cacheSize);

  // Constructors and destructor
  MapCalibrator(bool doNotDelete=false);
  virtual ~MapCalibrator();

  // Destructs the objects correctly (i.e., if memory has not been allocated by new)
  static void destruct(MapCalibrator *object);

  // Adds a calibration point
  void addCalibrationPoint(MapPosition pos);

  // Updates the geographic coordinates (longitude and latitude) from the given picture coordinates
  virtual void setGeographicCoordinates(MapPosition &pos) = 0;

  // Updates the picture coordinates from the given geographic coordinates
  virtual bool setPictureCoordinates(MapPosition &pos) = 0;

  // Compute the distance in pixels for the given points
  double computePixelDistance(MapPosition a, MapPosition b);

  // Returns the number of calibration points
  Int numberOfCalibrationPoints() const
  {
    return calibrationPoints.size();
  }

  // Store the contents of the object in a binary file
  void store(std::ofstream *ofs);

  // Reads the contents of the object from a binary file
  static MapCalibrator *retrieve(char *&cacheData, Int &cacheSize);

  // Getters and setters
  std::list<MapPosition*> *getCalibrationPoints()
  {
      return &calibrationPoints;
  }

  MapCalibratorType getType() const {
    return type;
  }

};

}

#endif /* MAPCALIBRATOR_H_ */
