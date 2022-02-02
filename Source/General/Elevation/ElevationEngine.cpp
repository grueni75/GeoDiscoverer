//============================================================================
// Name        : ElevationEngine.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2021 Matthias Gruenewald
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
#include <ElevationEngine.h>

namespace GEODISCOVERER {

// Constructor
ElevationEngine::ElevationEngine() {
  isInitialized=false;
  demFolderPath=core->getHomePath() + "/DEM";
  demDatasetFullRes=NULL;
  demDatasetLowRes=NULL;
  demDatasetBusy=NULL;  
  workerCount=core->getConfigStore()->getIntValue("MapTileServer","workerCount",__FILE__,__LINE__);
  accessMutex=core->getThread()->createMutex("elevation engine access mutex");
  demDatasetReadySignal=core->getThread()->createSignal();
  tileNumber=0;
  
  // Create the DEM directory (if it does not exist)
  struct stat st;
  if (core->statFile(demFolderPath, &st) != 0)
  {
    if (mkdir(demFolderPath.c_str(),S_IRWXU | S_IRWXG | S_IRWXO)!=0) {
      FATAL("can not create DEM directory",NULL);
      return;
    }
  }

  // Read all the additional tilesX.gda files
  struct dirent *dp;
  DIR *dfd;
  dfd=core->openDir(demFolderPath);
  if (dfd==NULL) {
    FATAL("can not read directory <%s>",demFolderPath.c_str());
    return;
  }
  while ((dp = readdir(dfd)) != NULL)
  {
    std::string filename(dp->d_name);
    //DEBUG("filename=%s",filename.c_str());
    if (filename.substr(0,5)=="warp_") {
      remove((demFolderPath + "/" + filename).c_str());
    }
  }
  closedir(dfd);
}

// Destructor
ElevationEngine::~ElevationEngine() {
  deinit();
  core->getThread()->destroyMutex(accessMutex);
}

}
