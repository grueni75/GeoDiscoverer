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

  // Updates the currently recorded track
  void updateTrack();

public:

  // Constructor
  NavigationEngine();

  // Destructor
  virtual ~NavigationEngine();

  // Initializes the engine
  void init();

  // Deinitializes the engine
  void deinit();

  // Updates the current location
  void newLocationFix(MapPosition newLocationPos);

  // Updates the compass
  void newCompassBearing(double bearing);

  // Saves the recorded track if required
  void backup();

  // Creates a new track
  void createNewTrack();

  // Switches the track recording
  void setRecordTrack(bool recordTrack);

  // Updates navigation-related graphic that is overlayed on the screen
  void updateScreenGraphic(bool scaleHasChanged);

  // Updates navigation-related graphic that is overlayed on the map tiles
  void updateMapGraphic();

  // Indicates that textures have been invalidated
  void graphicInvalidated();

  // Recreate the objects to reduce the number of graphic point buffers
  void optimizeGraphic();

  // Adds the visualization for the given map container
  void addGraphics(MapContainer *container);

  // Removes the viualization for the given map container
  void removeGraphics(MapContainer *container);

  // Loads all pathes in the background
  void backgroundLoader();

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

};

}

#endif /* NAVIGATIONENGINE_H_ */
