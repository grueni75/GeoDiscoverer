//============================================================================
// Name        : Main.h
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


#ifndef MAIN_H_
#define MAIN_H_

// System
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <map>
#include <string>
#include <list>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <dirent.h>
#include <limits>
#include <unistd.h>
#include <algorithm>

// Mandatory application includes for the core class
#include <Types.h>
#include <Thread.h>
#include <GraphicColor.h>

namespace GEODISCOVERER {

// Core class that contains all application components
class Debug;
class ProfileEngine;
class ConfigStore;
class Thread;
class Clock;
class Screen;
class FontEngine;
class GraphicEngine;
class WidgetEngine;
class MapCache;
class MapEngine;
class MapSource;
class UnitConverter;
class Image;
class Dialog;
class NavigationEngine;
class Commander;

class Core {

protected:

  // Path to the home directory
  std::string homePath;

  // Density of the screen
  Int screenDPI;

  // Current thread info about the maintenance thread
  ThreadInfo *maintenanceThreadInfo;

  // Used to interrupt the maintenance thread
  ThreadMutexInfo *maintenanceMutex;

  // Current thread info about the map update thread
  ThreadInfo *mapUpdateThreadInfo;

  // Current thread info about the late init thread
  ThreadInfo *lateInitThreadInfo;

  // Used to interrupt the map update thread
  ThreadMutexInfo *mapUpdateInterruptMutex;

  // Used to interrupt the screen update thread
  ThreadMutexInfo *screenUpdateInterruptMutex;

  // Used to trigger the map update thread
  ThreadSignalInfo *mapUpdateStartSignal;
  ThreadSignalInfo *mapUpdateTileTextureProcessedSignal;

  // Indicates if the core is initialized
  bool isInitialized;

  // Used to force an exit of the map update thread
  bool quitMapUpdateThread;

  // Indicates if the screen update thread is not updating the map
  bool mapUpdateStopped;

  // Indicates that no interruption is allowed
  bool noInterruptAllowed;

  // Time in seconds between storing of unsaved data
  Int maintenancePeriod;

  // Components
  Debug *debug;
  ProfileEngine *profileEngine;
  ConfigStore *configStore;
  Thread *thread;
  Clock *clock;
  Screen *screen;
  UnitConverter *unitConverter;
  FontEngine *fontEngine;
  GraphicEngine *graphicEngine;
  WidgetEngine *widgetEngine;
  MapCache *mapCache;
  MapEngine *mapEngine;
  MapSource *mapSource;
  Image *image;
  Dialog *dialog;
  NavigationEngine *navigationEngine;
  Commander *commander;

public:

  // Constructor and destructor
  Core(std::string homePath, Int screenDPI);
  virtual ~Core();

  // Initializes the application
  bool init();

  // Updates the screen
  void updateScreen(bool forceRedraw);

  // Allows an external interrupt
  void interruptAllowedHere();

  // Updates the map
  void updateMap();

  // Stores unsaved data
  void maintenance(bool endlessLoop=true);

  // Indicates the map cache has prepared a new texture
  void tileTextureAvailable();

  // Called if the textures have been lost
  bool graphicInvalidated();

  // Does a late initialization of certain objects
  void lateInit();

  // Stops the map update thread
  void interruptMapUpdate();

  // Continue the map update thread
  void continueMapUpdate();

  // Getters and setters
  std::string getHomePath() const
  {
      return homePath;
  }

  Clock *getClock() const
  {
      return clock;
  }

  ProfileEngine *getProfileEngine() const
  {
      return profileEngine;
  }

  Debug *getDebug() const
  {
      return debug;
  }

  ConfigStore *getConfigStore() const
  {
      return configStore;
  }

  UnitConverter *getUnitConverter() const
  {
      return unitConverter;
  }

  GraphicEngine *getGraphicEngine() const
  {
      return graphicEngine;
  }

  WidgetEngine *getWidgetEngine() const
  {
      return widgetEngine;
  }

  Image *getImage() const
  {
      return image;
  }

  MapCache *getMapCache() const
  {
      return mapCache;
  }

  MapEngine *getMapEngine() const
  {
      return mapEngine;
  }

  MapSource *getMapSource() const
  {
      return mapSource;
  }

  FontEngine *getFontEngine() const
  {
      return fontEngine;
  }

  Screen *getScreen() const
  {
      return screen;
  }

  Thread *getThread() const
  {
      return thread;
  }

  Dialog *getDialog() const
  {
      return dialog;
  }

  NavigationEngine *getNavigationEngine() const
  {
      return navigationEngine;
  }

  Commander *getCommander() const
  {
      return commander;
  }

  ThreadMutexInfo *getMapUpdateInterruptMutex() const
  {
      return mapUpdateInterruptMutex;
  }

  bool getQuitMapUpdateThread() const
  {
      return quitMapUpdateThread;
  }

  bool getIsInitialized() const
  {
      return isInitialized;
  }

};

// Pointer to the core
extern Core *core;

}

// Application includes
#include <FloatingPoint.h>
#include <Integer.h>
#include <Debug.h>
#include <Clock.h>
#include <ProfileBlockResult.h>
#include <ProfileMethodResult.h>
#include <ProfileEngine.h>
#include <ConfigStore.h>
#include <UnitConverter.h>
#include <Image.h>
#include <Storage.h>
#include <Dialog.h>
#include <Screen.h>
#include <GraphicPosition.h>
#include <GraphicPrimitive.h>
#include <GraphicPointBuffer.h>
#include <GraphicLine.h>
#include <GraphicRectangle.h>
#include <GraphicRectangleListSegment.h>
#include <GraphicRectangleList.h>
#include <GraphicObject.h>
#include <FontCharacter.h>
#include <FontCharacterPosition.h>
#include <FontString.h>
#include <Font.h>
#include <FontEngine.h>
#include <MapTile.h>
#include <MapPosition.h>
#include <WidgetPrimitive.h>
#include <WidgetButton.h>
#include <WidgetCheckbox.h>
#include <WidgetMeter.h>
#include <WidgetScale.h>
#include <WidgetPage.h>
#include <GraphicEngine.h>
#include <WidgetEngine.h>
#include <MapArea.h>
#include <MapCalibrator.h>
#include <MapCalibratorLinear.h>
#include <MapCalibratorMercator.h>
#include <MapContainer.h>
#include <MapContainerTreeNode.h>
#include <MapCache.h>
#include <MapEngine.h>
#include <MapSource.h>
#include <MapSourceCalibratedPictures.h>
#include <MapSourceMercatorTiles.h>
#include <NavigationPathTileInfo.h>
#include <NavigationPathVisualization.h>
#include <NavigationPath.h>
#include <NavigationEngine.h>
#include <Commander.h>

#endif /* MAIN_H_ */