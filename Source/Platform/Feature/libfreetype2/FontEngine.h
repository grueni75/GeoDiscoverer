//============================================================================
// Name        : FontEngine.h
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

#include <Font.h>
#include <FontString.h>

#ifndef FONTENGINE_H_
#define FONTENGINE_H_

#include <ft2build.h>
#include FT_FREETYPE_H

namespace GEODISCOVERER {

// Types for the font map
typedef std::map<std::string, Font*> FontTypeMap;
typedef std::pair<std::string, Font*> FontTypePair;

class FontEngine {

protected:

  FT_Library freeTypeLib;     // Pointer to the library
  Int DPI;                    // DPI of the screen this font engine renders for
  FontTypeMap fontTypeMap;    // Holds all available fonts
  Font *currentFont;          // Font that is currently used for drawing
  Int backgroundStrokeWidth;  // Width of the stroke (for 12 pt font) behind the font letters for better contrast on black background
  Int stringCacheSize;        // Maximum size of the cache string map
  Int fadeOutOffset;          // Distance to the right border when to start fading out the character
  ThreadMutexInfo *accessMutex; // Mutex to access the font engine
  Screen *screen;             // The screen this engine renders for

  // Loads a font
  bool loadFont(std::string fontType, std::string fontFilename, Int fontSize);

  // Looks up a font from a type name
  Font *findFont(std::string fontType);

public:

  // Constructor
  FontEngine(Screen *screen);

  // Destructor
  virtual ~FontEngine();

  // Inits the engine
  void init();

  // Creates the graphic of the fonts
  void createGraphic();

  // Clears the graphic of the fonts
  void destroyGraphic();

  // Deinits the engine
  void deinit();

  // Sets the current font to use
  void lockFont(std::string type, const char *file, int line);

  // Unlocks the current font
  void unlockFont() const {
    core->getThread()->unlockMutex(accessMutex);
  }

  // Returns the height of the current font
  Int getFontHeight();

  // Returns the line height of the current font
  Int getLineHeight();

  // Creates a text with the current font type
  FontString *createString(std::string contents, Int widthLimit=-1, Int keepEndCharCount=-1);

  // Creates or updates a text with the current font type
  void updateString(FontString **fontString, std::string contents, Int widthLimit=-1, Int keepEndCharCount=-1);

  // Destroys a drawn text of the current font type
  void destroyString(FontString *fontString);

  // Getters and setters
  Int getBackgroundStrokeWidth() const
  {
      return backgroundStrokeWidth;
  }

  Int getStringCacheSize() const
  {
      return stringCacheSize;
  }

  Int getFadeOutOffset() const {
    return fadeOutOffset;
  }

  Int getDPI() const {
    return DPI;
  }

  Screen* getScreen() {
    return screen;
  }
};

}

#endif /* FONTENGINE_H_ */
