//============================================================================
// Name        : Color.h
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


#ifndef GRAPHICCOLOR_H_
#define GRAPHICCOLOR_H_

namespace GEODISCOVERER {

class GraphicColor {

protected:

  UByte red;
  UByte green;
  UByte blue;
  UByte alpha;

public:

  // Constructor
  GraphicColor();
  GraphicColor(UByte red, UByte green, UByte blue, UByte alpha=255);

  // Destructor
  virtual ~GraphicColor();

  // Operators
  bool operator==(const GraphicColor &rhs);

  // Getters and setters
  UByte getBlue() const
  {
      return blue;
  }

  UByte getGreen() const
  {
      return green;
  }

  UByte getRed() const
  {
      return red;
  }

  void setBlue(UByte blue)
  {
      this->blue = blue;
  }

  void setGreen(UByte green)
  {
      this->green = green;
  }

  void setRed(UByte red)
  {
      this->red = red;
  }

  UByte getAlpha() const
  {
      return alpha;
  }

  void setAlpha(UByte alpha)
  {
      this->alpha = alpha;
  }

};

}

#endif /* GRAPHICCOLOR_H_ */
