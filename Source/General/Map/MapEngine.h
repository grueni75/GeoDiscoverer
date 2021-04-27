//============================================================================
// Name        : MapEngine.h
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

#include <GraphicObject.h>
#include <MapPosition.h>
#include <GraphicPosition.h>
#include <MapArea.h>
#include <MapCache.h>
#include <MapSource.h>

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
  double tileOffScreenFactor;                     // Factor to scale the screen length for rendering additional tiles outside the screen 
  Int maxTiles;                                   // Maximum number of tiles to show
  bool abortUpdate;                               // Indicates that the current update shall be stopped
  bool updateInProgress;                          // Indicates if the map is currently being updated
  bool forceMapUpdate;                            // Force an update of the map on the next call
  bool forceMapRecreation;                        // Force a complete recreation of the map on the next call
  bool forceZoomReset;                            // Force a reset of the zoom level on the next call
  bool forceCacheUpdate;                          // Force an update of the map cache on the next call
  bool redownloadAllZoomLevels;                   // Indicates if all zoom levels shall be re-downloaded
  bool forceMapRedownload;                        // Forces a redownload of all visble tiles
  ThreadMutexInfo *forceMapUpdateMutex;           // Mutex for accessing the force map update flag
  ThreadMutexInfo *forceCacheUpdateMutex;         // Mutex for accessing the force cache update flag
  ThreadMutexInfo *forceMapRedownloadMutex;       // Mutex for accessing the force map redownload flag
  TimestampInMicroseconds returnToLocationTimeout; // Time that must elapse before the map is repositioned to the current location
  bool returnToLocation;                          // Indicates if the map shall be centered around the location if the returnToLocationTimeout has elapsed
  bool zoomLevelLock;                             // Indicates if the zoom level of the map shall not be changed when zooming
  bool returnToLocationOneTime;                   // Return immediately to the current location the next time
  bool isInitialized;                             // Indicates if the map source is initialized
  std::list<MapTile*> centerMapTiles;             // All tiles around the neighborhood of the map center

  // Does all action to remove a tile from the map
  void deinitTile(MapTile *t, const char *file, int line);

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
  bool mapUpdateIsRequired(GraphicPosition visPos, Int *diffVisX=NULL, Int *diffVisY=NULL, double *diffZoom=NULL, bool checkLocationPos=true);

  // Saves the current position
  void backup();

  // Convert the distance in meters to pixels for the given map state
  bool calculateDistanceInScreenPixels(MapPosition src, MapPosition dst, double &distance);

  // Convert the distance in pixels to max possible meters for the given map state
  bool calculateMaxDistanceInMeters(Int distanceInPixels, double &distanceInMeters);

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
  MapPosition *lockLocationPos(const char *file, int line)
  {
      core->getThread()->lockMutex(locationPosMutex, file, line);
      return &locationPos;
  }
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
  void setForceMapRecreation()
  {
    forceMapRecreation=true;
  }
  void setForceZoomReset()
  {
    forceZoomReset=true;
  }
  void setForceMapRedownload(bool redownloadAllZoomLevels, const char *file, int line)
  {
    core->getThread()->lockMutex(forceMapRedownloadMutex, file, line);
    this->redownloadAllZoomLevels=redownloadAllZoomLevels;
    forceMapRedownload=true;
    core->getThread()->unlockMutex(forceMapRedownloadMutex);
  }
  void setForceMapUpdate(const char *file, int line)
  {
    core->getThread()->lockMutex(forceMapUpdateMutex, file, line);
    forceMapUpdate=true;
    core->getThread()->unlockMutex(forceMapUpdateMutex);
  }
  void setForceCacheUpdate(const char *file, int line)
  {
    core->getThread()->lockMutex(forceCacheUpdateMutex, file, line);
    forceCacheUpdate=true;
    core->getThread()->unlockMutex(forceCacheUpdateMutex);
  }

  MapPosition *lockMapPos(const char *file, int line)
  {
      core->getThread()->lockMutex(mapPosMutex, file, line);
      return &mapPos;
  }
  void unlockMapPos()
  {
      core->getThread()->unlockMutex(mapPosMutex);
  }
  void setMapPos(MapPosition mapPos);

  std::list<MapTile*> *lockCenterMapTiles(const char *file, int line)
  {
      core->getMapSource()->lockAccess(file,line);
      return &centerMapTiles;
  }
  void unlockCenterMapTiles()
  {
      core->getMapSource()->unlockAccess();
  }

  MapArea *lockDisplayArea(const char *file, int line)
  {
      core->getThread()->lockMutex(displayAreaMutex, file, line);
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

  void setZoomLevel(Int zoomLevel);

  Int getZoomLevel() {
    Int zoomLevel;
    lockDisplayArea(__FILE__,__LINE__);
    zoomLevel=displayArea.getZoomLevel();
    unlockDisplayArea();
    return zoomLevel;
  }

  void toggleZoomLevelLock()
  {
    setZoomLevelLock(!zoomLevelLock);
  }

};

}

#endif /* MAPENGINE_H_ */
