//============================================================================
// Name        : FontInfo.cpp
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

// For the UTF conversion data types, constants and methods only:
/*
 * Copyright 2001-$now Unicode, Inc.
 *
 * Disclaimer
 *
 * This source code is provided as is by Unicode, Inc. No claims are
 * made as to fitness for any particular purpose. No warranties of any
 * kind are expressed or implied. The recipient agrees to determine
 * applicability of information provided. If this file has been
 * purchased on magnetic or optical media from Unicode, Inc., the
 * sole remedy for any claim will be exchange of defective media
 * within 90 days of receipt.
 *
 * Limitations on Rights to Redistribute This Code
 *
 * Unicode, Inc. hereby grants the right to freely use the information
 * supplied in this file in the creation of products supporting the
 * Unicode Standard, and to make copies of this file in any form
 * for internal or external distribution as long as this notice
 * remains attached.
 */


#include <Core.h>

namespace GEODISCOVERER {

// Constructor
Font::Font(FT_Library freeTypeLib, std::string filename, Int size) {

  // Set variables
  this->freeTypeLib=freeTypeLib;

  // Load the font
  FT_Error error=FT_New_Face( freeTypeLib, filename.c_str(), 0, &face );
  if (error) {
    FATAL("can not load font <%s> (error code %d)",filename.c_str(),error);
    return;
  }

  // Set the size of the font
  this->size=size;
  double multiplier=(double)core->getScreen()->getDPI()/120.0;
  height=multiplier*size;
  if (FT_Set_Pixel_Sizes(face,0,height)) {
    FATAL("can not set font size",NULL);
    FT_Done_Face(face);
    return;
  }

}

// Destructor
Font::~Font() {

  // Clean up
  deinit();

  // Release the characters
  FontCharacterMap::iterator i;
  for(i = characterMap.begin(); i!=characterMap.end(); i++) {
    delete i->second;
  }
  characterMap.clear();

  // Free the face
  FT_Done_Face(face);

}

// Cleans up the engine
void Font::deinit() {

  destroyGraphic();

  // Release the strings
  FontStringMap::iterator j;
  for(j = usedStringMap.begin(); j!=usedStringMap.end(); j++) {
    delete j->second;
  }
  usedStringMap.clear();
  for(j = cachedStringMap.begin(); j!=cachedStringMap.end(); j++) {
    delete j->second;
  }
  cachedStringMap.clear();

}

// Destroys the graphic of the font
void Font::destroyGraphic() {
  FontStringMap::iterator j;
  for(j = usedStringMap.begin(); j!=usedStringMap.end(); j++) {
    j->second->deinit();
  }
  for(j = cachedStringMap.begin(); j!=cachedStringMap.end(); j++) {
    j->second->deinit();
  }

  // Release all textures
  for(std::list<GraphicTextureInfo>::iterator i=unusedTextures.begin();i!=unusedTextures.end();i++) {
    core->getScreen()->destroyTextureInfo(*i,"Font");
  }
  unusedTextures.clear();
}

// Recreates the graphic of the font
void Font::createGraphic() {
}

// Sets a new texture
void Font::setTexture(FontString *fontString) {

  // Get the texture from the unused texture list
  // If the unused texture list is empty, create a new texture
  if (fontString->getTexture()==core->getScreen()->getTextureNotDefined()) {
    if (unusedTextures.size()>0) {
      fontString->setTexture(unusedTextures.front());
      unusedTextures.pop_front();
    } else {
      fontString->setTexture(core->getScreen()->createTextureInfo());
    }
  }

}

// Creates the bitmap for the font string
void Font::createStringBitmap(FontString *fontString) {

  UTF32 *UTF32Text=NULL,*UTF32TextCopy;
  std::string text=fontString->getContents();
  const UTF8 *UTF8Text=(UTF8*)text.c_str();
  ConversionResult result;
  Int penX=0;
  Int penY=0;
  UShort *characterBitmap=NULL;
  FT_UInt glyphIndex,previousGlyphIndex;
  FT_Stroker stroker;
  FT_Glyph strokeGlyph=NULL;
  FT_Glyph normalGlyph=NULL;
  std::list<FontCharacterPosition> drawingList;
  UShort *textureBitmap=NULL;
  Int top,left,bottom,right,width,height;
  Int textureWidth,textureHeight;
  Int fadeOutOffset=core->getFontEngine()->getFadeOutOffset();
	
  // Reset variables
  top=std::numeric_limits<Int>::min();
  left=std::numeric_limits<Int>::max();
  bottom=std::numeric_limits<Int>::max();
  right=std::numeric_limits<Int>::min();
  width=0;
  height=0;

  // Convert the UTF8 string to UTF32
  if (!(UTF32Text=(UTF32*)malloc(sizeof(UTF32)*(text.size()+1)))) {
    FATAL("no memory for UTF8 to UTF32 conversion",NULL);
    goto cleanup;
  }
  UTF32TextCopy=UTF32Text;
  result=convertUTF8toUTF32(&UTF8Text,&UTF8Text[text.size()+1],
  &UTF32TextCopy,&UTF32TextCopy[text.size()+1],strictConversion);
  if (result!=conversionOK) {
    FATAL("could not convert UTF8 text <%s> to UTF32",UTF8Text);
    goto cleanup;
  }

  // Create the drawing list inclusive updating the character cache
  for(Int i=0;UTF32Text[i]!=0;i++) {

    // Get the character
    UTF32 c=UTF32Text[i];
    glyphIndex = FT_Get_Char_Index( face, c );

    // Check if it is in the cache
    FontCharacter *fontCharacter;
    FontCharacterMap::iterator j;
    j=characterMap.find(c);
    if (j==characterMap.end()) {

      // Load the glyph
      if (FT_Load_Glyph(face,glyphIndex,FT_LOAD_NO_BITMAP) != 0) {
        FATAL("can not load glyph",NULL);
        goto cleanup;
      }

      // Obtain the glyph
      if (FT_Get_Glyph(face->glyph, &normalGlyph) != 0)
      {
        FATAL("can not obtain the normal glyph from the face",NULL);
        goto cleanup;
      }
      if (FT_Get_Glyph(face->glyph, &strokeGlyph) != 0)
      {
        FATAL("can not obtain the stroke glyph from the face",NULL);
        goto cleanup;
      }

      // Render the normal glyph
      if (FT_Glyph_To_Bitmap(&normalGlyph,FT_RENDER_MODE_LCD,NULL,1)!=0) {
        FATAL("can not render the glyph",NULL);
        goto cleanup;
      }

      // Add a border around the glyph
      FT_Stroker_New(freeTypeLib, &stroker);
      FT_Stroker_Set(stroker,
                     core->getFontEngine()->getBackgroundStrokeWidth() * 64 * size / 12,
                     FT_STROKER_LINECAP_ROUND,
                     FT_STROKER_LINEJOIN_ROUND,
                     0);
      FT_Glyph_Stroke(&strokeGlyph, stroker, 1);
      FT_Stroker_Done(stroker);

      // Render the stroke glyph
      if (FT_Glyph_To_Bitmap(&strokeGlyph,FT_RENDER_MODE_LCD,NULL,1)!=0) {
        FATAL("can not render the stroke glyph",NULL);
        goto cleanup;
      }

      // Do some sanity checks on the bitmap
      FT_BitmapGlyph strokeBGlyph=(FT_BitmapGlyph)strokeGlyph;
      FT_Bitmap *strokeBitmap=&strokeBGlyph->bitmap;
      if (strokeBitmap->pixel_mode!=FT_PIXEL_MODE_LCD) {
        FATAL("unknown pixel mode",NULL);
        goto cleanup;
      }
      Int strokeBitmapWidth=strokeBitmap->width/3;
      Int strokeBitmapHeight=strokeBitmap->rows;

      // Create a new font character description
      if (!(fontCharacter=new FontCharacter(strokeBitmapWidth,strokeBitmapHeight))) {
        FATAL("can not create font character object",NULL);
        goto cleanup;
      }
      FontCharacterPair p=FontCharacterPair(c,fontCharacter);
      characterMap.insert(p);

      // Copy character infos
      fontCharacter->setPenOffsetLeft(strokeBGlyph->left);
      fontCharacter->setPenOffsetTop(strokeBGlyph->top);
      fontCharacter->setPenAdvanceX(face->glyph->advance.x>>6);
      characterBitmap=fontCharacter->getBitmap();

      // Merge the bitmaps
      FT_BitmapGlyph normalBGlyph=(FT_BitmapGlyph)normalGlyph;
      FT_Bitmap *normalBitmap=&normalBGlyph->bitmap;
      Int normalBitmapWidth=normalBitmap->width/3;
      Int normalBitmapHeight=normalBitmap->rows;
      Int offsetX=(strokeBitmapWidth-normalBitmapWidth)/2;
      Int offsetY=(strokeBitmapHeight-normalBitmapHeight)/2;
      for (Int y=0;y<strokeBitmapHeight;y++) {
        for (Int x=0;x<strokeBitmapWidth;x++) {
          Int strokeRed=(strokeBitmap->buffer[y*strokeBitmap->pitch+3*x])>>4;
          Int strokeGreen=(strokeBitmap->buffer[y*strokeBitmap->pitch+3*x+1])>>4;
          Int strokeBlue=(strokeBitmap->buffer[y*strokeBitmap->pitch+3*x+2])>>4;
          Int strokeAlpha=(strokeRed+strokeGreen+strokeBlue)/3;
          Int red=strokeRed;
          Int green=strokeGreen;
          Int blue=strokeBlue;
          Int alpha=strokeAlpha;
          if ((x>=offsetX)&&(x-offsetX<normalBitmapWidth)&&(y>=offsetY)&&(y-offsetY<normalBitmapHeight)) {
            Int normalRed=(normalBitmap->buffer[(y-offsetY)*normalBitmap->pitch+3*(x-offsetX)])>>4;
            Int normalGreen=(normalBitmap->buffer[(y-offsetY)*normalBitmap->pitch+3*(x-offsetX)+1])>>4;
            Int normalBlue=(normalBitmap->buffer[(y-offsetY)*normalBitmap->pitch+3*(x-offsetX)+2])>>4;
            Int normalAlpha=(normalRed+normalGreen+normalBlue)/3;
            normalRed=15-normalRed;
            normalGreen=15-normalGreen;
            normalBlue=15-normalBlue;
            if (normalAlpha>0) {
              red=normalRed;
              green=normalGreen;
              blue=normalBlue;
              alpha=15;
            }
          }
          UShort value=red<<12|green<<8|blue<<4|alpha;
          characterBitmap[y*strokeBitmapWidth+x]=value;
        }
      }

      // Cleanup
      FT_Done_Glyph(normalGlyph);
      normalGlyph=NULL;
      FT_Done_Glyph(strokeGlyph);
      strokeGlyph=NULL;

    } else {

      // Get the cached character
      fontCharacter=j->second;

    }

    // Get the kerning info
    if (FT_HAS_KERNING(face)) {
      if ((glyphIndex)&&(i>0)) {
        FT_Vector delta;
        FT_Get_Kerning(face,previousGlyphIndex,glyphIndex,FT_KERNING_DEFAULT,&delta);
        penX+=delta.x>>6;
      }
      previousGlyphIndex=glyphIndex;
    }

    // Add the character to the drawing list
    Int x1=penX;
    Int y2=penY;
    x1=x1+fontCharacter->getPenOffsetLeft();
    y2=y2+fontCharacter->getPenOffsetTop();
    Int x2=x1+fontCharacter->getWidth();
    Int y1=y2-fontCharacter->getHeight();
    FontCharacterPosition pos;
    pos.setCharacter(fontCharacter);
    pos.setX(x1);
    pos.setY(y2);
    drawingList.push_back(pos);

    // Update the dimensions
    if (y2>top) {
      top=y2;
    }
    if (y1<bottom) {
      bottom=y1;
    }
    if (x1<left) {
      left=x1;
    }
    if (x2>right) {
      right=x2;
    }

    // Increase the pen
    penX+=fontCharacter->getPenAdvanceX();

    // Abort if the current width is above the limit
    width=right-left;
    if ((fontString->getWidthLimit()>-1)&&(width>=fontString->getWidthLimit())) {
      width=fontString->getWidthLimit();
      break;
    }

  }
  height=top-bottom;

  // Decide on the width and height to use in the texture
  //DEBUG("contents=%s left=%d top=%d bottom=%d width=%d height=%d",fontString->getContents().c_str(),left,top,bottom,width,height);
  textureWidth=1;
  while(textureWidth<width) {
    textureWidth<<=1;
  }
  textureHeight=1;
  while(textureHeight<height) {
    textureHeight<<=1;
  }

  // Reserve memory for the texture
  if (!(textureBitmap=(UShort*)malloc(sizeof(*textureBitmap)*textureWidth*textureHeight))) {
    FATAL("no memory for the texture bitmap",NULL);
    goto cleanup;
  }
  memset(textureBitmap,0,sizeof(*textureBitmap)*textureWidth*textureHeight);

  // Copy the characters to the bitmap
  for(std::list<FontCharacterPosition>::iterator i=drawingList.begin();i!=drawingList.end();i++) {
    FontCharacterPosition pos=*i;
    Int offsetX=pos.getX()-left;
    Int offsetY=top-pos.getY();
    offsetY+=(textureHeight-height);
    //DEBUG("textureWidth=%d textureHeight=%d width=%d height=%d offsetX=%d offsetY=%d characterWidth=%d characterHeight=%d",textureWidth,textureHeight,width,height,offsetX,offsetY,c->getWidth(),c->getHeight());
    FontCharacter *c=pos.getCharacter();
    UShort *characterBitmap=c->getBitmap();
    for(Int y=0;y<c->getHeight();y++) {
      for(Int x=0;x<c->getWidth();x++) {
        UShort value=characterBitmap[y*c->getWidth()+x];
        if ((value&0xF)!=0) {
          Int absX=offsetX+x;
          bool copyValue=true;
          if ((fontString->getWidthLimit()!=-1)&&(width==fontString->getWidthLimit())) {
            if (absX>=width)
              copyValue=false;
            else if (absX>=width-fadeOutOffset) {
              Int t=width-1-absX;
              Int alpha=(value&0xF)*t/fadeOutOffset;
              value=(value&0xFFF0)|alpha;
            }
          }
          if (copyValue)
            textureBitmap[(offsetY+y)*textureWidth+offsetX+x]=value;
        }
      }
    }
  }

  // Update the font string object (you also need to update the cache code in createString if you change this)
  fontString->setTextureBitmap(textureBitmap);
  textureBitmap=NULL;
  fontString->setIconWidth(width);
  fontString->setIconHeight(height);
  fontString->setWidth(textureWidth);
  fontString->setHeight(textureHeight);
  fontString->setBaselineOffsetY(bottom);

  // Cleanup
cleanup:
  if (normalGlyph) FT_Done_Glyph(normalGlyph);
  normalGlyph=NULL;
  if (strokeGlyph) FT_Done_Glyph(strokeGlyph);
  strokeGlyph=NULL;
  if (UTF32Text) free(UTF32Text);
  if (textureBitmap) free(textureBitmap);

}

// Creates a new string
FontString *Font::createString(std::string contents, Int widthLimit) {

  FontString *fontString;

  // First check if the string is available in the used string map or the cached string map
  FontStringMap::iterator k;
  std::stringstream key;
  key << contents  << "[" << widthLimit << "]";
  k=usedStringMap.find(key.str());
  if (k!=usedStringMap.end()) {

    // Increase the use count
    k->second->increaseUseCount();

    // Copy the contents from the used font string (you also need to update the cache code in createString if you change this)
    if (!(fontString=new FontString(this,k->second))) {
      FATAL("can not create font string object",NULL);
      return NULL;
    }
    fontString->setContents(contents);
    fontString->setIconWidth(k->second->getIconWidth());
    fontString->setIconHeight(k->second->getIconHeight());
    fontString->setWidth(k->second->getWidth());
    fontString->setHeight(k->second->getHeight());
    fontString->setBaselineOffsetY(k->second->getBaselineOffsetY());
    fontString->setWidthLimit(k->second->getWidthLimit());
    //DEBUG("using string from used cache",NULL);
    return fontString;

  }
  k=cachedStringMap.find(key.str());
  if (k!=cachedStringMap.end()) {

    // Add the string to the used string map before returning it
    fontString=k->second;
    fontString->increaseUseCount();
    cachedStringMap.erase(k);
    FontStringPair p=FontStringPair(key.str(),fontString);
    usedStringMap.insert(p);
    //DEBUG("using string from unused cache",NULL);
    return fontString;

  }

  // Create a new font string
  //DEBUG("creating new string",NULL);
  if (!(fontString=new FontString(this,NULL))) {
    FATAL("can not create font string object",NULL);
    return NULL;
  }
  fontString->setContents(contents);
  fontString->setWidthLimit(widthLimit);
  fontString->increaseUseCount();
  FontStringPair p=FontStringPair(key.str(),fontString);
  usedStringMap.insert(p);

  // Update the bitmap
  createStringBitmap(fontString);

  // Return the result
  return fontString;
}


// Destroys a string
void Font::destroyString(FontString *fontString) {

  // Delete the string from the used string map if its use count is 0
  FontStringMap::iterator k;
  std::stringstream key;
  key << fontString->getContents()  << "[" << fontString->getWidthLimit() << "]";
  k=usedStringMap.find(key.str());
  //DEBUG("usedStringMap.size=%d",usedStringMap.size());
  if (k!=usedStringMap.end()) {

    // Destroy the handed over string if it is not from the map
    if (k->second!=fontString) {
      fontString->setTexture(core->getScreen()->getTextureNotDefined());
      delete fontString;
    }
    fontString=k->second;

    // Remove the font string if its use count is zero
    fontString->decreaseUseCount();
    if (fontString->getUseCount()>0) {
      return;
    } else {
      usedStringMap.erase(k);
    }

  } else {
    FATAL("can not erase font string that is not in the used string map",NULL);
    return;
  }

  // Delete the oldest entry from the cache if it has reached its size
  if (cachedStringMap.size()>=core->getFontEngine()->getStringCacheSize()) {
    //DEBUG("reducing font cache",NULL);
    TimestampInSeconds oldestFontStringAccess=std::numeric_limits<TimestampInSeconds>::max();
    FontString *oldestFontString;
    for(FontStringMap::iterator i=cachedStringMap.begin(); i!=cachedStringMap.end(); i++) {
      if (i->second->getLastAccess()<oldestFontStringAccess) {
        oldestFontString=i->second;
        oldestFontStringAccess=oldestFontString->getLastAccess();
      }
    }
    std::stringstream key;
    key << oldestFontString->getContents()  << "[" << oldestFontString->getWidthLimit() << "]";
    if (cachedStringMap.erase(key.str())!=1) {
      FATAL("can not erase font string in cached string map",NULL);
      return;
    }
		if (oldestFontString->getTexture()!=core->getScreen()->getTextureNotDefined())
		  unusedTextures.push_back(oldestFontString->getTexture());
    oldestFontString->setTexture(core->getScreen()->getTextureNotDefined());
    delete oldestFontString;
  }

  // Add this one to the cache
  fontString->setLastAccess(core->getClock()->getSecondsSinceEpoch());
  FontStringPair p=FontStringPair(key.str(),fontString);
  cachedStringMap.insert(p);

}

/*
 * Magic values subtracted from a buffer value during UTF8 conversion.
 * This table contains as many values as there might be trailing bytes
 * in a UTF-8 sequence.
 */
static const UTF32 offsetsFromUTF8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL,
         0x03C82080UL, 0xFA082080UL, 0x82082080UL };

/*
 * Index into the table below with the first byte of a UTF-8 sequence to
 * get the number of trailing bytes that are supposed to follow it.
 * Note that *legal* UTF-8 values can't have 4 or 5-bytes. The table is
 * left as-is for anyone who may want to do such conversion, which was
 * allowed in earlier algorithms.
 */
static const char trailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

// Checks if the source is a legal UTF8 sequence
bool Font::isLegalUTF8(const UTF8 *source, int length) {
  UTF8 a;
  const UTF8 *srcptr = source+length;
  switch (length) {
    default: return false;
    /* Everything else falls through when "true"... */
    case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 2: if ((a = (*--srcptr)) > 0xBF) return false;
    switch (*source) {
      /* no fall-through in this inner switch */
      case 0xE0: if (a < 0xA0) return false; break;
      case 0xED: if (a > 0x9F) return false; break;
      case 0xF0: if (a < 0x90) return false; break;
      case 0xF4: if (a > 0x8F) return false; break;
      default:   if (a < 0x80) return false;
    }
    case 1: if (*source >= 0x80 && *source < 0xC2) return false;
  }
  if (*source > 0xF4) return false;
  return true;
}

// Converts a UTF8 sequence to a UTF32 sequence
ConversionResult Font::convertUTF8toUTF32 (const UTF8** sourceStart, const UTF8* sourceEnd,
UTF32** targetStart, UTF32* targetEnd, ConversionFlags flags) {

  ConversionResult result = conversionOK;
  const UTF8* source = *sourceStart;
  UTF32* target = *targetStart;

  while (source < sourceEnd) {
    UTF32 ch = 0;

    unsigned short extraBytesToRead = trailingBytesForUTF8[*source];
    if (source + extraBytesToRead >= sourceEnd) {
      result = sourceExhausted;
      break;
    }

    /* Do this check whether lenient or strict */
    if (! isLegalUTF8(source, extraBytesToRead+1)) {
      result = sourceIllegal;
      break;
    }

    /*
     * The cases all fall through. See "Note A" below.
     */
    switch (extraBytesToRead) {
      case 5: ch += *source++; ch <<= 6;
      case 4: ch += *source++; ch <<= 6;
      case 3: ch += *source++; ch <<= 6;
      case 2: ch += *source++; ch <<= 6;
      case 1: ch += *source++; ch <<= 6;
      case 0: ch += *source++;
    }
    ch -= offsetsFromUTF8[extraBytesToRead];

    if (target >= targetEnd) {
      source -= (extraBytesToRead+1); /* Back up the source pointer! */
      result = targetExhausted;
      break;
    }
    if (ch <= UNI_MAX_LEGAL_UTF32) {
      /*
       * UTF-16 surrogate values are illegal in UTF-32, and anything
       * over Plane 17 (> 0x10FFFF) is illegal.
       */
      if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END) {
        if (flags == strictConversion) {
          source -= (extraBytesToRead+1); /* return to the illegal value itself */
          result = sourceIllegal;
          break;
        } else {
          *target++ = UNI_REPLACEMENT_CHAR;
        }
      } else {
        *target++ = ch;
      }

    } else { /* i.e., ch > UNI_MAX_LEGAL_UTF32 */
      result = sourceIllegal;
      *target++ = UNI_REPLACEMENT_CHAR;
    }
  }
  *sourceStart = source;
  *targetStart = target;
  return result;
}


}
