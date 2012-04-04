//============================================================================
// Name        : WidgetStatus.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
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
  WidgetStatus();
  virtual ~WidgetStatus();

  // Let the widget work
  virtual bool work(TimestampInMicroseconds t);

  // Called when the widget must be drawn
  virtual void draw(Screen *screen, TimestampInMicroseconds t);

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
