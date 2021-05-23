//============================================================================
// Name        : GraphicFloatBuffer.cpp
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
#include <GraphicFloatBuffer.h>

namespace GEODISCOVERER {

// Constructor
GraphicFloatBuffer::GraphicFloatBuffer(Screen *screen, Int numberOfPoints) {
  this->screen=screen;
  this->numberOfPoints=numberOfPoints;
  insertPos=0;
  buffer=Screen::getBufferNotDefined();
  bufferOutdated=true;
  if (numberOfPoints>0) {
    if (!(points=(Float*)malloc(sizeof(*points)*numberOfPoints))) {
      FATAL("can not create point array",NULL);
      return;
    }
  } else {
    points=NULL;
  }
}

// Destructor
GraphicFloatBuffer::~GraphicFloatBuffer() {
  invalidate();
  if (points) {
    free(points);
    points=NULL;
  }
}

// Adds one point to the internal array
bool GraphicFloatBuffer::addPoint(Float point) {
  if (insertPos>=numberOfPoints) {
    return false;
  } else {
    points[insertPos]=point;
    insertPos++;
    bufferOutdated=true;
    return true;
  }
}

// Updates the buffer contents
void GraphicFloatBuffer::updateBuffer() {
  if ((points!=NULL)&&(bufferOutdated)) {
    if (buffer==screen->getBufferNotDefined()) {
      buffer=screen->createBufferInfo();
    }
    screen->setArrayBufferData(buffer,(Byte*)points,insertPos*sizeof(Float));
    bufferOutdated=false;
  }
}

// Recreates any textures or buffers
void GraphicFloatBuffer::invalidate() {
  bufferOutdated=true;
  if (buffer!=Screen::getBufferNotDefined()) {
    screen->destroyBufferInfo(buffer);
    buffer=Screen::getBufferNotDefined();
  }
}

// Removes all points
void GraphicFloatBuffer::reset() {
  insertPos=0;
  bufferOutdated=true;
}

// Adds a list of points to the internal array
bool GraphicFloatBuffer::addPoints(std::list<Float> *points) {
  for(std::list<Float>::iterator i = points->begin();i!=points->end();i++) {
    if (!addPoint(*i))
      return false;
  }
  return true;
}

}
