//============================================================================
// Name        : Storage.h
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


#ifndef BINARYSTORAGE_H_
#define BINARYSTORAGE_H_

namespace GEODISCOVERER {

class Storage {
public:
  Storage();
  virtual ~Storage();

  // Writes an integer into file
  static void storeInt(std::ofstream *ofs, Int value);

  // Read integer from file
  static void retrieveInt(char *&cacheData, Int &cacheSize, Int &value);

  // Writes a short into file
  static void storeShort(std::ofstream *ofs, Short value);

  // Read short from file
  static void retrieveShort(char *&cacheData, Int &cacheSize, Short &value);

  // Writes a byte into file
  static void storeByte(std::ofstream *ofs, Byte value);

  // Read byte from file
  static void retrieveByte(char *&cacheData, Int &cacheSize, Byte &value);

  // Write string into file
  static void storeString(std::ofstream *ofs, std::string string);

  // Write string into file
  static void storeString(std::ofstream *ofs, char *string);

  // Read string from file
  static void retrieveString(char *&cacheData, Int &cacheSize, char **string);

  // Write an integer array into file
  static void storeVectorOfInt(std::ofstream *ofs, std::vector<Int> vector);

  // Read an integer array from file
  static void retrieveVectorOfInt(char *&cacheData, Int &cacheSize, std::vector<Int> &vector);

  // Writes a double into file
  static void storeDouble(std::ofstream *ofs, double value);

  // Read a double from file
  static void retrieveDouble(char *&cacheData, Int &cacheSize, double &value);

  // Writes a bool into file
  static void storeBool(std::ofstream *ofs, bool value);

  // Read a bool from file
  static void retrieveBool(char *&cacheData, Int &cacheSize, bool &value);

  // Writes a piece of memory into file
  static void storeMem(std::ofstream *ofs, char *mem, Int size, bool wordAligned);

  // Skips any padding added by storeMem in the given pointer
  static void skipPadding(char *& mem, Int & memSize);

  // Writes a color into file
  static void storeGraphicColor(std::ofstream *ofs, GraphicColor color);

  // Read a color from file
  static void retrieveGraphicColor(char *&cacheData, Int &cacheSize, GraphicColor &color);

  // Computes a hash of a file
  static std::string computeMD5(std::string filepath);

  // Aligns the output to the given size
  static void storeAlignment(std::ofstream *ofs, Int alignment);

  // Aligns the input to the given size
  static void retrieveAlignment(char *&cacheData, Int &cacheSize, Int alignment);

};

}

#endif /* BINARYSTORAGE_H_ */
