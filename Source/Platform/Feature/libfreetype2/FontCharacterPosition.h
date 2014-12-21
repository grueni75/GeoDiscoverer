//============================================================================
// Name        : FontCharacterPosition.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef FONTCHARACTERPOSITION_H_
#define FONTCHARACTERPOSITION_H_

namespace GEODISCOVERER {

class FontCharacterPosition {

protected:

  Int x;                      // x Position of the character
  Int y;                      // y Position of the character
  FontCharacter *character;   // Character description

public:

  // Constructor
  FontCharacterPosition();

  // Destructor
  virtual ~FontCharacterPosition();

  // Setters and getters
  FontCharacter *getCharacter() const
  {
      return character;
  }

  Int getX() const
  {
      return x;
  }

  Int getY() const
  {
      return y;
  }

  void setCharacter(FontCharacter *character)
  {
      this->character = character;
  }

  void setX(Int x)
  {
      this->x = x;
  }

  void setY(Int y)
  {
      this->y = y;
  }

};

}

#endif /* FONTCHARACTERPOSITION_H_ */
