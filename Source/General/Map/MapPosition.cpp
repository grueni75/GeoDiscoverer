//============================================================================
// Name        : MapPosition.cpp
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
#include <MapPosition.h>
#include <Storage.h>
#include <NavigationPath.h>

namespace GEODISCOVERER {

const double MapPosition::earthRadius = 6.371 * 1e6;

MapPosition::MapPosition(bool doNotDelete) {
  this->doNotDelete=doNotDelete;
  if (!doNotDelete) {
    x=-1; y=-1;
    cartesianX=-std::numeric_limits<double>::max();
    cartesianY=std::numeric_limits<double>::max();
    mapTile=NULL;
    lng=0;
    lat=0;
    altitude=-std::numeric_limits<double>::max();
    distance=0;
    latScale=0;
    lngScale=0;
    timestamp=0;
    accuracy=std::numeric_limits<double>::max();
    bearing=0;
    speed=0;
    hasAltitude=false;
    hasBearing=false;
    hasSpeed=false;
    hasAccuracy=false;
    isUpdated=false;
    invalidate();
    hasTimestamp=false;
    index=0;
  }
}

MapPosition::MapPosition(const MapPosition &pos) {
  bool doNotDelete=this->doNotDelete;
  *this=pos;
  this->doNotDelete=doNotDelete;
}

MapPosition::~MapPosition() {
}

// Operators
bool MapPosition::operator==(const MapPosition &rhs)
{
  if ((lng==rhs.getLng())&&(lat==rhs.getLat())&&
      (x==rhs.getX())&&(y==rhs.getY())&&
      (cartesianX==rhs.getCartesianX())&&(cartesianY==rhs.getCartesianY())&&
      (mapTile==rhs.getMapTile())&&
      (hasAltitude==rhs.getHasAltitude())&&(altitude==rhs.getAltitude())&&(distance==rhs.getDistance())&&
      (latScale==rhs.getLatScale())&&(lngScale==rhs.getLngScale())&&
      (hasBearing==rhs.getHasBearing())&&(bearing==rhs.getBearing())&&
      (hasSpeed==rhs.getHasSpeed())&&(speed==rhs.getSpeed())&&
      (hasAccuracy==rhs.getHasAccuracy())&&(accuracy==rhs.getAccuracy())&&
      (hasTimestamp==rhs.getHasTimestamp())&&(timestamp==rhs.getTimestamp()))
    return true;
  else
    return false;
}
bool MapPosition::operator!=(const MapPosition &rhs)
{
  return !(*this==rhs);
}
MapPosition &MapPosition::operator=(const MapPosition &rhs)
{
  memcpy((void*)this,(void*)&rhs,sizeof(MapPosition));
  return *this;
}

// Computes the destination point from the given bearing (degrees, clockwise from north) and distance (meters)
MapPosition MapPosition::computeTarget(double bearing, double distance) {
  MapPosition target;
  bearing=FloatingPoint::degree2rad(bearing);
  double latInRad=getLatRad();
  double lngInRad=getLngRad();
  double angularDistance=distance/earthRadius;
  target.setLat(FloatingPoint::rad2degree(asin(sin(latInRad)*cos(angularDistance)+cos(latInRad)*sin(angularDistance)*cos(bearing))));
  target.setLng(FloatingPoint::rad2degree(lngInRad+atan2(sin(bearing)*sin(angularDistance)*cos(latInRad),cos(angularDistance)-sin(latInRad)*sin(latInRad))));
  return target;
}

// Computes the bearing in degrees clockwise from north (0°) to the given destination point
double MapPosition::computeBearing(MapPosition target) {
  double latDist = target.getLatRad()-getLatRad();
  double lngDist = target.getLngRad()-getLngRad();
  if ((latDist==0)&&(lngDist==0))
    return 0;
  double y = sin(lngDist)*cos(target.getLatRad());
  double x = cos(getLatRad())*sin(target.getLatRad()) -
             sin(getLatRad())*cos(target.getLatRad())*cos(lngDist);
  double bearing = FloatingPoint::rad2degree(atan2(y,x));
  bearing += 360;
  if (bearing >= 360)
    bearing -= 360;
  return bearing;
}

// Computes the distance in meters to the given destination point
double MapPosition::computeDistance(MapPosition target)
{
  double latDist = FloatingPoint::degree2rad(target.getLat() - getLat());
  double lngDist = FloatingPoint::degree2rad(target.getLng() - getLng());
  double t1 = sin(latDist / 2) * sin(latDist / 2) + cos(FloatingPoint::degree2rad(getLat())) * cos(FloatingPoint::degree2rad(target.getLat())) * sin(lngDist / 2) * sin(lngDist / 2);
  double t2 = 2 * atan2(sqrt(t1), sqrt(1 - t1));
  return earthRadius * t2;
}

// Store the contents of the object in a binary file
void MapPosition::store(std::ofstream *ofs)
{
  // Write the size of the object for detecting changes later
  Int size = sizeof (*this);
  Storage::storeInt(ofs, size);
  // Sanity checks
  if (mapTile != NULL) {
      FATAL("storing map positions with tile reference is not supported", NULL);
      return;
  }

  // Store all relevant fields
  MapTile *mapTile=this->mapTile;
  this->mapTile=NULL;
  Storage::storeMem(ofs,(char*)this,sizeof(MapPosition),true);
  this->mapTile=mapTile;
}

// Reads the contents of the object from a binary file
MapPosition *MapPosition::retrieve(char *& cacheData, Int & cacheSize)
{
  //PROFILE_START;
  // Check if the class has changed
  Int size = sizeof (MapPosition);
#ifdef TARGET_LINUX
  if (size!=176) {
    FATAL("unknown size of object (%d), please adapt class storage",size);
    return NULL;
  }
#endif

  // Read the size of the object and check with current size
  size=0;
  Storage::retrieveInt(cacheData,cacheSize,size);
  if (size!=sizeof(MapPosition)) {
    DEBUG("stored size of object does not match implemented object size, aborting retrieve",NULL);
    return NULL;
  }
  //PROFILE_ADD("sanity check");

  // Create a new map position object
  Storage::skipPadding(cacheData,cacheSize);
  cacheSize-=sizeof(MapPosition);
  if (cacheSize<0) {
    DEBUG("can not create map position object",NULL);
    return NULL;
  }
  MapPosition *mapPosition=new(cacheData) MapPosition(true);
  cacheData+=sizeof(MapPosition);
  //PROFILE_ADD("object creation");

  // Return result
  return mapPosition;
}

// Destructs the objects correctly (i.e., if memory has not been allocated by new)
void MapPosition::destruct(MapPosition *object) {
  if (object->doNotDelete) {
    object->~MapPosition();
  } else {
    delete object;
  }
}

// Converts WGS84 height to height above mean sea level
/*
 * Some code copied from geoid.c -- ECEF to WGS84 conversions, including ellipsoid-to-MSL height
 *
 * Geoid separation code by Oleg Gusev, from data by Peter Dana.
 * ECEF conversion by Rob Janssen.
 *
 * This file is Copyright (c) 2010 by the GPSD project
 * BSD terms apply: see the file COPYING in the distribution root for details.
 */
void MapPosition::toMSLHeight() {

  const Int GEOID_ROW=19;
  const Int GEOID_COL=37;
  const signed char geoid_delta[GEOID_COL*GEOID_ROW] = {
      /* 90S */ -30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30, -30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,
      /* 80S */ -53,-54,-55,-52,-48,-42,-38,-38,-29,-26,-26,-24,-23,-21,-19,-16,-12, -8, -4, -1,  1,  4,  4,  6,  5,  4,   2, -6,-15,-24,-33,-40,-48,-50,-53,-52,-53,
      /* 70S */ -61,-60,-61,-55,-49,-44,-38,-31,-25,-16, -6,  1,  4,  5,  4,  2,  6, 12, 16, 16, 17, 21, 20, 26, 26, 22,  16, 10, -1,-16,-29,-36,-46,-55,-54,-59,-61,
      /* 60S */ -45,-43,-37,-32,-30,-26,-23,-22,-16,-10, -2, 10, 20, 20, 21, 24, 22, 17, 16, 19, 25, 30, 35, 35, 33, 30,  27, 10, -2,-14,-23,-30,-33,-29,-35,-43,-45,
      /* 50S */ -15,-18,-18,-16,-17,-15,-10,-10, -8, -2,  6, 14, 13,  3,  3, 10, 20, 27, 25, 26, 34, 39, 45, 45, 38, 39,  28, 13, -1,-15,-22,-22,-18,-15,-14,-10,-15,
      /* 40S */  21,  6,  1, -7,-12,-12,-12,-10, -7, -1,  8, 23, 15, -2, -6,  6, 21, 24, 18, 26, 31, 33, 39, 41, 30, 24,  13, -2,-20,-32,-33,-27,-14, -2,  5, 20, 21,
      /* 30S */  46, 22,  5, -2, -8,-13,-10, -7, -4,  1,  9, 32, 16,  4, -8,  4, 12, 15, 22, 27, 34, 29, 14, 15, 15,  7,  -9,-25,-37,-39,-23,-14, 15, 33, 34, 45, 46,
      /* 20S */  51, 27, 10,  0, -9,-11, -5, -2, -3, -1,  9, 35, 20, -5, -6, -5,  0, 13, 17, 23, 21,  8, -9,-10,-11,-20, -40,-47,-45,-25,  5, 23, 45, 58, 57, 63, 51,
      /* 10S */  36, 22, 11,  6, -1, -8,-10, -8,-11, -9,  1, 32,  4,-18,-13, -9,  4, 14, 12, 13, -2,-14,-25,-32,-38,-60, -75,-63,-26,  0, 35, 52, 68, 76, 64, 52, 36,
      /* 00N */  22, 16, 17, 13,  1,-12,-23,-20,-14, -3, 14, 10,-15,-27,-18,  3, 12, 20, 18, 12,-13, -9,-28,-49,-62,-89,-102,-63, -9, 33, 58, 73, 74, 63, 50, 32, 22,
      /* 10N */  13, 12, 11,  2,-11,-28,-38,-29,-10,  3,  1,-11,-41,-42,-16,  3, 17, 33, 22, 23,  2, -3, -7,-36,-59,-90, -95,-63,-24, 12, 53, 60, 58, 46, 36, 26, 13,
      /* 20N */   5, 10,  7, -7,-23,-39,-47,-34, -9,-10,-20,-45,-48,-32, -9, 17, 25, 31, 31, 26, 15,  6,  1,-29,-44,-61, -67,-59,-36,-11, 21, 39, 49, 39, 22, 10,  5,
      /* 30N */  -7, -5, -8,-15,-28,-40,-42,-29,-22,-26,-32,-51,-40,-17, 17, 31, 34, 44, 36, 28, 29, 17, 12,-20,-15,-40, -33,-34,-34,-28,  7, 29, 43, 20,  4, -6, -7,
      /* 40N */ -12,-10,-13,-20,-31,-34,-21,-16,-26,-34,-33,-35,-26,  2, 33, 59, 52, 51, 52, 48, 35, 40, 33, -9,-28,-39, -48,-59,-50,-28,  3, 23, 37, 18, -1,-11,-12,
      /* 50N */  -8,  8,  8,  1,-11,-19,-16,-18,-22,-35,-40,-26,-12, 24, 45, 63, 62, 59, 47, 48, 42, 28, 12,-10,-19,-33, -43,-42,-43,-29, -2, 17, 23, 22,  6,  2, -8,
      /* 60N */   2,  9, 17, 10, 13,  1,-14,-30,-39,-46,-42,-21,  6, 29, 49, 65, 60, 57, 47, 41, 21, 18, 14,  7, -3,-22, -29,-32,-32,-26,-15, -2, 13, 17, 19,  6,  2,
      /* 70N */   2,  2,  1, -1, -3, -7,-14,-24,-27,-25,-19,  3, 24, 37, 47, 60, 61, 58, 51, 43, 29, 20, 12,  5, -2,-10, -14,-12,-10,-14,-12, -6, -2,  3,  6,  4,  2,
      /* 80N */   3,  1, -2, -3, -3, -3, -1,  3,  1,  5,  9, 11, 19, 27, 31, 34, 33, 34, 33, 34, 28, 23, 17, 13,  9,  4,   4,  1, -2, -2,  0,  2,  3,  2,  1,  1,  3,
      /* 90N */  13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,  13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13
  };
  Int ilat, ilon;
  Int ilat1, ilat2, ilon1, ilon2;
  double altitudeCorrection;

  // Height relative to WGS84 ellopsiod?
  if (isWGS84Altitude) {

    ilat = (Int)floor((90. + lat) / 10);
    ilon = (Int)floor((180. + lng) / 10);

    // sanity checks to prevent segfault on bad data
    if ((ilat > 90) || (ilat < -90)) {
      altitudeCorrection=0.0;
    } else {
      if ((ilon > 180) || (ilon < -180)) {
        altitudeCorrection=0.0;
      } else {

        // Compute the correction factor
        ilat1 = ilat;
        ilon1 = ilon;
        ilat2 = (ilat < GEOID_ROW - 1) ? ilat + 1 : ilat;
        ilon2 = (ilon < GEOID_COL - 1) ? ilon + 1 : ilon;
        altitudeCorrection=FloatingPoint::bilinear(ilon1 * 10. - 180., ilat1 * 10. - 90.,
                                                   ilon2 * 10. - 180., ilat2 * 10. - 90.,
                                                   lng, lat,
                                                   (double)geoid_delta[ilon1 + ilat1 * GEOID_COL],
                                                   (double)geoid_delta[ilon2 + ilat1 * GEOID_COL],
                                                   (double)geoid_delta[ilon1 + ilat2 * GEOID_COL],
                                                   (double)geoid_delta[ilon2 + ilat2 * GEOID_COL]);
      }
    }

    // Correct the altitude
    altitude-=altitudeCorrection;
    isWGS84Altitude=false;
    //DEBUG("WGS84 altitude=%f correction=%f MSL altitude=%f",altitude-altitudeCorrection,altitudeCorrection,altitude);

  }
}

// Get the tile x and y number if a mercator projection is used (as in OSM)
void MapPosition::computeMercatorTileXY(Int zoomLevel, Int &x, Int &y) {
  Int t=pow(2.0, zoomLevel);
  x=(Int)(floor((lng + 180.0) / 360.0 * t));
  y=(Int)(floor((1.0 - log( tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0) ) / M_PI) / 2.0 * t));
  if (x<0)
    x=0;
  if (x>t-1)
    x=t-1;
  if (y<0)
    y=0;
  if (y>t-1)
    y=t-1;
}

// Get the bounds of the tile this position lies in if a mercator project is used
void MapPosition::computeMercatorTileBounds(Int zoomLevel, double &latNorth, double &latSouth, double &lngWest, double &lngEast) {
  Int x,y;
  computeMercatorTileXY(zoomLevel,x,y);
  double unit = 1.0 / pow(2.0, zoomLevel);
  lngWest = x * unit * 360.0 - 180.0;
  lngEast = (x+1) * unit * 360.0 - 180.0;
  double n = M_PI - 2.0 * M_PI * y * unit;
  latNorth = 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
  n = M_PI - 2.0 * M_PI * (y+1) * unit;
  latSouth = 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
}

// Sets the longitude and latitude based on the given mercator x and y coordinates
void MapPosition::setFromMercatorTileXY(Int zoomLevel, Int x, Int y) {
  double unit = 1.0 / pow(2.0, zoomLevel);
  setLng((x+0.5) * unit * 360.0 - 180.0);
  double n = M_PI - 2.0 * M_PI * (y+0.5) * unit;
  setLat(180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n))));
}

// Checks if the location is valid
bool MapPosition::isValid() {
  if ((lng==-std::numeric_limits<double>::max())&&(lat==-std::numeric_limits<double>::max()))
    return false;
  else
    return true;
}

// Invalidates the position
void MapPosition::invalidate() {
  lng=-std::numeric_limits<double>::max();
  lat=-std::numeric_limits<double>::max();
}

// Compute the normal distance from the locationPos to the vector spanned from the prevPos and this pos
double MapPosition::computeNormalDistance(MapPosition prevPos, MapPosition locationPos, double overlapInMeters, bool insideOnly, bool debugMsgs, MapPosition *normalPos) {
  double distance = std::numeric_limits<double>::max();
  if (*this!=NavigationPath::getPathInterruptedPos()) {
    if (prevPos!=NavigationPath::getPathInterruptedPos()) {
      MapPosition pos=*this;
      /*if (overlapInMeters>0) {
        //DEBUG("before: prevPos=\"N%f E%f\" pos=\"N%f E%f\"",prevPos.getLat(),prevPos.getLng(),pos.getLat(),pos.getLng());
        double bearing=pos.computeBearing(prevPos);
        double distance=prevPos.computeDistance(pos);
        prevPos=pos.computeTarget(bearing,distance+overlapInMeters);
        pos=prevPos.computeTarget(bearing-180,distance+2*overlapInMeters);
        //DEBUG("after: prevPos=\"N%f E%f\" pos=\"N%f E%f\"",prevPos.getLat(),prevPos.getLng(),pos.getLat(),pos.getLng());
      }*/
      double a = prevPos.computeDistance(pos);
      double b = locationPos.computeDistance(prevPos);
      double c = locationPos.computeDistance(pos);
      if ((a!=0.0)&&(b!=0.0)&&(c!=0.0)) {
        double t = 2*a*c;
        t = (a*a+c*c-b*b)/t;
        double alpha;
        if (t>=1) 
          alpha=0;
        else
          alpha=acos(t);
        distance = sin(alpha)*c;
        t = 2*a*b;
        t = (a*a+b*b-c*c)/t;
        double beta;
        if (t>=1) 
          beta=0;
        else
          beta=acos(t);
        if (debugMsgs)
          DEBUG("a=%.2f b=%.2f c=%.2f alpha=%f beta=%f distance=%.2f",a,b,c,FloatingPoint::rad2degree(alpha),FloatingPoint::rad2degree(beta),distance);
        if (insideOnly) {

          // Check if the normal lies around the line
          if (fabs(alpha)>M_PI_2) {
            //DEBUG("alpha=%f",alpha);
            distance=std::numeric_limits<double>::max();
          }
          if (fabs(beta)>M_PI_2) {
            //DEBUG("beta=%f",beta);
            distance=std::numeric_limits<double>::max();
          }

          // In case an overlap is given and the normal is not inside the line
          // check if the end points are within the overlap
          bool normalPosSet=false;
          if (overlapInMeters>0) {
            if (distance==std::numeric_limits<double>::max()) {
              double t=prevPos.computeDistance(locationPos);
              distance=pos.computeDistance(locationPos);
              if (t<distance) {
                distance=t;
                if (normalPos) *normalPos=prevPos;              
              } else {
                if (normalPos) *normalPos=pos;
              }
              normalPosSet=true;
            }
            if (distance>overlapInMeters) 
              distance=std::numeric_limits<double>::max();
          }

          // Now compute the normal pos (only if not yet set from previous step)
          if ((distance!=std::numeric_limits<double>::max())&&(normalPos!=NULL)&&(!normalPosSet)) {
            double normalBearing, locationBearing;
            double bearingDiff=prevPos.computeBearing(pos)-prevPos.computeBearing(locationPos);
            //DEBUG("pathBearing=%f locationBearing=%f bearingDiff=%f",prevPos.computeBearing(pos),prevPos.computeBearing(locationPos),bearingDiff);
            if (bearingDiff>180) bearingDiff=bearingDiff-360;
            if (bearingDiff<-180) bearingDiff=bearingDiff+360;
            locationBearing=locationPos.computeBearing(pos);
            if (bearingDiff>=0)
              normalBearing=locationBearing+FloatingPoint::rad2degree(M_PI_2-alpha);
            else
              normalBearing=locationBearing-FloatingPoint::rad2degree(M_PI_2-alpha);
            //DEBUG("a=%.2f b=%.2f c=%.2f alpha=%f beta=%f distance=%.2f",a,b,c,FloatingPoint::rad2degree(alpha),FloatingPoint::rad2degree(beta),distance);
            //DEBUG("bearingDiff=%f bearingToPathPoint=%f alpha=%f beta=%f bearing=%f distance=%f",bearingDiff,locationBearing,FloatingPoint::rad2degree(alpha),FloatingPoint::rad2degree(beta),normalBearing,distance);
            *normalPos=locationPos.computeTarget(normalBearing,distance);
          }
        }        
      }
    }
  }
  return distance;
}

}
