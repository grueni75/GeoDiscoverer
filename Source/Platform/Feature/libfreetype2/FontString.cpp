//============================================================================
// Name        : FontString.cpp
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
#include <FontString.h>
#include <Font.h>

namespace GEODISCOVERER {

// Constructor
FontString::FontString(Screen *screen, Font *font, FontString *fontStringReference) : GraphicRectangle(screen) {
  this->font=font;
  this->fontStringReference=fontStringReference;
  this->baselineOffsetY=0;
  lastAccess=0;
  color.setRed(255);
  color.setGreen(255);
  color.setBlue(255);
  color.setAlpha(255);
  useCount=0;
  widthLimit=-1;
  keepEndCharCount=-1;
  textureBitmap=NULL;
}

// Destructor
FontString::~FontString() {
  if (textureBitmap) free(textureBitmap);
}

// Called to update the texture of the font
void FontString::updateTexture() {

  // Update the texture
  FontString *s=this;
  if (fontStringReference)
    s=fontStringReference;
  if (s->getTexture()==Screen::getTextureNotDefined()) {
    font->setTexture(s);
    if (!(screen->setTextureImage(s->texture,(UByte*)s->textureBitmap,s->width,s->height,GraphicTextureFormatRGBA4444))) {
      FATAL("can not update texture image",NULL);
    }
  }
  if (getTexture()==Screen::getTextureNotDefined()) {
    setTexture(s->getTexture());
  }
}

// Called when the font must be drawn
void FontString::draw(TimestampInMicroseconds t) {

  // Update the texture
  updateTexture();

  // Call parent function
  GraphicRectangle::draw(t);

}

}
