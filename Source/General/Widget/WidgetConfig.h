//============================================================================
// Name        : WidgetConfig.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

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
};

} /* namespace GEODISCOVERER */

#endif /* WIDGETCONFIG_H_ */
