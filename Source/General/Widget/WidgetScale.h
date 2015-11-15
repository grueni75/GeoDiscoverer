//============================================================================
// Name        : WidgetScale.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
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

  // Current layer name
  std::string layerName;

  // Horizontal offset to use for the scale values
  Int tickLabelOffsetX;

  // Vertical offset to use for the map label
  Int mapLabelOffsetY;

  // Vertical offset to use for the layer label
  Int layerLabelOffsetY;

  // Font string objects for drawing
  std::vector<FontString*> scaledNumberFontString;
  FontString *mapNameFontString;
  FontString *layerNameFontString;

public:

  // Constructor
  WidgetScale(WidgetPage *widgetPage);

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

  void setTickLabelOffsetX(Int tickLabelOffsetX)
  {
      this->tickLabelOffsetX = tickLabelOffsetX;
  }

  void setMapLabelOffsetY(Int mapLabelOffsetY)
  {
      this->mapLabelOffsetY = mapLabelOffsetY;
  }

  void setLayerLabelOffsetY(Int layerLabelOffsetY) {
    this->layerLabelOffsetY = layerLabelOffsetY;
  }
};

}

#endif /* WIDGETSCALE_H_ */
