//============================================================================
// Name        : Main.cpp
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

// Constructor of the main application
Core::Core(std::string homePath, Int screenDPI) {

  // Set variables
  this->homePath=homePath;
  this->screenDPI=screenDPI;

  // Reset variables
  this->maintenanceThreadInfo=NULL;
  this->maintenanceMutex=NULL;
  this->lateInitThreadInfo=NULL;
  this->mapUpdateThreadInfo=NULL;
  this->mapUpdateInterruptMutex=NULL;
  this->screenUpdateInterruptMutex=NULL;
  this->mapUpdateStartSignal=NULL;
  this->mapUpdateTileTextureProcessedSignal=NULL;
  this->isInitializedMutex=NULL;
  this->isInitializedSignal=NULL;
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
  screen = NULL;
  graphicEngine = NULL;
  widgetEngine = NULL;
  mapCache = NULL;
  mapEngine = NULL;
  mapSource = NULL;
  unitConverter = NULL;
  image = NULL;
  commander = NULL;
  fontEngine = NULL;
  navigationEngine = NULL;

  // Create core objects that are required early
  if (!(thread=new Thread())) {
    puts("FATAL: can not create thread object!");
    exit(1);
    return;
  }
  isInitializedMutex=thread->createMutex();
  isInitializedSignal=thread->createSignal(true);
  thread->lockMutex(isInitializedMutex);
}

// Destructor of the main application
Core::~Core() {

  DEBUG("deinitializing core",NULL);

  // Wait until the core is initialized
  quitCore=true;
  thread->lockMutex(isInitializedMutex);
  isInitialized=false;
  thread->unlockMutex(isInitializedMutex);

  // Wait until the late init thread has finished
  if (lateInitThreadInfo) {
    thread->waitForThread(lateInitThreadInfo);
    thread->destroyThread(lateInitThreadInfo);
  }

  // Wait until the maintenance thread has finished
  maintenance(false);
  thread->lockMutex(maintenanceMutex);
  if (maintenanceThreadInfo)
    thread->destroyThread(maintenanceThreadInfo);

  // Wait until the map update thread has finished
  interruptMapUpdate();
  quitMapUpdateThread=true;
  continueMapUpdate();
  if (mapUpdateThreadInfo) {
    DEBUG("requesting map update thread to quit",NULL);
    thread->issueSignal(mapUpdateStartSignal);
    thread->issueSignal(mapUpdateTileTextureProcessedSignal);
    DEBUG("waiting until map update thread quits",NULL);
    thread->waitForThread(mapUpdateThreadInfo);
    DEBUG("map update thread has quitted",NULL);
    thread->destroyThread(mapUpdateThreadInfo);
  }

  // Free mutexes and signals
  if (maintenanceMutex) {
    thread->destroyMutex(maintenanceMutex);
  }
  if (screenUpdateInterruptMutex) {
    thread->destroyMutex(screenUpdateInterruptMutex);
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

  // Delete the components
  screen->setAllowDestroying(true);
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
  DEBUG("deleting widgetEngine",NULL);
  if (widgetEngine) delete widgetEngine;
  DEBUG("deleting graphicEngine",NULL);
  if (graphicEngine) delete graphicEngine;
  DEBUG("deleting fontEngine",NULL);
  if (fontEngine) delete fontEngine;
  DEBUG("deleting screen",NULL);
  if (screen) delete screen;
  DEBUG("deleting unit converter",NULL);
  if (unitConverter) delete unitConverter;
  DEBUG("deleting image",NULL);
  if (image) delete image;
  DEBUG("deleting dialog",NULL);
  if (dialog) delete dialog;
  DEBUG("deleting thread",NULL);
  if (thread) delete thread;
  DEBUG("deleting clock",NULL);
  if (clock) delete clock;
  DEBUG("deleting config",NULL);
  if (configStore) delete configStore;
  DEBUG("deleting profile engine",NULL);
  if (profileEngine) delete profileEngine;
  DEBUG("deleting debug",NULL);
  if (debug) delete debug;
}

// Starts the application
bool Core::init() {

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
  mapUpdateInterruptMutex=thread->createMutex();
  interruptMapUpdate();

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
  DEBUG("initializing screen",NULL);
  if (!(screen=new Screen(screenDPI))) {
    FATAL("can not create screen object",NULL);
    return false;
  }
  DEBUG("initializing fontEngine",NULL);
  if (!(fontEngine=new FontEngine())) {
    FATAL("can not create font engine object",NULL);
    return false;
  }
  DEBUG("initializing graphicEngine",NULL);
  if (!(graphicEngine=new GraphicEngine())) {
    FATAL("can not create graphic engine object",NULL);
    return false;
  }
  DEBUG("initializing widgetEngine",NULL);
  if (!(widgetEngine=new WidgetEngine())) {
    FATAL("can not create widget engine object",NULL);
    return false;
  }
  DEBUG("initializing mapSource",NULL);
  if (!(mapSource=MapSource::newMapSource())) {
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
  maintenancePeriod=configStore->getIntValue("General","maintenancePeriod");

  // Create mutexes and signals for thread communication
  mapUpdateStartSignal=thread->createSignal();
  mapUpdateTileTextureProcessedSignal=thread->createSignal();
  maintenanceMutex=thread->createMutex();

  // Create the map update thread
  mapUpdateThreadInfo=thread->createThread(mapUpdateThread,NULL);

  // Create the maintenance thread
  maintenanceThreadInfo=thread->createThread(maintenanceThread,NULL);

  // Create the late init thread
  lateInitThreadInfo=thread->createThread(lateInitThread,NULL);

  // We are initialized
  continueMapUpdate();
  thread->unlockMutex(isInitializedMutex);

  return true;

}

// Updates the screen
void Core::updateScreen(bool forceRedraw) {

  bool wakeupMapUpdateThread=false;

  // Interrupt the map update thread
  //interruptMapUpdate();

  // Check if all objects are initialized
  if ((mapSource->getIsInitialized())&&(navigationEngine->getIsInitialized())) {
    mapUpdateStopped=false;
  } else {
    mapUpdateStopped=true;
  }

  // Only work if the required objects are initialized
  if (!mapUpdateStopped) {

    // Init the map engine if required
    if (!mapEngine->getIsInitialized()) {
      mapCache->recreateGraphic();
      mapEngine->initMap();
      thread->lockMutex(isInitializedMutex);
      isInitialized=true;
      thread->unlockMutex(isInitializedMutex);
      core->getThread()->issueSignal(isInitializedSignal);
    }

    // Check if a new texture is ready
    if (mapCache->getTileTextureAvailable()) {
      mapCache->setNextTileTexture();
      thread->issueSignal(mapUpdateTileTextureProcessedSignal);
    }

  }

  // Redraw the scene
  graphicEngine->draw(forceRedraw);

  // Only work if the required objects are initialized
  if (!mapUpdateStopped) {

    // Get the status of the map update
    if (!mapEngine->getUpdateInProgress()) {
      wakeupMapUpdateThread=true;
    } else {

      // Abort the update if the pos has changed
      GraphicPosition visPos=*(graphicEngine->lockPos());
      if (mapEngine->mapUpdateIsRequired(visPos,NULL,NULL,NULL,false)) {
        mapEngine->setAbortUpdate();
      }
      graphicEngine->unlockPos();

    }
  }

  // Let the map update thread continue
  //DEBUG("before unlock of update interrupt mutex",NULL);
  //continueMapUpdate();
  //DEBUG("after unlock of update interrupt mutex",NULL);

  // Check if the map update thread is still running
  if (wakeupMapUpdateThread) {

    // Check if an update of the map is required
    bool updateRequired=false;
    GraphicPosition visPos=*(graphicEngine->lockPos());
    if (mapEngine->mapUpdateIsRequired(visPos))
      updateRequired=true;
    graphicEngine->unlockPos();

    // Request an update of the map
    //DEBUG("waking up map update thread",NULL);
    if (updateRequired)
      thread->issueSignal(mapUpdateStartSignal);

  }

}

// Main loop of the map update thread
void Core::updateMap() {

  // This is a background thread
  core->getThread()->setThreadPriority(threadPriorityBackgroundHigh);

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
    thread->lockMutex(mapUpdateInterruptMutex);
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
    interruptMapUpdate();
    if ((!mapEngine->getUpdateInProgress())&&(mapUpdateStopped)) {
      wait=false;
    }
    continueMapUpdate();
  }
  while (wait);

  // Take care that the maintenance thread detects that objects are not initialized
  thread->lockMutex(maintenanceMutex);
  thread->unlockMutex(maintenanceMutex);

  // Initialize map source if required
  if (!mapSource->getIsInitialized()) {
    DEBUG("late initializing mapSource",NULL);
    mapSource->init();
  }

  // Stop here if quit requested
  if (quitCore)
    return;

  // Initialize navigation engine if required
  if (!navigationEngine->getIsInitialized()) {
    DEBUG("late initializing navigationEngine",NULL);
    navigationEngine->init();
  }

  DEBUG("late initialization finished",NULL);

  // If you add other objects, please update also
  // the mapUpdateStopped and mapEngine init code in
  // the updateScreen method

}

// Main loop of the maintenance thread
void Core::maintenance(bool endlessLoop) {

  // Set the priority
  core->getThread()->setThreadPriority(threadPriorityBackgroundLow);

  // Do an endless loop
  while (1) {

    // Ensure that only one thread is executing this at most
    thread->lockMutex(maintenanceMutex);

    // Do the backup
    if (navigationEngine->getIsInitialized())
      navigationEngine->backup();
    if (mapEngine->getIsInitialized()) {
      mapEngine->backup();
    }

    // Optimize the graphic
    if (navigationEngine->getIsInitialized()) {
      navigationEngine->optimizeGraphic();
    }

    // Output the profiling result
#ifdef PROFILING_ENABLED
    core->getProfileEngine()->outputResult("",true);
    core->getGraphicEngine()->outputStats();
#endif

    // Call the maintenance in the map source
    if (mapSource->getIsInitialized()) {
      mapSource->maintenance();
    }

    // Other threads can access now
    thread->unlockMutex(maintenanceMutex);

    // Wait for the next maintenance slot
    if (endlessLoop)
      sleep(maintenancePeriod);
    else
      return;

  }

}

// Allows an external interrupt
void Core::interruptAllowedHere() {
  if (noInterruptAllowed==false) {
    thread->unlockMutex(mapUpdateInterruptMutex);
    //sleep(1); // enable to get debug rectangles in the map
    thread->lockMutex(mapUpdateInterruptMutex);
  }
}

// Indicates the graphics engine that a new texture is ready
void Core::tileTextureAvailable() {
  thread->unlockMutex(mapUpdateInterruptMutex);
  if (!quitMapUpdateThread)
    thread->waitForSignal(mapUpdateTileTextureProcessedSignal);
  thread->lockMutex(mapUpdateInterruptMutex);
}

// Called if the textures or buffers have been lost or must be recreated
void Core::updateGraphic(bool graphicInvalidated) {

  // Ensure that the core is not currently being initialized
  thread->lockMutex(isInitializedMutex);

  // Wait until the map update thread is in a clean state
  bool mapUpdateThreadFinished=false;
  do {

    // Interrupt the map update thread
    interruptMapUpdate();

    // Check if a new texture is ready
    if (mapCache->getTileTextureAvailable()) {
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

  DEBUG("recreating graphic textures and buffers",NULL);

  // First deinit everything
  screen->setAllowDestroying(true);
  if (graphicInvalidated)
    screen->graphicInvalidated();
  screen->recreateGraphic();
  fontEngine->recreateGraphic();
  graphicEngine->recreateGraphic();
  widgetEngine->recreateGraphic();
  if (isInitialized)
    mapCache->recreateGraphic();
  navigationEngine->recreateGraphic();
  screen->setAllowDestroying(false);

  // Trigger an update of the map
  mapEngine->setForceMapUpdate();

  // Let the map update thread continue
  continueMapUpdate();

  // Let the init continue
  thread->unlockMutex(isInitializedMutex);
}

// Stops the map update thread
void Core::interruptMapUpdate() {
  thread->lockMutex(mapUpdateInterruptMutex);
  noInterruptAllowed=true;
}

// Continue the map update thread
void Core::continueMapUpdate() {
  noInterruptAllowed=false;
  thread->unlockMutex(mapUpdateInterruptMutex);
}

}
