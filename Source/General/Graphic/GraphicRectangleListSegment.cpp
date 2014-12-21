//============================================================================
// Name        : GraphicRectangleListSegment.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#include <Core.h>

namespace GEODISCOVERER {

// Constructor
GraphicRectangleListSegment::GraphicRectangleListSegment(Int numberOfRectangles) {
  if (!(textureCoordinates=new GraphicPointBuffer(2*3*numberOfRectangles))) {
    FATAL("can not create texture coordinate buffer",NULL);
    return;
  }
  if (!(triangleCoordinates=new GraphicPointBuffer(2*3*numberOfRectangles))) {
    FATAL("can not create triangle coordinate buffer",NULL);
    return;
  }
}

// Destructor
GraphicRectangleListSegment::~GraphicRectangleListSegment() {
  delete textureCoordinates;
  delete triangleCoordinates;
}

// Adds a new rectangle
bool GraphicRectangleListSegment::addRectangle(Short x[4], Short y[4]) {

  // No space left?
  if (triangleCoordinates->getIsFull())
    return false;

  // Add the points
  triangleCoordinates->addPoint(x[0],y[0]);
  textureCoordinates->addPoint(0,1);
  triangleCoordinates->addPoint(x[1],y[1]);
  textureCoordinates->addPoint(1,1);
  triangleCoordinates->addPoint(x[3],y[3]);
  textureCoordinates->addPoint(0,0);
  triangleCoordinates->addPoint(x[3],y[3]);
  textureCoordinates->addPoint(0,0);
  triangleCoordinates->addPoint(x[2],y[2]);
  textureCoordinates->addPoint(1,0);
  triangleCoordinates->addPoint(x[1],y[1]);
  textureCoordinates->addPoint(1,1);
}

// Draws the rectangle list
void GraphicRectangleListSegment::draw(Screen *screen, GraphicTextureInfo textureInfo) {
  triangleCoordinates->drawAsTexturedTriangles(screen,textureInfo,textureCoordinates);
}

// Copies the contents to an other segment
void GraphicRectangleListSegment::copy(GraphicRectangleListSegment *otherSegment) {
  for (Int j=0;j<getSize();j++) {
    Short x,y;
    triangleCoordinates->getPoint(j,x,y);
    if (!otherSegment->triangleCoordinates->addPoint(x,y)) {
      FATAL("can not add point",NULL);
    }
    textureCoordinates->getPoint(j,x,y);
    if (!otherSegment->textureCoordinates->addPoint(x,y)) {
      FATAL("can not add point",NULL);
    }
  }
}

// Recreates any textures or buffers
void GraphicRectangleListSegment::invalidate() {
  triangleCoordinates->invalidate();
  textureCoordinates->invalidate();
}

}
