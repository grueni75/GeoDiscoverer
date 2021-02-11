//============================================================================
// Name        : NavigationEngine.cpp
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
#include <cstring>
#include <NavigationEngine.h>
#include <GraphicEngine.h>
#include <NavigationPath.h>
#include <Commander.h>
#include <MapEngine.h>
#include <Integer.h>
#include <Device.h>
#include <UnitConverter.h>
#include <Storage.h>

namespace GEODISCOVERER {

// Background loader thread
void *navigationEngineBackgroundLoaderThread(void *args) {
  ((NavigationEngine*)args)->backgroundLoader();
  return NULL;
}

// Compute navigation info thread
void *navigationEngineComputeNavigationInfoThread(void *args) {
  ((NavigationEngine*)args)->computeNavigationInfo();
  return NULL;
}

// Google Maps bookmark synchronization thread
void *navigationEngineSynchronizeGoogleBookmarksThread(void *args) {
  ((NavigationEngine*)args)->synchronizeGoogleBookmarks();
  return NULL;
}

// Constructor
NavigationEngine::NavigationEngine() :
  navigationPointsGraphicObject(core->getDefaultScreen(),true)
{

  // Init variables
  locationOutdatedThreshold=core->getConfigStore()->getIntValue("Navigation","locationOutdatedThreshold", __FILE__, __LINE__);
  locationSignificantlyInaccurateThreshold=core->getConfigStore()->getIntValue("Navigation","locationSignificantlyInaccurateThreshold", __FILE__, __LINE__);
  trackRecordingMinDistance=core->getConfigStore()->getDoubleValue("Navigation","trackRecordingMinDistance", __FILE__, __LINE__);
  visualizationMaxContainerCountPerRound=core->getConfigStore()->getIntValue("Map","visualizationMaxContainerCountPerRound", __FILE__, __LINE__);
  backgroundLoaderFinishedMutex=core->getThread()->createMutex("navigation engine background loader finished mutex");
  activeRouteMutex=core->getThread()->createMutex("navigation engine active route mutex");
  recordedTrackMutex=core->getThread()->createMutex("navigation engine recorded track mutex");
  routesMutex=core->getThread()->createMutex("navigation engine routes mutex");
  locationPosMutex=core->getThread()->createMutex("navigation engine location pos mutex");
  compassBearingMutex=core->getThread()->createMutex("navigation engine compass bearing mutex");
  recordTrack=core->getConfigStore()->getIntValue("Navigation","recordTrack", __FILE__, __LINE__);
  compassBearing=0;
  isInitialized=false;
  recordedTrack=NULL;
  activeRoute=NULL;
  backgroundLoaderThreadInfo=NULL;
  statusMutex=core->getThread()->createMutex("navigation engine status mutex");
  targetPosMutex=core->getThread()->createMutex("navigation engine target pos mutex");
  targetVisible=false;
  arrowVisible=false;
  arrowX=std::numeric_limits<Int>::min();
  arrowY=std::numeric_limits<Int>::min();
  arrowAngle=-1;
  arrowDiameter=0;
  arrowInitialTranslateDuration=core->getConfigStore()->getIntValue("Graphic","arrowInitialTranslateDuration", __FILE__, __LINE__);
  arrowNormalTranslateDuration=core->getConfigStore()->getIntValue("Graphic","arrowNormalTranslateDuration", __FILE__, __LINE__);
  arrowMinPositionDiffForRestartingAnimation=core->getConfigStore()->getIntValue("Graphic","arrowMinPositionDiffForRestartingAnimation", __FILE__, __LINE__);
  targetInitialScaleDuration=core->getConfigStore()->getIntValue("Graphic","targetInitialScaleDuration", __FILE__, __LINE__);
  targetNormalScaleDuration=core->getConfigStore()->getIntValue("Graphic","targetNormalScaleDuration", __FILE__, __LINE__);
  targetRotateDuration=core->getConfigStore()->getIntValue("Graphic","targetRotateDuration", __FILE__, __LINE__);
  targetScaleMaxFactor=core->getConfigStore()->getDoubleValue("Graphic","targetScaleMaxFactor", __FILE__, __LINE__);
  targetScaleMinFactor=core->getConfigStore()->getDoubleValue("Graphic","targetScaleMinFactor", __FILE__, __LINE__);
  targetScaleNormalFactor=core->getConfigStore()->getDoubleValue("Graphic","targetScaleNormalFactor", __FILE__, __LINE__);
  backgroundLoaderFinished=false;
  navigationInfosMutex=core->getThread()->createMutex("navigation engine navigation infos mutex");
  nearestAddressPointValid=false;
  nearestAddressPointAlarm=false;
  nearestAddressPointMutex=core->getThread()->createMutex("navigation engine nearest navigation points mutex");
  nearestAddressPointDistance=std::numeric_limits<double>::max();
  minDistanceToNavigationUpdate=core->getConfigStore()->getDoubleValue("Navigation","minDistanceToNavigationUpdate", __FILE__, __LINE__);
  minSpeedToSignalOffRoute=core->getConfigStore()->getDoubleValue("Navigation","minSpeedToSignalOffRoute", __FILE__, __LINE__);
  forceNavigationInfoUpdate=false;
  computeNavigationInfoThreadInfo=NULL;
  computeNavigationInfoSignal=core->getThread()->createSignal();
  synchronizeGoogleBookmarksThreadInfo=NULL;
  quitSynchronizeGoogleBookmarksThread=false;
  synchronizeGoogleBookmarksSignal=core->getThread()->createSignal();
  overlayGraphicHash="";
  maxAddressPointAlarmDistance=core->getConfigStore()->getDoubleValue("Navigation","maxAddressPointAlarmDistance",__FILE__,__LINE__);
  nearestAddressPointName="";
  nearestAddressPointNameUpdate=0;

  // Create the track directory if it does not exist
  struct stat st;
  if (core->statFile(getTrackPath(), &st) != 0)
  {
    if (mkdir(getTrackPath().c_str(),S_IRWXU | S_IRWXG | S_IRWXO)!=0) {
      FATAL("can not create track directory!",NULL);
      return;
    }
  }

  // Create the route directory if it does not exist
  if (core->statFile(getRoutePath(), &st) != 0)
  {
    if (mkdir(getRoutePath().c_str(),S_IRWXU | S_IRWXG | S_IRWXO)!=0) {
      FATAL("can not create route directory!",NULL);
      return;
    }
  }

  // Create the directory for exporting routes if it does not exist
  if (core->statFile(getExportRoutePath(), &st) != 0)
  {
    if (mkdir(getExportRoutePath().c_str(),S_IRWXU | S_IRWXG | S_IRWXO)!=0) {
      FATAL("can not create export route directory!",NULL);
      return;
    }
  }
  
  // Cleanup the path caches
  cleanupNavigationPathCache(getRoutePath());
  cleanupNavigationPathCache(getTrackPath());  
}

// Remove cache files that do not have their original gpx files anymore
void NavigationEngine::cleanupNavigationPathCache(std::string filepath) {

  // Set the directories
  std::string cachepath=filepath + "/.cache";

  // Check if cache exists
  struct stat st;
  if (core->statFile(cachepath, &st) != 0) {
    DEBUG("skipping <%s> since it does not exist",cachepath.c_str());
    return;
  }

  // Clean up the route cache
  DIR *dp = core->openDir(cachepath.c_str());
  if (dp == NULL){
    FATAL("can not open directory <%s> for reading available paths",cachepath.c_str());
    return;
  }
  struct dirent *dirp;
  struct stat filestat;
  while ((dirp = readdir( dp )))
  {
    std::string filename = std::string(dirp->d_name);
    std::string originalFilepath = filepath + "/" + dirp->d_name;
    std::string cacheFilepath = cachepath + "/" + dirp->d_name;

    // If the file is a directory (or is in some way invalid) we'll skip it
    if (core->statFile( cacheFilepath, &filestat )) continue;
    if (S_ISDIR( filestat.st_mode ))                continue;

    // Process the file
    //DEBUG("checking if original gpx <%s> exists",originalFilepath.c_str());
    if (core->statFile(originalFilepath, &st) != 0) {
      DEBUG("removing cache <%s> since original does not exist anymore",cacheFilepath.c_str());
      remove(cacheFilepath.c_str());      
    }
  }
  closedir(dp);  
}

// Destructor
NavigationEngine::~NavigationEngine() {

  // Deinit the object
  deinit();

  // Delete mutexes
  core->getThread()->destroyMutex(recordedTrackMutex);
  core->getThread()->destroyMutex(routesMutex);
  core->getThread()->destroyMutex(locationPosMutex);
  core->getThread()->destroyMutex(compassBearingMutex);
  core->getThread()->destroyMutex(statusMutex);
  core->getThread()->destroyMutex(targetPosMutex);
  core->getThread()->destroyMutex(backgroundLoaderFinishedMutex);
  core->getThread()->destroyMutex(navigationInfosMutex);
  core->getThread()->destroyMutex(nearestAddressPointMutex);
  core->getThread()->destroyMutex(activeRouteMutex);
}

// Initializes the engine
void NavigationEngine::init() {

  // Set the animation of the target
  GraphicRectangle *targetIcon = core->getDefaultGraphicEngine()->lockTargetIcon(__FILE__, __LINE__);
  targetIcon->setRotateAnimation(0,0,360,true,targetRotateDuration,GraphicRotateAnimationTypeLinear);
  core->getDefaultGraphicEngine()->unlockTargetIcon();

  // Create a new recorded track
  NavigationPath *recordedTrack;
  ConfigStore *c=core->getConfigStore();
  if (!(recordedTrack=new NavigationPath)) {
    FATAL("can not create track",NULL);
    return;
  }
  recordedTrack->setNormalColor(c->getGraphicColorValue("Navigation/TrackColor", __FILE__, __LINE__), __FILE__, __LINE__);

  // Prepare the last recorded track if it does exist
  std::string lastRecordedTrackFilename=c->getStringValue("Navigation","lastRecordedTrackFilename", __FILE__, __LINE__);
  std::string filepath=recordedTrack->getGpxFilefolder()+"/"+lastRecordedTrackFilename;
  if ((lastRecordedTrackFilename!="")&&(access(filepath.c_str(),F_OK)==0)) {
    //DEBUG("track gpx file exists, using it",NULL);
    recordedTrack->setGpxFilename(lastRecordedTrackFilename);
  } else {
    //DEBUG("track gpx file does not exist, starting new track",NULL);
    c->setStringValue("Navigation","lastRecordedTrackFilename",recordedTrack->getGpxFilename(), __FILE__, __LINE__);
    recordedTrack->setIsInit(true);
  }

  // Set the new recorded track
  lockRecordedTrack(__FILE__, __LINE__);
  this->recordedTrack=recordedTrack;
  unlockRecordedTrack();

  // Set the track recording
  setRecordTrack(recordTrack, true, false);

  // Update the route lists
  updateRoutes();

  // Prepare any routes
  std::list<NavigationPath*> routes;
  std::string path="Navigation/Route";
  std::list<std::string> routeNames=c->getAttributeValues(path,"name",__FILE__,__LINE__);
  std::list<std::string>::iterator j;
  std::string activeRouteName = c->getStringValue("Navigation","activeRoute",__FILE__,__LINE__);
  for(std::list<std::string>::iterator i=routeNames.begin();i!=routeNames.end();i++) {
    std::string routePath=path + "[@name='" + *i + "']";
    if (c->getIntValue(routePath,"visible",__FILE__,__LINE__)) {

      // Create the route
      NavigationPath *route=new NavigationPath();
      if (!route) {
        FATAL("can not create route",NULL);
        return;
      }
      GraphicColor highlightColor = c->getGraphicColorValue(routePath + "/HighlightColor",__FILE__,__LINE__);
      route->setHighlightColor(highlightColor,__FILE__,__LINE__);
      route->setNormalColor(c->getGraphicColorValue(routePath + "/NormalColor",__FILE__,__LINE__), __FILE__, __LINE__);
      route->setBlinkMode(false, __FILE__, __LINE__);
      route->setReverse(c->getIntValue(routePath,"reverse",__FILE__, __LINE__));
      route->setImportWaypoints((NavigationPatImportWaypointsType)c->getIntValue(routePath,"importWaypoints",__FILE__, __LINE__));
      route->setName(*i);
      route->setDescription("route number " + *i);
      route->setGpxFilefolder(getRoutePath());
      route->setGpxFilename(*i);
      routes.push_back(route);

      // Check if it is selected for navigation
      if (activeRouteName==route->getGpxFilename()) {
        setActiveRoute(route);
      }

    }
  }

  // Set the new routes
  lockRoutes(__FILE__, __LINE__);
  this->routes=routes;
  unlockRoutes();

  // Prepare the target
  if (c->getIntValue("Navigation/Target","visible", __FILE__, __LINE__)) {
    showTarget(false);
  }

  // Load the address points
  initAddressPoints();

  // Inform the graphic engine about the new objects
  core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
  core->getDefaultGraphicEngine()->setNavigationPoints(&navigationPointsGraphicObject);
  core->getDefaultGraphicEngine()->unlockDrawing();

  // Start the background loader
  if (!(backgroundLoaderThreadInfo=core->getThread()->createThread("navigation engine background loader thread",navigationEngineBackgroundLoaderThread,this)))
    FATAL("can not start background loader thread",NULL);

  // Start the navigation info calculator
  if (!(computeNavigationInfoThreadInfo=core->getThread()->createThread("navigation engine compute navigation info thread",navigationEngineComputeNavigationInfoThread,this)))
    FATAL("can not start compute navigation info thread",NULL);

  // Start the google maps bookmark synchronizer
  if (core->getConfigStore()->getIntValue("GoogleBookmarksSync","active",__FILE__,__LINE__)) {    
    if (!(synchronizeGoogleBookmarksThreadInfo=core->getThread()->createThread("navigation engine synchronize google maps bookmarks thread",navigationEngineSynchronizeGoogleBookmarksThread,this)))
      FATAL("can not start synchronize google maps bookmarks thread",NULL);
  }
  
  // Inform the remote map
  core->getCommander()->dispatch("forceRemoteMapUpdate()");

  // Object is initialized
  isInitialized=true;
}

/**
 * Removes any routes from the config that do not exist anymore and adds new
 * ones
 */
void NavigationEngine::updateRoutes() {

  lockRoutes(__FILE__, __LINE__);

  // First get a list of all available route filenames
  DIR *dp = core->openDir(getRoutePath());
  struct dirent *dirp;
  struct stat filestat;
  std::list<std::string> routes;
  if (dp == NULL){
    FATAL("can not open directory <%s> for reading available routes",getRoutePath().c_str());
    return;
  }
  while ((dirp = readdir( dp )))
  {
    std::string filename = std::string(dirp->d_name);
    std::string filepath = getRoutePath() + "/" + dirp->d_name;

    // If the file is a directory (or is in some way invalid) we'll skip it
    if (core->statFile( filepath, &filestat ))      continue;
    if (S_ISDIR( filestat.st_mode ))                continue;

    // If the file is a backup, skip it
    if (filename.substr(filename.length()-1)=="~")  continue;

    // If the file is a cache, skip it
    if (filename.substr(filename.length()-4)==".bin")  continue;

    // Add the found route
    routes.push_back(filename);
  }
  closedir(dp);
   
  // Go through all routes in the config and remove the ones that do not exist anymore
  std::list<std::string> routeNames = core->getConfigStore()->getAttributeValues("Navigation/Route", "name", __FILE__, __LINE__);
  for(std::list<std::string>::iterator i=routeNames.begin(); i!=routeNames.end(); i++) {
    std::string path = "Navigation/Route[@name='" + *i + "']";
    std::string filepath = getRoutePath() + "/" + *i;
    if (access(filepath.c_str(), F_OK)==0) {
      routes.remove(*i);
    } else {
      core->getConfigStore()->removePath(path);
    }
  }

  // Add new ones
  for(std::list<std::string>::iterator i=routes.begin(); i!=routes.end(); i++) {
    std::string path = "Navigation/Route[@name='" + *i + "']";
    core->getConfigStore()->setIntValue(path,"visible", 1, __FILE__, __LINE__);
  }

  unlockRoutes();
}

// Deinitializes a path
void NavigationEngine::deletePath(NavigationPath *path) {
  core->onPathChange(path,NavigationPathChangeTypeWillBeRemoved);
  delete path;
}

// Deinitializes the engine
void NavigationEngine::deinit() {

  // Finish the navigation info calculator thread
  if (computeNavigationInfoThreadInfo) {
    core->getThread()->lockMutex(activeRouteMutex, __FILE__, __LINE__);
    core->getThread()->cancelThread(computeNavigationInfoThreadInfo);
    core->getThread()->waitForThread(computeNavigationInfoThreadInfo);
    core->getThread()->destroyThread(computeNavigationInfoThreadInfo);
    core->getThread()->destroySignal(computeNavigationInfoSignal);
    core->getThread()->unlockMutex(activeRouteMutex);
  }

  // Finish the background thread
  if (backgroundLoaderThreadInfo) {
    core->getThread()->lockMutex(backgroundLoaderFinishedMutex, __FILE__, __LINE__);
    if (!backgroundLoaderFinished) {
      core->getThread()->cancelThread(backgroundLoaderThreadInfo);
      core->getThread()->waitForThread(backgroundLoaderThreadInfo);
    }
    core->getThread()->unlockMutex(backgroundLoaderFinishedMutex);
    core->getThread()->destroyThread(backgroundLoaderThreadInfo);
  }

  // Finish the google maps bookmark synchronization thread
  if (synchronizeGoogleBookmarksThreadInfo) {
    quitSynchronizeGoogleBookmarksThread=true;
    core->getThread()->issueSignal(synchronizeGoogleBookmarksSignal);
    core->getThread()->waitForThread(synchronizeGoogleBookmarksThreadInfo);
    core->getThread()->destroyThread(synchronizeGoogleBookmarksThreadInfo);
    core->getThread()->destroySignal(synchronizeGoogleBookmarksSignal);
  }
  
  // Reset the address points graphic object
  core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
  core->getDefaultGraphicEngine()->setNavigationPoints(NULL);
  core->getDefaultGraphicEngine()->unlockDrawing();

  // Save the track first
  if (recordedTrack) {
    recordedTrack->writeGPXFile(); // locking is handled within writeGPXFile()
    NavigationPath *path=recordedTrack;
    lockRecordedTrack(__FILE__, __LINE__);
    recordedTrack=NULL;
    unlockRecordedTrack();
    deletePath(path);
  }

  // Free all routes
  lockRoutes(__FILE__, __LINE__);
  for (std::list<NavigationPath*>::iterator i=routes.begin();i!=routes.end();i++) {
    deletePath(*i);
  }
  routes.clear();
  unlockRoutes();

  // Object is not initialized
  isInitialized=false;
}

// Sets a location pos directly
void NavigationEngine::setLocationPos(MapPosition newLocationPos, bool computeNavigationInfos, const char *file, int line) {

  // Store the new fix
  lockLocationPos(file, line);
  locationPos=newLocationPos;
  unlockLocationPos();
  MapPosition *pos=core->getMapEngine()->lockLocationPos(__FILE__, __LINE__);
  *pos=locationPos;
  core->getMapEngine()->unlockLocationPos();

  // Shall we update the navigation infos?
  if (computeNavigationInfos) {

    // Update the navigation infos
    core->getThread()->issueSignal(computeNavigationInfoSignal);

    // Update the current track
    updateTrack();
    //PROFILE_ADD("track update");

  }

  // Inform the widget engine
  core->onLocationChange(locationPos);

  // Update the graphics
  updateScreenGraphic(false);
  //PROFILE_ADD("graphics update");
}

// Updates the current location
void NavigationEngine::newLocationFix(MapPosition newLocationPos) {

  bool updatePos=false;
  bool isNewer=false;

  //PROFILE_START;
  //DEBUG("checking new location pos (locationPos.getTimestamp()=%ld)",locationPos.getTimestamp());

  // Check if the new fix is older or newer
  if (newLocationPos.getTimestamp()>locationPos.getTimestamp()) {

    isNewer=true;

    // If the new fix is significantly newer, use it
    if (newLocationPos.getTimestamp()-locationPos.getTimestamp()>=locationOutdatedThreshold) {
      updatePos=true;
    }

  } else {

    // If it is significantly older, discard it
    if (locationPos.getTimestamp()-newLocationPos.getTimestamp()>=locationOutdatedThreshold) {
      //DEBUG("new location pos is too old, discarding it",NULL);
      return;
    }

  }

  // If the new fix is more accurate, use it
  int accuracyDiff=(int)newLocationPos.getAccuracy()-locationPos.getAccuracy();
  if (accuracyDiff<0) {
    updatePos=true;
  } else {

    // Is the new fix newer?
    if (isNewer) {

      // If the new fix is not less accurate, use it
      if (accuracyDiff==0) {
        updatePos=true;
      } else {

        // If the new fix is from the same provider and is not significantly less accurate, use it
        if ((newLocationPos.getSource()==locationPos.getSource())&&(newLocationPos.getAccuracy()<=locationSignificantlyInaccurateThreshold)) {
          updatePos=true;
        }

      }
    }
  }

  //PROFILE_ADD("position check");

  // Update the position
  if (updatePos) {
    /*DEBUG("new location fix received: source=%s lng=%f lat=%f hasAltitude=%d altitude=%f hasBearing=%d bearing=%f hasSpeed=%d speed=%f hasAccuracy=%d accuracy=%f",
          newLocationPos.getSource().c_str(),
          newLocationPos.getLng(),
          newLocationPos.getLat(),
          newLocationPos.getHasAltitude(),
          newLocationPos.getAltitude(),
          newLocationPos.getHasBearing(),
          newLocationPos.getBearing(),
          newLocationPos.getHasSpeed(),
          newLocationPos.getSpeed(),
          newLocationPos.getHasAccuracy(),
          newLocationPos.getAccuracy());*/

    // Convert to MSL height if required
    newLocationPos.toMSLHeight();

    // If the new fix has no heading, keep the previous one
    if (!newLocationPos.getHasBearing()&&locationPos.getHasBearing()) {
      newLocationPos.setHasBearing(true);
      newLocationPos.setBearing(locationPos.getBearing());
    }

    // Store the new fix
    setLocationPos(newLocationPos, true, __FILE__, __LINE__);

  } else {
    //DEBUG("location pos has not been used",NULL);
  }

  //PROFILE_END;

}

// Updates the compass
void NavigationEngine::newCompassBearing(double bearing) {
  lockCompassBearing(__FILE__, __LINE__);
  compassBearing=bearing;
  unlockCompassBearing();
  updateScreenGraphic(false);
}

// Updates the currently recorded track
void NavigationEngine::updateTrack() {

  bool pointMeetsCriterias=false;
  double distance=0;

  // Check if recording is enabled
  if (!recordTrack) {
    return;
  }

  // Store only gps positions
  if (locationPos.getSource()!="gps") {
    return;
  }

  // Do not update the track if it is not yet initialized
  if (!recordedTrack->getIsInit())
    return;

  // Get access to the recorded track
  //DEBUG("before recorded track update",NULL);

  // If the track was just loaded, we need to add this point as a stable one
  recordedTrack->lockAccess(__FILE__, __LINE__);
  if (recordedTrack->getHasBeenLoaded()) {
    pointMeetsCriterias=true;
    recordedTrack->setHasBeenLoaded(false);
  } else {

    // Check if the point meets the criteras to become part of the path
    if (recordedTrack->getHasLastPoint()) {
      MapPosition lastPoint=recordedTrack->getLastPoint();
      if (lastPoint==NavigationPath::getPathInterruptedPos()) {
        pointMeetsCriterias=true;
      } else {
        distance=lastPoint.computeDistance(locationPos);
        if (distance>=trackRecordingMinDistance)
          pointMeetsCriterias=true;
      }
    } else {
      pointMeetsCriterias=true;
    }
  }
  recordedTrack->unlockAccess();

  // Add the new point if it meets the criterias
  if (pointMeetsCriterias) {
    //DEBUG("adding new point (%f,%f): navigationDistance=%.2f pointMeetsCriterias=%d",locationPos.getLat(),locationPos.getLng(),navigationDistance,pointMeetsCriterias);
    recordedTrack->addEndPosition(locationPos);
  }
}

// Saves the recorded track if required
void NavigationEngine::backup() {

  // Store the recorded track
  recordedTrack->writeGPXFile(); // locking is handled within method

}

// Switches the track recording
bool NavigationEngine::setRecordTrack(bool recordTrack, bool ignoreIsInit, bool showInfo)
{
  // Ignore command if track is not initialized
  if ((!recordedTrack->getIsInit())&&(!ignoreIsInit)) {
    WARNING("can not change track recording status because track is currently loading",NULL);
    return false;
  }

  // Interrupt the track if there is a previous point
  if ((recordTrack)&&(!this->recordTrack)) {
    recordedTrack->lockAccess(__FILE__, __LINE__);
    bool addPathInterruptedPos=false;
    if (recordedTrack->getHasLastPoint()) {
      if (recordedTrack->getLastPoint()!=NavigationPath::getPathInterruptedPos()) {
        addPathInterruptedPos=true;
      }
    }
    recordedTrack->unlockAccess();
    if (addPathInterruptedPos)
      recordedTrack->addEndPosition(NavigationPath::getPathInterruptedPos());
  }
  lockRecordedTrack(__FILE__, __LINE__);
  this->recordTrack=recordTrack;
  core->getConfigStore()->setIntValue("Navigation","recordTrack",recordTrack,__FILE__,__LINE__);
  unlockRecordedTrack();
  if (showInfo) {
    if (recordTrack) {
      INFO("track recording is enabled",NULL);
    } else {
      INFO("track recording is disabled",NULL);
    }
  }
  updateScreenGraphic(false);
  return true;
}

// Creates a new track
void NavigationEngine::createNewTrack() {

  if (!recordedTrack->getIsInit()) {
    WARNING("can not change track recording status because track is currently loading",NULL);
    return;
  }
  recordedTrack->writeGPXFile(); // locking is handled within writeGPXFile
  recordedTrack->lockAccess(__FILE__, __LINE__);
  recordedTrack->deinit();
  recordedTrack->init();
  recordedTrack->setIsInit(true);
  recordedTrack->unlockAccess();
  lockRecordedTrack(__FILE__, __LINE__);
  core->getConfigStore()->setStringValue("Navigation","lastRecordedTrackFilename",recordedTrack->getGpxFilename(), __FILE__, __LINE__);
  unlockRecordedTrack();
  INFO("new %s created",recordedTrack->getGpxFilename().c_str());
  updateScreenGraphic(false);
}

// Updates navigation-related graphic that is overlayed on the screen
void NavigationEngine::updateScreenGraphic(bool scaleHasChanged) {
  Int mapDiffX, mapDiffY;
  bool showCursor,showTarget;
  bool updatePosition;
  Int visPosX, visPosY, visRadiusX, visRadiusY;
  double visAngle;
  MapPosition mapPos;
  MapEngine *mapEngine=core->getMapEngine();

  // We need a map tile to compute the coordinates
  //DEBUG("before map pos lock",NULL);
  mapPos=*(mapEngine->lockMapPos(__FILE__, __LINE__));
  mapEngine->unlockMapPos();
  //DEBUG("after map pos lock",NULL);
  if (!mapPos.getMapTile()) {
    return;
  }

  // Copy the current display area
  //DEBUG("before display area lock",NULL);
  MapArea displayArea=*(mapEngine->lockDisplayArea(__FILE__, __LINE__));
  mapEngine->unlockDisplayArea();
  //DEBUG("after display area lock",NULL);

  // Copy the current visual position
  GraphicPosition visPos=*(core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__));
  core->getDefaultGraphicEngine()->unlockPos();

  // Update the location icon
  //DEBUG("before location pos lock",NULL);
  lockLocationPos(__FILE__, __LINE__);
  showCursor=false;
  updatePosition=false;
  if ((locationPos.isValid())&&(displayArea.containsGeographicCoordinate(locationPos))&&(mapPos.getMapTile())) {
    showCursor=true;
    MapPosition newLocationPos=locationPos;
    MapCalibrator *calibrator=mapPos.getMapTile()->getParentMapContainer()->getMapCalibrator();
    //DEBUG("calibrator=%08x",calibrator);
    if (!calibrator->setPictureCoordinates(newLocationPos)) {
      showCursor=false;
    } else {
      locationPos=newLocationPos;
      //DEBUG("locationPos.getX()=%d locationPos.getY()=%d",locationPos.getX(),locationPos.getY());
      if (!Integer::add(locationPos.getX(),-mapPos.getX(),mapDiffX))
        showCursor=false;
      if (!Integer::add(locationPos.getY(),-mapPos.getY(),mapDiffY))
        showCursor=false;
      //DEBUG("mapDiffX=%d mapDiffY=%d",mapDiffX,mapDiffY);
      if (!Integer::add(displayArea.getRefPos().getX(),mapDiffX,visPosX))
        showCursor=false;
      if (!Integer::add(displayArea.getRefPos().getY(),-mapDiffY,visPosY))
        showCursor=false;
      if (showCursor) {
        updatePosition=true;
        visAngle=-locationPos.getBearing()-mapPos.getMapTile()->getNorthAngle();
        if (locationPos.getHasAccuracy()) {
          MapPosition t=locationPos.computeTarget(0,locationPos.getAccuracy());
          //DEBUG("locationPos.getLat=%f locationPos.getLng=%f t.getLat=%f t.getLng=%f",locationPos.getLat(),locationPos.getLng(),t.getLat(),t.getLng());
          calibrator->setPictureCoordinates(t);
          visRadiusY=sqrt((double)(t.getX()-locationPos.getX())*(t.getX()-locationPos.getX())+(t.getY()-locationPos.getY())*(t.getY()-locationPos.getY()));
          //DEBUG("visRadiusY=%d",visRadiusY);
          t=locationPos.computeTarget(90,locationPos.getAccuracy());
          //DEBUG("locationPos.getLat=%f locationPos.getLng=%f t.getLat=%f t.getLng=%f",locationPos.getLat(),locationPos.getLng(),t.getLat(),t.getLng());
          calibrator->setPictureCoordinates(t);
          visRadiusX=sqrt((double)(t.getX()-locationPos.getX())*(t.getX()-locationPos.getX())+(t.getY()-locationPos.getY())*(t.getY()-locationPos.getY()));
          //DEBUG("visRadiusX=%d",visRadiusX);
        } else {
          visRadiusX=0;
          visRadiusY=0;
        }
      }
    }
  }
  unlockLocationPos();
  //DEBUG("after location pos to vis pos lock",NULL);
  //DEBUG("before location icon lock",NULL);
  GraphicRectangle *locationIcon=core->getDefaultGraphicEngine()->lockLocationIcon(__FILE__, __LINE__);
  if (showCursor) {
    if (updatePosition) {
      if ((locationIcon->getX()!=visPosX)||(locationIcon->getY()!=visPosY)||
          (locationIcon->getAngle()!=visAngle)||
          (core->getDefaultGraphicEngine()->getLocationAccuracyRadiusX()!=visRadiusX)||
          (core->getDefaultGraphicEngine()->getLocationAccuracyRadiusY()!=visRadiusY)) {
        locationIcon->setX(visPosX);
        locationIcon->setY(visPosY);
        locationIcon->setAngle(visAngle);
        //DEBUG("locationIcon.getX()=%d locationIcon.getY()=%d",locationIcon->getX(),locationIcon->getY());
        core->getDefaultGraphicEngine()->setLocationAccuracyRadiusX(visRadiusX);
        core->getDefaultGraphicEngine()->setLocationAccuracyRadiusY(visRadiusY);
        locationIcon->setIsUpdated(true);
      }

    }
    if (locationIcon->getColor().getAlpha()!=255) {
      locationIcon->setColor(GraphicColor(255,255,255,255));
      locationIcon->setIsUpdated(true);
    }
  } else {
    if (locationIcon->getColor().getAlpha()!=0) {
      locationIcon->setColor(GraphicColor(255,255,255,0));
      locationIcon->setIsUpdated(true);
    }
  }
  core->getDefaultGraphicEngine()->unlockLocationIcon();
  //DEBUG("after location icon lock",NULL);

  // Update the compass bearing
  //DEBUG("before compass bearing lock",NULL);
  lockCompassBearing(__FILE__, __LINE__);
  double compassBearing=this->compassBearing;
  unlockCompassBearing();
  //DEBUG("after compass bearing lock",NULL);
  //DEBUG("before compass cone icon lock",NULL);
  GraphicRectangle *compassConeIcon=core->getDefaultGraphicEngine()->lockCompassConeIcon(__FILE__, __LINE__);
  compassConeIcon->setAngle(-compassBearing-mapPos.getMapTile()->getNorthAngle());
  compassConeIcon->setIsUpdated(true);
  core->getDefaultGraphicEngine()->unlockCompassConeIcon();
  //DEBUG("after compass cone icon lock",NULL);

  // Update the target icon
  //DEBUG("before location pos lock",NULL);
  lockTargetPos(__FILE__, __LINE__);
  showCursor=false;
  updatePosition=false;
  bool updateAnimation=false;
  double screenZoom=visPos.getZoom()*core->getDefaultGraphicEngine()->getMapTileToScreenScale(core->getDefaultScreen());
  double screenAngle=FloatingPoint::degree2rad(visPos.getAngle());
  Int zoomedScreenWidth=floor(((double)core->getDefaultScreen()->getWidth())/screenZoom);
  Int zoomedScreenHeight=floor(((double)core->getDefaultScreen()->getHeight())/screenZoom);
  //DEBUG("zoomedScreenWidth=%d zoomedScreenHeight=%d angle=%f",zoomedScreenWidth,zoomedScreenHeight,visPos.getAngle());
  //DEBUG("screenZoom=%f zoomedScreenWidth=%d zoomedScreenHeight=%d",screenZoom,zoomedScreenWidth,zoomedScreenHeight);
  if ((targetPos.isValid())&&(displayArea.containsGeographicCoordinate(targetPos))&&(mapPos.getMapTile())) {
    showCursor=true;
    MapCalibrator *calibrator=mapPos.getMapTile()->getParentMapContainer()->getMapCalibrator();
    //DEBUG("calibrator=%08x",calibrator);
    if (!calibrator->setPictureCoordinates(targetPos)) {
      showCursor=false;
    } else {
      //DEBUG("targetPos.getX()=%d targetPos.getY()=%d",targetPos.getX(),targetPos.getY());
      if (!Integer::add(targetPos.getX(),-mapPos.getX(),mapDiffX))
        showCursor=false;
      if (!Integer::add(targetPos.getY(),-mapPos.getY(),mapDiffY))
        showCursor=false;
      //DEBUG("mapDiffX=%d mapDiffY=%d",mapDiffX,mapDiffY);
      if (!Integer::add(displayArea.getRefPos().getX(),mapDiffX,visPosX))
        showCursor=false;
      if (!Integer::add(displayArea.getRefPos().getY(),-mapDiffY,visPosY))
        showCursor=false;
      //DEBUG("visPosX=%d visPosY=%d",visPosX,visPosY);
      if (showCursor) {

        // Check if target lies within visible screen
        //DEBUG("alpha=%f",visPos.getAngle());
        //DEBUG("mapDiffX=%d mapDiffY=%d",mapDiffX,mapDiffY);
        double l=sqrt((double)mapDiffX*(double)mapDiffX+(double)mapDiffY*(double)mapDiffY);
        double beta;
        if (mapDiffX!=0) {
          beta=atan(fabs((double)mapDiffY)/fabs((double)mapDiffX));
        } else {
          beta=M_PI_2;
        }
        if (mapDiffY>0)
          if (mapDiffX>0)
            beta=4*M_PI_2-beta;
          else
            beta=2*M_PI_2+beta;
        else
          if (mapDiffX>0)
            beta=0*M_PI_2+beta;
          else
            beta=2*M_PI_2-beta;
        //DEBUG("beta=%f",FloatingPoint::rad2degree(beta));
        Int rotMapDiffX=round(l*cos(visPos.getAngleRad()+beta));
        Int rotMapDiffY=round(-l*sin(visPos.getAngleRad()+beta));
        //DEBUG("rotMapDiffX=%d rotMapDiffY=%d",rotMapDiffX,rotMapDiffY);
        if (abs(rotMapDiffX)>zoomedScreenWidth/2)
          showCursor=false;
        if (abs(rotMapDiffY)>zoomedScreenHeight/2)
          showCursor=false;
      }
      if (showCursor) {
        updatePosition=true;
      }
    }
    if (targetPos.getIsUpdated()) {
      updateAnimation=true;
      targetPos.setIsUpdated(false);
    }
  }
  unlockTargetPos();
  GraphicRectangle *targetIcon=core->getDefaultGraphicEngine()->lockTargetIcon(__FILE__, __LINE__);
  if (updateAnimation) {
    std::list<GraphicScaleAnimationParameter> scaleAnimationSequence;
    TimestampInMicroseconds startTime = core->getClock()->getMicrosecondsSinceStart();
    GraphicScaleAnimationParameter parameter;
    parameter.setStartFactor(targetScaleMaxFactor);
    parameter.setEndFactor(targetScaleMinFactor);
    parameter.setStartTime(startTime);
    parameter.setDuration(targetInitialScaleDuration);
    parameter.setInfinite(false);
    scaleAnimationSequence.push_back(parameter);
    parameter.setStartFactor(targetScaleMinFactor);
    parameter.setEndFactor(targetScaleNormalFactor);
    parameter.setStartTime(startTime+targetInitialScaleDuration);
    parameter.setDuration(targetNormalScaleDuration);
    parameter.setInfinite(true);
    scaleAnimationSequence.push_back(parameter);
    targetIcon->setScaleAnimationSequence(scaleAnimationSequence);
    targetIcon->setIsUpdated(true);
  }
  if (showCursor) {
    if (updatePosition) {
      if ((targetIcon->getX()!=visPosX)||((targetIcon->getY()!=visPosY))) {
        targetIcon->setX(visPosX);
        targetIcon->setY(visPosY);
        //DEBUG("target icon => visPosX=%d visPosY=%d",visPosX,visPosY);
        targetIcon->setIsUpdated(true);
      }
    }
    if (!targetVisible) {
      GraphicColor endColor=targetIcon->getColor();
      endColor.setAlpha(255);
      targetIcon->setFadeAnimation(core->getClock()->getMicrosecondsSinceStart(),targetIcon->getColor(),endColor,false,core->getDefaultGraphicEngine()->getFadeDuration());
      targetIcon->setIsUpdated(true);
    }
    targetVisible=true;
  } else {
    if (targetVisible) {
      GraphicColor endColor=targetIcon->getColor();
      endColor.setAlpha(0);
      targetIcon->setFadeAnimation(core->getClock()->getMicrosecondsSinceStart(),targetIcon->getColor(),endColor,false,core->getDefaultGraphicEngine()->getFadeDuration());
      targetIcon->setIsUpdated(true);
      targetVisible=false;
    }
  }
  core->getDefaultGraphicEngine()->unlockTargetIcon();

  // Update the arrow icon
  //DEBUG("before location pos lock",NULL);
  lockTargetPos(__FILE__, __LINE__);
  updateAnimation=false;
  showCursor=false;
  Int translateEndX, translateEndY;
  if ((targetPos.isValid())&&(!showCursor)) {
    if (!targetVisible) {
      showCursor=true;
      if (!arrowVisible)
        updateAnimation=true;

      // Compute the angle to the target
      visAngle = - mapPos.computeBearing(targetPos) - mapPos.getMapTile()->getNorthAngle();
      double alpha = - FloatingPoint::degree2rad(visAngle);
      double l1 = fabs(zoomedScreenHeight/2 / cos(alpha-screenAngle));
      double l2 = fabs(zoomedScreenWidth/2 / sin(alpha-screenAngle));
      double l;
      if (l1>l2) {
        l=l2;
      } else {
        l=l1;
      }
      double arrowRadiusZoomed=((double)arrowDiameter)/screenZoom/2;
      //DEBUG("arowDiameterZoomed=%f",arrowDiameterZoomed);
      l-=arrowRadiusZoomed;
      visPosX = displayArea.getRefPos().getX() + l * sin(alpha);
      visPosY = displayArea.getRefPos().getY() + l * cos(alpha);
      translateEndX = displayArea.getRefPos().getX() + (l-arrowRadiusZoomed) * sin(alpha);
      translateEndY = displayArea.getRefPos().getY() + (l-arrowRadiusZoomed) * cos(alpha);

    } else {
      showCursor=false;
      if (arrowVisible)
        updateAnimation=true;
    }
  } else {
    updateAnimation=true;
    showCursor=false;
  }
  unlockTargetPos();
  GraphicRectangle *arrowIcon=core->getDefaultGraphicEngine()->lockArrowIcon(__FILE__, __LINE__);
  if (showCursor) {
    if ((arrowX!=visPosX)||((arrowY!=visPosY))||(arrowAngle!=visAngle)) {
      if (scaleHasChanged) {
        arrowIcon->setX(visPosX);
        arrowIcon->setY(visPosY);
        arrowIcon->setAngle(visAngle);
        arrowIcon->setTranslateAnimationSequence(std::list<GraphicTranslateAnimationParameter>());
        arrowIcon->setTranslateAnimation(core->getClock()->getMicrosecondsSinceStart(),visPosX,visPosY,translateEndX,translateEndY,true,arrowNormalTranslateDuration,GraphicTranslateAnimationTypeLinear);
      } else {
        Int diffX=abs(visPosX-arrowX);
        Int diffY=abs(visPosY-arrowY);
        if ((arrowIcon->getTranslateAnimationSequence().size()==0)&&(diffX<arrowMinPositionDiffForRestartingAnimation)&&(diffY<arrowMinPositionDiffForRestartingAnimation)) {
          if ((arrowIcon->getTranslateEndX()==arrowX)&&(arrowIcon->getTranslateEndY()==arrowY)) {
            arrowIcon->setTranslateEndX(visPosX);
            arrowIcon->setTranslateEndY(visPosY);
            arrowIcon->setTranslateStartX(translateEndX);
            arrowIcon->setTranslateStartY(translateEndY);
          } else {
            arrowIcon->setTranslateStartX(visPosX);
            arrowIcon->setTranslateStartY(visPosY);
            arrowIcon->setTranslateEndX(translateEndX);
            arrowIcon->setTranslateEndY(translateEndY);
          }
          if ((arrowIcon->getTranslateEndTime()==arrowIcon->getTranslateStartTime())) {
            arrowIcon->setX(arrowIcon->getTranslateEndX());
            arrowIcon->setY(arrowIcon->getTranslateEndY());
          }
          arrowIcon->setAngle(visAngle);
        } else {
          std::list<GraphicTranslateAnimationParameter> translateAnimationSequence = arrowIcon->getTranslateAnimationSequence();
          TimestampInMicroseconds startTime = core->getClock()->getMicrosecondsSinceStart();
          if (translateAnimationSequence.size()>0) {
            translateAnimationSequence.pop_back(); // remove the last infinite translation
          }
          if (translateAnimationSequence.size()>0) {
            startTime=translateAnimationSequence.back().getStartTime()+translateAnimationSequence.back().getDuration();
          }
          GraphicTranslateAnimationParameter translateParameter;
          translateParameter.setStartTime(startTime);
          translateParameter.setStartX(arrowIcon->getX());
          translateParameter.setStartY(arrowIcon->getY());
          translateParameter.setEndX(visPosX);
          translateParameter.setEndY(visPosY);
          translateParameter.setDuration(arrowInitialTranslateDuration);
          translateParameter.setInfinite(false);
          translateAnimationSequence.push_back(translateParameter);
          translateParameter.setStartTime(startTime+arrowInitialTranslateDuration);
          translateParameter.setStartX(visPosX);
          translateParameter.setStartY(visPosY);
          translateParameter.setEndX(translateEndX);
          translateParameter.setEndY(translateEndY);
          translateParameter.setDuration(arrowNormalTranslateDuration);
          translateParameter.setInfinite(true);
          translateAnimationSequence.push_back(translateParameter);
          arrowIcon->setTranslateAnimationSequence(translateAnimationSequence);
          std::list<GraphicRotateAnimationParameter> rotateAnimationSequence = arrowIcon->getRotateAnimationSequence();
          GraphicRotateAnimationParameter rotateParameter;
          rotateParameter.setStartTime(startTime);
          rotateParameter.setStartAngle(arrowIcon->getAngle());
          rotateParameter.setEndAngle(visAngle);
          rotateParameter.setDuration(arrowInitialTranslateDuration);
          rotateParameter.setInfinite(false);
          rotateAnimationSequence.push_back(rotateParameter);
          arrowIcon->setRotateAnimationSequence(rotateAnimationSequence);
        }
      }
      arrowX=visPosX;
      arrowY=visPosY;
      arrowAngle=visAngle;
      arrowIcon->setIsUpdated(true);
    }
    if (updateAnimation) {
      GraphicColor endColor=arrowIcon->getColor();
      endColor.setAlpha(255);
      arrowIcon->setFadeAnimation(core->getClock()->getMicrosecondsSinceStart(),arrowIcon->getColor(),endColor,false,core->getDefaultGraphicEngine()->getFadeDuration());
      arrowIcon->setIsUpdated(true);
    }
    arrowVisible=true;
  } else {
    if (arrowVisible) {
      if (updateAnimation) {
        GraphicColor endColor=arrowIcon->getColor();
        endColor.setAlpha(0);
        arrowIcon->setFadeAnimation(core->getClock()->getMicrosecondsSinceStart(),arrowIcon->getColor(),endColor,false,core->getDefaultGraphicEngine()->getFadeDuration());
        arrowIcon->setIsUpdated(true);
      }
      arrowVisible=false;
    }
  }
  if (core->getDefaultDevice()->getIsWatch()) {
    arrowIcon->setColor(GraphicColor(255,255,255,0));
  }
  core->getDefaultGraphicEngine()->unlockArrowIcon();

  // Update the visualization of the points
  core->getDefaultGraphicEngine()->lockDrawing(__FILE__,__LINE__);
  TimestampInMicroseconds t = core->getClock()->getMicrosecondsSinceStart();
  for (std::list<NavigationPointVisualization>::iterator i=navigationPointsVisualization.begin();i!=navigationPointsVisualization.end();i++) {
    (*i).updateVisualization(t,mapPos,displayArea);
  }
  core->getDefaultGraphicEngine()->unlockDrawing();
}

// Updates navigation-related graphic that is overlayed on the map
void NavigationEngine::updateMapGraphic() {

  std::list<MapContainer*> containers;

  // Get the current unfinished list
  core->getMapSource()->lockAccess(__FILE__,__LINE__);
  for (Int i=0;i<visualizationMaxContainerCountPerRound;i++) {
    if (unvisualizedMapContainers.empty())
      break;
    containers.push_back(unvisualizedMapContainers.front());
    unvisualizedMapContainers.pop_front();
  }
  core->getMapSource()->unlockAccess();

  // Process the unfinished tile list
  for(std::list<NavigationPath*>::iterator i=routes.begin();i!=routes.end();i++) {
    //PROFILE_START;
    (*i)->lockAccess(__FILE__, __LINE__);
    //PROFILE_ADD("path locking");
    (*i)->addVisualization(&containers);
    //PROFILE_ADD("add visualization");
    (*i)->unlockAccess();
    //PROFILE_ADD("path unlocking");
  }
  //core->getProfileEngine()->outputResult("",false);
  if (recordedTrack) {
    recordedTrack->lockAccess(__FILE__, __LINE__);
    recordedTrack->addVisualization(&containers);
    recordedTrack->unlockAccess();
  }

  // Indicate that all tiles have been processed
  core->getMapSource()->lockAccess(__FILE__,__LINE__);
  for (std::list<MapContainer*>::iterator i=containers.begin();i!=containers.end();i++) {
    (*i)->setOverlayGraphicInvalid(false);
  }
  core->getMapSource()->unlockAccess();
}

// Destroys all graphics
void NavigationEngine::destroyGraphic() {
  for(std::list<NavigationPath*>::iterator i=routes.begin();i!=routes.end();i++) {
    (*i)->lockAccess(__FILE__, __LINE__);
    (*i)->destroyGraphic();
    (*i)->unlockAccess();
  }
  if (recordedTrack) {
    recordedTrack->lockAccess(__FILE__, __LINE__);
    recordedTrack->destroyGraphic();
    recordedTrack->unlockAccess();
  }
  core->getDefaultGraphicEngine()->lockDrawing(__FILE__,__LINE__);
  for (std::list<NavigationPointVisualization>::iterator i=navigationPointsVisualization.begin();i!=navigationPointsVisualization.end();i++) {
    (*i).destroyGraphic();
  }
  core->getDefaultGraphicEngine()->unlockDrawing();
}

// Creates all graphics
void NavigationEngine::createGraphic() {

  // Get the radius of the arrow icon
  GraphicRectangle *arrowIcon=core->getDefaultGraphicEngine()->lockArrowIcon(__FILE__, __LINE__);
  arrowDiameter=sqrt((double)(arrowIcon->getIconWidth()*arrowIcon->getIconWidth()+arrowIcon->getIconHeight()*arrowIcon->getIconHeight()));
  core->getDefaultGraphicEngine()->unlockArrowIcon();

  // Creates the buffers used in the path object
  for(std::list<NavigationPath*>::iterator i=routes.begin();i!=routes.end();i++) {
    (*i)->lockAccess(__FILE__, __LINE__);
    (*i)->createGraphic();
    (*i)->unlockAccess();
  }
  if (recordedTrack) {
    recordedTrack->lockAccess(__FILE__, __LINE__);
    recordedTrack->createGraphic();
    recordedTrack->unlockAccess();
  }
  core->getDefaultGraphicEngine()->lockDrawing(__FILE__,__LINE__);
  for (std::list<NavigationPointVisualization>::iterator i=navigationPointsVisualization.begin();i!=navigationPointsVisualization.end();i++) {
    (*i).createGraphic();
  }
  core->getDefaultGraphicEngine()->unlockDrawing();
}

// Recreate the objects to reduce the number of graphic point buffers
void NavigationEngine::optimizeGraphic() {

  // Optimize the buffers used in the path object
  for(std::list<NavigationPath*>::iterator i=routes.begin();i!=routes.end();i++) {
    (*i)->lockAccess(__FILE__, __LINE__);
    (*i)->optimizeGraphic();
    (*i)->unlockAccess();
  }
  if (recordedTrack) {
    recordedTrack->lockAccess(__FILE__, __LINE__);
    recordedTrack->optimizeGraphic();
    recordedTrack->unlockAccess();
  }
}

// Adds the visualization for the given tile
void NavigationEngine::addGraphics(MapContainer *container) {
  std::list<MapContainer*>::iterator i;
  bool inserted=false;
  for (i=unvisualizedMapContainers.begin();i!=unvisualizedMapContainers.end();i++) {
    if ((*i)->getZoomLevelMap() > container->getZoomLevelMap()) {
      unvisualizedMapContainers.insert(i,container);
      inserted=true;
      break;
    }
  }
  if (!inserted)
    unvisualizedMapContainers.push_back(container);
}

// Removes the viualization for the given map container
void NavigationEngine::removeGraphics(MapContainer *container) {

  // Process the unfinished tile list
  for(std::list<NavigationPath*>::iterator i=routes.begin();i!=routes.end();i++) {
    (*i)->lockAccess(__FILE__, __LINE__);
    (*i)->removeVisualization(container);
    (*i)->unlockAccess();
  }
  if (recordedTrack) {
    recordedTrack->lockAccess(__FILE__, __LINE__);
    recordedTrack->removeVisualization(container);
    recordedTrack->unlockAccess();
  }
}

// Loads all pathes in the background
void NavigationEngine::backgroundLoader() {

  std::list<NavigationPath*>::iterator i;

  // Set the priority
  core->getThread()->setThreadPriority(threadPriorityBackgroundLow);

  // Thread can be cancelled
  core->getThread()->setThreadCancable();

  // Load the recorded track
  if (!recordedTrack->getIsInit()) {
    if (core->getQuitCore()) {
      goto exitThread;
    }
    if (recordedTrack->readGPXFile())
      recordedTrack->setIsInit(true);
  }

  // Load all routes
  i=routes.begin();
  while(i!=routes.end()) {
    if (core->getQuitCore()) {
      goto exitThread;
    }
    if (!(*i)->readGPXFile()) { // locking is handled within method
      NavigationPath *path=*i;
      i=routes.erase(i);
      deletePath(path);
    } else {
      (*i)->setIsInit(true);

      // Update the position of the start and end flags
      std::string routePath="Navigation/Route[@name='" + (*i)->getGpxFilename() + "']";
      Int startIndex=core->getConfigStore()->getIntValue(routePath,"startFlagIndex", __FILE__, __LINE__);
      //DEBUG("%s: startIndex=%d",(*i)->getGpxFilename().c_str(),startIndex);
      if (startIndex==-1) startIndex=0;
      //DEBUG("%s: startIndex=%d",(*i)->getGpxFilename().c_str(),startIndex);
      Int endIndex=core->getConfigStore()->getIntValue(routePath,"endFlagIndex", __FILE__, __LINE__);
      //DEBUG("%s: endIndex=%d",(*i)->getGpxFilename().c_str(),endIndex);
      if (endIndex==-1) endIndex=(*i)->getSelectedSize()-1;
      //DEBUG("%s: endIndex=%d",(*i)->getGpxFilename().c_str(),endIndex);
      if ((*i)->getReverse()) {
        if (startIndex>endIndex) {
          (*i)->setStartFlag(startIndex, __FILE__, __LINE__);
          (*i)->setEndFlag(endIndex, __FILE__, __LINE__);
        } else {
          (*i)->setStartFlag(endIndex, __FILE__, __LINE__);
          (*i)->setEndFlag(startIndex, __FILE__, __LINE__);
        }
      } else {
        if (startIndex<endIndex) {
          (*i)->setStartFlag(startIndex, __FILE__, __LINE__);
          (*i)->setEndFlag(endIndex, __FILE__, __LINE__);
        } else {
          (*i)->setStartFlag(endIndex, __FILE__, __LINE__);
          (*i)->setEndFlag(startIndex, __FILE__, __LINE__);
        }
      }

      // Add the visualization of the start and end flags
      updateFlagVisualization(*i);

      // Trigger navigation update if necessary
      if (*i==activeRoute)
        triggerNavigationInfoUpdate();
      i++;
    }
  }

  // Trigger download job processing
  core->getMapSource()->triggerDownloadJobProcessing();

  // Thread is finished
exitThread:
  core->getThread()->lockMutex(backgroundLoaderFinishedMutex, __FILE__, __LINE__);
  backgroundLoaderFinished=true;
  core->getThread()->unlockMutex(backgroundLoaderFinishedMutex);
}

// Sets the target to the center of the map
void NavigationEngine::setTargetAtMapCenter() {
  MapPosition pos = *(core->getMapEngine()->lockMapPos(__FILE__, __LINE__));
  core->getMapEngine()->unlockMapPos();
  NavigationPoint point;
  std::stringstream name;
  name << "(" << pos.getLat() << "," << pos.getLng() << ")";
  point.setName(name.str());
  point.setAddress(name.str());
  point.setLng(pos.getLng());
  point.setLat(pos.getLat());
  point.setGroup(core->getConfigStore()->getStringValue("Navigation","selectedAddressPointGroup",__FILE__,__LINE__));
  core->getNavigationEngine()->addAddressPoint(point);
  setTargetAtGeographicCoordinate(pos.getLng(),pos.getLat(),false);
}

// Makes the target invisible
void NavigationEngine::hideTarget() {
  std::string path="Navigation/Target";
  core->getConfigStore()->setIntValue(path,"visible",0,__FILE__,__LINE__);
  lockTargetPos(__FILE__, __LINE__);
  targetPos.invalidate();
  unlockTargetPos();
  core->getMapEngine()->setForceMapUpdate(__FILE__, __LINE__);
  triggerNavigationInfoUpdate();
}

// Shows the current target
void NavigationEngine::showTarget(bool repositionMap) {
  std::string path="Navigation/Target";
  core->getConfigStore()->setIntValue(path,"visible",1, __FILE__,__LINE__);
  double lng = core->getConfigStore()->getDoubleValue(path,"lng", __FILE__, __LINE__);
  double lat = core->getConfigStore()->getDoubleValue(path,"lat", __FILE__, __LINE__);
  lockTargetPos(__FILE__, __LINE__);
  targetPos.setLng(lng);
  targetPos.setLat(lat);
  targetPos.setIsUpdated(true);
  unlockTargetPos();
  if (repositionMap)
    core->getMapEngine()->setMapPos(targetPos);
  core->getMapEngine()->setForceMapUpdate(__FILE__, __LINE__);
  triggerNavigationInfoUpdate();
}

// Shows the current target at the given position
void NavigationEngine::setTargetAtGeographicCoordinate(double lng, double lat, bool repositionMap) {
  std::string path="Navigation/Target";
  core->getConfigStore()->setDoubleValue(path,"lng",lng,__FILE__,__LINE__);
  core->getConfigStore()->setDoubleValue(path,"lat",lat,__FILE__,__LINE__);
  showTarget(repositionMap);
}

// Sets the target pos directly
void NavigationEngine::setTargetPos(double lng, double lat) {
  lockTargetPos(__FILE__, __LINE__);
  if ((targetPos.getLng()!=lng)||(targetPos.getLat()!=lat))
    targetPos.setIsUpdated(true);
  targetPos.setLng(lng);
  targetPos.setLat(lat);
  unlockTargetPos();
}

// Updates the flags of a route
void NavigationEngine::updateFlagVisualization(NavigationPath *path) {
  core->getDefaultGraphicEngine()->lockDrawing(__FILE__,__LINE__);
  std::list<NavigationPointVisualization>::iterator i=navigationPointsVisualization.begin();
  MapPosition startFlagPos=path->getStartFlagPos();
  MapPosition endFlagPos=path->getEndFlagPos();
  bool replaceStartFlag=true;
  bool replaceEndFlag=true;
  while (i!=navigationPointsVisualization.end()) {
    if ((*i).getReference()==(void*)path) {
      bool remove=false;
      switch((*i).getVisualizationType()) {
      case NavigationPointVisualizationTypeStartFlag:
        if (((*i).getPos().getLat()==path->getStartFlagPos().getLat())&&((*i).getPos().getLng()==path->getStartFlagPos().getLng()))
          replaceStartFlag=false;
        else
          remove=true;
        break;
      case NavigationPointVisualizationTypeEndFlag:
        if (((*i).getPos().getLat()==path->getEndFlagPos().getLat())&&((*i).getPos().getLng()==path->getEndFlagPos().getLng()))
          replaceEndFlag=false;
        else
          remove=true;
        break;
      default:
        FATAL("unhandled case value",NULL);
      }
      if (remove) {
        core->getDefaultGraphicEngine()->lockDrawing(__FILE__,__LINE__);
        navigationPointsGraphicObject.removePrimitive((*i).getGraphicPrimitiveKey(),true);
        core->getDefaultGraphicEngine()->unlockDrawing();
        i=navigationPointsVisualization.erase(i);
      } else {
        i++;
      }
    } else {
      i++;
    }
  }
  if (replaceStartFlag) {
    NavigationPointVisualization startFlagVis(&navigationPointsGraphicObject, startFlagPos.getLat(),startFlagPos.getLng(), NavigationPointVisualizationTypeStartFlag,path->getGpxFilename(),(void*)path);
    navigationPointsVisualization.push_back(startFlagVis);
  }
  if (replaceEndFlag) {
    NavigationPointVisualization endFlagVis(&navigationPointsGraphicObject, endFlagPos.getLat(),endFlagPos.getLng(), NavigationPointVisualizationTypeEndFlag,path->getGpxFilename(),(void*)path);
    navigationPointsVisualization.push_back(endFlagVis);
  }
  resetOverlayGraphicHash();
  core->getDefaultGraphicEngine()->unlockDrawing();
  if ((replaceStartFlag)||(replaceEndFlag))
    updateScreenGraphic(false);
}

// Sets the start flag on the nearest route
void NavigationEngine::setStartFlag(NavigationPath *path, Int index, const char *file, int line) {
  if (path==recordedTrack) {
    WARNING("flags can not be set on tracks",NULL);
  } else {
    path->setStartFlag(index, file, line);
    updateFlagVisualization(path);
  }
  triggerNavigationInfoUpdate();
}

// Sets the end flag on the nearest route
void NavigationEngine::setEndFlag(NavigationPath *path, Int index, const char *file, int line) {
  if (path==recordedTrack) {
    WARNING("flags can not be set on tracks",NULL);
  } else {
    path->setEndFlag(index, file, line);
    updateFlagVisualization(path);
  }
  triggerNavigationInfoUpdate();
}

// Sets the active route
void NavigationEngine::setActiveRoute(NavigationPath *route) {
  if (route==recordedTrack) {
    WARNING("the currently recorded track can not be set as the active route",NULL);
    return;
  }
  if (activeRoute!=NULL) {
    activeRoute->lockAccess(__FILE__, __LINE__);
    activeRoute->setBlinkMode(false, __FILE__, __LINE__);
    activeRoute->unlockAccess();
  }
  core->getThread()->lockMutex(activeRouteMutex, __FILE__, __LINE__);
  activeRoute=route;
  core->getThread()->unlockMutex(activeRouteMutex);
  if (activeRoute) {
    activeRoute->lockAccess(__FILE__, __LINE__);
    activeRoute->setBlinkMode(true, __FILE__, __LINE__);
    activeRoute->unlockAccess();
    core->getConfigStore()->setStringValue("Navigation","activeRoute",activeRoute->getGpxFilename(),__FILE__, __LINE__);
  } else {
    core->getConfigStore()->setStringValue("Navigation","activeRoute","none",__FILE__, __LINE__);
  }
  triggerNavigationInfoUpdate();
}

// Calculates navigation infos such as bearing, distance, ...
void NavigationEngine::computeNavigationInfo() {

  std::stringstream infos;

  // Set the priority
  core->getThread()->setThreadPriority(threadPriorityBackgroundLow);
  
  // This thread can be cancelled
  core->getThread()->setThreadCancable();

  // Do an endless loop
  while(true) {

    // Wait for an compute trigger
    core->getThread()->waitForSignal(computeNavigationInfoSignal);

    // Copy location pos
    lockLocationPos(__FILE__, __LINE__);
    /*if (!this->locationPos.getHasBearing()) {
      this->locationPos.setHasBearing(true);
      this->locationPos.setBearing(0);
    }
    if (!this->locationPos.getHasSpeed()) {
      this->locationPos.setHasSpeed(true);
      this->locationPos.setSpeed(0);
    }
    this->locationPos.setBearing(this->locationPos.getBearing()+4);
    this->locationPos.setSpeed(this->locationPos.getSpeed()+1);*/
    MapPosition locationPos=this->locationPos;
    unlockLocationPos();

    // Use the last bearing if the new location has no bearing

    // Check if the infos need to be updated
    if ((lastNavigationLocationPos.isValid())&&(!forceNavigationInfoUpdate)) {
      double travelledDistance = locationPos.computeDistance(lastNavigationLocationPos);
      if (travelledDistance < minDistanceToNavigationUpdate)
        continue;
    }
    lastNavigationLocationPos=locationPos;
    forceNavigationInfoUpdate=false;

    // Copy target pos
    lockTargetPos(__FILE__, __LINE__);
    MapPosition targetPos=this->targetPos;
    unlockTargetPos();

    // Update the nearest address point
    bool pointValid=false;
    NavigationPoint point;
    double distance=std::numeric_limits<double>::max();
    if (locationPos.isValid()) {
      core->getDefaultGraphicEngine()->lockDrawing(__FILE__,__LINE__);
      for (std::list<NavigationPoint>::iterator i=addressPoints.begin();i!=addressPoints.end();i++) {
        MapPosition pos;
        pos.setLat((*i).getLat());
        pos.setLng((*i).getLng());
        double t=locationPos.computeDistance(pos);
        if (t<distance) {
          distance=t;
          point=*i;
          pointValid=true;
        }
      }
      core->getDefaultGraphicEngine()->unlockDrawing();
    }
    core->getThread()->lockMutex(nearestAddressPointMutex,__FILE__,__LINE__);
    this->nearestAddressPoint=point;
    this->nearestAddressPointValid=pointValid;
    this->nearestAddressPointDistance=distance;
    
    // If a route is active, compute the details for the given route
    NavigationInfo navigationInfo=NavigationInfo();
    if (locationPos.isValid()) {
      if (locationPos.getHasBearing()) {
        navigationInfo.setLocationBearing(locationPos.getBearing());
      }
      if (locationPos.getHasSpeed()) {
        navigationInfo.setLocationSpeed(locationPos.getSpeed());
      }

      // If a target is active, compute the details for the given target
      if (targetPos.isValid()) {
        navigationInfo.setType(NavigationInfoTypeTarget);
        if (navigationInfo.getLocationBearing()!=NavigationInfo::getUnknownAngle())
          navigationInfo.setTargetBearing(locationPos.computeBearing(targetPos));
        navigationInfo.setTargetDistance(locationPos.computeDistance(targetPos));
      } else {

        core->getThread()->lockMutex(activeRouteMutex, __FILE__, __LINE__);
        NavigationPath *activeRoute=this->activeRoute;
        core->getThread()->unlockMutex(activeRouteMutex);
        if (activeRoute) {

          // Compute the navigation details for the given route
          //static MapPosition prevTurnPos;
          navigationInfo.setType(NavigationInfoTypeRoute);
          activeRoute->computeNavigationInfo(locationPos,targetPos,navigationInfo);
          //WARNING("enable route locking",NULL);
          if (targetPos.isValid()) {
            //setTargetAtGeographicCoordinate(targetPos.getLng(),targetPos.getLat(),false);
            if (navigationInfo.getLocationBearing()!=NavigationInfo::getUnknownAngle())
              navigationInfo.setTargetBearing(locationPos.computeBearing(targetPos));
          }
          /*if (navigationInfo.getTurnDistance()!=NavigationInfo::getUnknownDistance()) {
            if (navigationInfo.getTurnAngle()>0) {
              DEBUG("turn to the left by %f°in %f meters",navigationInfo.getTurnAngle(),navigationInfo.getTurnDistance());
            } else {
              DEBUG("turn to the right by %f° in %f meters",navigationInfo.getTurnAngle(),navigationInfo.getTurnDistance());
            }
            if ((!prevTurnPos.isValid())||(prevTurnPos!=turnPos)) {
              setTargetAtGeographicCoordinate(turnPos.getLng(),turnPos.getLat(),false);
              sleep(1);
              DEBUG("new target set",NULL);
            }
          }*/
          //prevTurnPos=turnPos;
          
          // If we are not moving, don't indicate that we are off route
          double speed=0;
          if (locationPos.getHasSpeed()) {
            speed=locationPos.getSpeed();
          }
          //DEBUG("speed=%f",speed);
          if (speed<minSpeedToSignalOffRoute) 
            navigationInfo.setOffRoute(false);          
        }
      }
      if ((nearestAddressPointValid)&&(nearestAddressPointDistance<=maxAddressPointAlarmDistance)) {
        navigationInfo.setNearestNavigationPointDistance(nearestAddressPointDistance);
        if (navigationInfo.getLocationBearing()!=NavigationInfo::getUnknownAngle()) {
          MapPosition nearestAddressPointPos;
          nearestAddressPointPos.setLat(nearestAddressPoint.getLat());
          nearestAddressPointPos.setLng(nearestAddressPoint.getLng());    
          navigationInfo.setNearestNavigationPointBearing(locationPos.computeBearing(nearestAddressPointPos));
        }
      }
    }
    if (navigationInfo.getLocationSpeed()!=NavigationInfo::getUnknownSpeed()) {
      if (navigationInfo.getTargetDistance()!=NavigationInfo::getUnknownDistance())
        navigationInfo.setTargetDuration(navigationInfo.getTargetDistance() / navigationInfo.getLocationSpeed());
    }

    // Update remaining fields
    lockRecordedTrack(__FILE__, __LINE__);
    if (recordedTrack)
      navigationInfo.setTrackLength(recordedTrack->getLength());
    unlockRecordedTrack();
    if (locationPos.getHasAltitude())
      navigationInfo.setAltitude(locationPos.getAltitude());

    // Set the new navigation info
    lockNavigationInfo(__FILE__, __LINE__);
    this->navigationInfo=navigationInfo;
    unlockNavigationInfo();

    // Copy target pos again to get the unmodified one
    lockTargetPos(__FILE__, __LINE__);
    targetPos=this->targetPos;
    unlockTargetPos();

    // Play alert if new address point is found
    if ((pointValid)&&(nearestAddressPointDistance<=maxAddressPointAlarmDistance)) {
      if (nearestAddressPointName!=point.getName()) {
        core->getCommander()->dispatch("playNewNearestAddressPointAlarm()");
        nearestAddressPointName=point.getName();
        nearestAddressPointNameUpdate=core->getClock()->getMicrosecondsSinceStart();
      }
      nearestAddressPointAlarm=true;
    } else {
      if (nearestAddressPointName!="") {
        nearestAddressPointNameUpdate=core->getClock()->getMicrosecondsSinceStart();
      }
      nearestAddressPointName="";
      nearestAddressPointAlarm=false;
    }
    core->getThread()->unlockMutex(nearestAddressPointMutex);
    core->onDataChange();

    //PROFILE_ADD("position update init");

    // Update the parent app
    infos.str("");
    std::string value,unit;
    if (navigationInfo.getLocationBearing()!=NavigationInfo::getUnknownAngle())
      infos << navigationInfo.getLocationBearing();
    else
      infos << "-";
    if (navigationInfo.getLocationSpeed()!=NavigationInfo::getUnknownSpeed()) {
      core->getUnitConverter()->formatMetersPerSecond(navigationInfo.getLocationSpeed(),value,unit);
      infos << ";" << value << " " << unit;
    } else
      infos << ";-";
    if (navigationInfo.getTargetBearing()!=NavigationInfo::getUnknownAngle())
      infos << ";" << navigationInfo.getTargetBearing();
    else
      infos << ";-";
    infos << ";";
    if (navigationInfo.getTargetDistance()!=NavigationInfo::getUnknownDistance()) {
      core->getUnitConverter()->formatMeters(navigationInfo.getTargetDistance(),value,unit);
      infos << value << " " << unit;
    } else {
      infos << "infinite";
    }
    infos << ";";
    if (navigationInfo.getOffRoute()) {
      infos << "off route!";
    } else if (navigationInfo.getTargetDuration()!=NavigationInfo::getUnknownDuration()) {
      core->getUnitConverter()->formatTime(navigationInfo.getTargetDuration(),value,unit);
      infos << value << " " << unit;
    } else {
      infos << "move!";
    }
    if (navigationInfo.getTurnDistance()!=NavigationInfo::getUnknownDistance()) {
      infos << ";" << navigationInfo.getTurnAngle();
      core->getUnitConverter()->formatMeters(navigationInfo.getTurnDistance(),value,unit);
      infos << ";" << value << " " << unit;
    } else {
      infos << ";-;-";
    }
    if (navigationInfo.getType() == NavigationInfoTypeRoute) {
      if (navigationInfo.getOffRoute()) {
        core->getUnitConverter()->formatMeters(navigationInfo.getRouteDistance(),value,unit);
        infos << ";off route;" << value << " " << unit;
      } else
        infos << ";on route;-";
    } else {
      infos << ";no route;-";
    }
    infos << ";";
    if (navigationInfo.getNearestNavigationPointBearing()!=NavigationInfo::getUnknownAngle())
      infos << navigationInfo.getNearestNavigationPointBearing();
    else
      infos << "-";
    infos << ";";
    if (navigationInfo.getNearestNavigationPointDistance()!=NavigationInfo::getUnknownDistance()) {
      core->getUnitConverter()->formatMeters(navigationInfo.getNearestNavigationPointDistance(),value,unit);
      infos << value << " " << unit;
    } else {
      infos << "infinite";
    }
    core->getCommander()->dispatch("setFormattedNavigationInfo(" + infos.str() + ")");
    std::string cmd="setAllNavigationInfo(" + infos.str() + ")";
    infos.str("");
    infos << navigationInfo.getType() << ",";
    infos << navigationInfo.getAltitude() << ",";
    infos << navigationInfo.getLocationBearing() << ",";
    infos << navigationInfo.getLocationSpeed() << ",";
    infos << navigationInfo.getTrackLength() << ",";
    infos << navigationInfo.getTargetBearing() << ",";
    infos << navigationInfo.getTargetDistance() << ",";
    infos << navigationInfo.getTargetDuration() << ",";
    infos << navigationInfo.getOffRoute() << ",";
    infos << navigationInfo.getRouteDistance() << ",";
    infos << navigationInfo.getTurnAngle() << ",";
    infos << navigationInfo.getTurnDistance() << ",";
    infos << navigationInfo.getNearestNavigationPointBearing() << ",";
    infos << navigationInfo.getNearestNavigationPointDistance();
    cmd += "(" + infos.str() + ")";
    infos.str("");
    infos << locationPos.getSource() << ",";
    infos << locationPos.getTimestamp() << ",";
    infos << locationPos.getLng() << ",";
    infos << locationPos.getLat() << ",";
    infos << locationPos.getHasAltitude() << ",";
    infos << locationPos.getAltitude() << ",";
    infos << locationPos.getIsWGS84Altitude() << ",";
    infos << locationPos.getHasBearing() << ",";
    infos << locationPos.getBearing() << ",";
    infos << locationPos.getHasSpeed() << ",";
    infos << locationPos.getSpeed() << ",";
    infos << locationPos.getHasAccuracy() << ",";
    infos << locationPos.getAccuracy();
    cmd += "(" + infos.str() + ")";
    infos.str("");
    infos << targetPos.getLng() << ",";
    infos << targetPos.getLat() << ",";
    cmd += "(" + infos.str() + ")";
    //DEBUG("%s",cmd.c_str());
    core->getCommander()->dispatch(cmd);
  }
}

// Adds a new address point (and deletes on old one if max history is exceeded)
void NavigationEngine::addAddressPoint(NavigationPoint point) {

  ConfigStore *configStore = core->getConfigStore();

  // Remember the new address point value
  point.writeToConfig("Navigation/AddressPoint");

  // Re-create list of address points
  initAddressPoints();

  // Inform widget engine that data has changed
  core->onDataChange();
}

// Renames an existing address point
std::string NavigationEngine::renameAddressPoint(std::string oldName, std::string newName) {

  ConfigStore *configStore = core->getConfigStore();

  // Check if the address point already exists
  std::string path = "Navigation/AddressPoint";
  NavigationPoint point;
  point.setName(oldName);
  if (point.readFromConfig(path)) {
    configStore->removePath(path + "[@name='" + point.getName() + "']");
    point.setName(newName);
    point.writeToConfig(path,point.getTimestamp());
    initAddressPoints();
  }

  // Inform widget engine that data has changed
  core->onDataChange();

  // Return new name (in case it has changed)
  return point.getName();
}

// Removes an address point
void NavigationEngine::removeAddressPoint(std::string name) {
  //DEBUG("name=%s",name.c_str());
  std::string path = "Navigation/AddressPoint[@name='" + name + "']";
  core->getConfigStore()->removePath(path);
  initAddressPoints();

  // Inform widget engine that data has changed
  core->onDataChange();
}

// Reads the address points from disk
void NavigationEngine::initAddressPoints() {

  // Read the new address points
  std::string path = "Navigation/AddressPoint";
  std::list<std::string> names = core->getConfigStore()->getAttributeValues(path,"name",__FILE__,__LINE__);
  std::string selectedAddressPointGroup = core->getConfigStore()->getStringValue("Navigation","selectedAddressPointGroup",__FILE__,__LINE__);
  std::list<NavigationPoint> storedAddressPoints;
  for (std::list<std::string>::iterator j=names.begin();j!=names.end();j++) {
    //DEBUG("name=%s",(*j).c_str());
    NavigationPoint addressPoint;
    addressPoint.setName(*j);
    addressPoint.readFromConfig(path);
    if (addressPoint.getGroup()==selectedAddressPointGroup) {
      storedAddressPoints.push_back(addressPoint);
    }
  }

  // Check which points have changed
  core->getDefaultGraphicEngine()->lockDrawing(__FILE__,__LINE__);
  std::list<NavigationPoint> addAddressPoints;
  std::list<NavigationPoint> removeAddressPoints;
  for (std::list<NavigationPoint>::iterator i=storedAddressPoints.begin();i!=storedAddressPoints.end();i++) {
    bool found=false;
    for (std::list<NavigationPoint>::iterator j=addressPoints.begin();j!=addressPoints.end();j++) {

      // Same address point?      
      if (j->getName()==i->getName()) {

        // Has the address point changed?
        if (*j!=*i) {

          // Remove the old one and remember the new one for later addition
          //DEBUG("address point <%s> will be removed",j->getName().c_str());
          removeAddressPoints.push_back(*j);
          break;
        } else {
          found=true;
          break;
        }
      }
    }
    if (!found) {
      //DEBUG("address point <%s> will be added",i->getName().c_str());
      addAddressPoints.push_back(*i);
    } else {
      //DEBUG("address point <%s> will be skipped",i->getName().c_str());
    }
  }

  // Check which points have been removed
  for (std::list<NavigationPoint>::iterator i=addressPoints.begin();i!=addressPoints.end();i++) {
    bool found=false;
    for (std::list<NavigationPoint>::iterator j=storedAddressPoints.begin();j!=storedAddressPoints.end();j++) {
      if (j->getName()==i->getName()) {
        found=true;
        break;
      }
    }
    if (!found) {
      //DEBUG("address point <%s> will be removed",i->getName().c_str());
      removeAddressPoints.push_back(*i);
    }
  }

  // Remove outdated address points
  for (std::list<NavigationPoint>::iterator i=removeAddressPoints.begin();i!=removeAddressPoints.end();i++) {
    for (std::list<NavigationPointVisualization>::iterator j=navigationPointsVisualization.begin();j!=navigationPointsVisualization.end();j++) {
      if ((j->getVisualizationType()==NavigationPointVisualizationTypePoint)&&(j->getName()==i->getName())) {
        core->getDefaultGraphicEngine()->lockDrawing(__FILE__,__LINE__);
        navigationPointsGraphicObject.removePrimitive(j->getGraphicPrimitiveKey(),true);
        core->getDefaultGraphicEngine()->unlockDrawing();
        navigationPointsVisualization.erase(j);
        break;
      }
    }
  }

  // Add new address points
  for (std::list<NavigationPoint>::iterator i=addAddressPoints.begin();i!=addAddressPoints.end();i++) {
    NavigationPointVisualization pointVis(&navigationPointsGraphicObject, i->getLat(),i->getLng(), NavigationPointVisualizationTypePoint, i->getName(),NULL);
    navigationPointsVisualization.push_back(pointVis);
  }
  addressPoints=storedAddressPoints;
  resetOverlayGraphicHash();
  core->getDefaultGraphicEngine()->unlockDrawing();

  // Trigger updates
  triggerNavigationInfoUpdate();
  core->getCommander()->dispatch("forceRemoteMapUpdate()");
}

// Finds a route with the given name
NavigationPath *NavigationEngine::findRoute(std::string name) {
  for (std::list<NavigationPath*>::iterator i=routes.begin();i!=routes.end();i++) {
    if ((*i)->getName()==name) {
      return *i;
    }
  }
  return NULL;
}

// Returns the name of the address point at the given position
std::string NavigationEngine::getAddressPointName(GraphicPosition visPos) {

  std::string result="";
  core->getDefaultGraphicEngine()->lockDrawing(__FILE__,__LINE__);
  std::list<GraphicPrimitive*> *visibleAddressPoints = navigationPointsGraphicObject.getDrawList();
  //DEBUG("visPos.getX()=%d visPos.getY()=%d visPos.getZoom()=%f",visPos.getX(),visPos.getY(),visPos.getZoom());
  for (std::list<GraphicPrimitive*>::iterator i=visibleAddressPoints->begin();i!=visibleAddressPoints->end();i++) {
    GraphicRectangle *r=(GraphicRectangle*)*i;
    double diffX = r->getX()-visPos.getX();
    double diffY = r->getY()-visPos.getY();
    double distance = sqrt( diffX*diffX + diffY*diffY ) * visPos.getZoom();
    //DEBUG("distance=%f",distance);
    //DEBUG("r.getX()=%d r.getY()=%d",r->getX(),r->getY());
    if (distance < r->getIconHeight()/8) {
      result=r->getName().front();
      //DEBUG("result=%s",result.c_str());
      break;
    }
  }
  core->getDefaultGraphicEngine()->unlockDrawing();
  return result;
}

// Exports the active route inclusive selection as an GPX file
void NavigationEngine::exportActiveRoute() {
  if (activeRoute!=NULL) {
    activeRoute->lockAccess(__FILE__,__LINE__);
    std::string name = activeRoute->getName() + " " + core->getClock()->getFormattedDate();
    std::string filepath = getExportRoutePath() + "/" + name + ".gpx";
    activeRoute->unlockAccess();
    activeRoute->writeGPXFile(true,true,true,true,name,filepath);
    INFO("exported to %s",getExportRoutePath().c_str());

  } else {
    WARNING("no route selected",NULL);
  }
}

// Stores the navigation points into a file
void NavigationEngine::storeOverlayGraphics(std::string filefolder, std::string filename) {

  // Create the overlay file
  std::string filepath = filefolder + "/" + filename;
  remove(filepath.c_str());
  std::ofstream ofs;
  ofs.open(filepath.c_str(),std::ios::binary);
  if (ofs.fail()) {
    FATAL("can not open <%s> for writing",filepath.c_str());
    return;
  }

  // Store the type of overlay
  Storage::storeByte(&ofs,OverlayArchiveTypeNavigationEngine);

  // Store all navigation points
  core->getDefaultGraphicEngine()->lockDrawing(__FILE__,__LINE__);
  for (std::list<NavigationPointVisualization>::iterator i=navigationPointsVisualization.begin();i!=navigationPointsVisualization.end();i++) {
    (*i).store(&ofs);
  }
  core->getDefaultGraphicEngine()->unlockDrawing();

  // Close the overlay file
  ofs.close();

  // Compute the hash
  overlayGraphicHash=Storage::computeMD5(filepath);
}

// Recreates the navigation points from a binary file
void NavigationEngine::retrieveOverlayGraphics(std::string filefolder, std::string filename) {

  // Open the file
  std::string filepath = filefolder + "/" + filename;
  std::ifstream ifs;
  ifs.open(filepath.c_str(),std::ios::binary);
  if (ifs.fail()) {
    ERROR("can not open <%s> for reading",filepath.c_str());
    return;
  }

  // Load the complete file into memory
  struct stat filestat;
  char *dataUnaligned;
  core->statFile(filepath,&filestat);
  if (!(dataUnaligned=(char*)malloc(filestat.st_size+1+sizeof(double)-1))) {
    FATAL("can not allocate memory for reading complete file",NULL);
    return;
  }
  char *data=dataUnaligned+(((ULong)dataUnaligned)%sizeof(double));
  ifs.read(data,filestat.st_size);
  ifs.close();
  data[filestat.st_size]=0; // to prevent that strings never end
  Int size=filestat.st_size;
  //DEBUG("size=%d",size);

  // Skip the type
  Byte t;
  Storage::retrieveByte(data,size,t);

  // Clear the navigation points
  core->getDefaultGraphicEngine()->lockDrawing(__FILE__,__LINE__);
  std::list<NavigationPointVisualization>::iterator i=navigationPointsVisualization.begin();
  while (i!=navigationPointsVisualization.end()) {
    navigationPointsGraphicObject.removePrimitive((*i).getGraphicPrimitiveKey(),true);
    i=navigationPointsVisualization.erase(i);
  }

  // Read all navigation points
  while (size>0) {
    NavigationPointVisualization navigationPointVisualization(&navigationPointsGraphicObject);
    navigationPointVisualization.retrieve(data,size);
    navigationPointsVisualization.push_back(navigationPointVisualization);
  }
  core->getDefaultGraphicEngine()->unlockDrawing();

  // That's it!
  if (size!=0) {
    ERROR("not all data consumed",NULL);
  }
  free(dataUnaligned);

  // Compute the hash
  overlayGraphicHash=Storage::computeMD5(filepath);
}

// Forces an update of the navigation infos
void NavigationEngine::triggerNavigationInfoUpdate() {
  forceNavigationInfoUpdate=true;
  core->getThread()->issueSignal(computeNavigationInfoSignal);
  core->getCommander()->dispatch("forceRemoteMapUpdate()");
}

// Updates the address point group that is displayed on screen
void NavigationEngine::addressPointGroupChanged() {
  initAddressPoints();
  core->onDataChange();
}

// Returns the address point that is the nearest to the current position
bool NavigationEngine::getNearestAddressPoint(NavigationPoint &navigationPoint, double &distance, TimestampInMicroseconds &updateTimestamp, bool &alarm) {
  core->getThread()->lockMutex(nearestAddressPointMutex,__FILE__,__LINE__);
  bool found=nearestAddressPointValid;
  if (found) {
    navigationPoint=nearestAddressPoint;
    distance=nearestAddressPointDistance;
  }
  updateTimestamp=nearestAddressPointNameUpdate;
  alarm=nearestAddressPointAlarm;
  core->getThread()->unlockMutex(nearestAddressPointMutex);
  return found;
}

// Forces an update of the google bookmarks
void NavigationEngine::triggerGoogleBookmarksSynchronization() {
  core->getThread()->issueSignal(synchronizeGoogleBookmarksSignal);
}

// Removes the path from the map and the disk
void NavigationEngine::trashPath(NavigationPath *path) {

  // The track in recording cannot be removed
  NavigationPath *nearestPath=lockRecordedTrack(__FILE__,__LINE__);
  if (path==recordedTrack) {
    WARNING("cannot delete the path (used for recording)",NULL);
    unlockRecordedTrack();
    return;
  }
  unlockRecordedTrack();

  // If the track is not yet loaded, it cannot be removed
  if (!path->getIsInit()) {
    WARNING("cannot delete the path (not yet loaded)",NULL);
    return;
  }

  // If the path is used as the active route, disable it
  if (path==getActiveRoute()) {
    setActiveRoute(NULL);
  }

  // First delete it from the disk
  remove((path->getGpxFilefolder()+"/"+path->getGpxFilename()).c_str());

  // Then remove the path from the route list
  lockRoutes(__FILE__, __LINE__);
  routes.remove(path);
  unlockRoutes();

  // Remove the flags
  std::list<NavigationPointVisualization>::iterator i=navigationPointsVisualization.begin();
  while (i!=navigationPointsVisualization.end()) {
    if ((*i).getReference()==(void*)path) {
      core->getDefaultGraphicEngine()->lockDrawing(__FILE__,__LINE__);
      navigationPointsGraphicObject.removePrimitive((*i).getGraphicPrimitiveKey(),true);
      core->getDefaultGraphicEngine()->unlockDrawing();
      i=navigationPointsVisualization.erase(i);
    } else {
      i++;
    }
  }

  // Ensure that no one is using the path anymore
  MapPosition pos = *core->getMapEngine()->lockMapPos(__FILE__,__LINE__);
  core->getMapEngine()->unlockMapPos();
  std::list<MapTile*> *centerMapTiles = core->getMapEngine()->lockCenterMapTiles(__FILE__,__LINE__);
  core->onMapChange(pos,centerMapTiles);
  core->getMapEngine()->unlockCenterMapTiles();

  // Delete the path from memory
  deletePath(path);
}
  
}

