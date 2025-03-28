//============================================================================
// Name        : WidgetMeter.h
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
#include <FontString.h>

#ifndef WIDGETMETER_H_
#define WIDGETMETER_H_

namespace GEODISCOVERER {

typedef enum { WidgetMeterTypeAltitude, WidgetMeterTypeSpeed, WidgetMeterTypeTrackLength } WidgetMeterType;

class WidgetMeter: public WidgetPrimitive {

protected:

  // Type of info to display
  WidgetMeterType meterType;

  // Label
  FontString *labelFontString;

  // Currently displayed value
  FontString *valueFontString;

  // Currently displayed unit
  FontString *unitFontString;

  // Update interval
  TimestampInMicroseconds updateInterval;

  // Next timestamp when to update the widget
  TimestampInMicroseconds nextUpdateTime;

  // For debugging
  TimestampInMicroseconds lastWorkTime;

  // Vertical position of the meter label
  Int labelY;

  // Vertical position of the meter value
  Int valueY;

  // Vertical position of the meter unit
  Int unitY;

public:

  // Constructor
  WidgetMeter(WidgetContainer *widgetContainer);

  // Destructor
  virtual ~WidgetMeter();

  // Let the button work
  virtual bool work(TimestampInMicroseconds t);

  // Called when the widget must be drawn
  virtual void draw(TimestampInMicroseconds t);

  // Called when the widget has changed his position
  virtual void updatePosition(Int x, Int y, Int z);

  // Getters and setters
  void setMeterType(WidgetMeterType meterType);

  void setUpdateInterval(TimestampInMicroseconds updateInterval) {
    this->updateInterval=updateInterval;
  }

  void setValueY(Int valueY) {
    this->valueY=valueY;
  }

  void setLabelY(Int labelY) {
    this->labelY=labelY;
  }

  void setUnitY(Int unitY) {
    this->unitY=unitY;
  }

};

}

#endif /* WIDGETMETER_H_ */
