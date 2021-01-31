//============================================================================
// Name        : GraphicRectangle.h
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

#include <GraphicPrimitive.h>

#ifndef GRAPHICRECTANGLE_H_
#define GRAPHICRECTANGLE_H_

namespace GEODISCOVERER {

class GraphicRectangle : public GraphicPrimitive {

protected:

  Int width;          // Width of the rectangle
  Int height;         // Height of the rectangle
  Int iconWidth;      // Width of the icon
  Int iconHeight;     // Height of the icon
  bool filled;        // Indicates if rectangle is filled or not

public:

  // Constructor
  GraphicRectangle(Screen *screen);

  // Loads an icon and sets it as the texture of the primitive
  void setTextureFromIcon(Screen *screen, std::string iconFilename);

  // Called when the widget must be drawn
  virtual void draw(TimestampInMicroseconds t);

  // Destructor
  virtual ~GraphicRectangle();

  // Getters and setters
  Int getHeight() const
  {
      return height;
  }

  Int getWidth() const
  {
      return width;
  }

  virtual void setHeight(Int height)
  {
      this->height = height;
  }

  virtual void setWidth(Int width)
  {
      this->width = width;
  }

  bool getFilled() const
  {
      return filled;
  }

  void setFilled(bool filled)
  {
      this->filled = filled;
  }

  Int getIconHeight() const
  {
      return iconHeight;
  }

  Int getIconWidth() const
  {
      return iconWidth;
  }

  virtual void setIconHeight(Int iconHeight)
  {
      this->iconHeight = iconHeight;
  }

  virtual void setIconWidth(Int iconWidth)
  {
      this->iconWidth = iconWidth;
  }
};

}

#endif /* GRAPHICRECTANGLE_H_ */
