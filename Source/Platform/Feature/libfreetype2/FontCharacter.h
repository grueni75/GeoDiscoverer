//============================================================================
// Name        : FontCharacter.h
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


#ifndef FONTCHARACTER_H_
#define FONTCHARACTER_H_

namespace GEODISCOVERER {

class FontCharacter {

  Int penOffsetTop;                // Offset from the current pen position to the top of the character bitmap
  Int penOffsetLeft;               // Offset from the current pen position to the left of the character bitmap
  Int penAdvanceX;                 // Distance to move the pen after the character has been drawn
  Int width;                       // Width of the character
  Int height;                      // Height of the character
  UShort *bitmap;                  // Bitmap of the character

public:

  // Constructor
  FontCharacter(Int width, Int height);

  // Destructor
  virtual ~FontCharacter();

  // Getters and setters
  Int getPenAdvanceX() const
  {
      return penAdvanceX;
  }

  Int getPenOffsetLeft() const
  {
      return penOffsetLeft;
  }

  Int getPenOffsetTop() const
  {
      return penOffsetTop;
  }

  void setPenAdvanceX(Int penAdvanceX)
  {
      this->penAdvanceX = penAdvanceX;
  }

  void setPenOffsetLeft(Int penOffsetLeft)
  {
      this->penOffsetLeft = penOffsetLeft;
  }

  void setPenOffsetTop(Int penOffsetTop)
  {
      this->penOffsetTop = penOffsetTop;
  }

  Int getHeight() const
  {
      return height;
  }

  Int getWidth() const
  {
      return width;
  }

  UShort *getBitmap() const
  {
      return bitmap;
  }

};

}

#endif /* FONTCHARACTER_H_ */
