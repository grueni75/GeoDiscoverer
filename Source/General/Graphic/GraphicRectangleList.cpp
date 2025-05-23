//============================================================================
// Name        : GraphicRectangleList.cpp
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
#include <GraphicRectangleList.h>

namespace GEODISCOVERER {

// Constructor
GraphicRectangleList::GraphicRectangleList(Screen *screen, Int numberOfRectangles, bool enableTimeColoring) : GraphicPrimitive(screen) {
  type=GraphicTypeRectangleList;
  this->numberOfRectanglesOtherSegments=4*core->getConfigStore()->getIntValue("Graphic","rectangleListNumberOfRectanglesOtherSegments",__FILE__, __LINE__);
  this->enableTimeColoring=enableTimeColoring;
  if (numberOfRectangles==0) {
    numberOfRectangles=numberOfRectanglesOtherSegments;
  }
  if (!(currentSegment=new GraphicRectangleListSegment(screen, numberOfRectangles, enableTimeColoring))) {
    FATAL("can not create rectangle list entry",NULL);
    return;
  }
  segments.push_back(currentSegment);
  cutEnabled=false;
}

// Destructor
GraphicRectangleList::~GraphicRectangleList() {
  deinit();
}

// Frees all memories
void GraphicRectangleList::deinit() {
  for(std::list<GraphicRectangleListSegment*>::iterator i=segments.begin();i!=segments.end();i++) {
    delete *i;
  }
  segments.clear();
}

// Adds a new rectangle (from center point and angle)
void GraphicRectangleList::addRectangle(double x, double y, double angle, Float timeOffset) {

  // Check if the point lies within the cut area
  if (cutEnabled) {
    if ((x<this->x-radius)||(x>this->cutWidth-1+radius))
      return;
    if ((y<this->y-radius)||(y>this->cutHeight-1+radius))
      return;
  }

  // Compute the center position from the rectangle point of view
  x=x+distanceToCenter*cos(angleToCenter+angle);
  y=y+distanceToCenter*sin(angleToCenter+angle);

  // Compute the edge points
  Short rx[4],ry[4];
  rx[0]=round(x+radius*cos(angle-M_PI/2-M_PI/4));
  ry[0]=round(y+radius*sin(angle-M_PI/2-M_PI/4));
  rx[1]=round(x+radius*cos(angle-M_PI/4));
  ry[1]=round(y+radius*sin(angle-M_PI/4));
  rx[2]=round(x+radius*cos(angle+M_PI/4));
  ry[2]=round(y+radius*sin(angle+M_PI/4));
  rx[3]=round(x+radius*cos(angle+M_PI/2+M_PI/4));
  ry[3]=round(y+radius*sin(angle+M_PI/2+M_PI/4));

  // Add the rectangle
  addRectangle(rx,ry,timeOffset);
}

// Adds a new rectangle (from center point and angle)
void GraphicRectangleList::addRectangle(Short x[4], Short y[4], Float timeOffset) {

  // Add the rectangle
  if (!currentSegment->addRectangle(x,y,timeOffset)) {
    if (!(currentSegment=new GraphicRectangleListSegment(screen, numberOfRectanglesOtherSegments, enableTimeColoring))) {
      FATAL("can not create rectangle list entry",NULL);
      return;
    }
    currentSegment->addRectangle(x,y,timeOffset);
    segments.push_back(currentSegment);
  }
}

// Draws the rectangle list
void GraphicRectangleList::draw() {
  for(std::list<GraphicRectangleListSegment*>::iterator i=segments.begin();i!=segments.end();i++) {
    (*i)->draw(texture);
  }
}

// Recreates any textures or buffers
void GraphicRectangleList::invalidate() {
  for (std::list<GraphicRectangleListSegment*>::iterator i=segments.begin();i!=segments.end();i++) {
    (*i)->invalidate();
  }
}

// Reduce the number of point buffers required
void GraphicRectangleList::optimize() {

  // Check if optimization is required
  if ((segments.size()==1)&&(currentSegment->getIsFull()))
    return;

  // Find out the total number of points required
  Int totalPoints=0;
  for (std::list<GraphicRectangleListSegment*>::iterator i=segments.begin();i!=segments.end();i++) {
    totalPoints+=(*i)->getSize();
  }

  // Create a new segment that holds all points
  if (!(currentSegment=new GraphicRectangleListSegment(screen, totalPoints, enableTimeColoring))) {
    FATAL("can not create rectangle list segment",NULL);
    return;
  }

  // Copy the existing points
  for (std::list<GraphicRectangleListSegment*>::iterator i=segments.begin();i!=segments.end();i++) {
    (*i)->copy(currentSegment);
  }

  // Clear the existing segments
  deinit();

  // Add the new segment
  segments.push_back(currentSegment);
}

}
