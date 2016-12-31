//============================================================================
// Name        : GraphicColor.cpp
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
