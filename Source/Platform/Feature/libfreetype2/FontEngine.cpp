//============================================================================
// Name        : Font.cpp
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
FontEngine::FontEngine() {

  // Init the free type 2 library
  if (FT_Init_FreeType(&freeTypeLib)) {
    FATAL("can not init free type 2 library",NULL);
    return;
  }

  // Get config parameters
  backgroundStrokeWidth=core->getConfigStore()->getIntValue("Graphic/Font","backgroundStrokeWidth", __FILE__, __LINE__);
  stringCacheSize=core->getConfigStore()->getIntValue("Graphic/Font","stringCacheSize", __FILE__, __LINE__);
  fadeOutOffset=core->getConfigStore()->getIntValue("Graphic/Font","fadeOutOffset", __FILE__, __LINE__);

  // Init the mutex
  accessMutex=core->getThread()->createMutex("font engine access mutex");

  // Load the supported fonts
  std::string fontBaseDir = core->getHomePath() + "/Font/";
  std::string sansFontFilename = fontBaseDir + core->getConfigStore()->getStringValue("Graphic/Font","sansFilename", __FILE__, __LINE__);
  std::string sansBoldFontFilename = fontBaseDir + core->getConfigStore()->getStringValue("Graphic/Font","sansBoldFilename", __FILE__, __LINE__);
  Int sansLargeSize = core->getConfigStore()->getIntValue("Graphic/Font","sansLargeSize", __FILE__, __LINE__);
  if (!loadFont("sansLarge",sansFontFilename,sansLargeSize))
    return;
  if (!loadFont("sansBoldLarge",sansBoldFontFilename,sansLargeSize))
    return;
  Int sansNormalSize = core->getConfigStore()->getIntValue("Graphic/Font","sansNormalSize", __FILE__, __LINE__);
  if (!loadFont("sansNormal",sansFontFilename,sansNormalSize))
    return;
  if (!loadFont("sansBoldNormal",sansBoldFontFilename,sansNormalSize))
    return;
  Int sansSmallSize = core->getConfigStore()->getIntValue("Graphic/Font","sansSmallSize", __FILE__, __LINE__);
  if (!loadFont("sansSmall",sansFontFilename,sansSmallSize))
    return;
  if (!loadFont("sansBoldSmall",sansBoldFontFilename,sansSmallSize))
    return;
  Int sansTinySize = core->getConfigStore()->getIntValue("Graphic/Font","sansTinySize", __FILE__, __LINE__);
  if (!loadFont("sansTiny",sansFontFilename,sansTinySize))
    return;
  if (!loadFont("sansBoldTiny",sansBoldFontFilename,sansTinySize))
    return;
}

// Loads a font
bool FontEngine::loadFont(std::string fontType, std::string fontFilename, Int fontSize) {

  // Create the font
  Font *font;
  if (!(font=new Font(freeTypeLib,fontFilename,fontSize)))
    return false;

  // And add it to the available fonts
  FontTypePair p=FontTypePair(fontType,font);
  fontTypeMap.insert(p);
  return true;
}

// Destructor
FontEngine::~FontEngine() {

  // Free all loaded fonts
  FontTypeMap::iterator i;
  for(i = fontTypeMap.begin(); i!=fontTypeMap.end(); i++) {
    std::string fontType;
    Font *font;
    fontType=i->first;
    font=i->second;
    delete font;
  }

  // Free the mutex
  core->getThread()->destroyMutex(accessMutex);

  // Close the library
  FT_Done_FreeType(freeTypeLib);
}

// Looks up a font from a type name
Font *FontEngine::findFont(std::string fontType) {
  Font *font;
  FontTypeMap::iterator i;
  i=fontTypeMap.find(fontType);
  if (i==fontTypeMap.end()) {
    FATAL("can not find font type <%s>",fontType.c_str());
    return NULL;
  }
  return i->second;
}

// Returns the height of the current font
Int FontEngine::getFontHeight() {
  return currentFont->getHeight();
}

// Returns the line height of the current font
Int FontEngine::getLineHeight() {
  return currentFont->getHeight()+currentFont->getHeight()/4;
}

// Creates a text with the current font type
FontString *FontEngine::createString(std::string contents, Int widthLimit) {
  return currentFont->createString(contents);
}

// Creates or updates a text with the current font type
void FontEngine::updateString(FontString **fontString, std::string contents, Int widthLimit) {
  if (*fontString) {
    if ((*fontString)->getContents()!=contents) {
      currentFont->destroyString(*fontString);
      *fontString=currentFont->createString(contents,widthLimit);
    }
  } else {
    *fontString=currentFont->createString(contents,widthLimit);
  }
}

// Destroys a drawn text of the current font type
void FontEngine::destroyString(FontString *fontString) {
  currentFont->destroyString(fontString);
}

// Sets the current font to use
void FontEngine::lockFont(std::string type, const char *file, int line) {

  // Lock access
  core->getThread()->lockMutex(accessMutex, file, line);

  // Lookup the font
  if (!(currentFont=findFont(type)))
    return;
}

// Cleans up the engine
void FontEngine::deinit() {
  FontTypeMap::iterator i;
  for(i = fontTypeMap.begin(); i!=fontTypeMap.end(); i++) {
    std::string fontType;
    Font *font;
    font=i->second;
    font->deinit();
  }
}

// Inits the engine
void FontEngine::init() {

}

// Clears the graphic of the font
void FontEngine::destroyGraphic() {
  FontTypeMap::iterator i;
  for(i = fontTypeMap.begin(); i!=fontTypeMap.end(); i++) {
    std::string fontType;
    Font *font;
    font=i->second;
    font->destroyGraphic();
  }
}

// Creates the graphic of the font
void FontEngine::createGraphic() {
  FontTypeMap::iterator i;
  for(i = fontTypeMap.begin(); i!=fontTypeMap.end(); i++) {
    std::string fontType;
    Font *font;
    font=i->second;
    font->createGraphic();
  }
}

}
