//============================================================================
// Name        : Font.h
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



#ifndef FONT_H_
#define FONT_H_

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_STROKER_H

namespace GEODISCOVERER {

typedef unsigned long UTF32;  /* at least 32 bits */
typedef unsigned short UTF16;  /* at least 16 bits */
typedef unsigned char UTF8;  /* typically 8 bits */

/* Some fundamental constants */
#define UNI_REPLACEMENT_CHAR (UTF32)0x0000FFFD
#define UNI_MAX_BMP (UTF32)0x0000FFFF
#define UNI_MAX_UTF16 (UTF32)0x0010FFFF
#define UNI_MAX_UTF32 (UTF32)0x7FFFFFFF
#define UNI_MAX_LEGAL_UTF32 (UTF32)0x0010FFFF

typedef enum {
  conversionOK,     /* conversion successful */
  sourceExhausted,  /* partial character in source, but hit end */
  targetExhausted,  /* insuff. room in target for conversion */
  sourceIllegal  /* source sequence is illegal/malformed */
} ConversionResult;

typedef enum {
  strictConversion = 0,
  lenientConversion
} ConversionFlags;

#define UNI_SUR_HIGH_START  (UTF32)0xD800
#define UNI_SUR_LOW_END     (UTF32)0xDFFF

// Types for the character map
typedef std::map<UTF32, FontCharacter*> FontCharacterMap;
typedef std::pair<UTF32, FontCharacter*> FontCharacterPair;
typedef std::map<std::string, FontString*> FontStringMap;
typedef std::pair<std::string, FontString*> FontStringPair;

class Font {

protected:

  FontEngine *fontEngine;                         // Font engine this font belongs to
  FT_Library freeTypeLib;                         // Pointer to the library
  FT_Face face;                                   // Holds the font data
  Int size;                                       // Size of the font
  Int height;                                     // Height of the font

  // Map of all cached characters
  FontCharacterMap characterMap;

  // Map of all strings that are in use
  FontStringMap usedStringMap;

  // Map of all strings that are not in use anymore
  FontStringMap cachedStringMap;

  // Unused textures
  std::list<GraphicTextureInfo> unusedTextures;

  // Checks if the source is a legal UTF8 sequence
  bool isLegalUTF8(const UTF8 *source, int length);

  // Converts a UTF8 sequence to a UTF32 sequence
  ConversionResult convertUTF8toUTF32 (const UTF8** sourceStart, const UTF8* sourceEnd,
  UTF32** targetStart, UTF32* targetEnd, ConversionFlags flags);

  // Copies the characters to the bitmap
  void copyCharacters(FontString *fontString, std::list<FontCharacterPosition> *drawingList, bool useStrokeBitmap, UShort *textureBitmap, Int textureWidth, Int textureHeight, Int top, Int left, Int width, Int maxWidth, Int height, Int fadeOutOffset);

public:

  // Constructor
  Font(FontEngine *fontEngine, FT_Library freeTypeLib, std::string filename, Int size);

  // Destructor
  virtual ~Font();

  // Creates the graphic of the font
  void createGraphic();

  // Destroy the graphic of the font
  void destroyGraphic();

  // Deinits the font
  void deinit();

  // Creates a string
  FontString *createString(std::string contents, Int widthLimit=-1, Int keepEndWidth=-1);

  // Creates the bitmap for the font string
  void createStringBitmap(FontString *fontString);

  // Frees a string
  void destroyString(FontString *fontString);

  // Sets a new texture
  void setTexture(FontString *fontString);

  // Getters and setters
  Int getHeight() const
  {
      return height;
  }

};

}

#endif /* FONT_H_ */
