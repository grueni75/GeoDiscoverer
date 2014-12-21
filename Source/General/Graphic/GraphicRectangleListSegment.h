//============================================================================
// Name        : GraphicRectangleListSegment.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
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
