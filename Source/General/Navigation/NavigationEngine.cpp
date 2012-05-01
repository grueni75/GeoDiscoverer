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
  recordedTrackMutex=core->getThread()->createMutex();
  routesMutex=core->getThread()->createMutex();
  locationPosMutex=core->getThread()->createMutex();
  compassBearingMutex=core->getThread()->createMutex();
  updateGraphicsMutex=core->getThread()->createMutex();
  recordTrack=core->getConfigStore()->getIntValue("Navigation","recordTrack");
  compassBearing=0;
  isInitialized=false;
  recordedTrack=NULL;
  backgroundLoaderThreadInfo=NULL;
  statusMutex=core->getThread()->createMutex();

  // Create the track directory if it does not exist
  struct stat st;
  if (stat(getTrackPath().c_str(), &st) != 0)
  {
    if (mkdir(getTrackPath().c_str(),S_IRWXU | S_IRWXG | S_IRWXO)!=0) {
      FATAL("can not create track directory!",NULL);
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
}

// Initializes the engine
void NavigationEngine::init() {

  // Set the color of the recorded track
  ConfigStore *c=core->getConfigStore();
  lockRecordedTrack();
  if (!(recordedTrack=new NavigationPath)) {
    FATAL("can not create track",NULL);
    return;
  }
  recordedTrack->setNormalColor(c->getGraphicColorValue("Graphic/TrackColor"));

  // Prepare the last recorded track if it does exist
  std::string lastRecordedTrackFilename=c->getStringValue("Navigation","lastRecordedTrackFilename");
  std::string filepath=recordedTrack->getGpxFilefolder()+"/"+lastRecordedTrackFilename;
  if ((lastRecordedTrackFilename!="")&&(access(filepath.c_str(),F_OK)==0)) {
    recordedTrack->setGpxFilename(lastRecordedTrackFilename);
  } else {
    c->setStringValue("Navigation","lastRecordedTrackFilename",recordedTrack->getGpxFilename());
    recordedTrack->setIsInit(true);
  }
  unlockRecordedTrack();

  // Set the track recording
  setRecordTrack(recordTrack, true);

  // Prepare any routes
  lockRoutes();
  std::string path="Navigation/Route";
  std::list<std::string> routeNames=c->getAttributeValues(path,"name");
  std::list<std::string>::iterator j;
  for(std::list<std::string>::iterator i=routeNames.begin();i!=routeNames.end();i++) {
    std::string routePath=path + "[@name='" + *i + "']";
    if (c->getIntValue(routePath,"visible")) {
      NavigationPath *route=new NavigationPath();
      if (!route) {
        FATAL("can not create route",NULL);
        return;
      }
      if (c->pathExists(routePath + "/HighlightColor")) {
        DEBUG("setting blink mode for path %s",route->getGpxFilename().c_str());
        route->setHighlightColor(c->getGraphicColorValue(routePath + "/HighlightColor"));
        route->setBlinkMode(true);
      }
      route->setNormalColor(c->getGraphicColorValue(routePath + "/NormalColor"));
      route->setName(*i);
      route->setDescription("route number " + *i);
      route->setGpxFilefolder(getRoutePath());
      route->setGpxFilename(c->getStringValue(routePath,"filename"));
      routes.push_back(route);
    }
  }
  unlockRoutes();

  // Start the background loader
  if (!(backgroundLoaderThreadInfo=core->getThread()->createThread(navigationEngineBackgroundLoaderThread,this)))
    FATAL("can not start background loader thread",NULL);

  // Object is initialized
  isInitialized=true;
}

// Deinitializes the engine
void NavigationEngine::deinit() {

  // Wait for the background loader thread to complete
  if (backgroundLoaderThreadInfo)
    core->getThread()->waitForThread(backgroundLoaderThreadInfo);

  // Save the track first
  lockRecordedTrack();
  if (recordedTrack) {
    recordedTrack->writeGPXFile();
    delete recordedTrack;
    recordedTrack=NULL;
  }
  unlockRecordedTrack();

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

  PROFILE_START;

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

  PROFILE_ADD("position check");

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
    //DEBUG("before location pos update",NULL);
    lockLocationPos();
    locationPos=newLocationPos;
    unlockLocationPos();
    //DEBUG("after location pos update",NULL);
    //DEBUG("before map location pos update",NULL);
    MapPosition *pos=core->getMapEngine()->lockLocationPos();
    *pos=locationPos;
    core->getMapEngine()->unlockLocationPos();
    //DEBUG("after map location pos update",NULL);

    PROFILE_ADD("position update init");

    // Update the current track
    updateTrack();
    PROFILE_ADD("track update");

    // Update the graphics
    updateScreenGraphic(false);
    PROFILE_ADD("graphics update");

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
  lockRecordedTrack();

  // If the track was just loaded, we need to add this point as a stable one
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

  // Add the new point if it meeets the criterias
  if (pointMeetsCriterias) {
    //DEBUG("adding new point (%f,%f): distance=%.2f pointMeetsCriterias=%d",locationPos.getLat(),locationPos.getLng(),distance,pointMeetsCriterias);
    recordedTrack->addEndPosition(locationPos);
  }

  // Free the recorded track
  unlockRecordedTrack();
  //DEBUG("after recorded track update",NULL);
}

// Saves the recorded track if required
void NavigationEngine::backup() {

  // Store the recorded track
  lockRecordedTrack();
  recordedTrack->writeGPXFile();
  unlockRecordedTrack();

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
  lockRecordedTrack();
  if ((recordTrack)&&(!this->recordTrack)) {
    if (recordedTrack->getHasLastPoint()) {
      if (recordedTrack->getLastPoint()!=NavigationPath::getPathInterruptedPos()) {
        recordedTrack->addEndPosition(NavigationPath::getPathInterruptedPos());
      }
    }
  }
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
  lockRecordedTrack();
  recordedTrack->writeGPXFile();
  recordedTrack->deinit();
  recordedTrack->init();
  recordedTrack->setIsInit(true);
  core->getConfigStore()->setStringValue("Navigation","lastRecordedTrackFilename",recordedTrack->getGpxFilename());
  unlockRecordedTrack();
  INFO("new %s created",recordedTrack->getGpxFilename().c_str());
  updateScreenGraphic(false);
}

// Updates navigation-related graphic that is overlayed on the screen
void NavigationEngine::updateScreenGraphic(bool scaleHasChanged) {
  Int mapDiffX, mapDiffY;
  bool showCursor;
  bool updatePosition;
  Int visPosX, visPosY, visRadiusX, visRadiusY;
  double visAngle;
  GraphicPosition visPos;
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

  // Update the location icon
  //DEBUG("before location pos lock",NULL);
  lockLocationPos();
  showCursor=false;
  updatePosition=false;
  if (locationPos.isValid()) {
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
  //DEBUG("after location pos lock",NULL);
  //DEBUG("before location pos to vis pos lock",NULL);
  mapEngine->lockLocationPos2visPosOffset();
  if (showCursor) {
    mapEngine->setLocationPos2visPosOffsetValid(true);
    mapEngine->setLocationPos2visPosOffsetX(mapDiffX);
    mapEngine->setLocationPos2visPosOffsetY(-mapDiffY);
  } else {
    mapEngine->setLocationPos2visPosOffsetValid(false);
  }
  mapEngine->unlockLocationPos2visPosOffset();
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
  lockRoutes();
  for(std::list<NavigationPath*>::iterator i=routes.begin();i!=routes.end();i++) {
    (*i)->addVisualization(&containers);
  }
  unlockRoutes();
  lockRecordedTrack();
  if (recordedTrack) {
    recordedTrack->addVisualization(&containers);
  }
  unlockRecordedTrack();

  // Indicate that all tiles have been processed
  core->getMapSource()->lockAccess();
  for (std::list<MapContainer*>::iterator i=containers.begin();i!=containers.end();i++) {
    (*i)->setOverlayGraphicInvalid(false);
  }
  core->getMapSource()->unlockAccess();
}

// Indicates that textures and buffers have been invalidated
void NavigationEngine::graphicInvalidated() {

  // Reset the buffers used in the path object
  lockRoutes();
  for(std::list<NavigationPath*>::iterator i=routes.begin();i!=routes.end();i++) {
    (*i)->graphicInvalidated();
  }
  unlockRoutes();
  lockRecordedTrack();
  if (recordedTrack) {
    recordedTrack->graphicInvalidated();
  }
  unlockRecordedTrack();
}

// Recreate the objects to reduce the number of graphic point buffers
void NavigationEngine::optimizeGraphic() {

  // Optimize the buffers used in the path object
  lockRoutes();
  for(std::list<NavigationPath*>::iterator i=routes.begin();i!=routes.end();i++) {
    (*i)->optimizeGraphic();
  }
  unlockRoutes();
  lockRecordedTrack();
  if (recordedTrack) {
    recordedTrack->optimizeGraphic();
  }
  unlockRecordedTrack();
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
  lockRoutes();
  for(std::list<NavigationPath*>::iterator i=routes.begin();i!=routes.end();i++) {
    (*i)->removeVisualization(container);
  }
  unlockRoutes();
  lockRecordedTrack();
  if (recordedTrack) {
    recordedTrack->removeVisualization(container);
  }
  unlockRecordedTrack();
}

// Loads all pathes in the background
void NavigationEngine::backgroundLoader() {

  // Load the recorded track
  if (!recordedTrack->getIsInit()) {
    recordedTrack->readGPXFile();
    recordedTrack->setIsInit(true);
  }

  // Load all routes
  for(std::list<NavigationPath*>::iterator i=routes.begin();i!=routes.end();i++) {
    (*i)->readGPXFile();
    (*i)->setIsInit(true);
  }

}


}
