//============================================================================
// Name        : MapArchiveFile.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2022 Matthias Gruenewald
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
#include <MapArchiveFile.h>

namespace GEODISCOVERER {

// Constructor
MapArchiveFile::MapArchiveFile(std::string filePath, Long modificationTime, Long diskUsage) {
  this->filePath=filePath;
  this->modificationTime=modificationTime;
  this->diskUsage=diskUsage;
  //DEBUG("diskUsage=%d",diskUsage);
}

// Destructor
MapArchiveFile::~MapArchiveFile() {
}

// Inserts this entry to the list 
void MapArchiveFile::updateFiles(std::list<MapArchiveFile> *files, MapArchiveFile file) {
  bool inserted=false;
  for (std::list<MapArchiveFile>::iterator i=files->begin();i!=files->end();i++) {
    if (file.getModificationTime()<i->getModificationTime()) {
      files->insert(i,file);
      inserted=true;
      break;
    }
  }
  if (!inserted) {
    files->push_back(file);
  }
  /*for (std::list<MapArchiveFile>::iterator i=files->begin();i!=files->end();i++) {
    DEBUG("%s: %d",i->getFilePath().c_str(),i->getModificationTime());
  }*/
}

}
