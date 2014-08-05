//============================================================================
// Name        : MapPosition.h
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


#ifndef MAPPOSITION_H_
#define MAPPOSITION_H_

namespace GEODISCOVERER {

// Namespaces used when reading gpx files
extern const char *GPX10Namespace;
extern const char *GPX11Namespace;
extern const char *GDNamespace;

class MapPosition {

protected:

  bool doNotDelete;                                 // Indicates if the object has been alloacted by an own memory handler
  static const char *unknownSource;                 // String indicating that the source is unknown
  static const double earthRadius = 6.371 * 1e6;    // Radius of earh in meter
  MapTile *mapTile;                                 // Tile this position belongs to
  Int x;                                            // X position in picture coordinate system
  Int y;                                            // Y position in picture coordinate system
  double cartesianX;                                // X position in cartesian coordinate system of selected map projection
  double cartesianY;                                // Y position in cartesian coordinate system of selected map projection
  double lng;                                       // WGS84 Longitude in degrees
  double lat;                                       // WGS84 Latitude in degrees
  bool hasAltitude;                                 // Indicates that a height value is included
  double altitude;                                  // Height in meters
  bool isWGS84Altitude;                             // True: Height is expressed relative to WGS84 reference ellipsoid, False: Height is expressed relative to mean sea level
  bool hasBearing;                                  // Indicates that a bearing value is included
  double bearing;                                   // Direction of travel in degrees east of true north
  bool hasSpeed;                                    // Indicates that a speed value is included
  double speed;                                     // Speed of travel in meters/second
  bool hasAccuracy;                                 // Indicates that an accuracy value is included
  double accuracy;                                  // Accuracy in meters (radius of a circle around lng and lat)
  double distance;                                  // Distance to a reference point
  bool hasTimestamp;                                // Indicates that an timestamp value is included
  TimestampInMilliseconds timestamp;                // Time in UTC notation when this position was generated
  double latScale;                                  // Scale factor for latitude
  double lngScale;                                  // Scale factor for longitude
  char *source;                                     // Source of this position
  bool isUpdated;                                   // Indicates that the position has been changed
  Int index;                                        // Index of this point within the vector it is part of

  // Returns the text contents of a element node
  bool getText(XMLNode node, std::string &contents);

  // Checks if the namespace of the given xml node is a valid gpx namespace
  bool isGPXNameSpace(XMLNode node);

  // Checks if the namespace of the given xml node is a valid geo discoverer namespace
  bool isGDNameSpace(XMLNode node);

public:

  // Constructors and destructor
  MapPosition(bool doNotDelete=false);
  MapPosition(const MapPosition &pos);
  virtual ~MapPosition();

  // Destructs the objects correctly (i.e., if memory has not been allocated by new)
  static void destruct(MapPosition *object);

  // Operators
  bool operator ==(const MapPosition &rhs);
  bool operator !=(const MapPosition &rhs);
  MapPosition &operator=(const MapPosition &rhs);

  // Computes the destination point from the given bearing and distance
  MapPosition computeTarget(double bearing, double distance);

  // Computes the bearing in degrees clockwise from north (0°) to the given destination point
  double computeBearing(MapPosition target);

  // Computes the distance in meters to the given destination point
  double computeDistance(MapPosition target);

  // Adds the point to the gpx xml tree
  void writeGPX(XMLNode parentNode);

  // Extracts the point from the gpx xml node list
  bool readGPX(XMLNode wptNode, std::string &error);

  // Compares two positions according to their geographic distance
  static bool distanceSortPredicate(const MapPosition *lhs, const MapPosition *rhs)
  {
    return lhs->getDistance() < rhs->getDistance();
  }

  // Store the contents of the object in a binary file
  void store(std::ofstream *ofs);

  // Reads the contents of the object from a binary file
  static MapPosition *retrieve(char *&cacheData, Int &cacheSize);

  // Converts WGS84 height to height above mean sea level
  void toMSLHeight();

  // Get the tile x and y number if a mercator projection is used (as in OSM)
  void computeMercatorTileXY(Int zoomLevel, Int &x, Int &y);

  // Get the bounds of the tile this position lies in if a mercator project is used
  void computeMercatorTileBounds(Int zoomLevel, double &latNorth, double &latSouth, double &lngWest, double &lngEast);

  // Checks if the location is valid
  bool isValid();

  // Invalidates the position
  bool invalidate();

  // Getters and setters
  bool getIsUpdated() const {
    return isUpdated;
  }

  void setIsUpdated(bool isUpdated) {
    this->isUpdated = isUpdated;
  }

  double getDistance() const
  {
      return distance;
  }

  double getLat() const
  {
      return lat;
  }

  double getLatRad() const
  {
      return FloatingPoint::degree2rad(lat);
  }

  double getLng() const
  {
      return lng;
  }

  double getLngRad() const
  {
      return FloatingPoint::degree2rad(lng);
  }

  MapTile *getMapTile() const
  {
      return mapTile;
  }

  Int getX() const
  {
      return x;
  }

  Int getY() const
  {
      return y;
  }

  void setLat(double lat)
  {
      this->lat = lat;
  }

  void setLng(double lng)
  {
      this->lng = lng;
  }

  void setMapTile(MapTile *mapTile)
  {
      this->mapTile = mapTile;
  }

  void setX(Int x)
  {
      this->x = x;
  }

  void setY(Int y)
  {
      this->y = y;
  }

  double getAltitude() const
  {
      return altitude;
  }

  void setAltitude(double altitude)
  {
      this->altitude = altitude;
  }

  void setDistance(double distance)
  {
      this->distance = distance;
  }

  double getLatScale() const
  {
      return latScale;
  }

  double getLngScale() const
  {
      return lngScale;
  }

  void setLatScale(double latScale)
  {
      this->latScale = latScale;
  }

  void setLngScale(double lngScale)
  {
      this->lngScale = lngScale;
  }

  double getAccuracy() const
  {
      return accuracy;
  }

  double getBearing() const
  {
      return bearing;
  }

  double getSpeed() const
  {
      return speed;
  }

  TimestampInMilliseconds getTimestamp() const
  {
      return timestamp;
  }

  void setAccuracy(double accuracy)
  {
      this->accuracy = accuracy;
  }

  void setBearing(double bearing)
  {
      this->bearing = bearing;
  }

  void setSpeed(double speed)
  {
      this->speed = speed;
  }

  void setTimestamp(TimestampInMilliseconds timestamp)
  {
      this->timestamp = timestamp;
  }

  std::string getSource() const
  {
      return source;
  }

  void setSource(std::string source)
  {
    if (doNotDelete) {
      FATAL("can not set new source because memory is statically allocated",NULL);
    } else {
      if ((this->source)&&(this->source!=unknownSource)) free(this->source);
      if (!(this->source=strdup(source.c_str()))) {
        FATAL("can not create string",NULL);
      }
    }
  }

  bool getHasAccuracy() const
  {
      return hasAccuracy;
  }

  bool getHasAltitude() const
  {
      return hasAltitude;
  }

  bool getHasBearing() const
  {
      return hasBearing;
  }

  bool getHasTimestamp() const
  {
      return hasTimestamp;
  }

  bool getHasSpeed() const
  {
      return hasSpeed;
  }

  void setHasAccuracy(bool hasAccuracy)
  {
      this->hasAccuracy = hasAccuracy;
  }

  void setHasAltitude(bool hasAltitude)
  {
      this->hasAltitude = hasAltitude;
  }

  void setHasBearing(bool hasBearing)
  {
      this->hasBearing = hasBearing;
  }

  void setHasSpeed(bool hasSpeed)
  {
      this->hasSpeed = hasSpeed;
  }

  bool getIsWGS84Altitude() const
  {
      return isWGS84Altitude;
  }

  void setIsWGS84Altitude(bool isWGS84Altitude)
  {
      this->isWGS84Altitude = isWGS84Altitude;
  }

  double getCartesianX() const {
    return cartesianX;
  }

  void setCartesianX(double cartesianX) {
    this->cartesianX = cartesianX;
  }

  double getCartesianY() const {
    return cartesianY;
  }

  void setCartesianY(double cartesianY) {
    this->cartesianY = cartesianY;
  }

  Int getIndex() const {
    return index;
  }

  void setIndex(Int index) {
    this->index = index;
  }
};

}

#endif /* MAPPOSITION_H_ */
