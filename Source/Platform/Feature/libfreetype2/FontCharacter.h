//============================================================================
// Name        : FontCharacter.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
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
  UShort *normalBitmap;            // Bitmap of the normal character
  UShort *strokeBitmap;            // Bitmap of the stroke character

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

  UShort* getNormalBitmap() const {
    return normalBitmap;
  }

  UShort* getStrokeBitmap() const {
    return strokeBitmap;
  }
};

}

#endif /* FONTCHARACTER_H_ */
