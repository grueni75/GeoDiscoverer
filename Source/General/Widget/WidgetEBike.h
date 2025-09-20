//============================================================================
// Name        : WidgetEBike.h
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

#include <WidgetPrimitive.h>
#include <GraphicCircularStrip.h>
#include <FontString.h>

#ifndef WIDGETEBIKE_H_
#define WIDGETEBIKE_H_

#include <Core.h>

namespace GEODISCOVERER {

class WidgetEBike: public WidgetPrimitive {

protected:

  // Set power level 
  FontString *powerLevelFontString;

  // Temperature within the engine
  FontString *engineTemperatureFontString;

  // Distance travelled with engine switched on
  FontString *distanceElectricFontString;

  // Battery percentage 
  GraphicCircularStrip batteryLevelBackground;
  GraphicCircularStrip batteryLevelForeground;
  
  // Indicates that the widget must be updated
  bool updateRequired;
  
  // Indicates if ebike is connected
  bool connected;
  
  // Indicates if this is the first run
  bool firstRun;
  
  // Position of the power level
  Int powerLevelOffsetX;
  Int powerLevelOffsetY;

  // Position of the engine temperature
  Int engineTemperatureOffsetX;
  Int engineTemperatureOffsetY;
  Int engineTemperatureBackgroundRadius;
  Int engineTemperatureForegroundRadius;
  Int engineTemperatureMaxHeight;
  Int engineTemperatureCurrentHeight;
  Int engineTemperatureBackgroundWidth;
  Int engineTemperatureForegroundWidth;

  // Position of the eletric distance
  Int distanceElectricOffsetX;
  Int distanceElectricOffsetY;
  
  // Position of the battery level gauge
  Int batteryLevelRadius;
  Int batteryLevelBackgroundWidth;
  Int batteryLevelForegroundWidth;
  
public:

  // Constructor
  WidgetEBike(WidgetContainer *widgetContainer);

  // Destructor
  virtual ~WidgetEBike();

  // Let the button work
  virtual bool work(TimestampInMicroseconds t);

  // Called when the widget must be drawn
  virtual void draw(TimestampInMicroseconds t);

  // Called when the widget has changed his position
  virtual void updatePosition(Int x, Int y, Int z);

  // Called when some data has changed
  virtual void onDataChange();

  // Getters and setters
  void setPowerLevelOffsetX(Int x) {
    this->powerLevelOffsetX=x;
  }

  void setPowerLevelOffsetY(Int y) {
    this->powerLevelOffsetY=y;
  }

  void setEngineTemperatureOffsetX(Int x) {
    this->engineTemperatureOffsetX=x;
  }

  void setEngineTemperatureOffsetY(Int y) {
    this->engineTemperatureOffsetY=y;
  }

  void setEngineTemperatureBackgroundRadius(Int radius) {
    this->engineTemperatureBackgroundRadius=radius;
  }

  Int getEngineTemperatureBackgroundRadius() {
    return engineTemperatureBackgroundRadius;
  }

  void setEngineTemperatureForegroundRadius(Int radius) {
    this->engineTemperatureForegroundRadius=radius;
  }

  void setEngineTemperatureBackgroundWidth(Int width) {
    this->engineTemperatureBackgroundWidth=width;
  }

  Int getEngineTemperatureBackgroundWidth() {
    return engineTemperatureBackgroundWidth;
  }

  void setEngineTemperatureForegroundWidth(Int width) {
    this->engineTemperatureForegroundWidth=width;
  }

  void setEngineTemperatureMaxHeight(Int height) {
    this->engineTemperatureMaxHeight=height;
  }

  void setDistanceElectricOffsetX(Int x) {
    this->distanceElectricOffsetX=x;
  }

  void setDistanceElectricOffsetY(Int y) {
    this->distanceElectricOffsetY=y;
  }

  void setBatteryLevelRadius(Int radius) {
    this->batteryLevelRadius=radius;
  }

  void setBatteryLevelBackgroundWidth(Int width) {
    this->batteryLevelBackgroundWidth=width;
  }

  Int getBatteryLevelBackgroundWidth() {
    return batteryLevelBackgroundWidth;
  }

  void setBatteryLevelForegroundWidth(Int width) {
    this->batteryLevelForegroundWidth=width;
  }

  virtual void setIsHidden(bool isHidden) {
    this->isHidden = isHidden;
  }

};

}

#endif /* WIDGETEBIKE_H_ */
