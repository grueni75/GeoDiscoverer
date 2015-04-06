//============================================================================
// Name        : FontString.cpp
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
  textureBitmap=NULL;
}

// Destructor
FontString::~FontString() {
  if (textureBitmap) free(textureBitmap);
}

// Called when the font must be drawn
void FontString::draw(TimestampInMicroseconds t) {

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

  // Call parent function
  GraphicRectangle::draw(t);

}

}
