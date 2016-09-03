//============================================================================
// Name        : NavigationPoint.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#ifndef NAVIGATIONPOINT_H_
#define NAVIGATIONPOINT_H_

namespace GEODISCOVERER {

class NavigationPoint {

  // Representative name
  std::string name;

  // Full address
  std::string address;

  // Longitude in degress
  double lng;

  // Latitude in degrees
  double lat;

  // Last update
  TimestampInSeconds timestamp;

  // Key of the graphic primitive that represents this point
  GraphicPrimitiveKey graphicPrimitiveKey;

public:

  // Constructor
  NavigationPoint();

  // Destructor
  virtual ~NavigationPoint();

  // Writes the point to the config
  void writeToConfig(std::string path, TimestampInSeconds timestamp = 0);

  // Reads the point from the config
  bool readFromConfig(std::string path);

  // Setters and getters
  const std::string& getAddress() const {
    return address;
  }

  void setAddress(const std::string& address) {
    this->address = address;
  }

  double getLat() const {
    return lat;
  }

  void setLat(double lat) {
    this->lat = lat;
  }

  double getLng() const {
    return lng;
  }

  void setLng(double lng) {
    this->lng = lng;
  }

  const std::string& getName() const {
    return name;
  }

  void setName(const std::string& name) {
    this->name = name;
  }

  TimestampInSeconds getTimestamp() const {
    return timestamp;
  }

  GraphicPrimitiveKey getGraphicPrimitiveKey() const {
    return graphicPrimitiveKey;
  }

  void setGraphicPrimitiveKey(GraphicPrimitiveKey graphicPrimitiveKey) {
    this->graphicPrimitiveKey = graphicPrimitiveKey;
  }
};

} /* namespace GEODISCOVERER */

#endif /* NAVIGATIONPOINT_H_ */
