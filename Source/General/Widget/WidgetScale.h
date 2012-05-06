//============================================================================
// Name        : WidgetScale.h
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


#ifndef WIDGETSCALE_H_
#define WIDGETSCALE_H_

namespace GEODISCOVERER {

class WidgetScale : public WidgetPrimitive {

protected:

  // Update interval
  TimestampInMicroseconds updateInterval;

  // Next timestamp when to update the widget
  TimestampInMicroseconds nextUpdateTime;

  // Current meters per tick of the scale icon
  Int metersPerTick;

  // Current map name
  std::string mapName;

  // Horizontal offset to use for the scale values
  Int tickLabelOffsetX;

  // Vertical offset to use for the map label
  Int mapLabelOffsetY;

  // Font string objects for drawing
  std::vector<FontString*> scaledNumberFontString;
  FontString *mapNameFontString;

public:

  // Constructor
  WidgetScale();

  // Destructor
  virtual ~WidgetScale();

  // Let the widget work
  virtual bool work(TimestampInMicroseconds t);

  // Called when the widget must be drawn
  virtual void draw(Screen *screen, TimestampInMicroseconds t);

  // Called when the widget has changed his position
  virtual void updatePosition(Int x, Int y, Int z);

  // Getters and setters
  void setUpdateInterval(TimestampInMicroseconds updateInterval) {
    this->updateInterval=updateInterval;
  }

  void setTickLabelOffsetX(Int tickLabelOffsetX)
  {
      this->tickLabelOffsetX = tickLabelOffsetX;
  }

  void setMapLabelOffsetY(Int mapLabelOffsetY)
  {
      this->mapLabelOffsetY = mapLabelOffsetY;
  }
};

}

#endif /* WIDGETSCALE_H_ */
