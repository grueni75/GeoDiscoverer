//============================================================================
// Name        : GraphicLine.cpp
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
GraphicLine::GraphicLine(Int numberOfStrokes, Short width) : GraphicPrimitive() {
  type=GraphicTypeLine;
  this->valuesPerTriangle=3;
  this->numberOfTrianglesOtherSegments=4*core->getConfigStore()->getIntValue("Graphic","lineNumberOfStrokesOtherSegemnts","Number of strokes to reserve if current segment has no strokes left anymore",32);
  this->width=width;
  this->cutEnabled=false;
  if (numberOfStrokes==0) {
    numberOfStrokes=numberOfTrianglesOtherSegments;
  }
  if (!(currentSegment=new GraphicPointBuffer(valuesPerTriangle*4*numberOfStrokes))) {
    FATAL("can not create point buffer",NULL);
    return;
  }
  segments.push_back(currentSegment);
}

// Destructor
GraphicLine::~GraphicLine() {
  deinit();
}

// Frees all memories
void GraphicLine::deinit() {
  for(std::list<GraphicPointBuffer*>::iterator i=segments.begin();i!=segments.end();i++) {
    delete *i;
  }
  segments.clear();
}

// Checks if an overflow has occured
bool GraphicLine::pointIsValid(double x, double y) {
  if (x>std::numeric_limits<Short>::max())
    return false;
  if (x<std::numeric_limits<Short>::min())
    return false;
  if (y>std::numeric_limits<Short>::max())
    return false;
  if (y<std::numeric_limits<Short>::min())
    return false;
  return true;
}

// Adds one point to the internal array
void GraphicLine::addPoint(Short x, Short y) {
  //DEBUG("x=%d y=%d insertPos=%d endPosCurrentSegment=%d",x,y,insertPos,endPosCurrentSegment);
  if (!currentSegment->addPoint(x,y)) {
    if (!(currentSegment=new GraphicPointBuffer(valuesPerTriangle*numberOfTrianglesOtherSegments))) {
      FATAL("can not create point buffer",NULL);
      return;
    }
    currentSegment->addPoint(x,y);
    segments.push_back(currentSegment);
  }
}

// Updates the point such that it lies within the cut range
void GraphicLine::cutPoint(Short &x, Short &y, double m, double ys) {
  if (x<this->x-width/2) {
    //DEBUG("before: x=%d y=%d",x,y);
    x=this->x-width/2;
    y=round(m*((double)x)+ys);
    //DEBUG("after: x=%d y=%d",x,y);
  }
  if (x>=this->x+this->cutWidth+width/2) {
    //DEBUG("before: x=%d y=%d",x,y);
    x=this->x+cutWidth+width/2-1;
    y=round(m*((double)x)+ys);
    //DEBUG("after: x=%d y=%d",x,y);
  }
  if (y<this->y-width/2) {
    //DEBUG("before: x=%d y=%d",x,y);
    y=this->y-width/2;
    if (m!=std::numeric_limits<double>::infinity())
      x=round((((double)y)-ys)/m);
    //DEBUG("after: x=%d y=%d",x,y);
    return;
  }
  if (y>=this->y+cutHeight+width/2) {
    //DEBUG("before: x=%d y=%d",x,y);
    y=this->y+cutHeight+width/2-1;
    if (m!=std::numeric_limits<double>::infinity())
      x=round((((double)y)-ys)/m);
    //DEBUG("after: x=%d y=%d",x,y);
    return;
  }
}

// Adds two points to the line without filling the gap to the previous point
void GraphicLine::addStroke(Short x0, Short y0, Short x1, Short y1) {
  Short diffX=x1-x0;
  Short diffY=y1-y0;
  double angle=FloatingPoint::computeAngle(diffX,diffY);
  const Int n=3*4;
  double x[n],y[n];

  // Compute the start points such that they lie within the cut area
  if (cutEnabled) {

    // Compute the line parameters
    double m,ys;
    double diffX=x1-x0;
    double diffY=y1-y0;
    if (diffX==0) {
      m=std::numeric_limits<double>::infinity();
      ys=m;
    } else {
      m=diffY/diffX;
      ys=y0-m*x0;
    }

    // Cut the points
    //DEBUG("x0=%d y0=%d x1=%d y1=%d m=%f ys=%f",x0,y0,x1,y1,m,ys);
    cutPoint(x0,y0,m,ys);
    cutPoint(x1,y1,m,ys);

    // Line did not hit the cut region?
    bool point0WithinCutRegion=(x0>=this->x-width/2)&&(x0<this->x+this->cutWidth+width/2)&&(y0>=this->y-width/2)&&(y0<this->y+this->cutHeight+width/2);
    bool point1WithinCutRegion=(x1>=this->x-width/2)&&(x1<this->x+this->cutWidth+width/2)&&(y1>=this->y-width/2)&&(y1<this->y+this->cutHeight+width/2);
    //DEBUG("point0WithinCutRegion=%d point1WithinCutRegion=%d strokeLength=%d",point0WithinCutRegion,point1WithinCutRegion,strokeLength);
    if ((!point0WithinCutRegion)&&(!point1WithinCutRegion))
      return;

    /* Sanity check if line within cut region
    if (!point0WithinCutRegion) {
      FATAL("Point (%d,%d) lies outside the cut region",x0,y0);
      return;
    }
    if (!point1WithinCutRegion) {
      FATAL("Point (%d,%d) lies outside the cut region",x1,y1);
      return;
    }*/

  }

  // First triangle
  x[0]=x0+width/2*cos(angle-M_PI/2);
  y[0]=y0+width/2*sin(angle-M_PI/2);
  x[1]=x0+width/2*cos(angle+M_PI/2);
  y[1]=y0+width/2*sin(angle+M_PI/2);
  x[2]=x1+width/2*cos(angle-M_PI/2);
  y[2]=y1+width/2*sin(angle-M_PI/2);

  // Second triangle
  x[3]=x1+width/2*cos(angle+M_PI/2);
  y[3]=y1+width/2*sin(angle+M_PI/2);
  x[4]=x1+width/2*cos(angle-M_PI/2);
  y[4]=y1+width/2*sin(angle-M_PI/2);
  x[5]=x0+width/2*cos(angle+M_PI/2);
  y[5]=y0+width/2*sin(angle+M_PI/2);

  // Edge roundings
  x[6]=x0+width/2*cos(angle-M_PI);
  y[6]=y0+width/2*sin(angle-M_PI);
  x[7]=x0+width/2*cos(angle-M_PI/2);
  y[7]=y0+width/2*sin(angle-M_PI/2);
  x[8]=x0+width/2*cos(angle+M_PI/2);
  y[8]=y0+width/2*sin(angle+M_PI/2);
  x[9]=x1+width/2*cos(angle);
  y[9]=y1+width/2*sin(angle);
  x[10]=x1+width/2*cos(angle-M_PI/2);
  y[10]=y1+width/2*sin(angle-M_PI/2);
  x[11]=x1+width/2*cos(angle+M_PI/2);
  y[11]=y1+width/2*sin(angle+M_PI/2);

  // Add points
  for (int i=0;i<n;i++) {
    x[i]=round(x[i]);
    y[i]=round(y[i]);
    if (!pointIsValid(x[i],y[i]))
      return;
  }
  for (int i=0;i<n;i++) {
    addPoint(x[i],y[i]);
  }
}

// Draws the line
void GraphicLine::draw(Screen *screen) {
  for(std::list<GraphicPointBuffer*>::iterator i=segments.begin();i!=segments.end();i++) {
    (*i)->drawAsTriangles(screen);
  }
}

// Recreates any textures or buffers
void GraphicLine::invalidate() {
  for (std::list<GraphicPointBuffer*>::iterator i=segments.begin();i!=segments.end();i++) {
    (*i)->invalidate();
  }
}

// Reduce the number of point buffers required
void GraphicLine::optimize() {

  // Check if optimization is required
  if ((segments.size()==1)&&(currentSegment->getIsFull()))
    return;

  // Find out the total number of points required
  Int totalPoints=0;
  for (std::list<GraphicPointBuffer*>::iterator i=segments.begin();i!=segments.end();i++) {
    totalPoints+=(*i)->getSize();
  }

  // Create a new segment that holds all points
  if (!(currentSegment=new GraphicPointBuffer(totalPoints))) {
    FATAL("can not create point buffer",NULL);
    return;
  }

  // Copy the existing points
  for (std::list<GraphicPointBuffer*>::iterator i=segments.begin();i!=segments.end();i++) {
    for (Int j=0;j<(*i)->getSize();j++) {
      Short x,y;
      (*i)->getPoint(j,x,y);
      if (!currentSegment->addPoint(x,y)) {
        FATAL("can not add point",NULL);
      }
    }
  }

  // Clear the existing segments
  deinit();

  // Add the new segment
  segments.push_back(currentSegment);
}

}
