//============================================================================
// Name        : WidgetCursorInfo.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#ifndef WIDGETCURSORINFO_H_
#define WIDGETCURSORINFO_H_

namespace GEODISCOVERER {

class WidgetCursorInfo: public GEODISCOVERER::WidgetPrimitive {

  // Width of the info label
  Int labelWidth;

  // Current info string
  std::string info;

  // Request to fade in the info
  bool fadeIn;

  // Request to fade out the info
  bool fadeOut;

  // Graphical representation of the info string
  FontString *infoFontString;

  // Update contents and position of the info font string
  void updateInfoFontString();

public:

  // Constructors and destructor
  WidgetCursorInfo(WidgetPage *widgetPage);
  virtual ~WidgetCursorInfo();

  // Let the widget work
  virtual bool work(TimestampInMicroseconds t);

  // Called when the widget must be drawn
  virtual void draw(TimestampInMicroseconds t);

  // Called when the widget has changed his position
  virtual void updatePosition(Int x, Int y, Int z);

  // Called when the map has changed
  virtual void onMapChange(bool widgetVisible, MapPosition pos);

  // Getters and setters
  void setLabelWidth(Int labelWidth) {
    this->labelWidth = labelWidth;
  }
};

} /* namespace GEODISCOVERER */
#endif /* WIDGETCURSORINFO_H_ */