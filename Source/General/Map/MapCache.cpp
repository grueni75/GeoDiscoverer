//============================================================================
// Name        : MapCache.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#include <Core.h>

namespace GEODISCOVERER {

// Constructor
MapCache::MapCache() {

  // Get some configuration
  size=0;
  abortUpdate=false;
  accessMutex=core->getThread()->createMutex("map cache access mutex");

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
  core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);

  // Determine size of cache
  size=core->getConfigStore()->getIntValue("Map","tileCacheSizeFactor",__FILE__, __LINE__);
  size*=core->getMapEngine()->getMaxTiles();

  // Ensure that no update is ongoing
  if (updateInProgress)
    FATAL("graphic recreation started while update is ongoing",NULL);

  // Create texture infos
  for (int i=0;i<size;i++) {
    GraphicTextureInfo t=core->getScreen()->createTextureInfo();
    unusedTextures.push_back(t);
  }

  // Put all tiles from the map source in the uncached list
  core->getMapSource()->lockAccess(__FILE__,__LINE__);
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
  core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);

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
  core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);
  tile->setIsCached(false);
  uncachedTiles.push_back(tile);
  core->getThread()->unlockMutex(accessMutex);
}

// Removes a tile from the cache
void MapCache::removeTile(MapTile *tile) {
  if (tile->isDrawn()) {
    FATAL("can not remove tile because it is currently drawn",NULL);
  }
  core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);
  std::list<MapTile*>::iterator i = std::find(cachedTiles.begin(), cachedTiles.end(), tile);
  if (i==cachedTiles.end()) {
    uncachedTiles.remove(tile);
    core->getThread()->unlockMutex(accessMutex);
  } else {
    if (!tile->getIsCached()) {
      FATAL("tile found in cached list but not marked as cached",NULL);
    }
    core->getThread()->unlockMutex(accessMutex);
    core->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
    GraphicRectangle *r=tile->getRectangle();
    r->setTextureAnimation(0,core->getScreen()->getTextureNotDefined(),core->getScreen()->getTextureNotDefined(),false,0);
    r->setZ(0);
    core->getGraphicEngine()->unlockDrawing();
    core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);
    usedTextures.remove(tile->getEndTexture());
    unusedTextures.push_back(tile->getEndTexture());
    cachedTiles.erase(i);
    tile->setIsCached(false);
    core->getThread()->unlockMutex(accessMutex);
  }
}

// Updates the map images of tiles
void MapCache::updateMapTileImages() {

  // Ensure that only one thread is executing this
  core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);

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

      // Prepare data
      if (currentImage)
        free(currentImage);
      currentImage=NULL;
      currentContainer=t->getParentMapContainer();
      //DEBUG("loading tiles from image <%s>",currentContainer->getImageFileName().c_str());

      // Check if the image exists in the map archive
      ZipArchive *mapArchive=NULL;
      std::list<ZipArchive*> *mapArchives = core->getMapSource()->lockMapArchives(__FILE__, __LINE__);
      for (std::list<ZipArchive*>::iterator i=mapArchives->begin();i!=mapArchives->end();i++) {
        if ((*i)->getEntrySize(currentContainer->getImageFilePath())>0) {
          mapArchive=*i;
          break;
        }
      }
      if (mapArchive) {

        // Create a temporary file that contains the image data
        std::string tempImageFilePath = core->getMapSource()->getFolderPath() + "/tile.bin";
        mapArchive->exportEntry(currentContainer->getImageFilePath(),tempImageFilePath);

        // Get the image from the temporary file
        switch(currentContainer->getImageType()) {
          case ImageTypeJPEG:
            currentImage=core->getImage()->loadJPEG(tempImageFilePath,currentImageWidth,currentImageHeight,currentImagePixelSize,true);
            break;
          case ImageTypePNG:
            currentImage=core->getImage()->loadPNG(tempImageFilePath,currentImageWidth,currentImageHeight,currentImagePixelSize,true);
            break;
          default:
            FATAL("unsupported image type",NULL);
            currentImage=NULL;
            break;
        }
      }
      core->getMapSource()->unlockMapArchives();
      if (!currentImage)
        continue; // Do not continue in case of error

    }

    // Remove the oldest tile from the cached list if it has already its max length
    if (cachedTiles.size()>=size) {
      core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);
      cachedTiles.sort(MapTile::lastAccessSortPredicate);
      MapTile *t=cachedTiles.front();
      cachedTiles.pop_front();
      uncachedTiles.push_back(t);
      core->getThread()->unlockMutex(accessMutex);
      GraphicTextureInfo m=t->getEndTexture();
      core->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
      GraphicRectangle *r=t->getRectangle();
      r->setZ(0);
      core->getGraphicEngine()->unlockDrawing();
      t->setIsCached(false);
      core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);
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
    core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);
    GraphicTextureInfo m=unusedTextures.front();
    unusedTextures.pop_front();
    usedTextures.push_back(m);
    for(std::list<MapTile*>::iterator i=cachedTiles.begin();i!=cachedTiles.end();i++) {
      MapTile *t=*i;
      if (t->getRectangle()->getTexture()==m)
        FATAL("cached tile uses textures that is marked as unused",NULL);
    }
    core->getThread()->unlockMutex(accessMutex);
    currentTile=t;
    tileTextureAvailable=true;
    core->tileTextureAvailable(__FILE__, __LINE__);
    tileTextureAvailable=false;

    // Remove the tile from the uncached list and add it to the cached
    //DEBUG("before setIsCached",NULL);
    t->setIsCached(true,usedTextures.back());
    //DEBUG("after setIsCached",NULL);
    core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);
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
