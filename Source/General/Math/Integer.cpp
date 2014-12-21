//============================================================================
// Name        : Integer.cpp
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
Integer::Integer() {
}

// Destructor
Integer::~Integer() {
}

// Executes c=a-b and checks for overflow
bool Integer::add(Int a, Int b, Int &c) {

  long long t=(long long)a+(long long)b;
  if ((t<std::numeric_limits<Int>::min())||(t>std::numeric_limits<Int>::max())) {
    c=0;
    return false;
  } else {
    c=(Int)t;
    return true;
  }

}

}
