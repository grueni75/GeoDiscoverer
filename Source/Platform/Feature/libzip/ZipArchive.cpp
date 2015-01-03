//============================================================================
// Name        : ZipArchive.cpp
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
}

// Initializes the object
bool ZipArchive::init() {
  struct stat stat_buffer;
  std::string archiveFilePath = archiveFolder + "/" + archiveName;
  if (stat(archiveFilePath.c_str(),&stat_buffer)==0) {
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

// Adds an entry
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
  if (!(out=fopen(diskFilename.c_str(),"w")))
    return false;
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
  else
    return false;
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
  if (stat((archiveFolder + "/" + archiveName).c_str(),&buffer)==0) {
    return buffer.st_size;
  } else {
    return 0;
  }
}

} /* namespace GEODISCOVERER */
