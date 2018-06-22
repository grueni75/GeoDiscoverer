//============================================================================
// Name        : NavigationPoint.h
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

  std::string getName() const {
    return name;
  }

  void setName(std::string name) {
    name.erase(std::remove(name.begin(), name.end(), '/'), name.end());
    this->name = core->getConfigStore()->encodeString(name);
  }

  TimestampInSeconds getTimestamp() const {
    return timestamp;
  }
};

} /* namespace GEODISCOVERER */

#endif /* NAVIGATIONPOINT_H_ */
