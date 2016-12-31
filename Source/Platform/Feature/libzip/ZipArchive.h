//============================================================================
// Name        : ZipArchive.h
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

#ifndef ZIPARCHIVE_H_
#define ZIPARCHIVE_H_

#include <zip.h>

namespace GEODISCOVERER {

typedef zip_file* ZipArchiveEntry;

class ZipArchive {

  struct zip *archive;        // Handle to the archive
  std::string archiveFolder;  // Folder where the archive is stored in
  std::string archiveName;    // Name of the archive within the folder
  Int last_error;             // Last error number
  std::list<void*> buffers;   // List of buffers that need to be freed

public:

  // Constructor
  ZipArchive(std::string archiveFolder, std::string archiveName);

  // Destructor
  virtual ~ZipArchive();

  // Inits the object
  bool init();

  // Returns the number of entries in the zip file
  Int getEntryCount();

  // Return the filename of the entry at the given position
  std::string getEntryFilename(Int index);

  // Return the real size of the entry with the given filename
  Int getEntrySize(std::string filename);

  // Opens an entry
  ZipArchiveEntry openEntry(std::string filename);

  // Reads from the entry
  Int readEntry(ZipArchiveEntry entry, void *buffer, Int size);

  // Close the entry
  void closeEntry(ZipArchiveEntry entry);

  // Adds an entry
  bool addEntry(std::string filename, void *buffer, Int size);

  // Writes the entry into a file
  bool exportEntry(std::string entryFilename, std::string diskFilename);

  // Writes any changes within the archive to disk
  bool writeChanges();

  // Returns the size of the archive on disk
  Int getUnchangedSize();

  // Removes the entry from the zip archive
  void removeEntry(std::string filename);

  // Getters and setters
  const std::string& getArchiveFolder() const {
    return archiveFolder;
  }

  const std::string& getArchiveName() const {
    return archiveName;
  }
};

} /* namespace GEODISCOVERER */
#endif /* ZIPARCHIVE_H_ */
