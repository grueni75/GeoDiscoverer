//============================================================================
// Name        : MapCalibrator.cpp
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


#include <Core.h>

namespace GEODISCOVERER {

// Constructor
MapCalibrator::MapCalibrator(bool doNotDelete) {
  this->doNotDelete=doNotDelete;
  this->accessMutex=core->getThread()->createMutex();
}

// Destructor
MapCalibrator::~MapCalibrator() {
  // Delete all calibration points
  for (std::list<MapPosition *>::const_iterator i=calibrationPoints.begin();i!=calibrationPoints.end();i++) {
    MapPosition *t=*i;
    MapPosition::destruct(t);
  }
  core->getThread()->destroyMutex(this->accessMutex);
}

// Adds a calibration point
void MapCalibrator::addCalibrationPoint(MapPosition pos) {
  //DEBUG("Adding calibration point (%d %d %.2f %.2f)",pos.getX(),pos.getY(),pos.getLng().toDouble(),pos.getLat().toDouble());
  MapPosition *t=new MapPosition();
  if (!t) {
    FATAL("can not create map position",NULL);
    return;
  }
  *t=pos;
  calibrationPoints.push_back(t);
}

// Finds the nearest n calibration points
void MapCalibrator::sortCalibrationPoints(MapPosition &pos, bool usePictureCoordinates) {
  for (std::list<MapPosition*>::const_iterator i=calibrationPoints.begin();i!=calibrationPoints.end();i++) {
    MapPosition *t=*i;
    if (usePictureCoordinates) {
      Int dX=pos.getX()-t->getX();
      Int dY=pos.getY()-t->getY();
      UInt d=dX*dX+dY*dY;
      t->setDistance(d);
    } else {
      double dLat=pos.getLat()-t->getLat();
      double dLng=pos.getLng()-t->getLng();
      double d=dLat*dLat+dLng*dLng;
      //DEBUG("dLat=%f dLng=%f d_double=%f d_uint=%d",dLat,dLng,d,(UInt)d);
      t->setDistance(d);
    }
    //DEBUG("d=%d",d);
  }
  calibrationPoints.sort(MapPosition::distanceSortPredicate);
}

// Store the contents of the object in a binary file
void MapCalibrator::store(std::ofstream *ofs, Int &memorySize) {

  // Calculate memory
  memorySize+=sizeof(*this);

  // Write the size of the object for detecting changes later
  Storage::storeInt(ofs,type);
  Int size=sizeof(*this);
  Storage::storeInt(ofs,size);

  // Store all relevant fields
  Storage::storeInt(ofs,calibrationPoints.size());
  for(std::list<MapPosition*>::iterator i=calibrationPoints.begin();i!=calibrationPoints.end();i++) {
    (*i)->store(ofs,memorySize);
  }

}

// Reads the contents of the object from a binary file
MapCalibrator *MapCalibrator::retrieve(char *&cacheData, Int &cacheSize, char *&objectData, Int &objectSize) {

  //PROFILE_START;

  // Read the type
  Int t;
  Storage::retrieveInt(cacheData,cacheSize,t);
  MapCalibratorType type=(MapCalibratorType)t;

  // Check if the class has changed
  Int expectedSize;
  switch(type) {
    case MapCalibratorTypeLinear:
      expectedSize=sizeof(MapCalibratorLinear);
#ifdef TARGET_LINUX
      if (expectedSize!=40) {
        FATAL("unknown size of object (%d), please adapt class storage",expectedSize);
        return NULL;
      }
#endif
      break;
    case MapCalibratorTypeMercator:
      expectedSize=sizeof(MapCalibratorMercator);
#ifdef TARGET_LINUX
      if (expectedSize!=40) {
        FATAL("unknown size of object (%d), please adapt class storage",expectedSize);
        return NULL;
      }
#endif
      break;
    default:
      DEBUG("unsupported map calibration type, aborting retrieve",NULL);
      return NULL;
      break;
  }

  // Read the size of the object and check with current size
  Int size=0;
  Storage::retrieveInt(cacheData,cacheSize,size);
  if (size!=expectedSize) {
    DEBUG("stored size of object does not match implemented object size, aborting retrieve",NULL);
    return NULL;
  }
  //PROFILE_ADD("sanity check");

  // Create a new map container object
  MapCalibrator *mapCalibrator=MapCalibrator::newMapCalibrator(type,objectData,objectSize);
  //PROFILE_ADD("object creation");

  // Read the fields
  Storage::retrieveInt(cacheData,cacheSize,size);
  for (Int i=0;i<size;i++) {
    MapPosition *p=MapPosition::retrieve(cacheData,cacheSize,objectData,objectSize);
    if (p==NULL) {
      MapCalibrator::destruct(mapCalibrator);
      return NULL;
    }
    mapCalibrator->calibrationPoints.push_back(p);
  }
  //PROFILE_ADD("map position retrieve");

  // Return result
  return mapCalibrator;
}

// Destructs the objects correctly (i.e., if memory has not been allocated by new)
void MapCalibrator::destruct(MapCalibrator *object) {
  if (object->doNotDelete) {
    object->~MapCalibrator();
  } else {
    delete object;
  }
}

// Creates a new map calibrator of the given type by reserving new memory
MapCalibrator *MapCalibrator::newMapCalibrator(MapCalibratorType type) {
  MapCalibrator *mapCalibrator=NULL;
  Int size;
  switch(type) {
    case MapCalibratorTypeLinear:
      return new MapCalibratorLinear();
      break;
    case MapCalibratorTypeMercator:
      return new MapCalibratorMercator();
      break;
    default:
      FATAL("unsupported map calibration type",NULL);
      break;
  }
  return NULL;
}

// Creates a new map calibrator of the given type by using given memory
MapCalibrator *MapCalibrator::newMapCalibrator(MapCalibratorType type, char *&objectData, Int &objectSize) {
  MapCalibrator *mapCalibrator=NULL;
  Int size;
  switch(type) {
    case MapCalibratorTypeLinear:
      objectSize-=sizeof(MapCalibratorLinear);
      if (objectSize<0) {
        DEBUG("can not create linear map calibrator object",NULL);
        return NULL;
      }
      mapCalibrator=new(objectData) MapCalibratorLinear(true);
      objectData+=sizeof(MapCalibrator);
      return mapCalibrator;
      break;
    case MapCalibratorTypeMercator:
      objectSize-=sizeof(MapCalibratorMercator);
      if (objectSize<0) {
        DEBUG("can not create mercator map calibrator object",NULL);
        return NULL;
      }
      mapCalibrator=new(objectData) MapCalibratorMercator(true);
      objectData+=sizeof(MapCalibrator);
      return mapCalibrator;
      break;
    default:
      FATAL("unsupported map calibration type",NULL);
      break;
  }
  return NULL;
}

// Compute the distance in pixels for the given points
double MapCalibrator::computePixelDistance(MapPosition a, MapPosition b) {
  setPictureCoordinates(a);
  setPictureCoordinates(b);
  double distX = a.getX()-b.getX();
  double distY = a.getY()-b.getY();
  return sqrt(distX*distX+distY*distY);
}

}
