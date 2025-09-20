//============================================================================
// Name        : WidgetHeartRate.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2025 Matthias Gruenewald
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

//#ifndef WIDGETHEARTRATE_H_
#ifndef WIDGETHEARTRATE_H_
#define WIDGETHEARTRATE_H_

#include <Core.h>

namespace GEODISCOVERER {

class WidgetHeartRate: public WidgetPrimitive {

protected:

  // Current heart rate
  FontString *heartRateFontString;
  
  // Alpha color component of the background
  Int backgroundAlpha;

  // Indicates that the widget must be updated
  bool updateRequired;
  
  // Indicates if ebike is connected
  bool connected;
  
  // Indicates if this is the first run
  bool firstRun;
  
  // Position of the heart rate label
  Int heartRateOffsetX;
  Int heartRateOffsetY;

  // Current battery level
  Int batteryLevel;
  
  // Variables for the battery gauge
  Int batteryGaugeOffsetX;
  Int batteryGaugeOffsetY;
  Int batteryGaugeMaxHeight;
  Int batteryGaugeBackgroundWidth;
  Int batteryGaugeForegroundWidth;
  Int batteryGaugeTipWidth;
  Int batteryGaugeTipHeight;

  // Variables for the zone indicator
  GraphicColor heartRateZoneColorBackground;
  GraphicColor heartRateZoneColorZoneOne;
  GraphicColor heartRateZoneColorZoneTwo;
  GraphicColor heartRateZoneColorZoneThree;
  GraphicColor heartRateZoneColorZoneFour;
  GraphicColor heartRateZoneColorZoneFive;
  Int heartRateZoneWidth;
  Int heartRateZoneHeight;
  Int heartRateZoneOffsetX;
  Int heartRateZoneGapX;
  Int heartRateZone;
  
public:

  // Constructor
  WidgetHeartRate(WidgetContainer *widgetContainer);

  // Destructor
  virtual ~WidgetHeartRate();

  // Let the button work
  virtual bool work(TimestampInMicroseconds t);

  // Called when the widget must be drawn
  virtual void draw(TimestampInMicroseconds t);

  // Called when the widget has changed his position
  virtual void updatePosition(Int x, Int y, Int z);

  // Called when some data has changed
  virtual void onDataChange();

  // Getters and setters
  void setHeartRateOffsetX(Int x) {
    this->heartRateOffsetX = x;
  }

  void setHeartRateOffsetY(Int y) {
    this->heartRateOffsetY = y;
  }

  void setBackgroundAlpha(Int alpha)
  {
      this->backgroundAlpha = alpha;
  }

  void setBatteryGaugeOffsetX(Int batteryGaugeOffsetX) {
    this->batteryGaugeOffsetX=batteryGaugeOffsetX;
  }

  void setBatteryGaugeOffsetY(Int batteryGaugeOffsetY) {
    this->batteryGaugeOffsetY=batteryGaugeOffsetY;
  }

  void setBatteryGaugeMaxHeight(Int batteryGaugeMaxHeight) {
    this->batteryGaugeMaxHeight=batteryGaugeMaxHeight;
  }

  void setBatteryGaugeBackgroundWidth(Int batteryGaugeBackgroundWidth) {
    this->batteryGaugeBackgroundWidth=batteryGaugeBackgroundWidth;
  }

  void setBatteryGaugeForegroundWidth(Int batteryGaugeForegroundWidth) {
    this->batteryGaugeForegroundWidth=batteryGaugeForegroundWidth;
  }

  void setBatteryGaugeTipWidth(Int batteryGaugeTipWidth) {
    this->batteryGaugeTipWidth=batteryGaugeTipWidth;
  }

  void setBatteryGaugeTipHeight(Int batteryGaugeTipHeight) {
    this->batteryGaugeTipHeight=batteryGaugeTipHeight;
  }

  void setHeartRateZoneColorBackground(const GraphicColor& heartRateZoneColorBackground) {
    this->heartRateZoneColorBackground = heartRateZoneColorBackground;
  }

  void setHeartRateZoneColorZoneOne(const GraphicColor& heartRateZoneColorZoneOne) {
    this->heartRateZoneColorZoneOne = heartRateZoneColorZoneOne;
  }

  void setHeartRateZoneColorZoneTwo(const GraphicColor& heartRateZoneColorZoneTwo) {
    this->heartRateZoneColorZoneTwo = heartRateZoneColorZoneTwo;
  }

  void setHeartRateZoneColorZoneThree(const GraphicColor& heartRateZoneColorZoneThree) {
    this->heartRateZoneColorZoneThree = heartRateZoneColorZoneThree;
  }

  void setHeartRateZoneColorZoneFour(const GraphicColor& heartRateZoneColorZoneFour) {
    this->heartRateZoneColorZoneFour = heartRateZoneColorZoneFour;
  }

  void setHeartRateZoneColorZoneFive(const GraphicColor& heartRateZoneColorZoneFive) {
    this->heartRateZoneColorZoneFive = heartRateZoneColorZoneFive;
  }

  void setHeartRateZoneWidth(Int width) {
    this->heartRateZoneWidth = width;
  }

  void setHeartRateZoneHeight(Int height) {
    this->heartRateZoneHeight = height;
  }

  void setHeartRateZoneOffsetX(Int offsetX) {
    this->heartRateZoneOffsetX = offsetX;
  }

  void setHeartRateZoneGapX(Int gapX) {
    this->heartRateZoneGapX = gapX;
  }
};

}

#endif /* WIDGETHEARTRATE_H_ */
