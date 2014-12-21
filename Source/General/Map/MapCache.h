//============================================================================
// Name        : MapCache.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef MAPCACHE_H_
#define MAPCACHE_H_

namespace GEODISCOVERER {

class MapCache {

protected:

  Int size;                                       // Number of tiles to cache
  UShort *tileImageScratch;                       // Holds the image of one tile for handover to the graphic system
  MapTile *currentTile;                           // Tile that is currently processed
  std::list<MapTile *> cachedTiles;               // List of tiles that are currently cached
  std::list<MapTile *> uncachedTiles;             // List of tiles that are currently not cached
  std::list<GraphicTextureInfo> usedTextures;     // List of used texture infos
  std::list<GraphicTextureInfo> unusedTextures;   // List of tiles that are currently not cached
  bool tileTextureAvailable;                      // Indicates that a new map textue is available
  bool abortUpdate;                               // Indicates that the current cache update shall be stopped
  bool updateInProgress;                          // Indicates if an update is currently ongoing
  bool isInitialized;                             // Indicates if the map cache is initialized
  ThreadMutexInfo *accessMutex;                   // Mutex for modifying the object

  // Updates the map images of tiles
  void updateMapTileImages();

public:

  // Constructors and destructor
  MapCache();
  virtual ~MapCache();

  // Inits the cache
  void init();

  // Creates all graphics
  void createGraphic();

  // Clears all graphics
  void destroyGraphic();

  // Clears the cache
  void deinit();

  // Indicates that there was a change in the visibility of tiles
  void tileVisibilityChanged();

  // Adds a new tile to the cache
  void addTile(MapTile *tile);

  // Removes a tile from the cache
  void removeTile(MapTile *tile);

  // Updates the currently waiting texture
  void setNextTileTexture();

  // Getters and setters
  bool getTileTextureAvailable() const
  {
    return tileTextureAvailable;
  }

  void setAbortUpdate()
  {
      if (updateInProgress) {
        this->abortUpdate = true;
        core->getImage()->setAbortLoad();
      }
  }

  bool getUpdateInProgress() const
  {
      return updateInProgress;
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

#endif /* MAPCACHE_H_ */
