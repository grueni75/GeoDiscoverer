//============================================================================
// Name        : GraphicRectangleListSegment.h
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

#include <GraphicPointBuffer.h>
#include <GraphicFloatBuffer.h>

#ifndef GRAPHICRECTANGLELISTSEGMENT_H_
#define GRAPHICRECTANGLELISTSEGMENT_H_

namespace GEODISCOVERER {

class GraphicRectangleListSegment {

protected:

  Screen *screen;                                // Screen this rectangle list belongs to
  GraphicFloatBuffer *timeColoringOffsets;       // Time offsets for vertex coloring
  GraphicPointBuffer *textureCoordinates;        // Coordinates of the texture
  GraphicPointBuffer *triangleCoordinates;       // Coordinates of the triangles

public:

  // Constructor
  GraphicRectangleListSegment(Screen *screen, Int numberOfRectangles, bool enableTimeColoring);

  // Destructor
  virtual ~GraphicRectangleListSegment();

  // Adds a new rectangle
  bool addRectangle(Short x[4], Short y[4], Float t=0);

  // Gets the rectangle for the given position
  void getRectangle(Int pos, Short *x, Short *y, Float *t=NULL);

  // Draws the rectangles
  void draw(GraphicTextureInfo textureInfo);

  // Recreates any textures or buffers
  virtual void invalidate();

  // Copies the contents to an other segment
  void copy(GraphicRectangleListSegment *otherSegment);

  // Getters and setters
  Int getSize() const
  {
    return triangleCoordinates->getSize();
  }

  Int getRectangleCount() const
  {
    return triangleCoordinates->getSize()/6;
  }

  bool getIsFull() const
  {
    return triangleCoordinates->getIsFull();
  }

  GraphicPointBuffer *getTextureCoordinates() const
  {
    return textureCoordinates;
  }

  GraphicPointBuffer *getTriangleCoordinates() const
  {
    return triangleCoordinates;
  }

  GraphicFloatBuffer *getTimeColoringOffsets() const
  {
    return timeColoringOffsets;
  }

};

}

#endif /* GRAPHICRECTANGLELISTSEGMENT_H_ */
