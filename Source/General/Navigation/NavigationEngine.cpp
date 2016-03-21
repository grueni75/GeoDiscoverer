//============================================================================
// Name        : NavigationEngine.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#include <Core.h>

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

// Constructor
NavigationEngine::NavigationEngine() {

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
  updateGraphicsMutex=core->getThread()->createMutex("navigation engine update graphics mutex");
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
  minDistanceToNavigationUpdate=core->getConfigStore()->getDoubleValue("Navigation","minDistanceToNavigationUpdate", __FILE__, __LINE__);
  forceNavigationInfoUpdate=false;
  computeNavigationInfoThreadInfo=NULL;
  computeNavigationInfoSignal=core->getThread()->createSignal();

  // Create the track directory if it does not exist
  struct stat st;
  if (stat(getTrackPath().c_str(), &st) != 0)
  {
    if (mkdir(getTrackPath().c_str(),S_IRWXU | S_IRWXG | S_IRWXO)!=0) {
      FATAL("can not create track directory!",NULL);
      return;
    }
  }

  // Create the route directory if it does not exist
  if (stat(getRoutePath().c_str(), &st) != 0)
  {
    if (mkdir(getRoutePath().c_str(),S_IRWXU | S_IRWXG | S_IRWXO)!=0) {
      FATAL("can not create route directory!",NULL);
      return;
    }
  }
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
  core->getThread()->destroyMutex(updateGraphicsMutex);
  core->getThread()->destroyMutex(statusMutex);
  core->getThread()->destroyMutex(targetPosMutex);
  core->getThread()->destroyMutex(backgroundLoaderFinishedMutex);
  core->getThread()->destroyMutex(navigationInfosMutex);
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
  recordedTrack->setNormalColor (c->getGraphicColorValue("Navigation/TrackColor", __FILE__, __LINE__), __FILE__, __LINE__);

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

  // Start the background loader
  if (!(backgroundLoaderThreadInfo=core->getThread()->createThread("navigation engine background loader thread",navigationEngineBackgroundLoaderThread,this)))
    FATAL("can not start background loader thread",NULL);

  // Start the navigation info calculator
  if (!(computeNavigationInfoThreadInfo=core->getThread()->createThread("navigation engine compute navigation info thread",navigationEngineComputeNavigationInfoThread,this)))
    FATAL("can not start compute navigation info thread",NULL);

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
  DIR *dp = opendir( getRoutePath().c_str() );
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
    if (stat( filepath.c_str(), &filestat ))        continue;
    if (S_ISDIR( filestat.st_mode ))                continue;

    // If the file is a backup, skip it
    if (filename.substr(filename.length()-1)=="~")  continue;

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

  // Save the track first
  if (recordedTrack) {
    recordedTrack->writeGPXFile(); // locking is handled within writeGPXFile()
    core->onPathChange(recordedTrack,NavigationPathChangeTypeWillBeRemoved);
    lockRecordedTrack(__FILE__, __LINE__);
    delete recordedTrack;
    recordedTrack=NULL;
    unlockRecordedTrack();
  }

  // Free all routes
  lockRoutes(__FILE__, __LINE__);
  for (std::list<NavigationPath*>::iterator i=routes.begin();i!=routes.end();i++) {
    core->onPathChange(*i,NavigationPathChangeTypeWillBeRemoved);
    delete *i;
  }
  routes.clear();
  unlockRoutes();

  // Object is not initialized
  isInitialized=false;
}

// Updates the current location
void NavigationEngine::newLocationFix(MapPosition newLocationPos) {

  bool updatePos=false;
  bool isNewer=false;

  //PROFILE_START;

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
    lockLocationPos(__FILE__, __LINE__);
    locationPos=newLocationPos;
    unlockLocationPos();
    MapPosition *pos=core->getMapEngine()->lockLocationPos(__FILE__, __LINE__);
    *pos=locationPos;
    core->getMapEngine()->unlockLocationPos();

    //PROFILE_ADD("position update init");

    // Update the navigation infos
    core->getThread()->issueSignal(computeNavigationInfoSignal);

    // Update the current track
    updateTrack();
    //PROFILE_ADD("track update");

    // Inform the widget engine
    core->onLocationChange(locationPos);

    // Update the graphics
    updateScreenGraphic(false);
    //PROFILE_ADD("graphics update");

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

  // Ensure that only one thread is executing this function at most
  //DEBUG("before graphics update",NULL);
  core->getThread()->lockMutex(updateGraphicsMutex, __FILE__, __LINE__);

  // We need a map tile to compute the coordinates
  //DEBUG("before map pos lock",NULL);
  mapPos=*(mapEngine->lockMapPos(__FILE__, __LINE__));
  mapEngine->unlockMapPos();
  //DEBUG("after map pos lock",NULL);
  if (!mapPos.getMapTile()) {
    core->getThread()->unlockMutex(updateGraphicsMutex);
    //DEBUG("after graphics update",NULL);
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
  if ((locationPos.isValid())&&(mapPos.getMapTile())) {
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
  double targetIconPosValid=false;
  updatePosition=false;
  bool updateAnimation=false;
  double screenZoom=visPos.getZoom();
  double screenAngle=FloatingPoint::degree2rad(visPos.getAngle());
  Int zoomedScreenWidth=floor(((double)core->getDefaultScreen()->getWidth())/screenZoom);
  Int zoomedScreenHeight=floor(((double)core->getDefaultScreen()->getHeight())/screenZoom);
  //DEBUG("screenZoom=%f zoomedScreenWidth=%d zoomedScreenHeight=%d",screenZoom,zoomedScreenWidth,zoomedScreenHeight);
  if ((targetPos.isValid())&&(mapPos.getMapTile())) {
    showCursor=true;
    MapCalibrator *calibrator=mapPos.getMapTile()->getParentMapContainer()->getMapCalibrator();
    //DEBUG("calibrator=%08x",calibrator);
    if (!calibrator->setPictureCoordinates(targetPos)) {
      showCursor=false;
    } else {
      //DEBUG("locationPos.getX()=%d locationPos.getY()=%d",locationPos.getX(),locationPos.getY());
      if (!Integer::add(targetPos.getX(),-mapPos.getX(),mapDiffX))
        showCursor=false;
      if (!Integer::add(targetPos.getY(),-mapPos.getY(),mapDiffY))
        showCursor=false;
      //DEBUG("mapDiffX=%d mapDiffY=%d",mapDiffX,mapDiffY);
      if (!Integer::add(displayArea.getRefPos().getX(),mapDiffX,visPosX))
        showCursor=false;
      if (!Integer::add(displayArea.getRefPos().getY(),-mapDiffY,visPosY))
        showCursor=false;
      //DEBUG("mapDiffX=%d mapDiffY=%d",mapDiffX,mapDiffY);
      if (showCursor) {
        targetIconPosValid=true;
        if (abs(mapDiffX)>zoomedScreenWidth/2)
          showCursor=false;
        if (abs(mapDiffY)>zoomedScreenHeight/2)
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
  if ((targetIconPosValid)&&(!showCursor)) {
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
  core->getDefaultGraphicEngine()->unlockArrowIcon();

  // Unlock the drawing mutex
  core->getThread()->unlockMutex(updateGraphicsMutex);
  //DEBUG("after graphics update",NULL);
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

  // Clear the buffers used in the path object
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

  // This thread can be cancelled
  core->getThread()->setThreadCancable();

  // Load the recorded track
  if (!recordedTrack->getIsInit()) {
    if (core->getQuitCore()) {
      goto exitThread;
    }
    recordedTrack->readGPXFile(); // locking is handled within method
    recordedTrack->setIsInit(true);
  }

  // Load all routes
  i=routes.begin();
  while(i!=routes.end()) {
    if (core->getQuitCore()) {
      goto exitThread;
    }
    if (!(*i)->readGPXFile()) { // locking is handled within method
      i=routes.erase(i);
    } else {
      (*i)->setIsInit(true);
      std::string routePath="Navigation/Route[@name='" + (*i)->getGpxFilename() + "']";
      Int startIndex=core->getConfigStore()->getIntValue(routePath,"startFlagIndex", __FILE__, __LINE__);
      if (startIndex==-1) startIndex=0;
      Int endIndex=core->getConfigStore()->getIntValue(routePath,"endFlagIndex", __FILE__, __LINE__);
      if (endIndex==-1) endIndex=(*i)->getSelectedSize()-1;
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
      if (*i==activeRoute)
        triggerNavigationInfoUpdate();
      i++;
    }
  }

  // Set the active route

  // Thread is finished
exitThread:
  core->getThread()->lockMutex(backgroundLoaderFinishedMutex, __FILE__, __LINE__);
  backgroundLoaderFinished=true;
  core->getThread()->unlockMutex(backgroundLoaderFinishedMutex);
}

// Adds a new point of interest
void NavigationEngine::newPointOfInterest(std::string name, std::string description, double lng, double lat) {
  DEBUG("name=%s description=%s lng=%f lat=%f",name.c_str(),description.c_str(),lng,lat);
  setTargetAtGeographicCoordinate(lng,lat,true);
}

// Sets the target to the center of the map
void NavigationEngine::setTargetAtMapCenter() {
  MapPosition *pos = core->getMapEngine()->lockMapPos(__FILE__, __LINE__);
  setTargetAtGeographicCoordinate(pos->getLng(),pos->getLat(),false);
  core->getMapEngine()->unlockMapPos();
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

// Sets the start flag on the nearest route
void NavigationEngine::setStartFlag(NavigationPath *path, Int index, const char *file, int line) {
  if (path==recordedTrack) {
    WARNING("flags can not be set on tracks",NULL);
  } else {
    path->setStartFlag(index, file, line);
  }
  triggerNavigationInfoUpdate();
}

// Sets the end flag on the nearest route
void NavigationEngine::setEndFlag(NavigationPath *path, Int index, const char *file, int line) {
  if (path==recordedTrack) {
    WARNING("flags can not be set on tracks",NULL);
  } else {
    path->setEndFlag(index, file, line);
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

    // Copy target possetPlain
    lockTargetPos(__FILE__, __LINE__);
    MapPosition targetPos=this->targetPos;
    unlockTargetPos();

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
    core->getCommander()->dispatch("setFormattedNavigationInfo(" + infos.str() + ")");
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
    infos << navigationInfo.getTurnDistance();
    //DEBUG("%s",infos.str().c_str());
    core->getCommander()->dispatch("setPlainNavigationInfo(" + infos.str() + ")");
  }
}


}

