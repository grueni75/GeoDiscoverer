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

#include <WidgetPage.h>
#include <WidgetFingerMenu.h>
#include <MapPosition.h>
#include <WidgetContainer.h>
#include <WidgetConfig.h>
#include <WidgetCheckbox.h>

#ifndef WIDGETENGINE_H_
#define WIDGETENGINE_H_

namespace GEODISCOVERER {

typedef std::map<std::string, WidgetPage*> WidgetPageMap;
typedef std::pair<std::string, WidgetPage*> WidgetPagePair;
typedef std::pair<std::string, WidgetCheckbox*> WidgetCommandPair;

class WidgetEngine {

protected:

  ThreadMutexInfo *accessMutex;                         // Mutex for accessing this object
  Device *device;                                       // Device that shall be used for this widget engine
  WidgetPageMap pageMap;                                // Holds all available widget pages
  WidgetPage *currentPage;                              // The currently selected page
  WidgetFingerMenu *fingerMenu;                         // Contains the widgets that appear in the finger menu
  GraphicColor selectedWidgetColor;                     // Color of selected widgets
  TimestampInMicroseconds buttonRepeatDelay;            // Time to wait before dispatching repeating commands
  TimestampInMicroseconds buttonLongPressDelay;         // Time to wait before dispatching long press command
  TimestampInMicroseconds buttonRepeatPeriod;           // Time distance between command dispatching
  TimestampInMicroseconds firstStableTouchDownTime;     // Time of the first touch down
  bool isTouched;                                       // Indicates that the screen is currently touched
  GraphicObject visiblePages;                           // The currently visible pages
  TimestampInMicroseconds changePageDuration;           // Time that the transition from the current page to the next page takes
  Int changePageOvershoot;                              // Distance that the page change shall overshoot
  TimestampInMicroseconds ignoreTouchesEnd;             // End time until touches shall be ignored
  TimestampInMicroseconds widgetsActiveTimeout;         // Time to show the widgets after last interaction
  NavigationPath *nearestPath;                          // Path that is currently the nearest to the map center
  Int nearestPathIndex;                                 // Index of the nearest point on the nearest path
  MapPosition nearestPathMapPos;                        // Position of the nearest point on the nearest path
  bool enableFingerMenu;                                // Indicates if finger menu is enabled
  Int maxPathDistance;                                  // Maximum distance in pixels to a path to show it to the user
  int readAccessCount;                                  // Number of read accesses to this object
  std::list<WidgetCommandPair> queuedCommands;          // List of commands to execute

  // Adds a widget to a page
  void addWidgetToPage(WidgetConfig config);

  // Deselects the currently selected page
  void deselectPage();

  // Updates the nearest path
  void updateNearestPath(MapPosition mapPos, std::list<MapTile*> *centerMapTiles);

  // Indicates that a read access has started
  void readAccessStart(const char *file, int line);

  // Indicates that a read access has ended
  void readAccessStop();

  // Waits until all read accesses are over
  void lockAccessAfterReadComplete(const char *file, int line);

  // Updates the positions of widgets with the given dimension
  void updateWidgetPositions(Int x, Int y, Int width, Int height, bool onlyWindowWidgets);

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

  // Toggles touch mode on/off
  void setTouchMode(Int mode);

  // Let the engine work
  bool work(TimestampInMicroseconds t);

  // Shows the context menu
  void showContextMenu();

  // Opens the finger menu
  void openFingerMenu();

  // Closes the finger menu
  void closeFingerMenu();

  // Closes or opens the finger menu
  void toggleFingerMenu();

  // Sets the target at an address
  void setTargetAtAddress();

  // Sets a new page
  void setPage(std::string name, Int direction);

  // Sets the widgets of the current page active
  void setWidgetsActive(bool widgetsActive);

  // Updates the positions of widgets that shall be positioned relative to a window
  void setWindow(Int x, Int y, Int width, Int height);

  // Schedules a command to execute after graphic operation is done
  void queueCommand(std::string command, WidgetCheckbox *checkbox);

  // Executes all queued commands
  void executeCommands();

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

  NavigationPath* getNearestPath(Int *index,MapPosition *mapPos) const {
    NavigationPath *nearestPath;
    core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
    nearestPath=this->nearestPath;
    if (index) *index=this->nearestPathIndex;
    if (mapPos) *mapPos=this->nearestPathMapPos;
    core->getThread()->unlockMutex(accessMutex);
    return nearestPath;
  }

  bool getFingerMenuEnabled() const {
    return enableFingerMenu;
  }

  FontEngine *getFontEngine();

  GraphicEngine *getGraphicEngine();

  Screen *getScreen();

  Device *getDevice();
};

}

#endif /* WIDGETENGINE_H_ */
