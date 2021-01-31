//============================================================================
// Name        : MapCalibrator.cpp
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

#include <Core.h>
#include <MapCalibrator.h>
#include <MapPosition.h>
#include <Storage.h>
#include <MapCalibratorLinear.h>
#include <MapCalibratorProj4.h>
#include <MapCalibratorSphericalNormalMercator.h>

namespace GEODISCOVERER {

// Constructor
MapCalibrator::MapCalibrator(bool doNotDelete) {
  this->doNotDelete=doNotDelete;
  args=NULL;
  if (doNotDelete) {
    new(&this->calibrationPoints) std::list<MapPosition*>();
  }
  this->accessMutex=core->getThread()->createMutex("map calibrator access mutex");
}

// Destructor
MapCalibrator::~MapCalibrator() {

  // Close all referenced libraries
  deinit();

  // Delete all calibration points
  for (std::list<MapPosition *>::const_iterator i=calibrationPoints.begin();i!=calibrationPoints.end();i++) {
    MapPosition *t=*i;
    MapPosition::destruct(t);
  }
  if (!doNotDelete) {
    if (args) free(args);
  }
  core->getThread()->destroyMutex(this->accessMutex);
}

// Adds a calibration point
void MapCalibrator::addCalibrationPoint(MapPosition pos) {
  //DEBUG("Adding calibration point (%d %d %.2f %.2f)",pos.getX(),pos.getY(),pos.getLng().toDouble(),pos.getLat().toDouble());

  // Ensure that only one thread is executing this function at a time
  core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);

  // Convert theg geographic longitude / latitude to cartesian X / Y coordinates
  convertGeographicToCartesian(pos);

  // Make a copy of the calibration point
  MapPosition *t=new MapPosition();
  if (!t) {
    FATAL("can not create map position",NULL);
  }
  *t=pos;

  // Put it into the list
  calibrationPoints.push_back(t);

  core->getThread()->unlockMutex(accessMutex);
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
      t->setDistance(pos.computeDistance(t));
    }
    //DEBUG("d=%d",d);
  }
  calibrationPoints.sort(MapPosition::distanceSortPredicate);
}

// Store the contents of the object in a binary file
void MapCalibrator::store(std::ofstream *ofs) {

  // Write the size of the object for detecting changes later
  Storage::storeInt(ofs,type);
  Int size;
  switch(type) {
    case MapCalibratorTypeLinear:
      size=sizeof(MapCalibratorLinear);
      break;
    case MapCalibratorTypeSphericalNormalMercator:
      size=sizeof(MapCalibratorSphericalNormalMercator);
      break;
    case MapCalibratorTypeProj4:
      size=sizeof(MapCalibratorProj4);
      break;
    default:
      FATAL("map calibrator type not supported",NULL);
      break;
  }
  Storage::storeInt(ofs,size);

  // Store all relevant fields
  Storage::storeMem(ofs,(char*)this,size,true);
  if (args==NULL)
    Storage::storeString(ofs,(char*)"");
  else
    Storage::storeString(ofs,args);
  Storage::storeInt(ofs,calibrationPoints.size());
  for(std::list<MapPosition*>::iterator i=calibrationPoints.begin();i!=calibrationPoints.end();i++) {
    (*i)->store(ofs);
  }

}

// Reads the contents of the object from a binary file
MapCalibrator *MapCalibrator::retrieve(char *&cacheData, Int &cacheSize) {

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
      if (expectedSize!=56) {
        FATAL("unknown size of object (%d), please adapt class storage",expectedSize);
        return NULL;
      }
#endif
      break;
    case MapCalibratorTypeSphericalNormalMercator:
      expectedSize=sizeof(MapCalibratorSphericalNormalMercator);
#ifdef TARGET_LINUX
      if (expectedSize!=64) {
        FATAL("unknown size of object (%d), please adapt class storage",expectedSize);
        return NULL;
      }
#endif
      break;
    case MapCalibratorTypeProj4:
      expectedSize=sizeof(MapCalibratorProj4);
#ifdef TARGET_LINUX
      if (expectedSize!=72) {
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
  Storage::skipPadding(cacheData,cacheSize);
  MapCalibrator *mapCalibrator=MapCalibrator::newMapCalibrator(type,cacheData,cacheSize);
  //PROFILE_ADD("object creation");

  // Read the fields
  Storage::retrieveString(cacheData,cacheSize,&mapCalibrator->args);

  // Init the calibrator
  mapCalibrator->init();

  // Read the calibration points
  Storage::retrieveInt(cacheData,cacheSize,size);
  for (Int i=0;i<size;i++) {
    MapPosition *p=MapPosition::retrieve(cacheData,cacheSize);
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
    case MapCalibratorTypeSphericalNormalMercator:
      return new MapCalibratorSphericalNormalMercator();
      break;
    case MapCalibratorTypeProj4:
      return new MapCalibratorProj4();
      break;
    default:
      FATAL("unsupported map calibration type",NULL);
      break;
  }
  return NULL;
}

// Creates a new map calibrator of the given type by using given memory
MapCalibrator *MapCalibrator::newMapCalibrator(MapCalibratorType type, char *&cacheData, Int &cacheSize) {
  MapCalibrator *mapCalibrator=NULL;
  MapCalibratorLinear *linear;
  MapCalibratorSphericalNormalMercator *mercator;
  Int size;
  switch(type) {
    case MapCalibratorTypeLinear:
      size=sizeof(MapCalibratorLinear);
      mapCalibrator=new(cacheData) MapCalibratorLinear(true);
      break;
    case MapCalibratorTypeSphericalNormalMercator:
      size=sizeof(MapCalibratorSphericalNormalMercator);
      mapCalibrator=new(cacheData) MapCalibratorSphericalNormalMercator(true);
      break;
    case MapCalibratorTypeProj4:
      size=sizeof(MapCalibratorProj4);
      mapCalibrator=new(cacheData) MapCalibratorProj4(true);
      break;
    default:
      FATAL("unsupported map calibration type",NULL);
      return NULL;
      break;
  }
  cacheSize-=size;
  if (cacheSize<0) {
    DEBUG("can not create map calibrator object",NULL);
    return NULL;
  }
  //mapCalibrator=(MapCalibrator*)cacheData;
  cacheData+=size;
  return mapCalibrator;
}

// Compute the distance in pixels for the given points
double MapCalibrator::computePixelDistance(MapPosition a, MapPosition b) {
  setPictureCoordinates(a);
  setPictureCoordinates(b);
  double distX = a.getX()-b.getX();
  double distY = a.getY()-b.getY();
  return sqrt(distX*distX+distY*distY);
}

// Inits the calibrator
void MapCalibrator::init() {

}

// Frees the calibrator
void MapCalibrator::deinit() {

}

// Returns the three nearest calibration points
bool MapCalibrator::findThreeNearestCalibrationPoints(bool usePictureCoordinates, MapPosition pos, std::vector<double> &pictureX, std::vector<double> &pictureY, std::vector<double> &cartesianX, std::vector<double> &cartesianY) {

  // Check that we have enough calibration points
  if (calibrationPoints.size()<3) {
    FATAL("at least 3 calibration points are required",NULL);
    return false;
  }

  // Prepare variables
  pictureX.resize(3);
  pictureY.resize(3);
  cartesianX.resize(3);
  cartesianY.resize(3);

  // Find the nearest calibration points
  sortCalibrationPoints(pos,usePictureCoordinates);

  // Solve the equation
  Int j=0;
  for (std::list<MapPosition*>::const_iterator i=calibrationPoints.begin();i!=calibrationPoints.end();i++) {
    MapPosition *t=*i;
    pictureX[j]=t->getX();
    pictureY[j]=t->getY();
    cartesianX[j]=t->getCartesianX();
    cartesianY[j]=t->getCartesianY();
    if (j==2)
      break;
    j++;
  }
  return true;
}

// Updates the geographic coordinates (longitude and latitude) from the given picture coordinates
bool MapCalibrator::setGeographicCoordinates(MapPosition &pos) {

  // Ensure that only one thread is executing this function at a time
  core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);

  // Find the nearest calibration points
  std::vector<double> picX;
  std::vector<double> picY;
  std::vector<double> cartX;
  std::vector<double> cartY;
  if (!findThreeNearestCalibrationPoints(true,pos,picX,picY,cartX,cartY)) {
    core->getThread()->unlockMutex(accessMutex);
    return false;
  }

  // Solve the equation to compute cartesian coordinates from picture coordinates
  std::vector<double> cHoriz,cVert;
  cHoriz=FloatingPoint::solveZEqualsC0XPlusC1YPlusC2(picX,picY,cartX);
  cVert=FloatingPoint::solveZEqualsC0XPlusC1YPlusC2(picX,picY,cartY);
  if ((cHoriz.size()==0)) {
    FATAL("can not solve equation cartX=cHoriz[0]*picX+cHoriz[1]*picY+cHoriz[2]",NULL);
    core->getThread()->unlockMutex(accessMutex);
    return false;
  }
  if ((cVert.size()==0)) {
    FATAL("can not solve equation cartY=cVert[0]*picX+cVert[1]*picY+cVert[2]",NULL);
    core->getThread()->unlockMutex(accessMutex);
    return false;
  }

  // Compute the position
  pos.setCartesianX(cHoriz[0]*pos.getX()+cHoriz[1]*pos.getY()+cHoriz[2]);
  pos.setCartesianY(cVert[0]*pos.getX()+cVert[1]*pos.getY()+cVert[2]);
  convertCartesianToGeographic(pos);

  // Check ranges
  if ((pos.getLng()>180.0)||(pos.getLng()<-180.0)) {
    core->getThread()->unlockMutex(accessMutex);
    return false;
  }
  if ((pos.getLat()>90.0)||(pos.getLat()<-90.0)) {
    core->getThread()->unlockMutex(accessMutex);
    return false;
  }

  // That's it
  core->getThread()->unlockMutex(accessMutex);
  return true;
}

// Updates the picture coordinates from the given geographic coordinates
bool MapCalibrator::setPictureCoordinates(MapPosition &pos) {

  // Ensure that only one thread is executing this function at a time
  core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);

  // Find the nearest calibration points
  std::vector<double> picX;
  std::vector<double> picY;
  std::vector<double> cartX;
  std::vector<double> cartY;
  if (!findThreeNearestCalibrationPoints(false,pos,picX,picY,cartX,cartY)) {
    core->getThread()->unlockMutex(accessMutex);
    return false;
  }

  // Solve the equation to compute picture coordinates from cartesian coordinates
  std::vector<double> cHoriz,cVert;
  cHoriz=FloatingPoint::solveZEqualsC0XPlusC1YPlusC2(cartX,cartY,picX);
  cVert=FloatingPoint::solveZEqualsC0XPlusC1YPlusC2(cartX,cartY,picY);
  if ((cHoriz.size()==0)) {
    FATAL("can not solve equation picX=cHoriz[0]*cartX+cHoriz[1]*cartY+cHoriz[2]",NULL);
    core->getThread()->unlockMutex(accessMutex);
    return false;
  }
  if ((cVert.size()==0)) {
    FATAL("can not solve equation picY=cVert[0]*cartX+cVert[1]*cartY+cVert[2]",NULL);
    core->getThread()->unlockMutex(accessMutex);
    return false;
  }

  // Compute the position
  convertGeographicToCartesian(pos);
  double x=round(cHoriz[0]*pos.getCartesianX()+cHoriz[1]*pos.getCartesianY()+cHoriz[2]);
  double y=round(cVert[0]*pos.getCartesianX()+cVert[1]*pos.getCartesianY()+cVert[2]);

  // Check ranges
  if ((x>std::numeric_limits<Int>::max())||(x<std::numeric_limits<Int>::min())) {
    core->getThread()->unlockMutex(accessMutex);
    return false;
  }
  if ((y>std::numeric_limits<Int>::max())||(y<std::numeric_limits<Int>::min())) {
    core->getThread()->unlockMutex(accessMutex);
    return false;
  }
  pos.setX(x);
  pos.setY(y);

  // That's it
  core->getThread()->unlockMutex(accessMutex);
  return true;
}

}
