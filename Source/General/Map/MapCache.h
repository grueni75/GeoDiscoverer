//============================================================================
// Name        : MapCache.h
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


#ifndef MAPCACHE_H_
#define MAPCACHE_H_

namespace GEODISCOVERER {

class MapCache {

protected:

  Int size;                                       // Number of tiles to cache
  UByte notCachedTileAlpha;                       // Alpha value of a tile that is not cached
  UByte cachedTileAlpha;                          // Alpha value of a tile that is cached
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

public:

  // Constructors and destructor
  MapCache();
  virtual ~MapCache();

  // Inits the cache
  void init();

  // Clears the cache
  void deinit();

  // Updates the map images of tiles
  void updateMapTileImages();

  // Indicates that there was a change in the visibility of tiles
  void tileVisibilityChanged();

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

  UByte getCachedTileAlpha() const
  {
      return cachedTileAlpha;
  }

  UByte getNotCachedTileAlpha() const
  {
      return notCachedTileAlpha;
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
