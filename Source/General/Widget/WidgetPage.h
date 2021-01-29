//============================================================================
// Name        : WidgetPage.h
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


#ifndef WIDGETPAGE_H_
#define WIDGETPAGE_H_

namespace GEODISCOVERER {

class WidgetPage: public GEODISCOVERER::WidgetContainer {

protected:

  std::string name;    
  bool widgetsActive;                                   // Indicates if the widgets are active                                 // Name of the page
  TimestampInMicroseconds hiddenAnimationDuration;      // Time duration in microseconds of the translate animation if a widget is outside the screen

public:

  // Constructors and destructor
  WidgetPage(WidgetEngine *widgetEngine, std::string name);
  virtual ~WidgetPage();

  // Called when the widget is touched
  virtual bool onTouchDown(TimestampInMicroseconds t, Int x, Int y);

  // Let the page work
  bool work(TimestampInMicroseconds t);

  // Sets the active state of the widgets
  void setWidgetsActive(TimestampInMicroseconds t, bool widgetsActive);

  // Getters and setters
  std::string getName() const
  {
      return name;
  }
  bool getWidgetsActive() const {
    return widgetsActive;
  }
};

}

#endif /* WIDGETPAGE_H_ */
