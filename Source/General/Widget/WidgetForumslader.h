//============================================================================
// Name        : WidgetForumslader.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2021 Matthias Gruenewald
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

#ifndef WIDGETFORUMSLADER_H_
#define WIDGETFORUMSLADER_H_

#include <Core.h>

namespace GEODISCOVERER {

class WidgetForumslader: public WidgetPrimitive {

protected:

  // Font strings for important infos
  FontString *phBatteryLevelFontString;
  FontString *flBatteryLevelFontString;
  FontString *powerDrawLevelFontString;
  FontString *flBatteryLabelFontString;
  FontString *phBatteryLabelFontString;

  // Power draw visualization
  GraphicCircularStrip powerDrawGaugeBackground;
  GraphicCircularStrip powerDrawGaugeForeground;
  
  // Gauge colors
  GraphicColor gaugeBackgroundColor;
  GraphicColor gaugeForegroundColor;

  // Indicates that the widget must be updated
  bool updateRequired;

  // Indicates if ebike is connected
  bool connected;

  // Indicates if this is the first run
  bool firstRun;
  
  // Position of the power draw gauge
  Int powerDrawLevelOffsetY;
  Int powerDrawGaugeOffsetY;
  Int powerDrawGaugeRadius;
  Int powerDrawGaugeBackgroundWidth;
  Int powerDrawGaugeForegroundWidth;

  // Position of the battery gauges
  Int batteryLevelOffsetY;
  Int batteryGaugeOffsetX;
  Int batteryGaugeOffsetY;
  Int batteryGaugeFlCurrentHeight;
  Int batteryGaugePhCurrentHeight;
  Int batteryGaugeMaxHeight;
  Int batteryGaugeBackgroundWidth;
  Int batteryGaugeForegroundWidth;
  Int batteryGaugeTipWidth;
  Int batteryGaugeTipHeight;
  
  // Draws a battery symbol
  void drawBattery(TimestampInMicroseconds t, Int offsetX, Int batteryGaugeCurrentHeight, FontString *label, FontString *level);

public:

  // Constructor
  WidgetForumslader(WidgetContainer *widgetContainer);

  // Destructor
  virtual ~WidgetForumslader();

  // Let the button work
  virtual bool work(TimestampInMicroseconds t);

  // Called when the widget must be drawn
  virtual void draw(TimestampInMicroseconds t);

  // Called when the widget has changed his position
  virtual void updatePosition(Int x, Int y, Int z);

  // Called when some data has changed
  virtual void onDataChange();

  // Getters and setters
  void setGaugeBackgroundColor(GraphicColor gaugeBackgroundColor) {
    this->gaugeBackgroundColor = gaugeBackgroundColor;
  }

  void setGaugeForegroundColor(GraphicColor gaugeForegroundColor) {
    this->gaugeForegroundColor = gaugeForegroundColor;
  }

  void setPowerDrawLevelOffsetY(Int powerDrawLevelOffsetY) {
    this->powerDrawLevelOffsetY=powerDrawLevelOffsetY;
  }

  void setPowerDrawGaugeOffsetY(Int powerDrawGaugeOffsetY) {
    this->powerDrawGaugeOffsetY=powerDrawGaugeOffsetY;
  }

  void setPowerDrawGaugeRadius(Int powerDrawGaugeRadius) {
    this->powerDrawGaugeRadius=powerDrawGaugeRadius;
  }

  void setPowerDrawGaugeBackgroundWidth(Int powerDrawGaugeBackgroundWidth) {
    this->powerDrawGaugeBackgroundWidth=powerDrawGaugeBackgroundWidth;
  }

  void setPowerDrawGaugeForegroundWidth(Int powerDrawGaugeForegroundWidth) {
    this->powerDrawGaugeForegroundWidth=powerDrawGaugeForegroundWidth;
  }

  void setBatteryLevelOffsetY(Int batteryLevelOffsetY) {
    this->batteryLevelOffsetY=batteryLevelOffsetY;
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
};

}

#endif /* WIDGETFORUMSLADER_H_ */
