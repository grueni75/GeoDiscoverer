//============================================================================
// Name        : Color.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
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
