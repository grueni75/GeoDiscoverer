//============================================================================
// Name        : GraphicColor.cpp
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

// Constructors
GraphicColor::GraphicColor() {
  setRed(0);
  setGreen(0);
  setBlue(0);
  setAlpha(255);
}
GraphicColor::GraphicColor(UByte red,UByte green,UByte blue,UByte alpha) {
  setRed(red);
  setGreen(green);
  setBlue(blue);
  setAlpha(alpha);
}

// Operators
bool GraphicColor::operator==(const GraphicColor &rhs)
{
  if ((red==rhs.red)&&(green==rhs.green)&&(blue==rhs.blue)&&(alpha==rhs.alpha))
    return true;
  else
    return false;
}

// Destructor
GraphicColor::~GraphicColor() {
}

}
