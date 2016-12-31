//============================================================================
// Name        : NavigationInfo.h
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

#ifndef NAVIGATIONINFO_H_
#define NAVIGATIONINFO_H_

namespace GEODISCOVERER {

typedef enum {NavigationInfoTypeUnknown, NavigationInfoTypeRoute, NavigationInfoTypeTarget} NavigationInfoType;

class NavigationInfo {

protected:

  // Indicates if navigating along a route or to a target
  NavigationInfoType type;

  // Undefined distance
  const static double unknownDistance;

  // Undefined duration
  const static double unknownDuration;

  // Undefined angle
  const static double unknownAngle;

  // Undefined speed
  const static double unknownSpeed;

  // Direction of current movement
  double locationBearing;

  // Speed of current movement
  double locationSpeed;

  // Bearing towards the given target
  double targetBearing;

  // Remaining distance to the target
  double targetDistance;

  // Remaining travel time to the target
  double targetDuration;

  // Remaining distance to next turn
  double turnDistance;

  // Remaining distance to route
  double routeDistance;

  // Angle by which the turn will change the direction
  double turnAngle;

  // Indicates if location is off route
  bool offRoute;

  // Length of current track
  double trackLength;

  // Current altitude
  double altitude;

public:

  // Constructor
  NavigationInfo();

  // Destructor
  virtual ~NavigationInfo();

  // Operators
  bool operator ==(const NavigationInfo &rhs);
  bool operator !=(const NavigationInfo &rhs);

  // Getters and setters
  double getLocationBearing() const {
    return locationBearing;
  }

  void setLocationBearing(double locationBearing) {
    this->locationBearing = locationBearing;
  }

  double getTargetBearing() const {
    return targetBearing;
  }

  void setTargetBearing(double targetBearing) {
    this->targetBearing = targetBearing;
  }

  double getTargetDistance() const {
    return targetDistance;
  }

  void setTargetDistance(double targetDistance) {
    this->targetDistance = targetDistance;
  }

  double getTargetDuration() const {
    return targetDuration;
  }

  void setTargetDuration(double targetDuration) {
    this->targetDuration = targetDuration;
  }

  double getTurnAngle() const {
    return turnAngle;
  }

  void setTurnAngle(double turnAngle) {
    this->turnAngle = turnAngle;
  }

  double getTurnDistance() const {
    return turnDistance;
  }

  void setTurnDistance(double turnDistance) {
    this->turnDistance = turnDistance;
  }

  static const double getUnknownAngle() {
    return unknownAngle;
  }

  static const double getUnknownDistance() {
    return unknownDistance;
  }

  static const double getUnknownDuration() {
    return unknownDuration;
  }

  static const double getUnknownSpeed() {
    return unknownSpeed;
  }

  bool getOffRoute() const {
    return offRoute;
  }

  void setOffRoute(bool offRoute) {
    this->offRoute = offRoute;
  }

  NavigationInfoType getType() const {
    return type;
  }

  void setType(NavigationInfoType type) {
    this->type = type;
  }

  double getRouteDistance() const {
    return routeDistance;
  }

  void setRouteDistance(double routeDistance) {
    this->routeDistance = routeDistance;
  }

  double getLocationSpeed() const {
    return locationSpeed;
  }

  void setLocationSpeed(double locationSpeed) {
    this->locationSpeed = locationSpeed;
  }

  double getAltitude() const {
    return altitude;
  }

  void setAltitude(double altitude) {
    this->altitude = altitude;
  }

  double getTrackLength() const {
    return trackLength;
  }

  void setTrackLength(double trackLength) {
    this->trackLength = trackLength;
  }
};

} /* namespace GEODISCOVERER */
#endif /* NAVIGATIONINFO_H_ */
