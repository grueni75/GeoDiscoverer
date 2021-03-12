//============================================================================
// Name        : WidgetFingerMenu.h
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

#include <WidgetCursorInfo.h>
#include <FontString.h>
#include <WidgetContainer.h>

#ifndef WIDGETFINGERMENU_H_
#define WIDGETFINGERMENU_H_

namespace GEODISCOVERER {

class WidgetFingerMenu: public GEODISCOVERER::WidgetContainer {

protected:

  Int circleRadius;                           // Radius in pixels to use for positioning the widgets on the circle
  Int rowDistance;                            // Distance in pixels to use for positioning the widgets on the row
  Int rowOffsetY;                             // Vertical offset in pixels of the widget row
  Int cursorInfoClosedOffsetY;                // Vertical offset in pixels of the cursor info in closed state
  Int cursorInfoOpenedOffsetY;                 // Vertical offset in pixels of the cursor info in opened state
  double angleOffset;                         // Angle in degree to start placing widgets from the south of the menu
  std::list<WidgetPrimitive*> circleWidgets;  // Contains the widgets of the circle
  std::list<WidgetPrimitive*> rowWidgets;     // Contains the widgets of the row
  WidgetCursorInfo *cursorInfo;               // Reference to the cursor info
  bool opened;                                // Indicates if the finger menu is open
  FontString *pathNameFontString;             // Graphical representation of the nearest path
  TimestampInMicroseconds animationDuration;  // Duration of the change animation
  bool pathNearby;                            // Indicates if a path is nearby
  TimestampInMicroseconds closeTimeout;       // Timeout in microseconds after the finger menu closes if not touched
  TimestampInMicroseconds closeTimestamp;     // Timestamp when to close the menu
  bool stateChanged;                          // Indicates that a state change has happened

  // Puts the widgets on a circle
  void positionWidgetOnCircle(TimestampInMicroseconds t, Int x, Int y, double radius);

  // Puts the widgets on a row
  void positionWidgetOnRow(TimestampInMicroseconds t, Int x, Int y, double distance);

public:

  // Constructors and destructor
  WidgetFingerMenu(WidgetEngine *widgetEngine);
  virtual ~WidgetFingerMenu();

  // Removes all widgets
  virtual void deinit(bool deleteWidgets=true);
  
  // Adds a widget to the menu
  void addWidgetToCircle(WidgetPrimitive *primitive);
  void addWidgetToRow(WidgetPrimitive *primitive);
  void setCursorInfoWidget(WidgetCursorInfo *primitive);

  // Opens the menu
  void open();

  // Closes the menu
  void close();

  // Let the finger menu work
  bool work(TimestampInMicroseconds t);

  // Called when the widget is touched
  virtual bool onTouchDown(TimestampInMicroseconds t, Int x, Int y);

  // Getters and setters
  bool isOpen()
  {
      return opened;
  }
};

}

#endif /* WIDGETFINGERMENU_H_ */
