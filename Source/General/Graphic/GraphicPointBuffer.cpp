//============================================================================
// Name        : GraphicLineSegment.cpp
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


#include <Core.h>

namespace GEODISCOVERER {

// Constructor
GraphicPointBuffer::GraphicPointBuffer(Screen *screen, Int numberOfPoints) {
  this->screen=screen;
  this->numberOfPoints=numberOfPoints;
  insertPos=0;
  buffer=Screen::getBufferNotDefined();
  bufferOutdated=true;
  if (!(points=(Short*)malloc(sizeof(*points)*2*numberOfPoints))) {
    FATAL("can not create point array",NULL);
    return;
  }
}

// Destructor
GraphicPointBuffer::~GraphicPointBuffer() {
  invalidate();
  free(points);
}

// Adds one point to the internal array
bool GraphicPointBuffer::addPoint(Short x, Short y) {
  if (insertPos>=numberOfPoints) {
    return false;
  } else {
    points[2*insertPos]=x;
    points[2*insertPos+1]=y;
    insertPos++;
    bufferOutdated=true;
    return true;
  }
}

// Adds one point to the internal array
bool GraphicPointBuffer::addPoint(GraphicPoint point) {
  return addPoint(point.getX(),point.getY());
}

// Uses the stored points to draw triangles
void GraphicPointBuffer::drawAsTriangles() {
  if (insertPos==0)
    return;
  updateBuffer();
  screen->drawTriangles(insertPos/3,buffer);
}

// Uses the stored points to draw textured triangles
void GraphicPointBuffer::drawAsTexturedTriangles(GraphicTextureInfo textureInfo, GraphicPointBuffer *textureCoordinates) {
  if (insertPos==0)
    return;
  updateBuffer();
  textureCoordinates->updateBuffer();
  screen->drawTriangles(insertPos/3,buffer,textureInfo,textureCoordinates->getBuffer());
}

// Updates the buffer contents
void GraphicPointBuffer::updateBuffer() {
  if (bufferOutdated) {
    if (buffer==screen->getBufferNotDefined()) {
      buffer=screen->createBufferInfo();
    }
    //DEBUG("insertPos=%d buffer=0x%08x",insertPos,buffer);
    screen->setArrayBufferData(buffer,(Byte*)points,insertPos*2*sizeof(Short));
    bufferOutdated=false;
  }
}

// Recreates any textures or buffers
void GraphicPointBuffer::invalidate() {
  bufferOutdated=true;
  if (buffer!=Screen::getBufferNotDefined()) {
    screen->destroyBufferInfo(buffer);
    buffer=Screen::getBufferNotDefined();
  }
}

// Removes all points
void GraphicPointBuffer::reset() {
  insertPos=0;
  bufferOutdated=true;
}

// Adds a list of points to the internal array
bool GraphicPointBuffer::addPoints(std::list<GraphicPoint> *points) {
  for(std::list<GraphicPoint>::iterator i = points->begin();i!=points->end();i++) {
    if (!addPoint(*i))
      return false;
  }
  return true;
}

}
