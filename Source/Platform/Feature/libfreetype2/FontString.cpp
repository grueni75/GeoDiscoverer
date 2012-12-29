//============================================================================
// Name        : FontString.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
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
FontString::FontString(Font *font, FontString *fontStringReference) : GraphicRectangle() {
  this->font=font;
  this->fontStringReference=fontStringReference;
  lastAccess=0;
  color.setRed(255);
  color.setGreen(255);
  color.setBlue(255);
  color.setAlpha(255);
  useCount=0;
  widthLimit=-1;
  textureBitmap=NULL;
}

// Destructor
FontString::~FontString() {
}

// Called when the font must be drawn
void FontString::draw(Screen *screen, TimestampInMicroseconds t) {

  // Update the texture
  FontString *s=this;
  if (fontStringReference)
    s=fontStringReference;
  if (s->getTexture()==core->getScreen()->getTextureNotDefined()) {
    font->setTexture(s);
  }
  if (s->textureBitmap) {
    screen->setTextureImage(s->texture,s->textureBitmap,s->width,s->height,graphicTextureFormatRGBA4);
    s->textureBitmap=NULL;
  }
  if (getTexture()==core->getScreen()->getTextureNotDefined()) {
    setTexture(s->getTexture());
  }

  // Call parent function
  GraphicRectangle::draw(screen,t);

}

}
