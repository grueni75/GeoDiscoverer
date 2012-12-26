//============================================================================
// Name        : FontString.h
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


#ifndef FONTSTRING_H_
#define FONTSTRING_H_

namespace GEODISCOVERER {

class Font;

class FontString : public GraphicRectangle {

protected:

  Font *font;                      // Font used to create this string
  FontString *fontStringReference; // Reference to the font string that holds the graphic data
  TimestampInSeconds lastAccess;   // Timestamp of the last access to the string
  std::string contents;            // String to display
  Int widthLimit;                  // Maximum allowed width
  Int baselineOffsetY;             // Vertical offset to the baseline
  Int useCount;                    // Number of objects that use the string bitmap

public:

  // Constructor
  FontString(Font *font, FontString *fontStringRef);

  // Destructor
  virtual ~FontString();

  // Called when the widget must be drawn
  virtual void draw(Screen *screen, TimestampInMicroseconds t);

  // Getters and setters
  TimestampInSeconds getLastAccess() const
  {
      return lastAccess;
  }

  void setLastAccess(TimestampInSeconds lastAccess)
  {
      this->lastAccess = lastAccess;
  }

  std::string getContents() const
  {
      return contents;
  }

  void setContents(std::string contents)
  {
      this->contents = contents;
  }

  void setBaselineOffsetY(Int baselineOffsetY)
  {
      this->baselineOffsetY = baselineOffsetY;
  }

  virtual void setY(Int y)
  {
      this->y = y+baselineOffsetY;
  }

  void increaseUseCount() {
    useCount++;
  }

  void decreaseUseCount() {
    useCount--;
  }

  Int getUseCount() const
  {
      return useCount;
  }

  Int getBaselineOffsetY() const
  {
      return baselineOffsetY;
  }

  Int getWidthLimit() const {
    return widthLimit;
  }

  void setWidthLimit(Int widthLimit) {
    this->widthLimit = widthLimit;
  }

};

}

#endif /* FONTSTRING_H_ */
