//============================================================================
// Name        : NavigationEngine.h
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

#include <NavigationPointVisualization.h>
#include <NavigationPoint.h>
#include <MapSource.h>
#include <GraphicPosition.h>
#include <NavigationInfo.h>
#include <MapPosition.h>
#include <GraphicEngine.h>

#ifndef NAVIGATIONENGINE_H_
#define NAVIGATIONENGINE_H_

namespace GEODISCOVERER {

class NavigationEngine {

protected:

  // Indicates if the engine is initialized
  bool isInitialized;

  // Current location
  MapPosition locationPos;

  // Current compass bearing in degrees
  double compassBearing;

  // Mutex for accessing the compass bearing
  ThreadMutexInfo *compassBearingMutex;

  // Duration in milliseconds after which a position will be discarded
  TimestampInMilliseconds locationOutdatedThreshold;

  // Distance in meter that indicates a significantly less accurate position
  Int locationSignificantlyInaccurateThreshold;

  // Number of map containers to visualize per map update round
  Int visualizationMaxContainerCountPerRound;

  // Indicates if the track recording is enabled or disabled
  bool recordTrack;

  // Current track
  NavigationPath *recordedTrack;

  // Mutex for accessing the current track
  ThreadMutexInfo *recordedTrackMutex;

  // Routes still in use
  std::list<NavigationPath*> routes;

  // Routes that are not used anymore
  std::list<NavigationPath*> unusedRoutes;

  // Active route that is for navigation
  NavigationPath *activeRoute;

  // Mutex for accessing the routes
  ThreadMutexInfo *routesMutex;

  // Required minimum navigationDistance in meter to the last track point such that the point is added to the track
  double trackRecordingMinDistance;

  // List of containers that need a graphic update
  std::list<MapContainer*> unvisualizedMapContainers;

  // Mutex for accessing the location pos
  ThreadMutexInfo *locationPosMutex;

  // Information about the background loader thread
  ThreadInfo *backgroundLoaderThreadInfo;

  // Status of the navigation engine
  std::list<std::string> status;

  // Mutex for accessing the status
  ThreadMutexInfo *statusMutex;

  // Current target position
  MapPosition targetPos;

  // Mutex for accessing the target position
  ThreadMutexInfo *targetPosMutex;

  // Indicates that the target is currently visible
  bool targetVisible;

  // Indicates that the arrow is currently visible
  bool arrowVisible;

  // Diameter of the arrow icon
  double arrowDiameter;

  // Position of the arrow icon
  Int arrowX;
  Int arrowY;
  double arrowAngle;

  // Parameters for the arrow animation
  TimestampInMicroseconds arrowInitialTranslateDuration;
  TimestampInMicroseconds arrowNormalTranslateDuration;
  Int arrowMinPositionDiffForRestartingAnimation;

  // Parameters for the target animation
  TimestampInMicroseconds targetInitialScaleDuration;
  TimestampInMicroseconds targetNormalScaleDuration;
  TimestampInMicroseconds targetRotateDuration;
  double targetScaleMaxFactor;
  double targetScaleMinFactor;
  double targetScaleNormalFactor;

  // Indicates that the background thread has quit
  bool backgroundLoaderFinished;

  // Mutex for access the backgroundLoaderFinished variable
  ThreadMutexInfo *backgroundLoaderFinishedMutex;

  // Parameters when to update the navigation information
  MapPosition lastNavigationLocationPos;
  double minDistanceToNavigationUpdate;
  bool forceNavigationInfoUpdate;

  // Mutex for accessing the active route
  ThreadMutexInfo *activeRouteMutex;

  // Mutex for accessing the navigation infos
  ThreadMutexInfo *navigationInfosMutex;

  // Navigation information
  NavigationInfo navigationInfo;

  // Information about the navigation compute thread
  ThreadInfo *computeNavigationInfoThreadInfo;

  // Signal to wakeup the navigation compute thread
  ThreadSignalInfo *computeNavigationInfoSignal;

  // Information about the google maps bookmarks synchronization thread
  ThreadInfo *synchronizeGoogleBookmarksThreadInfo;

  // Signal to wakeup the google maps bookmarks synchronization thread
  ThreadSignalInfo *synchronizeGoogleBookmarksSignal;
  
  // Indicates that the google maps bookmarks synchronization thread shall quit
  bool quitSynchronizeGoogleBookmarksThread;

  // List of address points
  std::list<NavigationPoint> addressPoints;

  // Stores the nearest address point
  NavigationPoint nearestAddressPoint;

  // Indicates if the nearest address point is valid
  bool nearestAddressPointValid;

  // Indicates if the nearest address point is near enough
  bool nearestAddressPointAlarm;

  // Double distance to the nearest address point
  double nearestAddressPointDistance;

  // Name of the last found nearby address point
  std::string nearestAddressPointName;

  // Timestamp when the last nearby address point was found
  TimestampInMicroseconds nearestAddressPointNameUpdate;

  // Mutex for accessing the nearest navigation point
  ThreadMutexInfo *nearestAddressPointMutex;

  // Maximum distance for which the widget shows infos about an address point
  double maxAddressPointAlarmDistance;

  // Graphic object that represents the address points
  GraphicObject navigationPointsGraphicObject;

  // List of navigation points that are visualized
  std::list<NavigationPointVisualization> navigationPointsVisualization;

  // Hash that represents the overlay content
  std::string overlayGraphicHash;

  // Forces an update of the navigation infos
  void triggerNavigationInfoUpdate();

  // Reads the address points from disk
  void initAddressPoints();

  // Deinitializes a path
  void deletePath(NavigationPath *path);

  // Updates the flags of a route
  void updateFlagVisualization(NavigationPath *path);

  // Resets the hash
  void resetOverlayGraphicHash() {
    overlayGraphicHash="";
  }

  // Remove cache files that do not have their original gpx files anymore
  void cleanupNavigationPathCache(std::string filepath);

    // Searches for a node with the given name in the given list of nodes
  XMLNode findNode(XMLNode node, const char *name);

public:

  // Constructor
  NavigationEngine();

  // Destructor
  virtual ~NavigationEngine();

  // Calculates navigation infos such as bearing, distance, ...
  void computeNavigationInfo();

  // Synchronizes Google Bookmarks
  void synchronizeGoogleBookmarks();

  // Forces an update of the google bookmarks
  void triggerGoogleBookmarksSynchronization();

  // Initializes the engine
  void init();

  // Deinitializes the engine
  void deinit();

  // Updates the currently recorded track
  void updateTrack();

  // Updates the current location
  void newLocationFix(MapPosition newLocationPos);

  // Updates the compass
  void newCompassBearing(double bearing);

  // Sets the target to the center of the map
  void setTargetAtMapCenter();

  // Shows the current target
  void showTarget(bool repositionMap);

  // Shows the current target at the given position
  void setTargetAtGeographicCoordinate(double lng, double lat, bool repositionMap);

  // Sets the target pos directly
  void setTargetPos(double lng, double lat);

  // Sets the active route
  void setActiveRoute(NavigationPath *route);

  // Exports the active route inclusive selection as an GPX file
  void exportActiveRoute();

  // Makes the target invisible
  void hideTarget();

  // Saves the recorded track if required
  void backup();

  // Creates a new track
  void createNewTrack();

  // Switches the track recording
  bool setRecordTrack(bool recordTrack, bool ignoreIsInit=false, bool showInfo=true);

  // Updates navigation-related graphic that is overlayed on the screen
  void updateScreenGraphic(bool scaleHasChanged);

  // Updates navigation-related graphic that is overlayed on the map tiles
  void updateMapGraphic();

  // Creates all graphics
  void createGraphic();

  // Destroys all graphics
  void destroyGraphic();

  // Recreate the objects to reduce the number of graphic point buffers
  void optimizeGraphic();

  // Adds the visualization for the given map container
  void addGraphics(MapContainer *container);

  // Removes the viualization for the given map container
  void removeGraphics(MapContainer *container);

  // Indicates if a graphic update is required
  bool mapGraphicUpdateIsRequired(const char *file, int line) {
    core->getMapSource()->lockAccess(file,line);
    Int size=unvisualizedMapContainers.size();
    core->getMapSource()->unlockAccess();
    if (size>0) {
      return true;
    } else {
      return false;
    }
  }

  // Loads all pathes in the background
  void backgroundLoader();

  // Updates the route list (remove stale ones, add new ones)
  void updateRoutes();

  // Sets the start flag
  void setStartFlag(NavigationPath *path, Int index, const char *file, int line);

  // Sets the end flag
  void setEndFlag(NavigationPath *path, Int index, const char *file, int line);

  // Adds a new address point (and deletes on old one if max history is exceeded)
  void addAddressPoint(NavigationPoint point);

  // Renames an existing address point
  std::string renameAddressPoint(std::string oldName, std::string newName);

  // Removes an address point
  void removeAddressPoint(std::string name);

  // Finds a route with the given name
  NavigationPath *findRoute(std::string name);

  // Returns the name of the address point at the given position
  std::string getAddressPointName(GraphicPosition visPos);

  // Returns information about the address point that is the nearest to the current position
  bool getNearestAddressPoint(NavigationPoint &navigationPoint, double &distance, TimestampInMicroseconds &updateTimestamp, bool &alarm);

  // Sets a location pos directly (e.g., on the watch)
  void setLocationPos(MapPosition newLocationPos, bool computeNavigationInfos, const char *file, int line);

  // Stores the navigation points into a file
  void storeOverlayGraphics(std::string filefolder, std::string filename);

  // Recreates the navigation points from a binary file
  void retrieveOverlayGraphics(std::string filefolder, std::string filename);

  // Updates the address point group that is displayed on screen
  void addressPointGroupChanged();

  // Removes the path from the map and the disk
  void trashPath(NavigationPath *path);
  
  // Getters and setters
  NavigationPath *lockRecordedTrack(const char *file, int line)
  {
      core->getThread()->lockMutex(recordedTrackMutex, file, line);
      return recordedTrack;
  }
  void unlockRecordedTrack()
  {
      core->getThread()->unlockMutex(recordedTrackMutex);
  }
  std::string getTrackPath() const {
    return core->getHomePath() + "/Track";
  }
  std::string getRoutePath() const {
    return core->getHomePath() + "/Route";
  }
  std::string getExportRoutePath() const {
    return core->getHomePath() + "/Route/Export";
  }
  MapPosition *lockTargetPos(const char *file, int line)
  {
      core->getThread()->lockMutex(targetPosMutex, file, line);
      return &targetPos;
  }
  void unlockTargetPos()
  {
      core->getThread()->unlockMutex(targetPosMutex);
  }

  std::list<NavigationPath*> *lockRoutes(const char *file, int line)
  {
      core->getThread()->lockMutex(routesMutex, file, line);
      return &routes;
  }
  void unlockRoutes()
  {
      core->getThread()->unlockMutex(routesMutex);
  }

  MapPosition *lockLocationPos(const char *file, int line)
  {
      core->getThread()->lockMutex(locationPosMutex, file, line);
      return &locationPos;
  }
  void setLocationPos(MapPosition pos, const char *file, int line);
  void unlockLocationPos()
  {
      core->getThread()->unlockMutex(locationPosMutex);
  }

  double *lockCompassBearing(const char *file, int line)
  {
      core->getThread()->lockMutex(compassBearingMutex, file, line);
      return &compassBearing;
  }
  void unlockCompassBearing()
  {
      core->getThread()->unlockMutex(compassBearingMutex);
  }

  bool getIsInitialized() const
  {
      return isInitialized;
  }

  void setIsInitialized(bool isInitialized)
  {
      this->isInitialized = isInitialized;
  }

  std::list<std::string> getStatus(const char *file, int line) const {
    core->getThread()->lockMutex(statusMutex, file, line);
    std::list<std::string> status=this->status;
    core->getThread()->unlockMutex(statusMutex);
    return status;
  }

  void setStatus(std::list<std::string> status, const char *file, int line) {
    core->getThread()->lockMutex(statusMutex, file, line);
    this->status = status;
    core->getThread()->unlockMutex(statusMutex);
  }

  NavigationInfo *lockNavigationInfo(const char *file, int line)
  {
      core->getThread()->lockMutex(navigationInfosMutex, file, line);
      return &navigationInfo;
  }
  void unlockNavigationInfo()
  {
      core->getThread()->unlockMutex(navigationInfosMutex);
  }

  TimestampInMicroseconds getTargetRotateDuration() const {
    return targetRotateDuration;
  }

  const NavigationPath *getActiveRoute() const {
    return activeRoute;
  }

  std::list<NavigationPoint>  *lockAddressPoints(const char *file, int line)
  {
    core->getDefaultGraphicEngine()->lockDrawing(__FILE__,__LINE__);
    return &addressPoints;
  }
  void unlockAddressPoints()
  {
    core->getDefaultGraphicEngine()->unlockDrawing();
  }

  void getArrowInfo(bool &visible, double &angle) {
    visible=arrowVisible;
    angle=arrowAngle;
  }

  const std::string getOverlayGraphicHash() const {
    core->getDefaultGraphicEngine()->lockDrawing(__FILE__,__LINE__);
    std::string hash=overlayGraphicHash;
    core->getDefaultGraphicEngine()->unlockDrawing();
    return hash;
  }
};

}

#endif /* NAVIGATIONENGINE_H_ */
