//============================================================================
// Name        : GraphicRectangleListSegment.cpp
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
