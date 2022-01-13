//============================================================================
// Name        : GraphicPosition.cpp
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
#include <GraphicPosition.h>
#include <GraphicEngine.h>
#include <FloatingPoint.h>

namespace GEODISCOVERER {

GraphicPosition::GraphicPosition() {
  set(0,0,1.0,0.0);
  animStartTime=0;
  animEndTime=0;
  endX=valueX;
  endY=valueY;
  lastUserModification=0;
  animSpeed=core->getConfigStore()->getDoubleValue("Graphic","animSpeed",__FILE__,__LINE__);
}

GraphicPosition::~GraphicPosition() {
  // TODO Auto-generated destructor stub
}

// Changes the rotation
void GraphicPosition::rotate(double degree) {
  valueAngle+=degree;
  while (valueAngle>=360.0) {
    valueAngle=valueAngle-360.0;
  }
  while (valueAngle<0.0) {
    valueAngle=360.0+valueAngle;
  }
  //DEBUG("degree=%f valueAngle=%f",degree,valueAngle);
  animStartTime=animEndTime;
  changed=true;
}

// Changes the zoom
void GraphicPosition::zoom(double scale) {
  //std::cout << "stepZoom=" << stepZoom.toString() << std::endl;
  //std::cout << "stepZoom=" << stepZoom.toDouble() << std::endl;
  //valueZoom+=FixedPoint(offset)*stepZoom;
  valueZoom*=scale;
  //DEBUG("valueZoom=%f",valueZoom);
  animStartTime=animEndTime;
  changed=true;
}

// Pans the position
void GraphicPosition::pan(Int xi, Int yi) {

  // Compute the displacement depending on the zoom
  //DEBUG("before zoom adaption: x=%d y=%d",xi,yi);
  //DEBUG("zoomValue=%f",valueZoom);
  double screenScale = core->getDefaultGraphicEngine()->getMapTileToScreenScale(core->getDefaultScreen());
  double xd=(double)xi/(valueZoom*screenScale);
  double yd=(double)yi/(valueZoom*screenScale);
  //DEBUG("after zoom adaption: x=%f y=%f",xd,yd);

  // Compute the new position
  double alpha=getAngleRad();
  double beta=FloatingPoint::computeAngle(xi,yi);
  double gamma=beta-alpha;
  double l=xd*xd+yd*yd;
  l=sqrt(l);
  double xt=l*cos(gamma);
  double yt=l*sin(gamma);
  //DEBUG("x=%d y=%d xt=%.4f yt=%.4f l=%.4f alpha=%.4f beta=%.4f gamma=%.4f",x,y,xt.toDouble(),yt.toDouble(),l.toDouble(),alpha.toDouble(),beta.toDouble(),gamma.toDouble());
  valueX += xt;
  valueY += yt;
  animStartTime=animEndTime;
  changed=true;
  //DEBUG("valueX=%.2f valueY=%.2f",valueX,valueY);
}

// Returns the angle in radiant
double GraphicPosition::getAngleRad() {
  return FloatingPoint::degree2rad(getAngle());
}

// Sets the position
void GraphicPosition::set(Int valueX, Int valueY, double valueZoom, double valueAngle) {
  this->valueAngle=valueAngle;
  this->valueZoom=valueZoom;
  this->valueX=valueX;
  this->valueY=valueY;
  animStartTime=animEndTime;
  changed=true;
}

// Operators
bool GraphicPosition::operator==(const GraphicPosition &rhs)
{
  if ((getAngle()==rhs.getAngle())&&(getZoom()==rhs.getZoom())&&(getX()==rhs.getX())
  &&(getY()==rhs.getY()))
    return true;
  else
    return false;
}
bool GraphicPosition::operator!=(const GraphicPosition &rhs)
{
  return (!(*this==rhs));
}

// Let the position work (for updating the animation)
bool GraphicPosition::work(TimestampInMicroseconds currentTime) {
  bool changed=false;
  if ((animStartTime<=currentTime)&&(animStartTime!=animEndTime)) {
    changed=true;
    if (currentTime<=animEndTime) {
      Int elapsedTime=currentTime-animStartTime;
      Int duration=animEndTime-animStartTime;
      double translateDiffX=endX-startX;
      double translateDiffY=endY-startY;
      double speedX,speedY,accelX,accelY;
      accelX=translateDiffX/(((double)duration)*((double)duration)/4.0);
      accelY=translateDiffY/(((double)duration)*((double)duration)/4.0);
      speedX=accelX*((double)duration)/2;
      speedY=accelY*((double)duration)/2;
      if (elapsedTime<=duration/2) {
        valueX=(Int)(startX+accelX*((double)elapsedTime)*((double)elapsedTime)/2);
        valueY=(Int)(startY+accelY*((double)elapsedTime)*((double)elapsedTime)/2);
      } else {
        double t=((double)elapsedTime)-((double)duration)/2;
        valueX=(Int)(startX+translateDiffX/2+speedX*t-accelX*t*t/2);
        valueY=(Int)(startY+translateDiffY/2+speedY*t-accelY*t*t/2);
      }
      //DEBUG("transitioning: targetX=%f targetY=%f currentX=%f currentY=%f",endX,endY,valueX,valueY);
    } else {
      valueX=endX;
      valueY=endY;
      animStartTime=animEndTime;
      //DEBUG("finished: currentX=%f currentY=%f",this->valueX,this->valueY);
    }
  }
  return changed;
}

// Sets the position to reach by animation
void GraphicPosition::setAnimated(Int valueX, Int valueY) {
  //DEBUG("targetX=%d targetY=%d",valueX,valueY);

  // Compute the remaining animation speed
  double diffX=valueX-this->valueX;
  double diffY=valueY-this->valueY;
  double dist=sqrt(diffX*diffX+diffY*diffY);
  Screen *screen=core->getDefaultScreen();  
  double distInches=dist/(double)screen->getDPI();
  double duration=distInches/animSpeed;
  //DEBUG("dist=%d distInches=%f duration=%f",dist,distInches,duration);

  // Set the animation
  animStartTime=core->getClock()->getMicrosecondsSinceStart();
  animEndTime=animStartTime+(TimestampInMicroseconds)round(duration*1000*1000);
  startX=this->valueX;
  startY=this->valueY;
  endX=valueX;
  endY=valueY;

}

}
