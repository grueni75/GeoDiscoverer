//============================================================================
// Name        : GraphicRectangle.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


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
  GraphicRectangle();

  // Loads an icon and sets it as the texture of the primitive
  void setTextureFromIcon(std::string iconFilename);

  // Called when the widget must be drawn
  virtual void draw(Screen *screen, TimestampInMicroseconds t);

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

  void setHeight(Int height)
  {
      this->height = height;
  }

  void setWidth(Int width)
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

  void setIconHeight(Int iconHeight)
  {
      this->iconHeight = iconHeight;
  }

  void setIconWidth(Int iconWidth)
  {
      this->iconWidth = iconWidth;
  }
};

}

#endif /* GRAPHICRECTANGLE_H_ */
