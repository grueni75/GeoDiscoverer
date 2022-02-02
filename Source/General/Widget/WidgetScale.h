//============================================================================
// Name        : WidgetScale.h
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

#ifndef WIDGETSCALE_H_
#define WIDGETSCALE_H_

namespace GEODISCOVERER {

class WidgetScale : public WidgetPrimitive {

protected:

  // Update interval
  TimestampInMicroseconds updateInterval;

  // Next timestamp when to update the widget
  TimestampInMicroseconds nextUpdateTime;

  // Vertical offset to use for the scale label
  Int topLabelOffsetY;

  // Vertical offset to use for the altitude label
  Int bottomLabelOffsetY;

  // Width of the background
  Int backgroundWidth;

  // Height of the background
  Int backgroundHeight;

  // Alpha color component of the background
  Int backgroundAlpha;

  // Font string objects for drawing
  FontString *topLabelFontString;
  FontString *bottomLabelLeftFontString;
  FontString *bottomLabelRightFontString;

  // Graphic indicating the altitude
  GraphicRectangle altitudeIcon;
  double altitudeIconScale;
  Int altitudeIconX;
  Int altitudeIconY;

public:

  // Constructor
  WidgetScale(WidgetContainer *widgetContainer);

  // Destructor
  virtual ~WidgetScale();

  // Let the widget work
  virtual bool work(TimestampInMicroseconds t);

  // Called when the widget must be drawn
  virtual void draw(TimestampInMicroseconds t);

  // Called when the widget has changed his position
  virtual void updatePosition(Int x, Int y, Int z);

  // Getters and setters
  void setUpdateInterval(TimestampInMicroseconds updateInterval) {
    this->updateInterval=updateInterval;
  }

  void setTopLabelOffsetY(Int topLabelOffsetY)
  {
      this->topLabelOffsetY = topLabelOffsetY;
  }

  void setBottomLabelOffsetY(Int bottomLabelOffsetY)
  {
      this->bottomLabelOffsetY = bottomLabelOffsetY;
  }

  void setBackgroundWidth(Int width)
  {
      this->backgroundWidth = width;
  }

  void setBackgroundHeight(Int height)
  {
      this->backgroundHeight = height;
  }

  void setBackgroundAlpha(Int alpha)
  {
      this->backgroundAlpha = alpha;
  }

  GraphicRectangle *getAltitudeIcon() {
    return &altitudeIcon;
  }
};

}

#endif /* WIDGETSCALE_H_ */
