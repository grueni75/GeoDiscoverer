//============================================================================
// Name        : BinaryStorage.cpp
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

Storage::Storage() {
}

Storage::~Storage() {
}

// Write string into file
void Storage::storeString(std::ofstream *ofs, std::string string) {
  ofs->write((char*)string.c_str(),string.size()+1);
}

// Read string from file
void Storage::retrieveString(char *&cacheData, Int &cacheSize, std::string &string) {
  string=std::string(cacheData);
  cacheSize-=string.size()+1;
  cacheData+=string.size()+1;
}


// Writes an integer into file
void Storage::storeInt(std::ofstream *ofs, Int value) {
  ofs->write((char*)&value,sizeof(value));
}

// Read integer from file
void Storage::retrieveInt(char *&cacheData, Int &cacheSize, Int &value) {
  if (cacheSize>=sizeof(value)) {
    value=*((Int *)cacheData);
    cacheSize-=sizeof(value);
    cacheData+=sizeof(value);
  } else {
    value=0;
    cacheSize=-1;
  }
}

// Write an integer array into file
void Storage::storeVectorOfInt(std::ofstream *ofs, std::vector<Int> vector) {
  Int size=vector.size();
  ofs->write((char*)&size,sizeof(size));
  for(Int i=0;i<size;i++) {
    Int value=vector[i];
    ofs->write((char*)&value,sizeof(value));
  }
}

// Read an integer array from file
void Storage::retrieveVectorOfInt(char *&cacheData, Int &cacheSize, std::vector<Int> &vector) {
  Int size;
  Int value;
  retrieveInt(cacheData,cacheSize,size);
  vector.clear();
  vector.resize(size);
  for(Int i=0;i<size;i++) {
    retrieveInt(cacheData,cacheSize,value);
    vector[i]=value;
  }
}

// Writes a double into file
void Storage::storeDouble(std::ofstream *ofs, double value) {
  ofs->write((char*)&value,sizeof(value));
}

// Read a double from file
void Storage::retrieveDouble(char *&cacheData, Int &cacheSize, double &value) {
  if (cacheSize>=sizeof(value)) {
    value=*((double *)cacheData);
    cacheSize-=sizeof(value);
    cacheData+=sizeof(value);
  } else {
    value=0;
    cacheSize=-1;
  }
}

// Writes a bool into file
void Storage::storeBool(std::ofstream *ofs, bool value) {
  ofs->write((char*)&value,sizeof(value));
}

// Read a bool from file
void Storage::retrieveBool(char *&cacheData, Int &cacheSize, bool &value) {
  if (cacheSize>=sizeof(value)) {
    value=*((bool *)cacheData);
    cacheSize-=sizeof(value);
    cacheData+=sizeof(value);
  } else {
    value=0;
    cacheSize=-1;
  }
}

// Writes a timestamp into file
void Storage::storeTimestampInMilliseconds(std::ofstream *ofs, TimestampInMilliseconds value) {
  ofs->write((char*)&value,sizeof(value));
}

// Read a timestamp from file
void Storage::retrieveTimestampInMilliseconds(char *&cacheData, Int &cacheSize, TimestampInMilliseconds &value) {
  if (cacheSize>=sizeof(value)) {
    value=*((TimestampInMilliseconds *)cacheData);
    cacheSize-=sizeof(value);
    cacheData+=sizeof(value);
  } else {
    value=0;
    cacheSize=-1;
  }
}


}
