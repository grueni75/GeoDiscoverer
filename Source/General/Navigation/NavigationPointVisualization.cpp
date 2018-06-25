//============================================================================
// Name        : NavigationPointVisualization.cpp
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
//============================================================================

#include <Core.h>

namespace GEODISCOVERER {

// Constructor
NavigationPointVisualization::NavigationPointVisualization(double lat, double lng, NavigationPointVisualizationType visualizationType, std::string name, void *reference) : pos() {
  pos.setLng(lng);
  pos.setLat(lat);
  this->visualizationType=visualizationType;
  this->name=name;
  this->reference=reference;
  graphicPrimitiveKey=0;
  createTime=core->getClock()->getMicrosecondsSinceStart();
  animationDuration=core->getConfigStore()->getIntValue("Graphic","animationDuration", __FILE__, __LINE__);
}


// Destructor
NavigationPointVisualization::~NavigationPointVisualization() {
}

// Updates the visualization
void NavigationPointVisualization::updateVisualization(TimestampInMicroseconds t, MapPosition mapPos, MapArea displayArea, GraphicObject *visualizationObject) {

  Int mapDiffX, mapDiffY;
  Int visPosX, visPosY;

  // Check if the address point is visible
  bool showPoint=true;
  MapCalibrator *calibrator=mapPos.getMapTile()->getParentMapContainer()->getMapCalibrator();
  if (!displayArea.containsGeographicCoordinate(pos)) {
    showPoint=false;
  } else {
    if (!calibrator->setPictureCoordinates(pos)) {
      showPoint=false;
    } else {
      if (!Integer::add(pos.getX(),-mapPos.getX(),mapDiffX))
        showPoint=false;
      if (!Integer::add(pos.getY(),-mapPos.getY(),mapDiffY))
        showPoint=false;
      if (!Integer::add(displayArea.getRefPos().getX(),mapDiffX,visPosX))
        showPoint=false;
      if (!Integer::add(displayArea.getRefPos().getY(),-mapDiffY,visPosY))
        showPoint=false;
    }
  }

  // Visualize the point if visible
  core->getDefaultGraphicEngine()->lockDrawing(__FILE__,__LINE__);
  if (showPoint) {

    // Check if the point already exists
    // If not, add it
    GraphicRectangle *graphicRectangle=(GraphicRectangle*)visualizationObject->getPrimitive(graphicPrimitiveKey);
    if (!graphicRectangle) {
      //DEBUG("%s => icon created",name.c_str());
      switch(visualizationType) {
      case NavigationPointVisualizationTypePoint:
        if (!(graphicRectangle=new GraphicRectangle(*core->getDefaultGraphicEngine()->getNavigationPointIcon()))) {
          FATAL("can not create graphic rectangle object",NULL);
        }
        break;
      case NavigationPointVisualizationTypeStartFlag:
        if (!(graphicRectangle=new GraphicRectangle(*core->getDefaultGraphicEngine()->getPathStartFlagIcon()))) {
          FATAL("can not create graphic rectangle object",NULL);
        }
        break;
      case NavigationPointVisualizationTypeEndFlag:
        if (!(graphicRectangle=new GraphicRectangle(*core->getDefaultGraphicEngine()->getPathEndFlagIcon()))) {
          FATAL("can not create graphic rectangle object",NULL);
        }
        break;
      default:
        FATAL("unknown navigation point visualization type",NULL);
      }
      graphicRectangle->setDestroyTexture(false);
      std::list<std::string> l;
      l.push_back(name);
      graphicRectangle->setName(l);
      graphicPrimitiveKey=visualizationObject->addPrimitive((GraphicPrimitive*)graphicRectangle);
    }

    // Update the position
    if (t<createTime+animationDuration) {
      graphicRectangle->setX(visPosX);
      graphicRectangle->setY(visPosY+core->getDefaultScreen()->getHeight());
      graphicRectangle->setTranslateAnimation(createTime,graphicRectangle->getX(),graphicRectangle->getY(),visPosX,visPosY,false,animationDuration,GraphicTranslateAnimationTypeAccelerated);
    } else {
      graphicRectangle->setX(visPosX);
      graphicRectangle->setY(visPosY);
    }

  } else {

    // Remove icon if it is still visible
    if (graphicPrimitiveKey!=0) {
      visualizationObject->removePrimitive(graphicPrimitiveKey,true);
      graphicPrimitiveKey=0;
      //DEBUG("%s => icon removed",i->getName().c_str());
    }
  }
  core->getDefaultGraphicEngine()->unlockDrawing();
}

} /* namespace GEODISCOVERER */
