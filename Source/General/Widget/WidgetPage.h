//============================================================================
// Name        : WidgetPage.h
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


#ifndef WIDGETPAGE_H_
#define WIDGETPAGE_H_

namespace GEODISCOVERER {

class WidgetPage {

protected:

  std::string name;              // Name of the page
  GraphicObject graphicObject;   // Contains the widgets of this page
  bool widgetsActive;            // Indicates if the widgets are active
  bool touchStartedOutside;      // Indicates that the widgets were not touched directly at the beginning
  bool firstTouch;               // Indicates that no touch was done before
  WidgetPrimitive *selectedWidget; // The currently selected widget

  // Sets the active state of the widgets
  void setWidgetsActive(TimestampInMicroseconds t, bool widgetsActive);

public:

  // Constructors and destructor
  WidgetPage(std::string name);
  virtual ~WidgetPage();

  // Adds a widget to the page
  void addWidget(WidgetPrimitive *primitive);

  // Removes all widgets
  void deinit(bool deleteWidgets=true);

  // Called when the widget is touched
  bool onTouchDown(TimestampInMicroseconds t, Int x, Int y);

  // Called when the widget is not touched anymore
  bool onTouchUp(TimestampInMicroseconds t, Int x, Int y);

  // Getters and setters
  GraphicObject *getGraphicObject()
  {
      return &graphicObject;
  }
  std::string getName() const
  {
      return name;
  }


};

}

#endif /* WIDGETPAGE_H_ */
