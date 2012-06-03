//============================================================================
// Name        : MapCache.cpp
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

// Constructor
MapCache::MapCache() {

  // Get some configuration
  size=core->getConfigStore()->getIntValue("Map","tileCacheSize");
  abortUpdate=false;
  accessMutex=core->getThread()->createMutex();

  // Reserve the memory for preparing a tile image
  //DEBUG("sizeof(*tileImageScratch)=%d",sizeof(*tileImageScratch));
  tileImageScratch=(UShort *)malloc(sizeof(*tileImageScratch)*core->getMapSource()->getMapTileWidth()*core->getMapSource()->getMapTileHeight());
  if (!tileImageScratch) {
    FATAL("could not reserve memory for the tile image",NULL);
    return;
  }

  // Init variables
  tileTextureAvailable=false;
  updateInProgress=false;
  isInitialized=false;
}

// Destructor
MapCache::~MapCache() {
  deinit();
  if (tileImageScratch)
    free(tileImageScratch);
  core->getThread()->destroyMutex(accessMutex);
}

// Initializes the cache
void MapCache::init() {
}

// Clears all graphic
void MapCache::destroyGraphic() {
  deinit();
}

// Creates all graphic
void MapCache::createGraphic() {

  // Ensure that only one thread is executing this
  core->getThread()->lockMutex(accessMutex);

  // Ensure that no update is ongoing
  if (updateInProgress)
    FATAL("graphic recreation started while update is ongoing",NULL);

  // Create texture infos
  for (int i=0;i<size;i++) {
    GraphicTextureInfo t=core->getScreen()->createTextureInfo();
    unusedTextures.push_back(t);
  }

  // Put all tiles from the map source in the uncached list
  core->getMapSource()->lockAccess();
  std::vector<MapContainer *> *maps=core->getMapSource()->getMapContainers();
  for (std::vector<MapContainer*>::const_iterator i=maps->begin();i!=maps->end();i++) {
    MapContainer *c=*i;
    //DEBUG("processing map image <%s>",c->getImageFileName().c_str());
    std::vector<MapTile *> *tiles=c->getMapTiles();
    for (std::vector<MapTile*>::const_iterator j=(*tiles).begin();j!=(*tiles).end();j++) {
      MapTile *t=*j;
      //DEBUG("adding tile at position <%d,%d> to uncached tile list",t->getMapX(),t->getMapY());
      t->setIsCached(false,core->getScreen()->getTextureNotDefined(),false);
      uncachedTiles.push_back(t);
    }
  }
  core->getMapSource()->unlockAccess();
  //DEBUG("number of tiles = %d",uncachedTiles.size());

  // Object is initialized
  isInitialized=true;

  // Let the other threads continue
  core->getThread()->unlockMutex(accessMutex);
}

// Clears the cache
void MapCache::deinit() {

  // Ensure that only one thread is executing this
  core->getThread()->lockMutex(accessMutex);

  // Clear all list
  for(std::list<MapTile*>::iterator i=cachedTiles.begin();i!=cachedTiles.end();i++) {
    MapTile *t=*i;
    t->getRectangle()->setTextureAnimation(0,core->getScreen()->getTextureNotDefined(),core->getScreen()->getTextureNotDefined(),false,0);
  }
  cachedTiles.clear();
  for(std::list<GraphicTextureInfo>::iterator i=unusedTextures.begin();i!=unusedTextures.end();i++) {
    core->getScreen()->destroyTextureInfo(*i,"MapCache (unused texture)");
  }
  unusedTextures.clear();
  for(std::list<GraphicTextureInfo>::iterator i=usedTextures.begin();i!=usedTextures.end();i++) {
    core->getScreen()->destroyTextureInfo(*i,"MapCache (used texture)");
  }
  usedTextures.clear();
  uncachedTiles.clear();

  // Object is not initialized anymore
  isInitialized=false;

  // Let the other threads continue
  core->getThread()->unlockMutex(accessMutex);

}

// Adds a new tile to the cache
void MapCache::addTile(MapTile *tile) {
  core->getThread()->lockMutex(accessMutex);
  tile->setIsCached(false);
  uncachedTiles.push_back(tile);
  core->getThread()->unlockMutex(accessMutex);
}

// Removes a tile from the cache
void MapCache::removeTile(MapTile *tile) {
  if (tile->isDrawn()) {
    FATAL("can not remove tile because it is currently drawn",NULL);
  }
  core->getThread()->lockMutex(accessMutex);
  std::list<MapTile*>::iterator i = std::find(cachedTiles.begin(), cachedTiles.end(), tile);
  if (i==cachedTiles.end()) {
    uncachedTiles.remove(tile);
    core->getThread()->unlockMutex(accessMutex);
  } else {
    core->getThread()->unlockMutex(accessMutex);
    tile->getVisualization()->lockAccess();
    GraphicRectangle *r=tile->getRectangle();
    GraphicTextureInfo m=r->getTexture();
    r->setTextureAnimation(0,core->getScreen()->getTextureNotDefined(),core->getScreen()->getTextureNotDefined(),false,0);
    r->setZ(0);
    tile->getVisualization()->unlockAccess();
    core->getThread()->lockMutex(accessMutex);
    usedTextures.remove(m);
    unusedTextures.push_back(m);
    cachedTiles.erase(i);
    tile->setIsCached(false);
    core->getThread()->unlockMutex(accessMutex);
  }
}

// Updates the map images of tiles
void MapCache::updateMapTileImages() {

  // Ensure that only one thread is executing this
  core->getThread()->lockMutex(accessMutex);

  // Check that all visible tile are cached
  // If not, add them to the load list
  std::list<MapTile *> mapTileLoadList;
  for (std::list<MapTile*>::const_iterator i=uncachedTiles.begin();i!=uncachedTiles.end();i++) {
    MapTile *t=*i;
    if ((t->isDrawn())&&(t->getParentMapContainer()->getDownloadComplete())) {
      mapTileLoadList.push_back(t);
    }
  }
  core->getThread()->unlockMutex(accessMutex);
  //return;
  if (mapTileLoadList.size()==0) {
    return; // Nothing to do
  }

  // Update has started
  updateInProgress=true;

  // Sort the load list according to the map container
  mapTileLoadList.sort(MapTile::parentSortPredicate);

  // Go through the list
  MapContainer *currentContainer=NULL;
  ImagePixel *currentImage=NULL;
  Int currentImageWidth,currentImageHeight;
  UInt currentImagePixelSize;
  for (std::list<MapTile*>::const_iterator i=mapTileLoadList.begin();i!=mapTileLoadList.end()&&!abortUpdate;i++) {
    MapTile *t=*i;

    // Load the new map if required
    if (currentContainer!=t->getParentMapContainer()) {

      if (currentImage)
        free(currentImage);
      currentImage=NULL;
      currentContainer=t->getParentMapContainer();
      //DEBUG("loading tiles from image <%s>",currentContainer->getImageFileName().c_str());
      if (access(currentContainer->getImageFilePath().c_str(),F_OK)==0) {
        switch(currentContainer->getImageType()) {
          case ImageTypeJPEG:
            currentImage=core->getImage()->loadJPEG(currentContainer->getImageFilePath(),currentImageWidth,currentImageHeight,currentImagePixelSize);
            break;
          case ImageTypePNG:
            currentImage=core->getImage()->loadPNG(currentContainer->getImageFilePath(),currentImageWidth,currentImageHeight,currentImagePixelSize);
            break;
          default:
            FATAL("unsupported image type",NULL);
            currentImage=NULL;
            break;
        }
      }
      if (!currentImage)
        continue; // Do not continue in case of error

    }

    // Remove the oldest tile from the cached list if it has already its max length
    if (cachedTiles.size()>=size) {
      core->getThread()->lockMutex(accessMutex);
      cachedTiles.sort(MapTile::lastAccessSortPredicate);
      MapTile *t=cachedTiles.front();
      cachedTiles.pop_front();
      uncachedTiles.push_back(t);
      core->getThread()->unlockMutex(accessMutex);
      GraphicTextureInfo m=t->getEndTexture();
      t->getVisualization()->lockAccess();
      GraphicRectangle *r=t->getRectangle();
      r->setZ(0);
      t->getVisualization()->unlockAccess();
      t->setIsCached(false);
      core->getThread()->lockMutex(accessMutex);
      usedTextures.remove(m);
      unusedTextures.push_back(m);
      core->getThread()->unlockMutex(accessMutex);
    }

    // Do some sanity checks
    if ((t->getWidth()!=core->getMapSource()->getMapTileWidth())||(t->getHeight()!=core->getMapSource()->getMapTileHeight())) {
      FATAL("tiles whose width or height does not match the default are not supported",NULL);
      break;
    }

    // Copy the image of the map tile from the map image
    // Do the conversion to RGB565
    //DEBUG("copying tile image from map image",NULL);
    for (Int y=0;y<t->getHeight();y++) {
      for (Int x=0;x<t->getWidth();x++) {
        Int imagePos=((t->getMapY()+y)*currentImageWidth+(t->getMapX()+x))*currentImagePixelSize;
        Int scratchPos=y*t->getWidth()+x;
        tileImageScratch[scratchPos]=((currentImage[imagePos]>>3)<<11 | (currentImage[imagePos+1]>>2)<<5 | (currentImage[imagePos+2]>>3));
      }
    }

    // Ensure that the tile is drawn first
    //t->getVisualization()->setZ(1);

    // Update the texture
    /*std::list<std::string> names = t->getVisName();
    std::string firstName = names.front();
    names.pop_front();
    std::string secondName = names.front();
    DEBUG("updating texture for tile %s (%s)",firstName.c_str(),secondName.c_str());*/
    core->getThread()->lockMutex(accessMutex);
    GraphicTextureInfo m=unusedTextures.front();
    unusedTextures.pop_front();
    usedTextures.push_back(m);
    core->getThread()->unlockMutex(accessMutex);
    currentTile=t;
    tileTextureAvailable=true;
    core->tileTextureAvailable();
    tileTextureAvailable=false;

    // Remove the tile from the uncached list and add it to the cached
    t->setIsCached(true,usedTextures.back());
    core->getThread()->lockMutex(accessMutex);
    uncachedTiles.remove(t);
    cachedTiles.push_back(t);
    core->getThread()->unlockMutex(accessMutex);

  }

  // Clean up
  if (currentImage)
    free(currentImage);
  updateInProgress=false;
  abortUpdate=false;
}

// Updates the currently waiting texture
void MapCache::setNextTileTexture()
{
  GraphicTextureInfo m=usedTextures.back();
  core->getScreen()->setTextureImage(m,tileImageScratch,currentTile->getWidth(),currentTile->getHeight());
  tileTextureAvailable=false;
}

// Indicates that there was a change in the map tile visibility
void MapCache::tileVisibilityChanged() {
  updateMapTileImages();
}


}
