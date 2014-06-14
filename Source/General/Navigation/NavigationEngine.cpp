//============================================================================
// Name        : NavigationEngine.cpp
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

// Background loader thread
void *navigationEngineBackgroundLoaderThread(void *args) {
  ((NavigationEngine*)args)->backgroundLoader();
  return NULL;
}

// Constructor
NavigationEngine::NavigationEngine() {

  // Init variables
  locationOutdatedThreshold=core->getConfigStore()->getIntValue("Navigation","locationOutdatedThreshold");
  locationSignificantlyInaccurateThreshold=core->getConfigStore()->getIntValue("Navigation","locationSignificantlyInaccurateThreshold");
  trackRecordingMinDistance=core->getConfigStore()->getDoubleValue("Navigation","trackRecordingMinDistance");
  backgroundLoaderFinishedMutex=core->getThread()->createMutex();
  recordedTrackMutex=core->getThread()->createMutex();
  routesMutex=core->getThread()->createMutex();
  locationPosMutex=core->getThread()->createMutex();
  compassBearingMutex=core->getThread()->createMutex();
  updateGraphicsMutex=core->getThread()->createMutex();
  recordTrack=core->getConfigStore()->getIntValue("Navigation","recordTrack");
  compassBearing=0;
  isInitialized=false;
  recordedTrack=NULL;
  activeRoute=NULL;
  backgroundLoaderThreadInfo=NULL;
  statusMutex=core->getThread()->createMutex();
  targetPosMutex=core->getThread()->createMutex();
  targetVisible=false;
  arrowVisible=false;
  arrowX=std::numeric_limits<Int>::min();
  arrowY=std::numeric_limits<Int>::min();
  arrowAngle=-1;
  arrowDiameter=0;
  arrowInitialTranslateDuration=core->getConfigStore()->getIntValue("Graphic","arrowInitialTranslateDuration");
  arrowNormalTranslateDuration=core->getConfigStore()->getIntValue("Graphic","arrowNormalTranslateDuration");
  targetInitialScaleDuration=core->getConfigStore()->getIntValue("Graphic","targetInitialScaleDuration");
  targetNormalScaleDuration=core->getConfigStore()->getIntValue("Graphic","targetNormalScaleDuration");
  targetRotateDuration=core->getConfigStore()->getIntValue("Graphic","targetRotateDuration");
  targetScaleMaxFactor=core->getConfigStore()->getDoubleValue("Graphic","targetScaleMaxFactor");
  targetScaleMinFactor=core->getConfigStore()->getDoubleValue("Graphic","targetScaleMinFactor");
  targetScaleNormalFactor=core->getConfigStore()->getDoubleValue("Graphic","targetScaleNormalFactor");
  backgroundLoaderFinished=false;
  navigationInfosMutex=core->getThread()->createMutex();
  minDistanceToNavigationUpdate=core->getConfigStore()->getDoubleValue("Navigation","minDistanceToNavigationUpdate");
  forceNavigationUpdate=false;

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
}

// Initializes the engine
void NavigationEngine::init() {

  // Set the animation of the target
  GraphicRectangle *targetIcon = core->getGraphicEngine()->lockTargetIcon();
  targetIcon->setRotateAnimation(0,0,360,true,targetRotateDuration,GraphicRotateAnimationTypeLinear);
  core->getGraphicEngine()->unlockTargetIcon();

  // Set the color of the recorded track
  ConfigStore *c=core->getConfigStore();
  lockRecordedTrack();
  if (!(recordedTrack=new NavigationPath)) {
    FATAL("can not create track",NULL);
    return;
  }
  recordedTrack->setNormalColor(c->getGraphicColorValue("Navigation/TrackColor"));

  // Prepare the last recorded track if it does exist
  std::string lastRecordedTrackFilename=c->getStringValue("Navigation","lastRecordedTrackFilename");
  std::string filepath=recordedTrack->getGpxFilefolder()+"/"+lastRecordedTrackFilename;
  if ((lastRecordedTrackFilename!="")&&(access(filepath.c_str(),F_OK)==0)) {
    //DEBUG("track gpx file exists, using it",NULL);
    recordedTrack->setGpxFilename(lastRecordedTrackFilename);
  } else {
    //DEBUG("track gpx file does not exist, starting new track",NULL);
    c->setStringValue("Navigation","lastRecordedTrackFilename",recordedTrack->getGpxFilename());
    recordedTrack->setIsInit(true);
  }
  unlockRecordedTrack();

  // Set the track recording
  setRecordTrack(recordTrack, true);

  // Update the route lists
  updateRoutes();

  // Prepare any routes
  lockRoutes();
  std::string path="Navigation/Route";
  std::list<std::string> routeNames=c->getAttributeValues(path,"name");
  std::list<std::string>::iterator j;
  std::string activeRouteName = c->getStringValue("Navigation","activeRoute");
  for(std::list<std::string>::iterator i=routeNames.begin();i!=routeNames.end();i++) {
    std::string routePath=path + "[@name='" + *i + "']";
    if (c->getIntValue(routePath,"visible")) {

      // Create the route
      NavigationPath *route=new NavigationPath();
      if (!route) {
        FATAL("can not create route",NULL);
        return;
      }
      GraphicColor highlightColor = c->getGraphicColorValue(routePath + "/HighlightColor");
      route->setHighlightColor(highlightColor);
      route->setNormalColor(c->getGraphicColorValue(routePath + "/NormalColor"));
      route->setBlinkMode(false);
      route->setReverse(c->getIntValue(routePath,"reverse"));
      route->setName(*i);
      route->setDescription("route number " + *i);
      route->setGpxFilefolder(getRoutePath());
      route->setGpxFilename(*i);
      routes.push_back(route);

      // Check if it is selected for navigation
      if (activeRouteName==route->getName()) {
        activeRoute=route;
        activeRoute->setBlinkMode(true);
      }

    }
  }
  unlockRoutes();

  // Prepare the target
  if (c->getIntValue("Navigation/Target","visible")) {
    showTarget(false);
  }

  // Start the background loader
  if (!(backgroundLoaderThreadInfo=core->getThread()->createThread(navigationEngineBackgroundLoaderThread,this)))
    FATAL("can not start background loader thread",NULL);

  // Object is initialized
  isInitialized=true;
}

/**
 * Removes any routes from the config that do not exist anymore and adds new
 * ones
 */
void NavigationEngine::updateRoutes() {

  lockRoutes();

  // First get a list of all available route filenames
  DIR *dp = opendir( getRoutePath().c_str() );
  struct dirent *dirp;
  struct stat filestat;
  std::list<std::string> routes;
  if (dp == NULL){
    ERROR("can not open directory <%s> for reading available routes",getRoutePath().c_str());
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
  std::list<std::string> routeNames = core->getConfigStore()->getAttributeValues("Navigation/Route", "name");
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
    core->getConfigStore()->setIntValue(path,"visible", 1);
  }

  unlockRoutes();
}

// Deinitializes the engine
void NavigationEngine::deinit() {

  // Finish the background thread
  if (backgroundLoaderThreadInfo) {
    core->getThread()->lockMutex(backgroundLoaderFinishedMutex);
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
    lockRecordedTrack();
    delete recordedTrack;
    recordedTrack=NULL;
    unlockRecordedTrack();
  }

  // Free all routes
  lockRoutes();
  for (std::list<NavigationPath*>::iterator i=routes.begin();i!=routes.end();i++) {
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
    lockLocationPos();
    locationPos=newLocationPos;
    unlockLocationPos();
    MapPosition *pos=core->getMapEngine()->lockLocationPos();
    *pos=locationPos;
    core->getMapEngine()->unlockLocationPos();

    //PROFILE_ADD("position update init");

    // Update the navigation infos
    updateNavigationInfos();

    // Update the current track
    updateTrack();
    //PROFILE_ADD("track update");

    // Inform the widget engine
    core->getWidgetEngine()->onLocationChange(locationPos);

    // Update the graphics
    updateScreenGraphic(false);
    //PROFILE_ADD("graphics update");

  }

  //PROFILE_END;

}

// Updates the compass
void NavigationEngine::newCompassBearing(double bearing) {
  lockCompassBearing();
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
  recordedTrack->lockAccess();
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

  // Add the new point if it meets the criterias
  if (pointMeetsCriterias) {
    //DEBUG("adding new point (%f,%f): navigationDistance=%.2f pointMeetsCriterias=%d",locationPos.getLat(),locationPos.getLng(),navigationDistance,pointMeetsCriterias);
    recordedTrack->addEndPosition(locationPos);
  }

  // Free the recorded track
  recordedTrack->unlockAccess();
  //DEBUG("after recorded track update",NULL);

  // Inform the widget engine
  core->getWidgetEngine()->onPathChange(recordedTrack);
}

// Saves the recorded track if required
void NavigationEngine::backup() {

  // Store the recorded track
  recordedTrack->writeGPXFile(); // locking is handled within method

}

// Switches the track recording
bool NavigationEngine::setRecordTrack(bool recordTrack, bool ignoreIsInit)
{
  // Ignore command if track is not initialized
  if ((!recordedTrack->getIsInit())&&(!ignoreIsInit)) {
    WARNING("can not change track recording status because track is currently loading",NULL);
    return false;
  }

  // Interrupt the track if there is a previous point
  if ((recordTrack)&&(!this->recordTrack)) {
    recordedTrack->lockAccess();
    if (recordedTrack->getHasLastPoint()) {
      if (recordedTrack->getLastPoint()!=NavigationPath::getPathInterruptedPos()) {
        recordedTrack->addEndPosition(NavigationPath::getPathInterruptedPos());
      }
    }
    recordedTrack->unlockAccess();
  }
  lockRecordedTrack();
  this->recordTrack=recordTrack;
  core->getConfigStore()->setIntValue("Navigation","recordTrack",recordTrack);
  unlockRecordedTrack();
  if (recordTrack) {
    INFO("track recording is enabled",NULL);
  } else {
    INFO("track recording is disabled",NULL);
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
  recordedTrack->lockAccess();
  recordedTrack->deinit();
  recordedTrack->init();
  recordedTrack->setIsInit(true);
  recordedTrack->unlockAccess();
  lockRecordedTrack();
  core->getConfigStore()->setStringValue("Navigation","lastRecordedTrackFilename",recordedTrack->getGpxFilename());
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
  core->getThread()->lockMutex(updateGraphicsMutex);

  // We need a map tile to compute the coordinates
  //DEBUG("before map pos lock",NULL);
  mapPos=*(mapEngine->lockMapPos());
  mapEngine->unlockMapPos();
  //DEBUG("after map pos lock",NULL);
  if (!mapPos.getMapTile()) {
    core->getThread()->unlockMutex(updateGraphicsMutex);
    //DEBUG("after graphics update",NULL);
    return;
  }

  // Copy the current display area
  //DEBUG("before display area lock",NULL);
  MapArea displayArea=*(mapEngine->lockDisplayArea());
  mapEngine->unlockDisplayArea();
  //DEBUG("after display area lock",NULL);

  // Copy the current visual position
  GraphicPosition visPos=*(core->getGraphicEngine()->lockPos());
  core->getGraphicEngine()->unlockPos();

  // Update the location icon
  //DEBUG("before location pos lock",NULL);
  lockLocationPos();
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
  GraphicRectangle *locationIcon=core->getGraphicEngine()->lockLocationIcon();
  if (showCursor) {
    if (updatePosition) {
      if ((locationIcon->getX()!=visPosX)||(locationIcon->getY()!=visPosY)||
          (locationIcon->getAngle()!=visAngle)||
          (core->getGraphicEngine()->getLocationAccuracyRadiusX()!=visRadiusX)||
          (core->getGraphicEngine()->getLocationAccuracyRadiusY()!=visRadiusY)) {
        locationIcon->setX(visPosX);
        locationIcon->setY(visPosY);
        locationIcon->setAngle(visAngle);
        //DEBUG("locationIcon.getX()=%d locationIcon.getY()=%d",locationIcon->getX(),locationIcon->getY());
        core->getGraphicEngine()->setLocationAccuracyRadiusX(visRadiusX);
        core->getGraphicEngine()->setLocationAccuracyRadiusY(visRadiusY);
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
  core->getGraphicEngine()->unlockLocationIcon();
  //DEBUG("after location icon lock",NULL);

  // Update the compass bearing
  //DEBUG("before compass bearing lock",NULL);
  lockCompassBearing();
  double compassBearing=this->compassBearing;
  unlockCompassBearing();
  //DEBUG("after compass bearing lock",NULL);
  //DEBUG("before compass cone icon lock",NULL);
  GraphicRectangle *compassConeIcon=core->getGraphicEngine()->lockCompassConeIcon();
  compassConeIcon->setAngle(-compassBearing-mapPos.getMapTile()->getNorthAngle());
  compassConeIcon->setIsUpdated(true);
  core->getGraphicEngine()->unlockCompassConeIcon();
  //DEBUG("after compass cone icon lock",NULL);

  // Update the target icon
  //DEBUG("before location pos lock",NULL);
  lockTargetPos();
  showCursor=false;
  double targetIconPosValid=false;
  updatePosition=false;
  bool updateAnimation=false;
  double screenZoom=visPos.getZoom();
  double screenAngle=FloatingPoint::degree2rad(visPos.getAngle());
  Int zoomedScreenWidth=floor(((double)core->getScreen()->getWidth())/screenZoom);
  Int zoomedScreenHeight=floor(((double)core->getScreen()->getHeight())/screenZoom);
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
  GraphicRectangle *targetIcon=core->getGraphicEngine()->lockTargetIcon();
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
      targetIcon->setFadeAnimation(core->getClock()->getMicrosecondsSinceStart(),targetIcon->getColor(),endColor,false,core->getGraphicEngine()->getFadeDuration());
      targetIcon->setIsUpdated(true);
    }
    targetVisible=true;
  } else {
    if (targetVisible) {
      GraphicColor endColor=targetIcon->getColor();
      endColor.setAlpha(0);
      targetIcon->setFadeAnimation(core->getClock()->getMicrosecondsSinceStart(),targetIcon->getColor(),endColor,false,core->getGraphicEngine()->getFadeDuration());
      targetIcon->setIsUpdated(true);
      targetVisible=false;
    }
  }
  core->getGraphicEngine()->unlockTargetIcon();

  // Update the arrow icon
  //DEBUG("before location pos lock",NULL);
  lockTargetPos();
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
  GraphicRectangle *arrowIcon=core->getGraphicEngine()->lockArrowIcon();
  if (showCursor) {
    if ((arrowX!=visPosX)||((arrowY!=visPosY))||(arrowAngle!=visAngle)) {
      arrowX=visPosX;
      arrowY=visPosY;
      arrowAngle=visAngle;
      if (scaleHasChanged) {
        arrowIcon->setX(visPosX);
        arrowIcon->setY(visPosY);
        arrowIcon->setAngle(visAngle);
        arrowIcon->setIsUpdated(true);
        arrowIcon->setTranslateAnimationSequence(std::list<GraphicTranslateAnimationParameter>());
        arrowIcon->setTranslateAnimation(core->getClock()->getMicrosecondsSinceStart(),visPosX,visPosY,translateEndX,translateEndY,true,arrowNormalTranslateDuration,GraphicTranslateAnimationTypeLinear);
      } else {
        std::list<GraphicTranslateAnimationParameter> translateAnimationSequence = arrowIcon->getTranslateAnimationSequence();
        TimestampInMicroseconds startTime = core->getClock()->getMicrosecondsSinceStart();
        if (translateAnimationSequence.size()>0) {
          translateAnimationSequence.pop_back(); // remove the last inifinite translation
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
      arrowIcon->setIsUpdated(true);
    }
    if (updateAnimation) {
      GraphicColor endColor=arrowIcon->getColor();
      endColor.setAlpha(255);
      arrowIcon->setFadeAnimation(core->getClock()->getMicrosecondsSinceStart(),arrowIcon->getColor(),endColor,false,core->getGraphicEngine()->getFadeDuration());
      arrowIcon->setIsUpdated(true);
    }
    arrowVisible=true;
  } else {
    if (arrowVisible) {
      if (updateAnimation) {
        GraphicColor endColor=arrowIcon->getColor();
        endColor.setAlpha(0);
        arrowIcon->setFadeAnimation(core->getClock()->getMicrosecondsSinceStart(),arrowIcon->getColor(),endColor,false,core->getGraphicEngine()->getFadeDuration());
        arrowIcon->setIsUpdated(true);
      }
      arrowVisible=false;
    }
  }
  core->getGraphicEngine()->unlockArrowIcon();

  // Unlock the drawing mutex
  core->getThread()->unlockMutex(updateGraphicsMutex);
  //DEBUG("after graphics update",NULL);
}

// Updates navigation-related graphic that is overlayed on the map
void NavigationEngine::updateMapGraphic() {

  std::list<MapContainer*> containers;

  // Get the current unfinished list
  core->getMapSource()->lockAccess();
  containers = unvisualizedMapContainers;
  unvisualizedMapContainers.clear();
  core->getMapSource()->unlockAccess();

  // Process the unfinished tile list
  for(std::list<NavigationPath*>::iterator i=routes.begin();i!=routes.end();i++) {
    (*i)->lockAccess();
    (*i)->addVisualization(&containers);
    (*i)->unlockAccess();
  }
  if (recordedTrack) {
    recordedTrack->lockAccess();
    recordedTrack->addVisualization(&containers);
    recordedTrack->unlockAccess();
  }

  // Indicate that all tiles have been processed
  core->getMapSource()->lockAccess();
  for (std::list<MapContainer*>::iterator i=containers.begin();i!=containers.end();i++) {
    (*i)->setOverlayGraphicInvalid(false);
  }
  core->getMapSource()->unlockAccess();
}

// Destroys all graphics
void NavigationEngine::destroyGraphic() {

  // Clear the buffers used in the path object
  for(std::list<NavigationPath*>::iterator i=routes.begin();i!=routes.end();i++) {
    (*i)->lockAccess();
    (*i)->destroyGraphic();
    (*i)->unlockAccess();
  }
  if (recordedTrack) {
    recordedTrack->lockAccess();
    recordedTrack->destroyGraphic();
    recordedTrack->unlockAccess();
  }
}

// Creates all graphics
void NavigationEngine::createGraphic() {

  // Get the radius of the arrow icon
  GraphicRectangle *arrowIcon=core->getGraphicEngine()->lockArrowIcon();
  arrowDiameter=sqrt((double)(arrowIcon->getIconWidth()*arrowIcon->getIconWidth()+arrowIcon->getIconHeight()*arrowIcon->getIconHeight()));
  core->getGraphicEngine()->unlockArrowIcon();

  // Creates the buffers used in the path object
  for(std::list<NavigationPath*>::iterator i=routes.begin();i!=routes.end();i++) {
    (*i)->lockAccess();
    (*i)->createGraphic();
    (*i)->unlockAccess();
  }
  if (recordedTrack) {
    recordedTrack->lockAccess();
    recordedTrack->createGraphic();
    recordedTrack->unlockAccess();
  }
}

// Recreate the objects to reduce the number of graphic point buffers
void NavigationEngine::optimizeGraphic() {

  // Optimize the buffers used in the path object
  for(std::list<NavigationPath*>::iterator i=routes.begin();i!=routes.end();i++) {
    (*i)->lockAccess();
    (*i)->optimizeGraphic();
    (*i)->unlockAccess();
  }
  if (recordedTrack) {
    recordedTrack->lockAccess();
    recordedTrack->optimizeGraphic();
    recordedTrack->unlockAccess();
  }
}

// Adds the visualization for the given tile
void NavigationEngine::addGraphics(MapContainer *container) {
  std::list<MapContainer*>::iterator i;
  bool inserted=false;
  for (i=unvisualizedMapContainers.begin();i!=unvisualizedMapContainers.end();i++) {
    if ((*i)->getZoomLevel() > container->getZoomLevel()) {
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
    (*i)->lockAccess();
    (*i)->removeVisualization(container);
    (*i)->unlockAccess();
  }
  if (recordedTrack) {
    recordedTrack->lockAccess();
    recordedTrack->removeVisualization(container);
    recordedTrack->unlockAccess();
  }
}

// Loads all pathes in the background
void NavigationEngine::backgroundLoader() {

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
  for(std::list<NavigationPath*>::iterator i=routes.begin();i!=routes.end();i++) {
    if (core->getQuitCore()) {
      goto exitThread;
    }
    (*i)->readGPXFile(); // locking is handled within method
    (*i)->setIsInit(true);
  }

  // Set the active route

  // Thread is finished
exitThread:
  core->getThread()->lockMutex(backgroundLoaderFinishedMutex);
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
  MapPosition *pos = core->getMapEngine()->lockMapPos();
  setTargetAtGeographicCoordinate(pos->getLng(),pos->getLat(),false);
  core->getMapEngine()->unlockMapPos();
}

// Makes the target invisible
void NavigationEngine::hideTarget() {
  std::string path="Navigation/Target";
  core->getConfigStore()->setIntValue(path,"visible",0);
  lockTargetPos();
  targetPos.invalidate();
  unlockTargetPos();
  core->getMapEngine()->setForceMapUpdate();
}

// Shows the current target
void NavigationEngine::showTarget(bool repositionMap) {
  std::string path="Navigation/Target";
  core->getConfigStore()->setIntValue(path,"visible",1);
  double lng = core->getConfigStore()->getDoubleValue(path,"lng");
  double lat = core->getConfigStore()->getDoubleValue(path,"lat");
  lockTargetPos();
  targetPos.setLng(lng);
  targetPos.setLat(lat);
  targetPos.setIsUpdated(true);
  unlockTargetPos();
  if (repositionMap)
    core->getMapEngine()->setMapPos(targetPos);
  core->getMapEngine()->setForceMapUpdate();
}

// Shows the current target at the given position
void NavigationEngine::setTargetAtGeographicCoordinate(double lng, double lat, bool repositionMap) {
  std::string path="Navigation/Target";
  core->getConfigStore()->setDoubleValue(path,"lng",lng);
  core->getConfigStore()->setDoubleValue(path,"lat",lat);
  showTarget(repositionMap);
  forceNavigationUpdate=true;
  updateNavigationInfos();
}

// Updates the navigation infos
void NavigationEngine::updateNavigationInfos() {
  std::stringstream infos;

  // Copy location pos
  lockLocationPos();
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
  if ((lastNavigationLocationPos.isValid())&&(!forceNavigationUpdate)) {
    double travelledDistance = locationPos.computeDistance(lastNavigationLocationPos);
    if (travelledDistance < minDistanceToNavigationUpdate)
      return;
  }
  lastNavigationLocationPos=locationPos;
  forceNavigationUpdate=false;

  // Copy target pos
  lockTargetPos();
  MapPosition targetPos=this->targetPos;
  unlockTargetPos();

  // If a route is active, compute the details for the given route
  lockNavigationInfo();
  navigationInfo=NavigationInfo();
  double speed=0;
  if (locationPos.isValid()) {
    if (locationPos.getHasBearing()) {
      navigationInfo.setLocationBearing(locationPos.getBearing());
    }
    if (locationPos.getHasSpeed()) {
      speed=locationPos.getSpeed();
    }

    // If a target is active, compute the details for the given target
    if (targetPos.isValid()) {
      if (navigationInfo.getLocationBearing()!=NavigationInfo::getUnknownAngle())
        navigationInfo.setTargetBearing(locationPos.computeBearing(targetPos));
      navigationInfo.setTargetDistance(locationPos.computeDistance(targetPos));
    } else {

      if (activeRoute) {

        // Compute the navigation details for the given route
        //static MapPosition prevTurnPos;
        activeRoute->lockAccess();
        activeRoute->computeNavigationInfo(locationPos,targetPos,navigationInfo);
        activeRoute->unlockAccess();
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
  if (speed>0) {
    if (navigationInfo.getTargetDistance()!=NavigationInfo::getUnknownDistance())
      navigationInfo.setTargetDuration(navigationInfo.getTargetDistance() / speed);
  }

  // Update the parent app
  std::string value,unit;
  if (navigationInfo.getLocationBearing()!=NavigationInfo::getUnknownAngle())
    infos << navigationInfo.getLocationBearing();
  else
    infos << "-";
  if (navigationInfo.getTargetBearing()!=NavigationInfo::getUnknownAngle())
    infos << ";" << navigationInfo.getTargetBearing();
  else
    infos << ";-";
  infos << ";Distance;";
  if (navigationInfo.getTargetDistance()!=NavigationInfo::getUnknownDistance()) {
    core->getUnitConverter()->formatMeters(navigationInfo.getTargetDistance(),value,unit);
    infos << value << " " << unit;
  } else {
    infos << "infinite";
  }
  infos << ";Duration;";
  if (navigationInfo.getTargetDuration()!=NavigationInfo::getUnknownDuration()) {
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
  unlockNavigationInfo();

  // Update other apps
  //DEBUG("updateNavigationInfos(%s)",infos.str().c_str());
  core->getCommander()->dispatch("updateNavigationInfos(" + infos.str() + ")");
}

}

