//============================================================================
// Name        : GraphicPoint.cpp
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

GraphicPoint::GraphicPoint() {
  x=0;
  y=0;
}

GraphicPoint::GraphicPoint(Short x, Short y) {
  this->x=x;
  this->y=y;
}

GraphicPoint::~GraphicPoint() {
  // TODO Auto-generated destructor stub
}

} /* namespace GEODISCOVERER */
