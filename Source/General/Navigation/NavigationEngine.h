//============================================================================
// Name        : NavigationEngine.h
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

  // Indicates if the track recording is enabled or disabled
  bool recordTrack;

  // Current track
  NavigationPath *recordedTrack;

  // Mutex for accessing the current track
  ThreadMutexInfo *recordedTrackMutex;

  // Routes
  std::list<NavigationPath*> routes;

  // Active route that is for navigation
  NavigationPath *activeRoute;

  // Mutex for accessing the routes
  ThreadMutexInfo *routesMutex;

  // Mutex for accessing the drawing function
  ThreadMutexInfo *updateGraphicsMutex;

  // Required minimum distance in meter to the last track point such that the point is added to the track
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

public:

  // Constructor
  NavigationEngine();

  // Destructor
  virtual ~NavigationEngine();

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

  // Adds a new point of interest
  void newPointOfInterest(std::string name, std::string description, double lng, double lat);

  // Sets the target to the center of the map
  void setTargetAtMapCenter();

  // Shows the current target
  void showTarget(bool repositionMap);

  // Shows the current target at the given position
  void setTargetAtGeographicCoordinate(double lng, double lat, bool repositionMap);

  // Makes the target invisible
  void hideTarget();

  // Saves the recorded track if required
  void backup();

  // Creates a new track
  void createNewTrack();

  // Switches the track recording
  bool setRecordTrack(bool recordTrack, bool ignoreIsInit=false);

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

  // Loads all pathes in the background
  void backgroundLoader();

  // Updates the route list (remove stale ones, add new ones)
  void updateRoutes();

  // Returns information for the dashboard
  std::string getDashboardInfos();

  // Getters and setters
  NavigationPath *lockRecordedTrack()
  {
      core->getThread()->lockMutex(recordedTrackMutex);
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
  MapPosition *lockTargetPos()
  {
      core->getThread()->lockMutex(targetPosMutex);
      return &targetPos;
  }
  void unlockTargetPos()
  {
      core->getThread()->unlockMutex(targetPosMutex);
  }

  std::list<NavigationPath*> *lockRoutes()
  {
      core->getThread()->lockMutex(routesMutex);
      return &routes;
  }
  void unlockRoutes()
  {
      core->getThread()->unlockMutex(routesMutex);
  }

  MapPosition *lockLocationPos()
  {
      core->getThread()->lockMutex(locationPosMutex);
      return &locationPos;
  }
  void unlockLocationPos()
  {
      core->getThread()->unlockMutex(locationPosMutex);
  }

  double *lockCompassBearing()
  {
      core->getThread()->lockMutex(compassBearingMutex);
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

  std::list<std::string> getStatus() const {
    core->getThread()->lockMutex(statusMutex);
    std::list<std::string> status=this->status;
    core->getThread()->unlockMutex(statusMutex);
    return status;
  }

  void setStatus(std::list<std::string> status) {
    core->getThread()->lockMutex(statusMutex);
    this->status = status;
    core->getThread()->unlockMutex(statusMutex);
  }

};

}

#endif /* NAVIGATIONENGINE_H_ */
