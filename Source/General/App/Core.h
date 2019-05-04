//============================================================================
// Name        : Main.h
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
#include <errno.h>
#ifdef TARGET_ANDROID
#include <sys/vfs.h>
#define statvfs statfs  // Android uses a statvfs-like statfs struct and call.
#else
#include <sys/statvfs.h>
#endif
#ifdef TARGET_LINUX
#include <execinfo.h>
#endif
#include <libgen.h>
#include <png.h>
#include <jpeglib.h>

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
class Device;
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
class MapPosition;
class MapTile;
class NavigationPath;
class NavigationPoint;

// Error types for downloads
typedef enum { DownloadResultSuccess, DownloadResultFileNotFound, DownloadResultOtherFail } DownloadResult;

// Types of changes to a navigation path object
typedef enum { NavigationPathChangeTypeEndPositionAdded, NavigationPathChangeTypeFlagSet, NavigationPathChangeTypeWillBeRemoved, NavigationPathChangeTypeWidgetEngineInit } NavigationPathChangeType;

class Core {

protected:

  // Path to the home directory
  std::string homePath;

  // Density of the screen
  Int defaultScreenDPI;

  // Diagonal of the screen
  Int defaultScreenDiagonal;

  // Current thread info about the maintenance thread
  ThreadInfo *maintenanceThreadInfo;

  // Used to interrupt the maintenance thread
  ThreadMutexInfo *maintenanceMutex;

  // Current thread info about the map update thread
  ThreadInfo *mapUpdateThreadInfo;

  // Current thread info about the late init thread
  ThreadInfo *lateInitThreadInfo;

  // Current thread info about the dashboard rendering thread
  ThreadInfo *updateDashboardScreensThreadInfo;

  // Used to wakeup the dashboard rendering thread
  ThreadSignalInfo *updateDashboardScreensWakeupSignal;

  // Used to interrupt the map update thread
  ThreadMutexInfo *mapUpdateInterruptMutex;

  // Used to trigger the map update thread
  ThreadSignalInfo *mapUpdateStartSignal;
  ThreadSignalInfo *mapUpdateTileTextureProcessedSignal;

  // Used to prevent deinit before init is complete
  ThreadMutexInfo *isInitializedMutex;
  ThreadSignalInfo *isInitializedSignal;

  // Default device
  Device *defaultDevice;

  // The dashboard device list
  std::list<Device*> dashboardDevices;

  // Used to control access to the devices list
  ThreadMutexInfo *dashboardDevicesMutex;

  // Indicates if the core is initialized
  bool isInitialized;

  // Used to force an exit of the map update thread
  bool quitMapUpdateThread;

  // Used to force an exit of the update cockpit screens thread
  bool quitUpdateDashboardScreensThread;

  // Used to force an exit of the core
  bool quitCore;

  // Indicates if the screen update thread is not updating the map
  bool mapUpdateStopped;

  // Indicates that no interruption is allowed
  bool noInterruptAllowed;

  // Time in seconds between storing of unsaved data
  Int maintenancePeriod;

  // Indicates if a remote server sends something
  boolean remoteServerActive;

  // Handling of retries when opening a file for writing or accessing it
  Int fileAccessRetries;
  TimestampInMicroseconds fileAccessWaitTime;

  // Components
  Debug *debug;
  ProfileEngine *profileEngine;
  ConfigStore *configStore;
  Thread *thread;
  Clock *clock;
  UnitConverter *unitConverter;
  MapCache *mapCache;
  MapEngine *mapEngine;
  MapSource *mapSource;
  Image *image;
  Dialog *dialog;
  NavigationEngine *navigationEngine;
  Commander *commander;

  // Battery info
  Int batteryLevel;
  Int batteryCharging;
  Int remoteBatteryLevel;
  Int remoteBatteryCharging;

  // Inits cURL
  void initCURL();

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

  // Updates the screen of cockpits
  void updateDashboardScreens();

    // Allows an external interrupt
  void interruptAllowedHere(const char *file, int line);

  // Updates the map
  void updateMap();

  // Stores unsaved data
  void maintenance(bool endlessLoop=true);

  // Indicates the map cache has prepared a new texture
  void tileTextureAvailable(const char *file, int line);

  // Called if the textures or buffers have been lost or must be recreated
  void updateGraphic(bool graphicInvalidated, bool destroyOnly);

  // Does a late initialization of certain objects
  void lateInit();

  // Informs the engines that the map has changed
  void onMapChange(MapPosition pos, std::list<MapTile*> *centerMapTiles);

  // Informs the engines that the location has changed
  void onLocationChange(MapPosition mapPos);

  // Informs the engines that a path has changed
  void onPathChange(NavigationPath *path, NavigationPathChangeType changeType);

  // Informs the engines that some data has changed
  void onDataChange();

  // Stops the map update thread
  void interruptMapUpdate(const char *file, int line);

  // Continue the map update thread
  void continueMapUpdate();

  // Downloads a URL
  UByte *downloadURL(std::string url, DownloadResult &result, UInt &size, bool generateMessages, bool ignoreFileNotFoundErrors, std::list<std::string> *httpHeader = NULL);

  // Get file attributes inclusive waiting until the file is available
  Int statFile(std::string path, struct stat *buffer);

  // Waits until the directory is available and open it
  DIR *openDir(std::string path);

  // Waits until the core is initialized
  void waitForInitialization() {
    thread->waitForSignal(isInitializedSignal);
  }

  // Adds a new dashboard device
  void addDashboardDevice(std::string host, Int port);

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
    return fileAccessRetries;
  }

  TimestampInMicroseconds getFileOpenForWritingWaitTime() const {
    return fileAccessWaitTime;
  }

  Screen *getDefaultScreen();

  Device *getDefaultDevice();

  WidgetEngine *getDefaultWidgetEngine();

  GraphicEngine *getDefaultGraphicEngine();

  boolean getRemoteServerActive() const {
    return remoteServerActive;
  }

  void setRemoteServerActive(boolean remoteServerActive) {
    this->remoteServerActive = remoteServerActive;
  }

  Int getBatteryCharging() const {
    return batteryCharging;
  }

  void setBatteryCharging(Int batteryCharging) {
    this->batteryCharging = batteryCharging;
  }

  Int getBatteryLevel() const {
    return batteryLevel;
  }

  void setBatteryLevel(Int batteryLevel) {
    this->batteryLevel = batteryLevel;
  }

  Int getRemoteBatteryCharging() const {
    return remoteBatteryCharging;
  }

  void setRemoteBatteryCharging(Int remoteBatteryCharging) {
    this->remoteBatteryCharging = remoteBatteryCharging;
  }

  Int getRemoteBatteryLevel() const {
    return remoteBatteryLevel;
  }

  void setRemoteBatteryLevel(Int remoteBatteryLevel) {
    this->remoteBatteryLevel = remoteBatteryLevel;
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
#include <ConfigSection.h>
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
#include <GraphicCircularStrip.h>
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
#include <WidgetCursorInfo.h>
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
#include <MapSourceRemote.h>
#include <NavigationPathTileInfo.h>
#include <NavigationPathVisualization.h>
#include <NavigationPath.h>
#include <NavigationPathSegment.h>
#include <NavigationPoint.h>
#include <NavigationPointVisualization.h>
#include <NavigationEngine.h>
#include <Commander.h>
#include <Device.h>

#endif /* MAIN_H_ */
