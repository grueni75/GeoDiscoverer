//============================================================================
// Name        : Main.cpp
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

#include <Core.h>

namespace GEODISCOVERER {

// Global handle to the application
Core *core=NULL;

// Map update thread
void *mapUpdateThread(void *args) {
  core->updateMap();
  return NULL;
}

// Maintenance thread
void *maintenanceThread(void *args) {
  core->maintenance();
  return NULL;
}

// Late init thread
void *lateInitThread(void *args) {
  core->lateInit();
  return NULL;
}

// Dashboard thread
void *updateDashboardScreensThread(void *args) {
  core->updateDashboardScreens();
  return NULL;
}

// Constructor of the main application
Core::Core(std::string homePath, Int screenDPI, double screenDiagonal) {

  // Set variables
  this->homePath=homePath;
  this->defaultScreenDPI=screenDPI;
  this->defaultScreenDiagonal=screenDiagonal;

  // Reset variables
  this->maintenanceThreadInfo=NULL;
  this->maintenanceMutex=NULL;
  this->lateInitThreadInfo=NULL;
  this->mapUpdateThreadInfo=NULL;
  this->updateDashboardScreensThreadInfo=NULL;
  this->mapUpdateInterruptMutex=NULL;
  this->mapUpdateStartSignal=NULL;
  this->mapUpdateTileTextureProcessedSignal=NULL;
  this->isInitializedMutex=NULL;
  this->isInitializedSignal=NULL;
  this->maintenancePeriod=0;
  this->dashboardDevicesMutex=NULL;
  this->updateDashboardScreensWakeupSignal=NULL;
  this->defaultDevice=NULL;
  isInitialized=false;
  quitMapUpdateThread=false;
  mapUpdateStopped=true;
  noInterruptAllowed=false;
  quitCore=false;
  debug = NULL;
  profileEngine = NULL;
  configStore = NULL;
  thread = NULL;
  dialog = NULL;
  clock = NULL;
  mapCache = NULL;
  mapEngine = NULL;
  mapSource = NULL;
  unitConverter = NULL;
  image = NULL;
  commander = NULL;
  navigationEngine = NULL;
  fileAccessRetries = 10;
  fileAccessWaitTime = 100;
  quitUpdateDashboardScreensThread = false;
  remoteServerActive=false;
  batteryCharging=false;
  batteryLevel=0;
  remoteBatteryCharging=false;
  remoteBatteryLevel=0;

  // Create core objects that are required early
  if (!(thread=new Thread())) {
    puts("FATAL: can not create thread object!");
    exit(1);
    return;
  }
  isInitializedMutex=thread->createMutex("core is initialized mutex");
  isInitializedSignal=thread->createSignal(true);
  thread->lockMutex(isInitializedMutex, __FILE__, __LINE__);
}

// Destructor of the main application
Core::~Core() {

  DEBUG("deinitializing core",NULL);

  // Wait until the core is initialized
  quitCore=true;
  thread->lockMutex(isInitializedMutex, __FILE__, __LINE__);
  isInitialized=false;
  thread->unlockMutex(isInitializedMutex);

  // Wait until the late init thread has finished
  if (lateInitThreadInfo) {
    thread->waitForThread(lateInitThreadInfo);
    thread->destroyThread(lateInitThreadInfo);
  }

  // Wait until the update dashboard screens thread has finished
  if (updateDashboardScreensThreadInfo) {
    quitUpdateDashboardScreensThread=true;
    thread->issueSignal(updateDashboardScreensWakeupSignal);
    thread->waitForThread(updateDashboardScreensThreadInfo);
    thread->destroyThread(updateDashboardScreensThreadInfo);
  }

  // Wait until the maintenance thread has finished
  maintenance(false);
  thread->lockMutex(maintenanceMutex, __FILE__, __LINE__);
  if (maintenanceThreadInfo) {
    core->getThread()->cancelThread(maintenanceThreadInfo);
    core->getThread()->waitForThread(maintenanceThreadInfo);
    core->getThread()->destroyThread(maintenanceThreadInfo);
  }

  // Wait until the map update thread has finished
  interruptMapUpdate(__FILE__, __LINE__);
  quitMapUpdateThread=true;
  continueMapUpdate();
  if (mapUpdateThreadInfo) {
    DEBUG("requesting map update thread to quit",NULL);
    thread->issueSignal(mapUpdateStartSignal);
    thread->issueSignal(mapUpdateTileTextureProcessedSignal);
    DEBUG("waiting until map update thread quits",NULL);
    thread->waitForThread(mapUpdateThreadInfo);
    DEBUG("map update thread has quitted",NULL);
    if (mapEngine->getUpdateInProgress()) {
      FATAL("something is wrong: map update thread should have quit but map engine still indicates update of map",NULL);
    }
    thread->destroyThread(mapUpdateThreadInfo);
  }

  // Ensure that all downloads have stopped
  if (mapSource) {
    mapSource->stopDownloadThreads();
  }

  // Delete the components
  getDefaultDevice()->getScreen()->setAllowDestroying(true);
  DEBUG("deleting commander",NULL);
  if (commander) delete commander;
  DEBUG("deleting navigationEngine",NULL);
  if (navigationEngine) delete navigationEngine;
  DEBUG("deleting mapEngine",NULL);
  if (mapEngine) delete mapEngine;
  DEBUG("deleting mapCache",NULL);
  if (mapCache) delete mapCache;
  DEBUG("deleting mapSource",NULL);
  if (mapSource) delete mapSource;
  DEBUG("deleting default device",NULL);
  delete defaultDevice;
  DEBUG("deleting unit converter",NULL);
  if (unitConverter) delete unitConverter;
  DEBUG("deleting image",NULL);
  if (image) delete image;
  DEBUG("deleting dialog",NULL);
  if (dialog) delete dialog;
  DEBUG("deleting clock",NULL);
  if (clock) delete clock;
  DEBUG("deleting config",NULL);
  if (configStore) delete configStore;
  DEBUG("deleting profile engine",NULL);
  if (profileEngine) delete profileEngine;

  // Free mutexes and signals
  DEBUG("deleting mutexes and signals",NULL);
  if (maintenanceMutex) {
    thread->destroyMutex(maintenanceMutex);
  }
  if (mapUpdateInterruptMutex) {
    thread->destroyMutex(mapUpdateInterruptMutex);
  }
  if (mapUpdateStartSignal) {
    thread->destroySignal(mapUpdateStartSignal);
  }
  if (mapUpdateTileTextureProcessedSignal) {
    thread->destroySignal(mapUpdateTileTextureProcessedSignal);
  }
  if (isInitializedMutex) {
    thread->destroyMutex(isInitializedMutex);
  }
  if (isInitializedSignal) {
    thread->destroySignal(isInitializedSignal);
  }
  if (dashboardDevicesMutex) {
    thread->destroyMutex(dashboardDevicesMutex);
  }
  if (updateDashboardScreensWakeupSignal) {
    thread->destroySignal(updateDashboardScreensWakeupSignal);
  }

  // Do the remaining stuff
  DEBUG("deleting debug",NULL);
  if (debug) delete debug; debug=NULL;
  DEBUG("deleting thread",NULL);
  if (thread) delete thread;
}

// Starts the applicationcreateTh
bool Core::init() {

  // Init cURL
  initCURL();

  // Create components
  if (!(clock=new Clock())) {
    puts("FATAL: can not create clock object!");
    return false;
  }
  if (!(debug=new Debug())) {
    puts("FATAL: can not create debug object!");
    return false;
  }
  /*
  DEBUG("core=0x%08x",core);
  DEBUG("clock=0x%08x",core->getClock());
  TimestampInSeconds t=core->getClock()->getSecondsSinceEpoch();
  DEBUG("seconds since epoch obtained",NULL);
  TimestampInMicroseconds t2=core->getClock()->getMicrosecondsSinceStart();
  DEBUG("microseconds since start obtained",NULL);
  std::string test=core->getClock()->getFormattedDate();
  DEBUG("string obtained",NULL);
  DEBUG("formattedDate=%s",test.c_str());
  */

  // Ensure that the core is not deinitialized before its initialized
  mapUpdateInterruptMutex=thread->createMutex("core map update interrupt mutex");
  interruptMapUpdate(__FILE__, __LINE__);

  // Continue creating components
  DEBUG("initializing profileEngine",NULL);
  if (!(profileEngine=new ProfileEngine())) {
    FATAL("can not create profile engine object",NULL);
    return false;
  }
  DEBUG("initializing config",NULL);
  if (!(configStore=new ConfigStore())) {
    FATAL("can not create config object",NULL);
    return false;
  }
  debug->init(); // open the trace and debug log
  DEBUG("initializing image",NULL);
  if (!(image=new Image())) {
    FATAL("can not create image object",NULL);
    return false;
  }
  DEBUG("initializing unit converter",NULL);
  if (!(unitConverter=new UnitConverter())) {
    FATAL("can not create unit converter object",NULL);
    return false;
  }
  DEBUG("initializing dialog",NULL);
  if (!(dialog=new Dialog())) {
    FATAL("can not create dialog object",NULL);
    return false;
  }
  std::string name=Commander::dispatch("getDeviceName()");
  DEBUG("initializing default device with name <%s>",name.c_str());
  if (!(defaultDevice=new Device(name,false,true))) {
    FATAL("can not create default device object",NULL);
    return false;
  }
  defaultDevice->setDPI(defaultScreenDPI);
  defaultDevice->setDiagonal(defaultScreenDiagonal);
  defaultDevice->init();
  defaultDevice->setInitDone();
  DEBUG("initializing mapSource",NULL);
  if (defaultDevice->getName()=="Watch") {
    mapSource=new MapSourceRemote();
  } else {
    mapSource=MapSource::newMapSource();
  }
  if (!mapSource) {
    FATAL("can not create map source object",NULL);
    return false;
  }
  DEBUG("initializing mapCache",NULL);
  if (!(mapCache=new MapCache())) {
    FATAL("can not create map cache object",NULL);
    return false;
  }
  DEBUG("initializing mapEngine",NULL);
  if (!(mapEngine=new MapEngine())) {
    FATAL("can not create map engine object",NULL);
    return false;
  }
  DEBUG("initializing navigationEngine",NULL);
  if (!(navigationEngine=new NavigationEngine())) {
    FATAL("can not create navigation engine object",NULL);
    return false;
  }
  DEBUG("initializing commander",NULL);
  if (!(commander=new Commander())) {
    FATAL("can not create commander object",NULL);
    return false;
  }

  // Get config
  maintenancePeriod=configStore->getIntValue("General","maintenancePeriod", __FILE__, __LINE__);
  fileAccessRetries=configStore->getIntValue("General","fileAccessRetries", __FILE__, __LINE__);
  fileAccessWaitTime=configStore->getIntValue("General","fileAccessWaitTime", __FILE__, __LINE__);

  // Create mutexes and signals for thread communication
  mapUpdateStartSignal=thread->createSignal();
  mapUpdateTileTextureProcessedSignal=thread->createSignal();
  maintenanceMutex=thread->createMutex("core maintenance mutex");
  dashboardDevicesMutex=thread->createMutex("devices mutex");
  updateDashboardScreensWakeupSignal=thread->createSignal(false);

  // Create the map update thread
  mapUpdateThreadInfo=thread->createThread("map update thread",mapUpdateThread,NULL);

  // Create the maintenance thread
  maintenanceThreadInfo=thread->createThread("maintenance thread",maintenanceThread,NULL);

  // Create the late init thread
  lateInitThreadInfo=thread->createThread("late init thread",lateInitThread,NULL);

  // Create the update dashboard screens thread
  if (configStore->getIntValue("Cockpit/App/Dashboard","active",__FILE__,__LINE__)) {
    updateDashboardScreensThreadInfo=thread->createThread("update dashboard screens thread",updateDashboardScreensThread,NULL);
  }

  // We are initialized
  continueMapUpdate();
  thread->unlockMutex(isInitializedMutex);

  return true;

}

// Updates the screen
void Core::updateScreen(bool forceRedraw) {

  bool wakeupMapUpdateThread=false;

  PROFILE_START;

  // Allow texture allocation
  getDefaultDevice()->getScreen()->setAllowAllocation(true);

  // Check if all objects are initialized
  if ((mapSource->getIsInitialized())&&(navigationEngine->getIsInitialized())&&(mapEngine->getIsInitialized())) {
    mapUpdateStopped=false;
  } else {
    mapUpdateStopped=true;
  }

  // Only work if the required objects are initialized
  if (!mapUpdateStopped) {

    // Init the map engine if required
    if (!mapCache->getIsInitialized()) {
      mapCache->createGraphic();
      PROFILE_ADD("init");
    }

    // Check if a new texture is ready
    if (mapCache->getTileTextureAvailable()) {
      mapCache->setNextTileTexture();
      thread->issueSignal(mapUpdateTileTextureProcessedSignal);
      PROFILE_ADD("tile texture transfer");
    }

  }
  PROFILE_ADD("pre draw");

  /* Redraw the scene
  for (std::list<Device*>::iterator i=devices.begin();i!=devices.end();i++) {
    (*i)->getGraphicEngine()->draw(forceRedraw);
  }*/
  getDefaultDevice()->getGraphicEngine()->draw(forceRedraw);
  PROFILE_ADD("draw");

  // Only work if the required objects are initialized
  if (!mapUpdateStopped) {

    // Get the status of the map update
    if (!mapEngine->getUpdateInProgress()) {
      wakeupMapUpdateThread=true;
    } else {

      // Abort the update if the pos has changed
      GraphicPosition visPos=*(getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__));
      getDefaultGraphicEngine()->unlockPos();
      if (mapEngine->mapUpdateIsRequired(visPos,NULL,NULL,NULL,false)) {
        mapEngine->setAbortUpdate();
      }
    }
    PROFILE_ADD("map update abort check");
  }

  // Let the map update thread continue
  //DEBUG("before unlock of update interrupt mutex",NULL);
  //continueMapUpdate();
  //DEBUG("after unlock of update interrupt mutex",NULL);

  // Check if the map update thread is still running
  //DEBUG("wakeupMapUpdateThread=%d",wakeupMapUpdateThread);
  if (wakeupMapUpdateThread) {

    // Check if an update of the map is required
    bool updateRequired=false;
    GraphicPosition visPos=*(getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__));
    getDefaultGraphicEngine()->unlockPos();
    if ((mapEngine->mapUpdateIsRequired(visPos))||(navigationEngine->mapGraphicUpdateIsRequired(__FILE__,__LINE__))) {
      updateRequired=true;
    }

    // Request an update of the map
    //DEBUG("waking up map update thread",NULL);
    if (updateRequired) {
      thread->issueSignal(mapUpdateStartSignal);
    }
    PROFILE_ADD("map update trigger check");

  }

  // Disallow texture allocation
  getDefaultDevice()->getScreen()->setAllowAllocation(false);
  PROFILE_ADD("post draw");
  PROFILE_END;
}

// Adds a new dashboard device
void Core::addDashboardDevice(std::string host, Int port) {

  // First check if device already exists
  std::stringstream name;
  name << host << ":" << port;
  core->getThread()->lockMutex(dashboardDevicesMutex,__FILE__,__LINE__);
  for (std::list<Device*>::iterator i=dashboardDevices.begin();i!=dashboardDevices.end();i++) {
    Device *d = *i;
    if (d->getName()==name.str()) {
      DEBUG("device %s is not added because it already exists",name.str().c_str());
      core->getThread()->unlockMutex(dashboardDevicesMutex);
      return;
    }
  }
  core->getThread()->unlockMutex(dashboardDevicesMutex);

  // Create the device
  Device *d;
  if (!(d=new Device(name.str(),configStore->getIntValue("Cockpit/App/Dashboard","whiteBackground",__FILE__,__LINE__),false))) {
    FATAL("can not create dashboard device object",NULL);
    return;
  }
  d->setHost(host);
  d->setPort(port);
  d->setDPI(configStore->getIntValue("Cockpit/App/Dashboard","dotsPerInch",__FILE__,__LINE__));

  // Check if the device is reachable
  if (!(d->openSocket())) {
    //DEBUG("dashboard device %s is not reachable",name.str().c_str());
    delete d;
    return;
  }
  d->closeSocket();

  // Add the device
  core->getThread()->lockMutex(dashboardDevicesMutex,__FILE__,__LINE__);
  DEBUG("adding device %04x",d);
  dashboardDevices.push_back(d);
  core->getThread()->unlockMutex(dashboardDevicesMutex);
  DEBUG("dashboard device %s has been added",name.str().c_str());
}

// Updates the screen of cokcpits
void Core::updateDashboardScreens() {

  // Set the priority
  core->getThread()->setThreadPriority(threadPriorityBackgroundHigh);

  DEBUG("updateDashboardScreens thread started",NULL);

  // Get config values
  TimestampInMilliseconds minUpdatePeriodNormal = configStore->getIntValue("Cockpit","minUpdatePeriodNormal",__FILE__,__LINE__);
  TimestampInMilliseconds minUpdatePeriodTurn = configStore->getIntValue("Cockpit","minUpdatePeriodTurn",__FILE__,__LINE__);

  // Repeat until quit is requested
  bool contextShutdown=true;
  while (!quitUpdateDashboardScreensThread) {

    // Update the update interval depending if a turn is near or not
    bool turnActive;
    TimestampInMicroseconds sleepTime;
    NavigationInfo *info=navigationEngine->lockNavigationInfo(__FILE__,__LINE__);
    turnActive=(info->getTurnDistance()!=NavigationInfo::getUnknownDistance());
    navigationEngine->unlockNavigationInfo();
    if (turnActive)
      sleepTime=minUpdatePeriodTurn;
    else
      sleepTime=minUpdatePeriodNormal;

    //DEBUG("sleepTime=%d",sleepTime);

    // Get all devices
    core->getThread()->lockMutex(dashboardDevicesMutex,__FILE__,__LINE__);
    std::list<Device*> devices = dashboardDevices;
    core->getThread()->unlockMutex(dashboardDevicesMutex);

    // Create a new EGL context if required
    bool createGraphic=false;
    if ((contextShutdown)&&(devices.size()>0)) {
      DEBUG("creating EGL context",NULL);
      contextShutdown=false;
      createGraphic=true;
      if (!Screen::setupContext())
        return;
    }

    // Go through all devices
    std::list<Device*> devicesToBeRemoved;
    for (std::list<Device*>::iterator i=devices.begin();i!=devices.end();i++) {
      Device *d = *i;

      // Recreate the graphic if context was just set up
      if (createGraphic) {
        d->createGraphic();
      }

      // Discover the device infos
      bool deviceAvailable=false;
      if (d->discover()) {

        // Do the drawing
        if (d->draw()) {
          deviceAvailable=true;
        }
      }

      // Remove the device if it is not available anymore
      if (!deviceAvailable) {
        devicesToBeRemoved.push_back(d);
      }
    }

    // Shutdown the context if devices were removed to prevent resource leaks
    core->getThread()->lockMutex(dashboardDevicesMutex,__FILE__,__LINE__);
    if (devicesToBeRemoved.size()>0) {
      for (std::list<Device*>::iterator i=dashboardDevices.begin();i!=dashboardDevices.end();i++) {
        Device *d=*i;
        DEBUG("destroying device 0x%04x",d);
        d->destroyGraphic(false);
      }
      DEBUG("shutting down EGL context",NULL);
      Screen::shutdownContext();
      contextShutdown=true;
    }

    // Remove any marked device
    for (std::list<Device*>::iterator i=devicesToBeRemoved.begin();i!=devicesToBeRemoved.end();i++) {
      Device *d=*i;
      dashboardDevices.remove(d);
      delete d;
    }
    devicesToBeRemoved.clear();
    core->getThread()->unlockMutex(dashboardDevicesMutex);

    // Sleep for the next round
    core->getThread()->waitForSignal(updateDashboardScreensWakeupSignal,sleepTime);
  }

  // Destroy the screens of the devices
  core->getThread()->lockMutex(dashboardDevicesMutex,__FILE__,__LINE__);
  for (std::list<Device*>::iterator i=dashboardDevices.begin();i!=dashboardDevices.end();i++) {
    Device *d = *i;
    d->destroyGraphic(false);
    delete d;
  }
  core->getThread()->unlockMutex(dashboardDevicesMutex);
  dashboardDevices.clear();

  // Destroy the EGL context
  if (!contextShutdown)
    Screen::shutdownContext();
}

// Main loop of the map update thread
void Core::updateMap() {

  // This is a background thread
  core->getThread()->setThreadPriority(threadPriorityForegroundLow);

  // Do an endless loop
  while (1) {

    // Wait for an update trigger
    thread->waitForSignal(mapUpdateStartSignal);

    // Shall we quit?
    // It's important to check that here to avoid deadlocks
    if (getQuitMapUpdateThread()) {
      DEBUG("quit requested",NULL);
      thread->exitThread();
    }

    // Do the map update
    thread->lockMutex(mapUpdateInterruptMutex, __FILE__, __LINE__);
    mapEngine->updateMap();
    thread->unlockMutex(mapUpdateInterruptMutex);

  }

}

// Does a late initialization of certain objects
void Core::lateInit() {

  // Set the priority
  core->getThread()->setThreadPriority(threadPriorityBackgroundHigh);

  // Take care that the map update and screen update thread detects that objects are not initialized
  bool wait=true;
  while (wait) {
    interruptMapUpdate(__FILE__, __LINE__);
    if ((!mapEngine->getUpdateInProgress())&&(mapUpdateStopped)) {
      wait=false;
    }
    continueMapUpdate();
  }
  while (wait);

  // Take care that the maintenance thread detects that objects are not initialized
  thread->lockMutex(maintenanceMutex, __FILE__, __LINE__);
  thread->unlockMutex(maintenanceMutex);

  // Initialize map source if required
  if (!mapSource->getIsInitialized()) {
    DEBUG("late initializing mapSource",NULL);
    if (!mapSource->init()) {
      return;
    }
  }

  // Stop here if quit requested
  if (quitCore)
    return;

  // Initialize navigation engine if required
  if (!navigationEngine->getIsInitialized()) {
    DEBUG("late initializing navigationEngine",NULL);
    navigationEngine->init();
  }

  // Initialize map engine if required
  if (!mapEngine->getIsInitialized()) {
    DEBUG("late initializing mapEngine",NULL);
    mapEngine->initMap();
  }

  DEBUG("late initialization finished",NULL);

  // If you add other objects, please update also
  // the mapUpdateStopped and the init code in
  // the updateScreen method

  // We are initialized
  thread->lockMutex(isInitializedMutex,__FILE__,__LINE__);
  isInitialized=true;
  thread->unlockMutex(isInitializedMutex);
  core->getThread()->issueSignal(isInitializedSignal);
  core->getCommander()->dispatch("lateInitComplete()");
}

// Main loop of the maintenance thread
void Core::maintenance(bool endlessLoop) {

  // If this is called in an endless loop, set the thread parameters
  if (endlessLoop) {
    core->getThread()->setThreadPriority(threadPriorityBackgroundLow);
    core->getThread()->setThreadCancable();
  }

  // Do an endless loop
  while (1) {

    // Ensure that only one thread is executing this at most
    thread->lockMutex(maintenanceMutex, __FILE__, __LINE__);

    //DEBUG("performing maintenance",NULL);

    // Do the backup
    if ((navigationEngine)&&(navigationEngine->getIsInitialized()))
      navigationEngine->backup();
    if (mapEngine->getIsInitialized()) {
      mapEngine->backup();
    }

    // Optimize the graphic
    if ((navigationEngine)&&(navigationEngine->getIsInitialized())) {
      navigationEngine->optimizeGraphic();
    }

    // Output the profiling result
#ifdef PROFILING_ENABLED
    core->getProfileEngine()->outputResult("",false);
    core->getDefaultGraphicEngine()->outputStats();
#endif

    // Call the maintenance in the map source
    if ((mapSource)&&(mapSource->getIsInitialized())) {
      mapSource->maintenance();
    }

    // Other threads can access now
    thread->unlockMutex(maintenanceMutex);

    // Wait for the next maintenance slot
    if (endlessLoop) {
      TimestampInSeconds sleepEndTime=core->getClock()->getSecondsSinceEpoch()+maintenancePeriod;
      TimestampInSeconds duration=1;
      while(duration>0) {
        duration=sleepEndTime-core->getClock()->getSecondsSinceEpoch();
        if (duration>0)
          sleep(duration);
      }
    } else
      return;

  }

}

// Allows an external interrupt
void Core::interruptAllowedHere(const char *file, int line) {
  if (noInterruptAllowed==false) {
    thread->unlockMutex(mapUpdateInterruptMutex);
    //sleep(1); // enable to get debug rectangles in the map
    thread->lockMutex(mapUpdateInterruptMutex, file, line);
  }
}

// Indicates the graphics engine that a new texture is ready
void Core::tileTextureAvailable(const char *file, int line) {
  thread->unlockMutex(mapUpdateInterruptMutex);
  if (!quitMapUpdateThread)
    thread->waitForSignal(mapUpdateTileTextureProcessedSignal);
  thread->lockMutex(mapUpdateInterruptMutex, file, line);
}

// Called if the textures or buffers have been lost or must be recreated
void Core::updateGraphic(bool graphicInvalidated, bool destroyOnly) {

  // Ensure that the core is not currently being initialized
  thread->lockMutex(isInitializedMutex, __FILE__, __LINE__);

  // Wait until the map update thread is in a clean state
  bool mapUpdateThreadFinished=false;
  do {

    // Interrupt the map update thread
    interruptMapUpdate(__FILE__, __LINE__);

    // Check if a new texture is ready
    if (mapCache->getTileTextureAvailable()) {
      mapCache->setNextTileTexture();
      thread->issueSignal(mapUpdateTileTextureProcessedSignal);
    }

    // Abort any ongoing cache update
    if (mapEngine->getUpdateInProgress()) {
      //DEBUG("waiting until map engine has finished working",NULL);
      mapEngine->setAbortUpdate();
      mapUpdateThreadFinished=false;
      continueMapUpdate();
    } else {
      //DEBUG("map engine is not working",NULL);
      mapUpdateThreadFinished=true;
    }

  }
  while (!mapUpdateThreadFinished);

  if (graphicInvalidated) {
    DEBUG("recreating graphic textures and buffers with graphic invalidation",NULL);
  } else {
    DEBUG("recreating graphic textures and buffers without graphic invalidation",NULL);
  }

  // First deinit everything
  Device *d=getDefaultDevice();
  d->getScreen()->setAllowDestroying(true);
  navigationEngine->destroyGraphic();
  mapSource->destroyGraphic();
  if (isInitialized)
    mapCache->destroyGraphic();
  d->getGraphicEngine()->destroyGraphic();
  d->getScreen()->destroyGraphic();
  d->getWidgetEngine()->destroyGraphic();
  d->getFontEngine()->destroyGraphic();
  if (graphicInvalidated) {
    d->getScreen()->graphicInvalidated(true);
  }
  d->getScreen()->setAllowDestroying(false);
  if (!destroyOnly) {
    d->getScreen()->setAllowAllocation(true);
    d->getScreen()->createGraphic();
    d->getFontEngine()->createGraphic();
    d->getWidgetEngine()->createGraphic();
    d->getGraphicEngine()->createGraphic();
    if (isInitialized)
      mapCache->createGraphic();
    mapSource->createGraphic();
    navigationEngine->createGraphic();
    d->getScreen()->setAllowAllocation(false);
  }

  // Trigger an update of the map
  mapEngine->setForceMapRecreation();

  // Let the map update thread continue
  continueMapUpdate();

  // Let the init continue
  thread->unlockMutex(isInitializedMutex);
}

// Stops the map update thread
void Core::interruptMapUpdate(const char *file, int line) {
  thread->lockMutex(mapUpdateInterruptMutex, file, line);
  noInterruptAllowed=true;
}

// Continue the map update thread
void Core::continueMapUpdate() {
  noInterruptAllowed=false;
  thread->unlockMutex(mapUpdateInterruptMutex);
}

// Called when the process is killed
void Core::unload() {
  ConfigStore::unload();
}

// Waits until the file is available and get its stats
Int Core::statFile(std::string path, struct stat *buffer) {
  Int result = 1;
  for (int i=0;i<fileAccessRetries;i++) {
    result=stat(path.c_str(),buffer);
    if (result) {
      if (errno==EACCES) {
        usleep(fileAccessWaitTime);
      } else {
        return result;
      }
    } else {
      return result;
    }
  }
  DEBUG("max retries reached but file still not available",NULL);
  return result;
}

// Waits until the directory is available and open it
DIR *Core::openDir(std::string path) {
  DIR *dfd;
  for (int i=0;i<fileAccessRetries;i++) {
    dfd=opendir(path.c_str());
    if (!dfd) {
      usleep(fileAccessWaitTime);
    } else {
      return dfd;
    }
  }
  DEBUG("max retries reached but directory still not available",NULL);
  return NULL;
}


// Returns the default screen
Screen *Core::getDefaultScreen() {
  return defaultDevice->getScreen();
}

// Returns the default device
Device *Core::getDefaultDevice() {
  return defaultDevice;
}

// Returns the default widget engine
WidgetEngine *Core::getDefaultWidgetEngine() {
  return defaultDevice->getWidgetEngine();
}

// Returns the default graphic engine
GraphicEngine *Core::getDefaultGraphicEngine() {
  return defaultDevice->getGraphicEngine();
}

// Informs the engines that the map has changed
void Core::onMapChange(MapPosition pos, std::list<MapTile*> *centerMapTiles) {
  defaultDevice->getWidgetEngine()->onMapChange(pos,centerMapTiles);
  core->getThread()->lockMutex(dashboardDevicesMutex,__FILE__,__LINE__);
  for (std::list<Device*>::iterator i=dashboardDevices.begin();i!=dashboardDevices.end();i++) {
    if (((*i)->isInitDone())&&((*i)->getWidgetEngine())) {
      (*i)->getWidgetEngine()->onMapChange(pos,centerMapTiles);
    }
  }
  core->getThread()->unlockMutex(dashboardDevicesMutex);
}

// Informs the engines that the location has changed
void Core::onLocationChange(MapPosition mapPos) {
  defaultDevice->getWidgetEngine()->onLocationChange(mapPos);
  core->getThread()->lockMutex(dashboardDevicesMutex,__FILE__,__LINE__);
  for (std::list<Device*>::iterator i=dashboardDevices.begin();i!=dashboardDevices.end();i++) {
    if (((*i)->isInitDone())&&((*i)->getWidgetEngine())) {
      (*i)->getWidgetEngine()->onLocationChange(mapPos);
    }
  }
  core->getThread()->unlockMutex(dashboardDevicesMutex);
}

// Informs the engines that a path has changed
void Core::onPathChange(NavigationPath *path, NavigationPathChangeType changeType) {
  defaultDevice->getWidgetEngine()->onPathChange(path,changeType);
  core->getThread()->lockMutex(dashboardDevicesMutex,__FILE__,__LINE__);
  for (std::list<Device*>::iterator i=dashboardDevices.begin();i!=dashboardDevices.end();i++) {
    if (((*i)->isInitDone())&&((*i)->getWidgetEngine())) {
      (*i)->getWidgetEngine()->onPathChange(path,changeType);
    }
  }
  core->getThread()->unlockMutex(dashboardDevicesMutex);
}

// Informs the engines that some data has changed
void Core::onDataChange() {
  defaultDevice->getWidgetEngine()->onDataChange();
  core->getThread()->lockMutex(dashboardDevicesMutex,__FILE__,__LINE__);
  for (std::list<Device*>::iterator i=dashboardDevices.begin();i!=dashboardDevices.end();i++) {
    if (((*i)->isInitDone())&&((*i)->getWidgetEngine())) {
      (*i)->getWidgetEngine()->onDataChange();
    }
  }
  core->getThread()->unlockMutex(dashboardDevicesMutex);
}


}
