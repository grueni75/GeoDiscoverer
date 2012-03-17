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

// Updates the geographic coordinates (longitude and latitude) from the given picture coordinates
void MapCalibrator::setGeographicCoordinates(MapPosition &pos) {

  // Ensure that only one thread is executing this function at a time
  core->getThread()->lockMutex(accessMutex);

  // Check that we have enough calibration points
  if (calibrationPoints.size()<3) {
    FATAL("at least 3 calibration points are required",NULL);
    core->getThread()->unlockMutex(accessMutex);
    return;
  }

  // Find the nearest calibration points
  for (std::list<MapPosition*>::const_iterator i=calibrationPoints.begin();i!=calibrationPoints.end();i++) {
    MapPosition *t=*i;
    Int dX=pos.getX()-t->getX();
    Int dY=pos.getY()-t->getY();
    UInt d=dX*dX+dY*dY;
    //DEBUG("d=%d",d);
    t->setDistance(d);
  }
  calibrationPoints.sort(MapPosition::distanceSortPredicate);

  // Solve the equation
  std::vector<double> x; x.resize(3);
  std::vector<double> y; y.resize(3);
  std::vector<double> lng; lng.resize(3);
  std::vector<double> lat; lat.resize(3);
  Int j=0;
  for (std::list<MapPosition*>::const_iterator i=calibrationPoints.begin();i!=calibrationPoints.end();i++) {
    MapPosition *t=*i;
    x[j]=t->getX();
    y[j]=t->getY();
    lng[j]=t->getLng();
    lat[j]=t->getLat();
    if (j==2)
      break;
    j++;
  }
  std::vector<double> cLng,cLat;
  cLng=FloatingPoint::solveZEqualsC1XPlusC2YPlusC3(x,y,lng);
  cLat=FloatingPoint::solveZEqualsC1XPlusC2YPlusC3(x,y,lat);
  if ((cLng.size()==0)) {
    FATAL("can not solve equation lng=cLng[0]*x+cLng[1]*y+cLng[2]",NULL);
    core->getThread()->unlockMutex(accessMutex);
    return;
  }
  if ((cLat.size()==0)) {
    FATAL("can not solve equation lat=cLat[0]*x+cLat[1]*y+cLat[2]",NULL);
    core->getThread()->unlockMutex(accessMutex);
    return;
  }

  // Compute the position
  double t=cLng[0]*pos.getX()+cLng[1]*pos.getY()+cLng[2];
  pos.setLng(t);
  t=cLat[0]*pos.getX()+cLat[1]*pos.getY()+cLat[2];
  pos.setLat(t);

  // That's it
  core->getThread()->unlockMutex(accessMutex);
}

/*

def lnglat2xy(lng,lat):

  # Find out the nearest two points
  solved=0
  l=calib_points
  while solved==0:
    l.sort(key=lambda x:(x.calc_dist(lng,lat)))
    (solved_x,cx1,cx2,cx3)=solve_equation(l,"x")
    (solved_y,cy1,cy2,cy3)=solve_equation(l,"y")
    if (solved_x==0) or (solved_y==0):
       print "ERROR: can not solve equation!";
       sys.exit(1)
    else:
       solved=1

  # Compute position
  #print "Computing position"
  #print lng
  #print lat
  x=cx1*lng+cx2*lat+cx3
  y=cy1*lng+cy2*lat+cy3
  #print x
  #print y
  return (int(x),int(y))
  #return (0,0)
*/

// Updates the picture coordinates from the given geographic coordinates
bool MapCalibrator::setPictureCoordinates(MapPosition &pos) {

  // Ensure that only one thread is executing this function at a time
  core->getThread()->lockMutex(accessMutex);

  // Check that we have enough calibration points
  if (calibrationPoints.size()<3) {
    FATAL("at least 3 calibration points are required",NULL);
    core->getThread()->unlockMutex(accessMutex);
    return false;
  }

  // Find the nearest calibration points
  for (std::list<MapPosition*>::const_iterator i=calibrationPoints.begin();i!=calibrationPoints.end();i++) {
    MapPosition *t=*i;
    double dLat=pos.getLat()-t->getLat();
    double dLng=pos.getLng()-t->getLng();
    double d=dLat*dLat+dLng*dLng;
    //DEBUG("dLat=%f dLng=%f d_double=%f d_uint=%d",dLat,dLng,d,(UInt)d);
    t->setDistance(d);
  }
  calibrationPoints.sort(MapPosition::distanceSortPredicate);

  // Solve the equation
  std::vector<double> x; x.resize(3);
  std::vector<double> y; y.resize(3);
  std::vector<double> lng; lng.resize(3);
  std::vector<double> lat; lat.resize(3);
  Int j=0;
  for (std::list<MapPosition*>::const_iterator i=calibrationPoints.begin();i!=calibrationPoints.end();i++) {
    MapPosition *t=*i;
    x[j]=t->getX();
    y[j]=t->getY();
    lng[j]=t->getLng();
    lat[j]=t->getLat();
    if (j==2)
      break;
    j++;
  }
  std::vector<double> cX,cY;
  cX=FloatingPoint::solveZEqualsC1XPlusC2YPlusC3(lng,lat,x);
  cY=FloatingPoint::solveZEqualsC1XPlusC2YPlusC3(lng,lat,y);
  if ((cX.size()==0)) {
    FATAL("can not solve equation x=cX[0]*lng+cX[1]*lat+cX[2]",NULL);
    core->getThread()->unlockMutex(accessMutex);
    return false;
  }
  if ((cY.size()==0)) {
    FATAL("can not solve equation y=cY[0]*lng+cY[1]*lat+cY[2]",NULL);
    core->getThread()->unlockMutex(accessMutex);
    return false;
  }

  // Compute the position
  double t=round(cX[0]*pos.getLng()+cX[1]*pos.getLat()+cX[2]);
  if ((t>std::numeric_limits<Int>::max())||(t<std::numeric_limits<Int>::min())) {
    core->getThread()->unlockMutex(accessMutex);
    return false;
  }
  pos.setX(t);
  t=round(cY[0]*pos.getLng()+cY[1]*pos.getLat()+cY[2]);
  if ((t>std::numeric_limits<Int>::max())||(t<std::numeric_limits<Int>::min())) {
    core->getThread()->unlockMutex(accessMutex);
    return false;
  }
  pos.setY(t);

  // That's it
  core->getThread()->unlockMutex(accessMutex);
  return true;
}

// Store the contents of the object in a binary file
void MapCalibrator::store(std::ofstream *ofs, Int &memorySize) {

  // Calculate memory
  memorySize+=sizeof(*this);

  // Write the size of the object for detecting changes later
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

  // Check if the class has changed
  Int size=sizeof(MapCalibrator);
#ifdef TARGET_LINUX
  if (size!=40) {
    FATAL("unknown size of object (%d), please adapt class storage",size);
    return NULL;
  }
#endif

  // Read the size of the object and check with current size
  size=0;
  Storage::retrieveInt(cacheData,cacheSize,size);
  if (size!=sizeof(MapCalibrator)) {
    DEBUG("stored size of object does not match implemented object size, aborting retrieve",NULL);
    return NULL;
  }
  //PROFILE_ADD("sanity check");

  // Create a new map container object
  MapCalibrator *mapCalibrator=NULL;
  objectSize-=sizeof(MapCalibrator);
  if (objectSize<0) {
    DEBUG("can not create map calibrator object",NULL);
    return NULL;
  }
  mapCalibrator=new(objectData) MapCalibrator(true);
  objectData+=sizeof(MapCalibrator);
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

}
