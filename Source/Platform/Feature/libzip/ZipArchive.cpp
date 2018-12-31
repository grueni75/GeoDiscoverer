//============================================================================
// Name        : ZipArchive.cpp
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

#include <Core.h>

namespace GEODISCOVERER {

// Constructor
ZipArchive::ZipArchive(std::string archiveFolder, std::string archiveName) {
  archive=NULL;
  this->archiveFolder=archiveFolder;
  this->archiveName=archiveName;
  this->last_error=0;
}

// Destructor
ZipArchive::~ZipArchive() {
  if (archive)
    zip_close(archive);
  archive=NULL;
}

// Initializes the object
bool ZipArchive::init() {
  struct stat stat_buffer;
  std::string archiveFilePath = archiveFolder + "/" + archiveName;
  if (core->statFile(archiveFilePath,&stat_buffer)==0) {
    archive = zip_open(archiveFilePath.c_str(),0,&last_error);
  } else {
    archive = zip_open(archiveFilePath.c_str(),ZIP_CREATE,&last_error);
  }
  if (archive==NULL) {
    DEBUG("zip_open failed with error=%d",last_error);
    return false;
  } else {
    return true;
  }
}

// Returns the number of entries in the zip file
Int ZipArchive::getEntryCount() {
  return zip_get_num_entries(archive,0);
}

// Return the filename of the entry at the given position
std::string ZipArchive::getEntryFilename(Int index) {
  struct zip_stat stat;
  if (zip_stat_index(archive,index,0,&stat)==0) {
    return std::string(stat.name);
  } else {
    return "";
  }
}

// Return the real size of the entry with the given filename
Int ZipArchive::getEntrySize(std::string filename) {
  struct zip_stat stat;
  if (zip_stat(archive,filename.c_str(),0,&stat)==0) {
    return stat.size;
  } else {
    return 0;
  }
}

// Removes the entry from the zip archive
void ZipArchive::removeEntry(std::string filename) {
  struct zip_stat stat;
  if (zip_stat(archive,filename.c_str(),0,&stat)==0) {
    zip_delete(archive,stat.index);
  }
}

// Opens an entry
ZipArchiveEntry ZipArchive::openEntry(std::string filename) {
  return zip_fopen(archive,filename.c_str(),0);
}

// Reads from the entry
Int ZipArchive::readEntry(ZipArchiveEntry entry, void *buffer, Int size) {
  return zip_fread(entry,buffer,size);
}

// Close the entry
void ZipArchive::closeEntry(ZipArchiveEntry entry) {
  zip_fclose(entry);
}

// Adds an entry from a file on the disk
bool ZipArchive::addEntry(std::string entryFilename, std::string diskFilename) {
  FILE *in;
  if (!(in=fopen(diskFilename.c_str(),"r"))) {
    DEBUG("can not open <%s> for reading", diskFilename.c_str());
    return false;
  }
  fseek(in, 0, SEEK_END);
  long size = ftell(in);
  fseek(in, 0, SEEK_SET);
  void *buffer;
  if (!(buffer=malloc(size))) {
    FATAL("can not create buffer of length %l",size);
    return false;
  }
  fread(buffer,size,1,in);
  fclose(in);
  //DEBUG("buffer=0x%08x size=%d",buffer,size);
  bool result = addEntry(entryFilename,buffer,size);
  return result;
}

// Adds an entry from a buffer
bool ZipArchive::addEntry(std::string filename, void *buffer, Int size) {

  // Compress the data
  struct zip_source *source;
  source=zip_source_buffer(archive,buffer,size,0);
  if (source==NULL) {
    free(buffer);
    return false;
  }

  // Add the data to the archive
  Int result=zip_add(archive,filename.c_str(),source);
  if (result<0) {
    DEBUG("zip_add result = %d",result);
    zip_source_free(source);
    free(buffer);
    return false;
  }

  // Remember that we need to free this buffer
  buffers.push_back(buffer);
  return true;
}

// Writes the entry into a file
bool ZipArchive::exportEntry(std::string entryFilename, std::string diskFilename) {
  const Int buffer_size=16384;
  UByte buffer[buffer_size];
  Int read_size;
  FILE *out;
  if (!(out=fopen(diskFilename.c_str(),"w"))) {
    DEBUG("can not open <%s> for writing",diskFilename.c_str());
    return false;
  }
  ZipArchiveEntry entry = openEntry(entryFilename);
  if (entry) {
    while ((read_size=readEntry(entry,buffer,buffer_size))>0) {
      fwrite(buffer,read_size,1,out);
    }
    closeEntry(entry);
  }
  fclose(out);
  if (entry)
    return true;
  else {
    DEBUG("can not find entry <%s> in archive <%s/%s>",entryFilename.c_str(),archiveFolder.c_str(),archiveName.c_str());
    return false;
  }
}

// Checks if the complete archive can be read
bool ZipArchive::checkIntegrity() {
  const Int buffer_size=16384;
  UByte buffer[buffer_size];
  Int read_size;
  zip_error_clear(archive);
  for (int i=0;i<getEntryCount();i++) {
    ZipArchiveEntry entry = openEntry(getEntryFilename(i));
    if (entry) {
      zip_file_error_clear(entry);
      while ((read_size=readEntry(entry,buffer,buffer_size))>0);
      int ze, se;
      zip_file_error_get(entry,&ze,&se);
      closeEntry(entry);
      if ((ze!=0)||(se!=0))
        return false;
    } else {
      return false;
    }
  }
  int ze, se;
  zip_error_get(archive,&ze,&se);
  if ((ze!=0)||(se!=0))
    return false;
  return true;
}

// Writes any changes within the archive to disk
bool ZipArchive::writeChanges() {
  Int result=zip_close(archive);
  for (std::list<void*>::iterator i=buffers.begin();i!=buffers.end();i++)
    free(*i);
  buffers.clear();
  init();
  if (result!=0)
    return false;
  else
    return true;
}

// Returns the size of the archive on disk
Int ZipArchive::getUnchangedSize() {
  struct stat buffer;
  if (core->statFile((archiveFolder + "/" + archiveName),&buffer)==0) {
    return buffer.st_size;
  } else {
    return 0;
  }
}

} /* namespace GEODISCOVERER */
