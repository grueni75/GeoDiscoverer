//============================================================================
// Name        : FontCharacter.cpp
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
FontCharacter::FontCharacter(Int width, Int height) {
  this->width=width;
  this->height=height;
  if (!(normalBitmap=(UShort*)malloc(sizeof(*normalBitmap)*width*height))) {
    FATAL("no memory for the bitmap",NULL);
  }
  if (!(strokeBitmap=(UShort*)malloc(sizeof(*strokeBitmap)*width*height))) {
    FATAL("no memory for the bitmap",NULL);
  }
}

// Destructor
FontCharacter::~FontCharacter() {
  free(normalBitmap);
  free(strokeBitmap);
}

}
