//============================================================================
// Name        : WidgetContainer.h
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

#include <GraphicObject.h>
#include <WidgetPrimitive.h>

#ifndef WIDGETCONTAINER_H_
#define WIDGETCONTAINER_H_

namespace GEODISCOVERER {

class WidgetContainer {

protected:

  std::string name;                                     // Name of the container
  WidgetEngine *widgetEngine;                           // Widget engine this page belongs to
  GraphicObject graphicObject;                          // Contains the widgets of this page
  bool touchStartedOutside;                             // Indicates that the widgets were not touched directly at the beginning
  bool firstTouch;                                      // Indicates that no touch was done before
  WidgetPrimitive *selectedWidget;                      // The currently selected widget
  TimestampInMicroseconds touchEndTime;                 // Last time no widget was touched
  bool lastTouchStartedOutside;                         // Indicates if the last touch was not hitting any widgets

public:

  // Constructors and destructor
  WidgetContainer(WidgetEngine *widgetEngine, std::string name);
  virtual ~WidgetContainer();

  // Adds a widget to the page
  virtual void addWidget(WidgetPrimitive *primitive);

  // Removes all widgets
  virtual void deinit(bool deleteWidgets=true);

  // Let the container work
  virtual bool work(TimestampInMicroseconds t) = 0;

  // Called when the screen is touched
  bool onTwoFingerGesture(TimestampInMicroseconds t, Int dX, Int dY, double angleDiff, double scaleDiff);

  // Called when the widget is touched
  virtual bool onTouchDown(TimestampInMicroseconds t, Int x, Int y);

  // Called when the widget is not touched anymore
  void onTouchUp(TimestampInMicroseconds t, Int x, Int y, bool cancel);

  // Deselects currently selected widget
  void deselectWidget(TimestampInMicroseconds t);

  // Called when the map has changed
  void onMapChange(bool pageVisible, MapPosition pos);

  // Called when the location has changed
  void onLocationChange(bool pageVisible, MapPosition pos);

  // Called when a path has changed
  void onPathChange(bool pageVisible, NavigationPath *path, NavigationPathChangeType changeType);

  // Called when some data has changed
  void onDataChange();

  // Called when touch mode is changed
  void setTouchMode(Int mode);

  // Getters and setters
  GraphicObject *getGraphicObject()
  {
      return &graphicObject;
  }
  std::string getName() const
  {
      return name;
  }

  FontEngine *getFontEngine();

  WidgetEngine *getWidgetEngine();

  GraphicEngine *getGraphicEngine();

  Screen *getScreen();
};

}

#endif /* WidgetContainer_H_ */
