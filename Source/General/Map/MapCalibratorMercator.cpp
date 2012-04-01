//============================================================================
// Name        : MapCalibratorMercator.cpp
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

MapCalibratorMercator::MapCalibratorMercator(bool doNotDelete) : MapCalibrator(doNotDelete) {
  type=MapCalibratorTypeMercator;
}

MapCalibratorMercator::~MapCalibratorMercator() {
  // TODO Auto-generated destructor stub
}

// Computes the mercator projection for the given value
double MapCalibratorMercator::computeMercatorProjection(double value) {
  double t=value*M_PI/180.0;
  return log((sin(t)+1)/cos(t));
}

// Computes the inverse mercator projection for the given value
double MapCalibratorMercator::computeInverseMercatorProjection(double value) {
  double t=pow(M_E,value);
  return 2.0*atan((t-1)/(t+1))*180.0/M_PI;
}

// Updates the geographic coordinates (longitude and latitude) from the given picture coordinates
void MapCalibratorMercator::setGeographicCoordinates(MapPosition &pos) {

  // Ensure that only one thread is executing this function at a time
  core->getThread()->lockMutex(accessMutex);

  // Check that we have enough calibration points
  if (calibrationPoints.size()<3) {
    FATAL("at least 3 calibration points are required",NULL);
    core->getThread()->unlockMutex(accessMutex);
    return;
  }

  // Find the nearest calibration points
  sortCalibrationPoints(pos,true);

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
    lat[j]=computeMercatorProjection(t->getLat());
    if (j==2)
      break;
    j++;
  }
  std::vector<double> cLng,cLat;
  cLng=FloatingPoint::solveZEqualsC0XPlusC1YPlusC2(x,y,lng);
  cLat=FloatingPoint::solveZEqualsC0XPlusC1YPlusC2(x,y,lat);
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
  pos.setLat(computeInverseMercatorProjection(t));

  // That's it
  core->getThread()->unlockMutex(accessMutex);
}

// Updates the picture coordinates from the given geographic coordinates
bool MapCalibratorMercator::setPictureCoordinates(MapPosition &pos) {

  // Ensure that only one thread is executing this function at a time
  core->getThread()->lockMutex(accessMutex);

  // Check that we have enough calibration points
  if (calibrationPoints.size()<3) {
    FATAL("at least 3 calibration points are required",NULL);
    core->getThread()->unlockMutex(accessMutex);
    return false;
  }

  // Find the nearest calibration points
  sortCalibrationPoints(pos,false);

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
    lat[j]=computeMercatorProjection(t->getLat());
    if (j==2)
      break;
    j++;
  }
  std::vector<double> cX,cY;
  cX=FloatingPoint::solveZEqualsC0XPlusC1YPlusC2(lng,lat,x);
  cY=FloatingPoint::solveZEqualsC0XPlusC1YPlusC2(lng,lat,y);
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
  double t=round(cX[0]*pos.getLng()+cX[1]*computeMercatorProjection(pos.getLat())+cX[2]);
  if ((t>std::numeric_limits<Int>::max())||(t<std::numeric_limits<Int>::min())) {
    core->getThread()->unlockMutex(accessMutex);
    return false;
  }
  pos.setX(t);
  t=round(cY[0]*pos.getLng()+cY[1]*computeMercatorProjection(pos.getLat())+cY[2]);
  if ((t>std::numeric_limits<Int>::max())||(t<std::numeric_limits<Int>::min())) {
    core->getThread()->unlockMutex(accessMutex);
    return false;
  }
  pos.setY(t);

  // That's it
  core->getThread()->unlockMutex(accessMutex);
  return true;
}

} /* namespace GEODISCOVERER */
