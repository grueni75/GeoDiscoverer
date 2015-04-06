//============================================================================
// Name        : WidgetStatus.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#ifndef WIDGETSTATUS_H_
#define WIDGETSTATUS_H_

namespace GEODISCOVERER {

class WidgetStatus: public GEODISCOVERER::WidgetPrimitive {

  // Update interval
  TimestampInMicroseconds updateInterval;

  // Width of the text label
  Int labelWidth;

  // Next timestamp when to update the widget
  TimestampInMicroseconds nextUpdateTime;

  // Graphical representation of the status string
  FontString *firstStatusFontString;
  FontString *secondStatusFontString;

public:

  // Constructors and destructor
  WidgetStatus(WidgetPage *widgetPage);
  virtual ~WidgetStatus();

  // Let the widget work
  virtual bool work(TimestampInMicroseconds t);

  // Called when the widget must be drawn
  virtual void draw(TimestampInMicroseconds t);

  // Called when the widget has changed his position
  virtual void updatePosition(Int x, Int y, Int z);

  // Getters and setters
  void setUpdateInterval(TimestampInMicroseconds updateInterval) {
    this->updateInterval = updateInterval;
  }

  void setLabelWidth(Int labelWidth) {
    this->labelWidth = labelWidth;
  }
};

} /* namespace GEODISCOVERER */
#endif /* WIDGETSTATUS_H_ */
