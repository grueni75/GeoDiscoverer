//============================================================================
// Name        : GraphicRectangleListSegment.h
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


#ifndef GRAPHICRECTANGLELISTSEGMENT_H_
#define GRAPHICRECTANGLELISTSEGMENT_H_

namespace GEODISCOVERER {

class GraphicRectangleListSegment {

protected:

  GraphicPointBuffer *textureCoordinates;     // Coordinates of the texture
  GraphicPointBuffer *triangleCoordinates;    // Coordinates of the triangles

public:

  // Constructor
  GraphicRectangleListSegment(Int numberOfRectangles);

  // Destructor
  virtual ~GraphicRectangleListSegment();

  // Adds a new rectangle
  bool addRectangle(Short x[4], Short y[4]);

  // Draws the rectangles
  void draw(Screen *screen, GraphicTextureInfo textureInfo);

  // Recreates any textures or buffers
  virtual void invalidate();

  // Copies the contents to an other segment
  void copy(GraphicRectangleListSegment *otherSegment);

  // Getters and setters
  Int getSize() const
  {
    return triangleCoordinates->getSize();
  }

  bool getIsFull() const
  {
    return triangleCoordinates->getIsFull();
  }


};

}

#endif /* GRAPHICRECTANGLELISTSEGMENT_H_ */
