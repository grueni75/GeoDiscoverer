//============================================================================
// Name        : BinaryStorage.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
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

  // Writes a timestamp into file
  static void storeTimestampInMilliseconds(std::ofstream *ofs, TimestampInMilliseconds value);

  // Read a timestamp from file
  static void retrieveTimestampInMilliseconds(char *&cacheData, Int &cacheSize, TimestampInMilliseconds &value);

  // Writes a piece of memory into file
  static void storeMem(std::ofstream *ofs, char *mem, Int size, bool wordAligned);

  // Skips any padding added by storeMem in the given pointer
  static void skipPadding(char *& mem, Int & memSize);

};

}

#endif /* BINARYSTORAGE_H_ */
