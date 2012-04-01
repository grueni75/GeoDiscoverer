//============================================================================
// Name        : WidgetEngine.h
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


#ifndef WIDGETENGINE_H_
#define WIDGETENGINE_H_

namespace GEODISCOVERER {

typedef std::map<std::string, WidgetPage*> WidgetPageMap;
typedef std::pair<std::string, WidgetPage*> WidgetPagePair;
typedef std::map<std::string, std::string> ParameterMap;
typedef std::pair<std::string, std::string> ParameterPair;

class WidgetEngine {

protected:

  WidgetPageMap pageMap;   // Holds all available widget pages
  WidgetPage *currentPage; // The currently selected page
  GraphicColor selectedWidgetColor; // Color of selected widgets
  TimestampInMicroseconds buttonRepeatDelay; // Time to wait before dispatching repeating commands
  TimestampInMicroseconds buttonRepeatPeriod; // Time distance between command dispatching

  // Adds a widget to a page
  void addWidgetToPage(
    std::string pageName,
    WidgetType widgetType,
    std::string widgetName,
    double portraitX, double portraitY, Int portraitZ,
    double landscapeX, double landscapeY, Int landscapeZ,
    UByte activeRed, UByte activeGreen, UByte activeBlue, UByte activeAlpha,
    UByte inactiveRed, UByte inactiveGreen, UByte inactiveBlue, UByte inactiveAlpha,
    ParameterMap parameters);

public:

  // Constructors and destructor
  WidgetEngine();
  virtual ~WidgetEngine();

  // (Re)creates all widget pages from the current config
  void init();

  // Clears all widget pages
  void deinit();

  // Updates the positions of the widgets in dependence of the current screen dimension
  void updateWidgetPositions();

  // Called when the screen is touched
  bool onTouchDown(TimestampInMicroseconds t, Int x, Int y);

  // Called when the screen is untouched
  bool onTouchUp(TimestampInMicroseconds t, Int x, Int y);

  // Getters and setters
  GraphicColor getSelectedWidgetColor() const
  {
      return selectedWidgetColor;
  }

  TimestampInMicroseconds getButtonRepeatDelay() const
  {
      return buttonRepeatDelay;
  }

  TimestampInMicroseconds getButtonRepeatPeriod() const
  {
      return buttonRepeatPeriod;
  }
};

}

#endif /* WIDGETENGINE_H_ */
