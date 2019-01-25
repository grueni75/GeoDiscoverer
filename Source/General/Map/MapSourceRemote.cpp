//============================================================================
// Name        : MapSourceRemote.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2018 Matthias Gruenewald
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

MapSourceRemote::MapSourceRemote()  : MapSource() {

  // Set important variables
  type=MapSourceTypeRemote;
  mapArchiveCacheSize=core->getConfigStore()->getIntValue("Map","mapArchiveCacheSize",__FILE__, __LINE__);
  nextFreeMapArchiveNumber=0;
  nextFreeOverlayArchiveNumber=0;
}

MapSourceRemote::~MapSourceRemote() {
  deinit();
}

// Clear the source
void MapSourceRemote::deinit()
{
  MapSource::deinit();
}

// Loads all calibrated pictures in the given directory
bool MapSourceRemote::collectMapTiles(std::string directory, std::list<std::vector<std::string> > &mapFilebases)
{
  std::string filename;

  // Go through all archives
  core->getMapSource()->lockMapArchives(__FILE__, __LINE__);
  for (std::list<ZipArchive*>::iterator i=mapArchives.begin();i!=mapArchives.end();i++) {

    // Go through all entries in the archive
    for (Int j=0;j<(*i)->getEntryCount();j++) {

      // Get the file name of the entry
      filename = (*i)->getEntryFilename(j);

      // If this file is not a calibration file, skip it
      Int pos=filename.find_last_of(".");
      std::string extension=filename.substr(pos+1);
      std::string filebase=filename.substr(0,pos);

      // Check for supported extensions
      if (!MapContainer::calibrationFileIsSupported(extension))
        continue;

      // Remember the basename of the file
      std::vector<std::string> names;
      names.push_back((*i)->getArchiveFolder());
      names.push_back((*i)->getArchiveName());
      names.push_back(filebase);
      names.push_back(extension);
      mapFilebases.push_back(names);
    }
  }
  unlockMapArchives();

  return true;
}

// Initializes the source
bool MapSourceRemote::init()
{
  Int progress;
  DialogKey dialog;
  std::string title;
  ZipArchive *mapArchive;
  std::string mapArchiveDir;
  std::string mapArchiveFile;
  char *mapArchivePathCStr = NULL;
  std::list<std::vector<std::string> > mapFilebases;
  MapContainer *mapContainer;

  // Check if the log directory exists
  std::string path = core->getHomePath() + "/Map";
  struct stat st;
  if (core->statFile(path.c_str(), &st) != 0)
  {
    if (mkdir(path.c_str(),S_IRWXU | S_IRWXG | S_IRWXO)!=0) {
      FATAL("can not create Map directory",NULL);
      return false;
    }
  }
  if (core->statFile(getFolderPath(), &st) != 0)
  {
    if (mkdir(getFolderPath().c_str(),S_IRWXU | S_IRWXG | S_IRWXO)!=0) {
      FATAL("can not create Map/Default directory",NULL);
      return false;
    }
  }

  // Cleanup the map folder if requested
  bool resetArchives=false;;
  if (core->getConfigStore()->getIntValue("Map/Remote","reset",__FILE__,__LINE__)) {
    resetArchives=true;
    core->getConfigStore()->setIntValue("Map/Remote","reset",0,__FILE__,__LINE__);
  }

  // Init important values
  this->minZoomLevel=core->getConfigStore()->getIntValue("Map/Remote","minZoomLevel",__FILE__, __LINE__);
  this->maxZoomLevel=core->getConfigStore()->getIntValue("Map/Remote","maxZoomLevel",__FILE__, __LINE__);
  DEBUG("minZoomLevel=%d maxZoomLevel=%d",minZoomLevel,maxZoomLevel);

  // Init the center position
  centerPosition=new MapPosition();
  if (!centerPosition) {
    FATAL("can not create map position object",NULL);
  }
  centerPosition->setLng(core->getConfigStore()->getDoubleValue("Map/Remote","centerLng",__FILE__, __LINE__));
  centerPosition->setLat(core->getConfigStore()->getDoubleValue("Map/Remote","centerLat",__FILE__, __LINE__));
  centerPosition->setLngScale(core->getConfigStore()->getDoubleValue("Map/Remote","centerLngScale",__FILE__, __LINE__));
  centerPosition->setLatScale(core->getConfigStore()->getDoubleValue("Map/Remote","centerLatScale",__FILE__, __LINE__));

  // Get all existing tiles of the map
  std::list<std::string> mapArchivePaths;
  struct dirent *dp;
  Int nr;
  DIR *dfd;
  dfd=core->openDir(getFolderPath());
  if (dfd==NULL) {
    FATAL("can not read directory <%s>",getFolderPath().c_str());
    return false;
  }
  while ((dp = readdir(dfd)) != NULL)
  {
    if ((sscanf(dp->d_name,"tile%d.gda",&nr)==1)&&(dp->d_name[strlen(dp->d_name)-1]!='o')) {
      mapArchivePaths.push_back(getFolderPath() + "/" + dp->d_name);
      if (nr>nextFreeMapArchiveNumber)
        nextFreeMapArchiveNumber=nr;
    }
  }
  closedir(dfd);
  nextFreeMapArchiveNumber++;
  if (resetArchives) {
    DEBUG("removing all map and overlay archives because map source has changed",NULL);
    for (std::list<std::string>::iterator i=mapArchivePaths.begin();i!=mapArchivePaths.end();i++) {
      remove((*i).c_str());
      std::string overlayArchive = (*i).substr(0,(*i).length()-1) + "o";
      DEBUG("overlayArchive=%s",overlayArchive.c_str());
      remove(overlayArchive.c_str());
    }
    mapArchivePaths.clear();
    nextFreeMapArchiveNumber=0;
  }
  mapArchivePaths.sort();
  while (mapArchivePaths.size()>mapArchiveCacheSize) {
    DEBUG("removing map archive %s from disk cache",mapArchivePaths.front().c_str());
    remove(mapArchivePaths.front().c_str());
    std::string overlayArchive = mapArchivePaths.front().substr(0,mapArchivePaths.front().length()-1) + "o";
    DEBUG("overlayArchive=%s",overlayArchive.c_str());
    remove(overlayArchive.c_str());
    mapArchivePaths.pop_front();
  }

  // Open the zip archive that contains the maps
  //title="Reading tiles of map " + folder;
  //dialog=core->getDialog()->createProgress(title,mapArchivePaths.size());
  progress=0;
  lockMapArchives(__FILE__, __LINE__);
  for(std::list<std::string>::iterator i=mapArchivePaths.begin();i!=mapArchivePaths.end();i++) {
    std::string mapArchivePath=*i;
    mapArchivePathCStr=strdup(mapArchivePath.c_str());
    mapArchiveDir=std::string(dirname(mapArchivePathCStr));
    strcpy(mapArchivePathCStr,mapArchivePath.c_str());
    mapArchiveFile=std::string(basename(mapArchivePathCStr));
    free(mapArchivePathCStr);
    if (!(mapArchive=new ZipArchive(mapArchiveDir,mapArchiveFile))) {
      FATAL("can not create zip archive object",NULL);
      return false;
    }
    bool useMapArchive=true;
    if (!mapArchive->init()) {
      DEBUG("map archive <%s> can not be opened, removing it",mapArchivePath.c_str());
      remove((*i).c_str());
      useMapArchive=false;
    }
    if (useMapArchive) {
      mapArchives.push_back(mapArchive);
    } else {
      delete mapArchive;
    }
    progress++;
    //core->getDialog()->updateProgress(dialog,title,progress);
  }
  unlockMapArchives();
  //core->getDialog()->closeProgress(dialog);

  // Prevent that phone switches off
  //core->getDefaultScreen()->setWakeLock(true, __FILE__, __LINE__, false);

  // Create a new progress dialog if required
  //title="Collecting files of map " + std::string(folder);
  //dialog=core->getDialog()->createProgress(title,0);

  // Go through all calibration files in the map directory
  if (!collectMapTiles(getFolderPath(),mapFilebases)) {
    return false;
  }
  //DEBUG("mapFilebases.size()=%d",mapFilebases.size());

  // Remove duplicates
  mapFilebases.sort();
  mapFilebases.unique();
  //core->getDialog()->closeProgress(dialog);

  // Create the progress dialog
  //title="Reading files of map " + std::string(folder);
  //dialog=core->getDialog()->createProgress(title,mapFilebases.size());

  // Go through all found maps
  progress=1;
  //DEBUG("mapContainers.size()=%d",mapContainers.size());
  for (std::list<std::vector<std::string> >::const_iterator i=mapFilebases.begin();i!=mapFilebases.end();i++) {

    std::string filebase=(*i)[2];
    std::string extension=(*i)[3];
    std::string filepath=filebase + "." + extension;
    //DEBUG("filebase=%s",filebase.c_str());

    // Output some info
    Int percentage=round(((double)progress*100)/(double)mapFilebases.size());
    //INFO("reading map <%s> (%d%%)", filepath.c_str(), percentage);
    //core->getDialog()->updateProgress(dialog,title,progress);
    progress++;

    // Create a new map container and read the calibration in
    mapContainer=new MapContainer();
    if (!mapContainer) {
      FATAL("can not create map container",NULL);
      return false;
    }
    mapContainer->setArchiveFileFolder((*i)[0]);
    mapContainer->setArchiveFileName((*i)[1]);
    if (!(mapContainer->readCalibrationFile(std::string(dirname((char*)filebase.c_str())),std::string(basename((char*)filebase.c_str())),extension))) {
      return false;
    }
    //DEBUG("archiveFileFolder=%s archiveFileName=%s archiveFilePath=%s",mapContainer->getArchiveFileFolder().c_str(),mapContainer->getArchiveFileName().c_str(),mapContainer->getArchiveFilePath().c_str());
    mapContainers.push_back(mapContainer);
    if ((mapContainer->getZoomLevelMap()>maxZoomLevel)||(mapContainer->getZoomLevelMap()<minZoomLevel)) {
      DEBUG("zoom level bounds reported by remote server are not correct, requesting reset of map the next time and aborting this time",NULL);
      core->getConfigStore()->setIntValue("Map/Remote","reset",1,__FILE__,__LINE__);
      return false;
    }

    // Read the overlay in if it exists
    std::string overlayFilename = mapContainer->getOverlayFileName();
    //DEBUG("overlayFilename=%s",overlayFilename.c_str());
    struct stat st;
    if (core->statFile(mapContainer->getArchiveFileFolder() + "/" + overlayFilename, &st)==0) {
      mapContainer->retrieveOverlayGraphics(mapContainer->getArchiveFileFolder(),overlayFilename);
    }

    // Normalize the zoom level
    mapContainer->setZoomLevelMap(mapContainer->getZoomLevelMap()-minZoomLevel+1);
  }

  // Init the zoom levels
  for (int z=0;z<(maxZoomLevel-minZoomLevel)+2;z++) {
    zoomLevelSearchTrees.push_back(NULL);
  }
  //core->getDialog()->closeProgress(dialog);

  // Update the search structures
  createSearchDataStructures(false);

  // Reset wakelock
  //core->getDefaultScreen()->setWakeLock(core->getConfigStore()->getIntValue("General","wakeLock",__FILE__, __LINE__),__FILE__, __LINE__,false);

  //DEBUG("centerPosition.lng=%e centerPosition.lat=%e",centerPosition->getLng(),centerPosition->getLat());

  // Finished
  isInitialized=true;
  return true;
}

// Returns the map tile in which the position lies
MapTile *MapSourceRemote::findMapTileByGeographicCoordinate(MapPosition pos, Int zoomLevel, bool lockZoomLevel, MapContainer *preferredMapContainer) {

  // Construct the command to send to the remote server
  std::stringstream cmd;
  cmd << "findRemoteMapTileByGeographicCoordinate("
      << pos.getLng() << "," << pos.getLat() << "," << pos.getLngScale() << "," << pos.getLatScale() << ","
      << zoomLevel << "," << (lockZoomLevel ? 1 : 0) << ",";
  if (preferredMapContainer)
    cmd << preferredMapContainer->getCalibrationFilePath();

  // Get all known map containers for all zoom levels
  for (Int z=1;z<zoomLevelSearchTrees.size();z++) {
    MapTile *t = MapSource::findMapTileByGeographicCoordinate(pos,z,true,NULL);
    if (t) {
      cmd << "," << t->getParentMapContainer()->getCalibrationFilePath() << "," << t->getParentMapContainer()->getOverlayGraphicHash();
    }
  }

  // Then get the best matching one
  MapTile *result = MapSource::findMapTileByGeographicCoordinate(pos,zoomLevel,lockZoomLevel,preferredMapContainer);

  // Ask the remote side to send any missing map containers
  cmd << ")";
  //DEBUG("cmd=%s",cmd.str().c_str());
  core->getCommander()->dispatch(cmd.str());

  return result;
}

// Fills the given area with tiles
void MapSourceRemote::fillGeographicAreaWithTiles(MapArea area, MapTile *preferredNeighbor, Int maxTiles, std::list<MapTile*> *tiles, bool *abort) {

  // Prepare the cmd for the remote side
  std::stringstream cmd;
  cmd << "fillGeographicAreaWithRemoteTiles("
      << area.getRefPos().getLng() << "," << area.getRefPos().getLat() << ","
      << area.getRefPos().getLngScale() << "," << area.getRefPos().getLatScale() << ","
      << area.getZoomLevel() << ","
      << area.getRefPos().getX() << "," << area.getRefPos().getY() << ","
      << area.getYNorth() << "," << area.getYSouth() << "," << area.getXEast() << "," << area.getXWest() << ","
      << area.getLatNorth() << "," << area.getLatSouth() << "," << area.getLngEast() << "," << area.getLngWest() << ","
      << maxTiles << ",";
  if (preferredNeighbor) {
    cmd << preferredNeighbor->getParentMapContainer()->getCalibrationFilePath() << ","
        << preferredNeighbor->getMapX() << "," << preferredNeighbor->getMapY();
  } else {
    cmd << ",0,0";
  }

  // First check what is available locally
  MapSource::fillGeographicAreaWithTiles(area,preferredNeighbor,maxTiles,tiles,abort);

  // Get all found map containers
  for (std::list<MapTile*>::iterator i=tiles->begin();i!=tiles->end();i++) {
    cmd << "," << (*i)->getParentMapContainer()->getCalibrationFilePath() << "," << (*i)->getParentMapContainer()->getOverlayGraphicHash();
  }

  // Ask the remote side to send any missing map containers
  cmd << ")";
  if (!*abort) {
    //DEBUG("cmd=%s",cmd.str().c_str());
    core->getCommander()->dispatch(cmd.str());
  } else {
    //DEBUG("search operation was aborted, skipping cmd",NULL);
  }
}

// Returns the next free map archive file name
std::string MapSourceRemote::getFreeMapArchiveFilePath() {
  lockAccess(__FILE__,__LINE__);
  std::stringstream path;
  path << getFolderPath() << "/tile" << nextFreeMapArchiveNumber << ".gda";
  nextFreeMapArchiveNumber++;
  unlockAccess();
  return path.str();
}

// Performs maintenance (e.g., recreate degraded search tree)
void MapSourceRemote::maintenance() {

  // Was the source modified?
  //DEBUG("maintenance called",NULL);
  if (contentsChanged) {
    contentsChanged=false;
  }
}

// Adds a new map archive
bool MapSourceRemote::addMapArchive(std::string path, std::string hash) {

  ZipArchive *mapArchive;
  MapContainer *mapContainer;

  DEBUG("addMapArchive called with path %s",path.c_str());

  // If file is corrupted, do not use it
  std::string computedHash = Storage::computeMD5(path);
  if (computedHash!=hash) {
    DEBUG("new map archive <%s> is corrupt, skipping it",path.c_str());
    remove(path.c_str());
    return false;
  }

  // Rename the file to match the namings used on this side
  std::string t=getFreeMapArchiveFilePath();
  rename(path.c_str(),t.c_str());
  path=t;

  // Read in the archive
  std::string archiveFolder = dirname((char*)path.c_str());
  std::string archiveFilename = basename((char*)path.c_str());
  if (!(mapArchive=new ZipArchive(archiveFolder,archiveFilename))) {
    FATAL("can not create zip archive object",NULL);
    return false;
  }
  if (!mapArchive->init()) {
    FATAL("can not open <%s> in map directory <%s>",archiveFilename.c_str(),folder.c_str());
    return false;
  }

  // Check if archive is already present
  lockMapArchives(__FILE__,__LINE__);
  for (std::list<ZipArchive*>::iterator i=mapArchives.begin();i!=mapArchives.end();i++) {
    //DEBUG("checking if %s is different to %s",(*i)->getArchiveName().c_str(),mapArchive->getArchiveName().c_str());
    if ((*i)->getEntryCount()==mapArchive->getEntryCount()) {
      bool archiveDiffers=false;
      for (Int j=0;j<(*i)->getEntryCount();j++) {
        bool found=false;
        for (Int k=0;k<mapArchive->getEntryCount();k++) {
          if (mapArchive->getEntryFilename(k)==(*i)->getEntryFilename(j)) {
            found=true;
            break;
          }
        }
        if (!found) {
          //DEBUG("archive %s and %s differ",(*i)->getArchiveName().c_str(),mapArchive->getArchiveName().c_str());
          archiveDiffers=true;
          break;
        }
      }
      if (!archiveDiffers) {
        DEBUG("new archive <%s> already received, skipping it",mapArchive->getArchiveName().c_str());
        delete mapArchive;
        remove(path.c_str());
        unlockMapArchives();
        return false;
      }
    }
  }
  mapArchives.push_back(mapArchive);

  // Go through all entries in the archive and create map containers
  std::list<MapContainer*> newMapContainers;
  for (Int i=0;i<mapArchive->getEntryCount();i++) {

    // Get the file name of the entry
    std::string filename = mapArchive->getEntryFilename(i);

    // If this file is not a calibration file, skip it
    Int pos=filename.find_last_of(".");
    std::string extension=filename.substr(pos+1);
    std::string filebase=filename.substr(0,pos);
    std::string filepath=filebase + "." + extension;

    // Check for supported extensions
    if (!MapContainer::calibrationFileIsSupported(extension))
      continue;

    // Create a new map container and read the calibration in
    mapContainer=new MapContainer();
    if (!mapContainer) {
      FATAL("can not create map container",NULL);
      return false;
    }
    mapContainer->setArchiveFileFolder(archiveFolder);
    mapContainer->setArchiveFileName(archiveFilename);
    if (!(mapContainer->readCalibrationFile(std::string(dirname((char*)filebase.c_str())),std::string(basename((char*)filebase.c_str())),extension))) {
      return false;
    }
    mapContainer->setZoomLevelMap(mapContainer->getZoomLevelMap()-minZoomLevel+1);
    newMapContainers.push_back(mapContainer);
  }
  unlockMapArchives();

  // Now add all map containers
  for (std::list<MapContainer*>::iterator i=newMapContainers.begin();i!=newMapContainers.end();i++) {
    mapContainer=*i;

    // Add the new map container
    lockAccess(__FILE__,__LINE__);
    mapContainers.push_back(mapContainer);
    insertNodeIntoSearchTree(mapContainer,mapContainer->getZoomLevelMap(),NULL,false,GeographicBorderLatNorth);
    insertNodeIntoSearchTree(mapContainer,0,NULL,false,GeographicBorderLatNorth);
    contentsChanged=true;
    unlockAccess();

    // Request the cache to add this tile
    std::vector<MapTile*> *tiles=mapContainer->getMapTiles();
    for (std::vector<MapTile*>::iterator j=tiles->begin();j!=tiles->end();j++) {
      core->getMapCache()->addTile(*j);
    }

    // Request the navigation engine to add overlays to the new tile
    core->getNavigationEngine()->addGraphics(mapContainer);
  }

  // Redraw the map
  core->getMapEngine()->setForceZoomReset();

  return true;
}

// Adds a new overlay archive
bool MapSourceRemote::addOverlayArchive(std::string path, std::string hash) {

  MapContainer *mapContainer;

  DEBUG("addOverlayArchive called with path %s",path.c_str());

  // If file is corrupted, do not use it
  std::string computedHash = Storage::computeMD5(path);
  if (computedHash!=hash) {
    DEBUG("new overlay archive <%s> is corrupt, skipping it",path.c_str());
    remove(path.c_str());
    return false;
  }

  // Open the file
  std::ifstream ifs;
  ifs.open(path.c_str(),std::ios::binary);
  if (ifs.fail()) {
    FATAL("can not open <%s> for reading",path.c_str());
    return false;
  }

  // Load the complete file into memory
  struct stat filestat;
  char *data;
  core->statFile(path,&filestat);
  if (!(data=(char*)malloc(filestat.st_size+1))) {
    FATAL("can not allocate memory for reading complete file",NULL);
    return false;
  }
  ifs.read(data,filestat.st_size);
  ifs.close();
  data[filestat.st_size]=0; // to prevent that strings never end
  Int size=filestat.st_size;
  //DEBUG("size=%d",size);

  // Get the map container name
  char *t;
  Storage::retrieveString(data,size,&t);
  std::string calibrationFilepath=std::string(t);
  free(data);

  // Find the map container this overlay archive belongs to
  DEBUG("calibrationFilepath=%s",calibrationFilepath.c_str());
  MapContainer *c=NULL;
  lockAccess(__FILE__,__LINE__);
  for (std::vector<MapContainer*>::iterator i=mapContainers.begin();i!=mapContainers.end();i++) {
    if ((*i)->getCalibrationFilePath()==calibrationFilepath) {
      c=*i;
      break;
    }
  }
  if (c==NULL) {
    DEBUG("no map container found that the overlay <%s> belongs to, skipping it",path.c_str());
    remove(path.c_str());
    unlockAccess();
    return false;
  }

  // Rename new archive to fit the expected name
  std::string newPath = getFolderPath() + "/" + c->getOverlayFileName();
  rename(path.c_str(),newPath.c_str());

  // Read in the overlay
  c->retrieveOverlayGraphics(c->getArchiveFileFolder(),c->getOverlayFileName());

  // Redraw the map
  core->getMapEngine()->setForceZoomReset();

  // That's it
  unlockAccess();
  return true;
}

}
