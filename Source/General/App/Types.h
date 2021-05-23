//============================================================================
// Name        : Types.h
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



#ifndef TYPES_H_
#define TYPES_H_

namespace GEODISCOVERER {

// Simple types
typedef char Byte;
typedef unsigned char UByte;
typedef int Int;
typedef unsigned int UInt;
typedef short Short;
typedef unsigned short UShort;
typedef long Long;
typedef unsigned long ULong;
typedef float Float;

// Direction enum for searching
typedef enum { north, south, west, east } Direction;

// Piece of a memory of a given size
struct Memory {
  UByte *data;
  UInt pos;
  UInt size;
};

}

#endif /* TYPES_H_ */
