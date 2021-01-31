//============================================================================
// Name        : FontCharacterPosition.h
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

#include <FontCharacter.h>

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
