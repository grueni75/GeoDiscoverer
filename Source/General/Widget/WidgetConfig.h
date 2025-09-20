//============================================================================
// Name        : WidgetConfig.h
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

#include <WidgetPosition.h>
#include <WidgetPrimitive.h>

#ifndef WIDGETCONFIG_H_
#define WIDGETCONFIG_H_

namespace GEODISCOVERER {

typedef std::map<std::string, std::string> ParameterMap;
typedef std::pair<std::string, std::string> ParameterPair;

class WidgetConfig {

  // Name of the widget
  std::string name;

  // Type of widget
  WidgetType type;

  // Page name this widget belongs to
  std::string pageName;

  // List of positions for the different screen diagonals
  std::list<WidgetPosition> positions;

  // Active color
  GraphicColor activeColor;

  // Inactive color
  GraphicColor inactiveColor;

  // Busy color
  GraphicColor busyColor;

  // Safety color
  GraphicColor safetyColor;
  bool hasSafetyColor;

  // Heart rate zone colors
  GraphicColor heartRateZoneColorBackground;
  GraphicColor heartRateZoneColorZoneOne;
  GraphicColor heartRateZoneColorZoneTwo;
  GraphicColor heartRateZoneColorZoneThree;
  GraphicColor heartRateZoneColorZoneFour;
  GraphicColor heartRateZoneColorZoneFive;

  // Additional parameters
  ParameterMap parameters;

public:

  // Constructor
  WidgetConfig();

  // Destructor
  virtual ~WidgetConfig();

  const GraphicColor& getActiveColor() const {
    return activeColor;
  }

  void setActiveColor(const GraphicColor& activeColor) {
    this->activeColor = activeColor;
  }

  const GraphicColor& getInactiveColor() const {
    return inactiveColor;
  }

  void setInactiveColor(const GraphicColor& inactiveColor) {
    this->inactiveColor = inactiveColor;
  }

  const GraphicColor& getSafetyColor() const {
    return safetyColor;
  }

  void setSafetyColor(const GraphicColor& safetyColor) {
    this->safetyColor = safetyColor;
    hasSafetyColor = true;
  }

  bool getHasSafetyColor() {
    return hasSafetyColor;
  }

  const GraphicColor& getBusyColor() const {
    return busyColor;
  }

  void setBusyColor(const GraphicColor& busyColor) {
    this->busyColor = busyColor;
  }

  const std::string& getName() const {
    return name;
  }

  void setName(const std::string& name) {
    this->name = name;
  }

  const std::string& getPageName() const {
    return pageName;
  }

  void setPageName(const std::string& pageName) {
    this->pageName = pageName;
  }

  ParameterMap *getParameters() {
    return &parameters;
  }

  void setParameter(std::string name, std::string value) {
    parameters[name]=value;
  }

  std::list<WidgetPosition> *getPositions() {
    return &positions;
  }

  void addPosition(const WidgetPosition position) {
    this->positions.push_back(position);
  }

  void clearPositions() {
    this->positions.clear();
  }

  WidgetType getType() const {
    return type;
  }

  void setType(WidgetType type) {
    this->type = type;
  }

  const GraphicColor& getHeartRateZoneColorBackground() const {
    return heartRateZoneColorBackground;
  }

  void setHeartRateZoneColorBackground(const GraphicColor& color) {
    this->heartRateZoneColorBackground = color;
  }

  const GraphicColor& getHeartRateZoneColorZoneOne() const {
    return heartRateZoneColorZoneOne;
  }

  void setHeartRateZoneColorZoneOne(const GraphicColor& color) {
    this->heartRateZoneColorZoneOne = color;
  }

  const GraphicColor& getHeartRateZoneColorZoneTwo() const {
    return heartRateZoneColorZoneTwo;
  }

  void setHeartRateZoneColorZoneTwo(const GraphicColor& color) {
    this->heartRateZoneColorZoneTwo = color;
  }

  const GraphicColor& getHeartRateZoneColorZoneThree() const {
    return heartRateZoneColorZoneThree;
  }

  void setHeartRateZoneColorZoneThree(const GraphicColor& color) {
    this->heartRateZoneColorZoneThree = color;
  }

  const GraphicColor& getHeartRateZoneColorZoneFour() const {
    return heartRateZoneColorZoneFour;
  }

  void setHeartRateZoneColorZoneFour(const GraphicColor& color) {
    this->heartRateZoneColorZoneFour = color;
  }

  const GraphicColor& getHeartRateZoneColorZoneFive() const {
    return heartRateZoneColorZoneFive;
  }

  void setHeartRateZoneColorZoneFive(const GraphicColor& color) {
    this->heartRateZoneColorZoneFive = color;
  }
};

} /* namespace GEODISCOVERER */

#endif /* WIDGETCONFIG_H_ */
