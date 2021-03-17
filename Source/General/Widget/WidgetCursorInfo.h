//============================================================================
// Name        : WidgetCursorInfo.h
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

#include <FontString.h>
#include <WidgetPrimitive.h>

#ifndef WIDGETCURSORINFO_H_
#define WIDGETCURSORINFO_H_

namespace GEODISCOVERER {

class WidgetCursorInfo: public GEODISCOVERER::WidgetPrimitive {

  // Current info string
  std::string info;

  // Request to fade in the info
  bool fadeIn;

  // Request to fade out the info
  bool fadeOut;

  // Request to update the info
  bool updateInfo;

  // Indicates if the widget is permanent visible
  bool permanentVisible;

  // Maximum distance in pixels to a path to show it in the info string
  Int maxPathDistance;
  
  // Indicates if a path is nearby
  bool pathNearby;

  // Indicates if an address pointis nearby
  bool addressPointNearby;

  // Number of characters to keep at the end of the info string
  int infoKeepEndCharCount;

  // Graphical representation of the info string
  FontString *infoFontString;

  // Update contents and position of the info font string
  void updateInfoFontString();

public:

  // Constructors and destructor
  WidgetCursorInfo(WidgetContainer *widgetContainer);
  virtual ~WidgetCursorInfo();

  // Let the widget work
  virtual bool work(TimestampInMicroseconds t);

  // Called when the widget must be drawn
  virtual void draw(TimestampInMicroseconds t);

  // Called when the widget has changed his position
  virtual void updatePosition(Int x, Int y, Int z);

  // Called when the map has changed
  virtual void onMapChange(bool widgetVisible, MapPosition pos);

  // Called when some data has changed
  virtual void onDataChange();

  // Changes the state of the widget
  void changeState(Int y, bool permanentVisible, TimestampInMicroseconds animationDuration);

  // Getters and setters
  bool getPathNearby() {
      return pathNearby;
  }
  bool getAddressPointNearby() {
      return addressPointNearby;
  }
};

} /* namespace GEODISCOVERER */
#endif /* WIDGETCURSORINFO_H_ */
