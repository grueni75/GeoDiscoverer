//============================================================================
// Name        : Main.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef MAIN_H_
#define MAIN_H_

// Enable profiling
//#define PROFILING_ENABLED

// System
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>
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
#include <signal.h>
#ifdef TARGET_LINUX
#include <execinfo.h>
#endif
#include <libgen.h>

// Mandatory application includes for the core class
#include <Types.h>
#include <Clock.h>
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
class MapSourceMercatorTiles;
class UnitConverter;
class Image;
class Dialog;
class NavigationEngine;
class Commander;

// Error types for downloads
typedef enum { DownloadResultSuccess, DownloadResultFileNotFound, DownloadResultOtherFail } DownloadResult;

// Types of changes to a navigation path object
typedef enum {NavigationPathChangeTypeEndPositionAdded, NavigationPathChangeTypeFlagSet, NavigationPathChangeTypeWillBeRemoved } NavigationPathChangeType;

class Core {

protected:

  // Path to the home directory
  std::string homePath;

  // Density of the screen
  Int screenDPI;

  // Diagonal of the screen
  Int screenDiagonal;

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

  // Used to trigger the map update thread
  ThreadSignalInfo *mapUpdateStartSignal;
  ThreadSignalInfo *mapUpdateTileTextureProcessedSignal;

  // Used to prevent deinit before init is complete
  ThreadMutexInfo *isInitializedMutex;
  ThreadSignalInfo *isInitializedSignal;

  // Indicates if the core is initialized
  bool isInitialized;

  // Used to force an exit of the map update thread
  bool quitMapUpdateThread;

  // Used to force an exit of the core
  bool quitCore;

  // Indicates if the screen update thread is not updating the map
  bool mapUpdateStopped;

  // Indicates that no interruption is allowed
  bool noInterruptAllowed;

  // Time in seconds between storing of unsaved data
  Int maintenancePeriod;

  // Handling of retries when opening a file for writing
  Int fileOpenForWritingRetries;
  TimestampInMicroseconds fileOpenForWritingWaitTime;

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
  Core(std::string homePath, Int screenDPI, double screenDiagonal);
  virtual ~Core();

  // Called when the process is killed
  static void unload();

  // Initializes the application
  bool init();

  // Updates the screen
  void updateScreen(bool forceRedraw);

  // Allows an external interrupt
  void interruptAllowedHere(const char *file, int line);

  // Updates the map
  void updateMap();

  // Stores unsaved data
  void maintenance(bool endlessLoop=true);

  // Indicates the map cache has prepared a new texture
  void tileTextureAvailable(const char *file, int line);

  // Called if the textures or buffers have been lost or must be recreated
  void updateGraphic(bool graphicInvalidated);

  // Does a late initialization of certain objects
  void lateInit();

  // Called

  // Stops the map update thread
  void interruptMapUpdate(const char *file, int line);

  // Continue the map update thread
  void continueMapUpdate();

  // Downloads a URL
  DownloadResult downloadURL(std::string url, std::string filePath, bool generateMessages=true, bool ignoreFileNotFoundErrors=false);

  // Waits until the core is initialized
  void waitForInitialization() {
    thread->waitForSignal(isInitializedSignal);
  }

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

  bool getQuitCore() const {
    return quitCore;
  }

  bool getIsInitialized() const
  {
    return isInitialized;
  }

  Int getFileOpenForWritingRetries() const {
    return fileOpenForWritingRetries;
  }

  TimestampInMicroseconds getFileOpenForWritingWaitTime() const {
    return fileOpenForWritingWaitTime;
  }
};

// Pointer to the core
extern Core *core;

}

// Application includes
#include <FloatingPoint.h>
#include <Integer.h>
#include <Debug.h>
#include <ProfileBlockResult.h>
#include <ProfileMethodResult.h>
#include <ProfileEngine.h>
#include <ConfigStore.h>
#include <UnitConverter.h>
#include <ZipArchive.h>
#include <Image.h>
#include <Storage.h>
#include <Dialog.h>
#include <Screen.h>
#include <NavigationInfo.h>
#include <GraphicPosition.h>
#include <GraphicAnimationParameter.h>
#include <GraphicScaleAnimationParameter.h>
#include <GraphicTranslateAnimationParameter.h>
#include <GraphicRotateAnimationParameter.h>
#include <GraphicFadeAnimationParameter.h>
#include <GraphicTextureAnimationParameter.h>
#include <GraphicPrimitive.h>
#include <GraphicPoint.h>
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
#include <WidgetNavigation.h>
#include <WidgetPathInfo.h>
#include <WidgetStatus.h>
#include <WidgetPage.h>
#include <WidgetPosition.h>
#include <WidgetConfig.h>
#include <GraphicEngine.h>
#include <WidgetEngine.h>
#include <MapArea.h>
#include <MapCalibrator.h>
#include <MapCalibratorLinear.h>
#include <MapCalibratorSphericalNormalMercator.h>
#include <MapCalibratorProj4.h>
#include <MapContainer.h>
#include <MapContainerTreeNode.h>
#include <MapCache.h>
#include <MapEngine.h>
#include <MapTileServer.h>
#include <MapDownloader.h>
#include <MapSource.h>
#include <MapSourceEmpty.h>
#include <MapSourceCalibratedPictures.h>
#include <MapSourceMercatorTiles.h>
#include <NavigationPathTileInfo.h>
#include <NavigationPathVisualization.h>
#include <NavigationPath.h>
#include <NavigationPathSegment.h>
#include <NavigationTarget.h>
#include <NavigationEngine.h>
#include <Commander.h>

#endif /* MAIN_H_ */
