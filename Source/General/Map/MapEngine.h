//============================================================================
// Name        : MapEngine.h
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


#ifndef MAPENGINE_H_
#define MAPENGINE_H_

namespace GEODISCOVERER {

class NavigationPath;

class MapEngine {

protected:

  // Dynamic data
  std::list<MapTile *> tiles;                     // List of tiles the map consists of
  GraphicObject *map;                             // Graphical representation of the map
  MapPosition mapPos;                             // The current position in the map
  MapPosition requestedMapPos;                    // The position to set during the next update
  ThreadMutexInfo *mapPosMutex;                   // Mutex for accessing the map position
  MapPosition locationPos;                        // The current location of the user
  ThreadMutexInfo *locationPosMutex;              // Mutex for accessing the location position
  double compassBearing;                          // The current bearing of the compass
  double prevCompassBearing;                      // The previous bearing of the compass
  ThreadMutexInfo *compassBearingMutex;           // Mutex for accessing the compass bearing
  MapPosition prevLocationPos;                    // The previous location of the user
  GraphicPosition visPos;                         // Current visual position
  MapArea displayArea;                            // Current display area
  ThreadMutexInfo *displayAreaMutex;              // Mutex for accessing the display area
  Int initDistance;                               // Minimum distance that trigger initialization of the map to avoid overflows
  Int maxTiles;                                   // Maximum number of tiles to show
  bool abortUpdate;                               // Indicates that the current update shall be stopped
  bool updateInProgress;                          // Indicates if the map is currently being updated
  bool forceMapUpdate;                            // Force an update of the map on the next call
  bool forceMapRecreation;                        // Force a complete recreation of the map on the next call
  bool forceCacheUpdate;                          // Force an update of the map cache on the next call
  ThreadMutexInfo *forceMapUpdateMutex;           // Mutex for accessing the force map update flag
  ThreadMutexInfo *forceCacheUpdateMutex;         // Mutex for accessing the force cache update flag
  TimestampInMicroseconds returnToLocationTimeout; // Time that must elapse before the map is repositioned to the current location
  bool returnToLocation;                          // Indicates if the map shall be centered around the location if the returnToLocationTimeout has elapsed
  bool zoomLevelLock;                             // Indicates if the zoom level of the map shall not be changed when zooming
  bool returnToLocationOneTime;                   // Return immediately to the current location the next time
  bool isInitialized;                             // Indicates if the map source is initialized
  std::list<MapTile*> centerMapTiles;             // All tiles around the neighborhood of the map center

  // Does all action to remove a tile from the map
  void deinitTile(MapTile *t);

  // Fills the given area with tiles
  void fillGeographicAreaWithTiles(MapArea area, MapTile *preferredNeighbor, bool activateVisPos);

  // Remove all debugging primitives
  void removeDebugPrimitives();

public:

  // Constructor
  MapEngine();

  // Initializes the current map
  void initMap();

  // Frees the current map
  void deinitMap();

  // Called when the position of the map has changed
  void updateMap();

  // Destructor
  virtual ~MapEngine();

  // Checks if a map update is required
  bool mapUpdateIsRequired(GraphicPosition &visPos, Int *diffVisX=NULL, Int *diffVisY=NULL, double *diffZoom=NULL, bool checkLocationPos=true);

  // Saves the current position
  void backup();

  // Getters and setters
  bool getUpdateInProgress() const
  {
      return updateInProgress;
  }

  void setAbortUpdate()
  {
      if (updateInProgress) {
        this->abortUpdate = true;
        core->getMapCache()->setAbortUpdate();
      }
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
  void setForceMapRecreation()
  {
    forceMapRecreation=true;
  }
  void setForceMapUpdate()
  {
    core->getThread()->lockMutex(forceMapUpdateMutex);
    forceMapUpdate=true;
    core->getThread()->unlockMutex(forceMapUpdateMutex);
  }
  void setForceCacheUpdate()
  {
    core->getThread()->lockMutex(forceCacheUpdateMutex);
    forceCacheUpdate=true;
    core->getThread()->unlockMutex(forceCacheUpdateMutex);
  }

  MapPosition *lockMapPos()
  {
      core->getThread()->lockMutex(mapPosMutex);
      return &mapPos;
  }
  void unlockMapPos()
  {
      core->getThread()->unlockMutex(mapPosMutex);
  }
  void setMapPos(MapPosition mapPos);

  MapArea *lockDisplayArea()
  {
      core->getThread()->lockMutex(displayAreaMutex);
      return &displayArea;
  }
  void unlockDisplayArea()
  {
      core->getThread()->unlockMutex(displayAreaMutex);
  }

  void setIsInitialized(bool value)
  {
      isInitialized=value;
  }

  bool getIsInitialized() const
  {
      return isInitialized;
  }

  int getMaxTiles() const
  {
      return maxTiles;
  }

  void setMaxTiles();

  void setReturnToLocation(bool returnToLocation, bool showInfo=true);

  void setZoomLevelLock(bool zoomLevelLock, bool showInfo=true);

};

}

#endif /* MAPENGINE_H_ */
