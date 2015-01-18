//============================================================================
// Name        : WidgetMeter.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef WIDGETINFO_H_
#define WIDGETINFO_H_

#include <Core.h>

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
  WidgetMeter(WidgetPage *widgetPage);

  // Destructor
  virtual ~WidgetMeter();

  // Let the button work
  virtual bool work(TimestampInMicroseconds t);

  // Called when the widget must be drawn
  virtual void draw(Screen *screen, TimestampInMicroseconds t);

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

#endif /* WIDGETINFO_H_ */
