//============================================================================
// Name        : MapArchiveFile.h
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

#include <string>
#include <list>
#include <Types.h>

#ifndef MAPARCHIVEFILE_H_
#define MAPARCHIVEFILE_H_

namespace GEODISCOVERER {

class MapArchiveFile {

protected:

  std::string filePath;         // Path of the map archive
  Long modificationTime;        // Last time the file was modified 
  Long diskUsage;               // Size of the file in bytes

public:

  // Constructors and destructor
  MapArchiveFile(std::string filePath, Long modificationTime, Long diskUsage);
  virtual ~MapArchiveFile();

  // Inserts this entry to the list in sorted order
  static void updateFiles(std::list<MapArchiveFile> *files, MapArchiveFile file);

  // Getters and setters
  std::string getFilePath() const
  {
      return filePath;
  }

  Long getModificationTime() const
  {
      return modificationTime;
  }

  Long getDiskUsage() const
  {
      return diskUsage;
  }

};

}

#endif /* MAPARCHIVEFILE_H_ */
