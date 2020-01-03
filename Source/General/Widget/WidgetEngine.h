//============================================================================
// Name        : WidgetEngine.h
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


#ifndef WIDGETENGINE_H_
#define WIDGETENGINE_H_

namespace GEODISCOVERER {

typedef std::map<std::string, WidgetPage*> WidgetPageMap;
typedef std::pair<std::string, WidgetPage*> WidgetPagePair;

class WidgetEngine {

protected:

  ThreadMutexInfo *accessMutex;                         // Mutex for accessing this object
  Device *device;                                       // Device that shall be used for this widget engine
  WidgetPageMap pageMap;                                // Holds all available widget pages
  WidgetPage *currentPage;                              // The currently selected page
  GraphicColor selectedWidgetColor;                     // Color of selected widgets
  TimestampInMicroseconds buttonRepeatDelay;            // Time to wait before dispatching repeating commands
  TimestampInMicroseconds buttonLongPressDelay;         // Time to wait before dispatching long press command
  TimestampInMicroseconds buttonRepeatPeriod;           // Time distance between command dispatching
  TimestampInMicroseconds firstStableTouchDownTime;     // Time of the first touch down
  Int lastTouchDownX, lastTouchDownY;                   // Coordinates of the last touch down
  bool isTouched;                                       // Indicates that the screen is currently touched
  bool contextMenuIsShown;                              // Indicates that the context menu is currently shown
  TimestampInMicroseconds contextMenuDelay;             // Time that must passed before the context menu is displayed
  Int contextMenuAllowedPixelJitter;                    // Allowed pixel jitter when checking if a context menu shall be displayed
  GraphicObject visiblePages;                           // The currently visible pages
  TimestampInMicroseconds changePageDuration;           // Time that the transition from the current page to the next page takes
  Int changePageOvershoot;                              // Distance that the page change shall overshoot
  TimestampInMicroseconds ignoreTouchesEnd;             // End time until touches shall be ignored
  TimestampInMicroseconds widgetsActiveTimeout;         // Time to show the widgets after last interaction
  NavigationPath *nearestPath;                          // Path that is currently the nearest to the map center
  Int nearestPathIndex;                                 // Index of the nearest point on the nearest path

  // Adds a widget to a page
  void addWidgetToPage(WidgetConfig config);

  // Deselects the currently selected page
  void deselectPage();

public:

  // Constructors and destructor
  WidgetEngine(Device *device);
  virtual ~WidgetEngine();

  // Inits the object
  void init();

  // Creates all widget pages from the current config
  void createGraphic();

  // Clears all widget pages
  void destroyGraphic();

  // Clears all widget pages
  void deinit();

  // Updates the positions of the widgets in dependence of the current screen dimension
  void updateWidgetPositions();

  // Called when the screen is touched
  bool onTwoFingerGesture(TimestampInMicroseconds t, Int dX, Int dY, double angleDiff, double scaleDiff);

  // Called when the screen is untouched
  bool onTouchUp(TimestampInMicroseconds t, Int x, Int y, bool cancel=false);

  // Called when a two fingure gesture is done on the screen
  bool onTouchDown(TimestampInMicroseconds t, Int x, Int y);

  // Informs the engine that the map has changed
  void onMapChange(MapPosition pos, std::list<MapTile*> *centerMapTiles);

  // Informs the engine that the location has changed
  void onLocationChange(MapPosition mapPos);

  // Informs the engine that a path has changed
  void onPathChange(NavigationPath *path, NavigationPathChangeType changeType);

  // Informs the engine that some data has changed
  void onDataChange();

  // Let the engine work
  bool work(TimestampInMicroseconds t);

  // Shows the context menu
  void showContextMenu();

  // Sets the target at an address
  void setTargetAtAddress();

  // Sets a new page
  void setPage(std::string name, Int direction);

  // Sets the widgets of the current page active
  void setWidgetsActive(bool widgetsActive);

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

  TimestampInMicroseconds getButtonLongPressDelay() const
  {
      return buttonLongPressDelay;
  }

  bool getWidgetsActive() const {
    return currentPage->getWidgetsActive();
  }

  TimestampInMicroseconds getWidgetsActiveTimeout() const {
    return widgetsActiveTimeout;
  }

  NavigationPath* getNearestPath() const {
    return nearestPath;
  }

  Int getNearestPathIndex() const {
    return nearestPathIndex;
  }

  FontEngine *getFontEngine();

  GraphicEngine *getGraphicEngine();

  Screen *getScreen();

  Device *getDevice();
};

}

#endif /* WIDGETENGINE_H_ */
