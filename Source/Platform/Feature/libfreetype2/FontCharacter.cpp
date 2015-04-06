//============================================================================
// Name        : FontCharacter.cpp
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
