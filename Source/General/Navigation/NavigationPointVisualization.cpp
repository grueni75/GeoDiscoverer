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
#include <NavigationPointVisualization.h>
#include <MapCalibrator.h>
#include <MapPosition.h>
#include <Integer.h>
#include <MapTile.h>
#include <GraphicEngine.h>
#include <Storage.h>
#include <MapContainer.h>

namespace GEODISCOVERER {

// Constructor
NavigationPointVisualization::NavigationPointVisualization(GraphicObject *visualizationObject) {
  init(visualizationObject,0);
}

// Constructor
NavigationPointVisualization::NavigationPointVisualization(GraphicObject *visualizationObject, double lat, double lng, NavigationPointVisualizationType visualizationType, std::string name, void *reference) : pos() {
  init(visualizationObject,core->getClock()->getMicrosecondsSinceStart());
  pos.setLng(lng);
  pos.setLat(lat);
  this->visualizationType=visualizationType;
  this->name=name;
  this->reference=reference;
}

// Code shared by the constructors
void NavigationPointVisualization::init(GraphicObject *visualizationObject, TimestampInMicroseconds createTime)  {
  this->visualizationType=NavigationPointVisualizationTypeUnknown;
  this->reference=NULL;
  this->visualizationObject=visualizationObject;
  graphicPrimitiveKey=0;
  this->createTime=createTime;
  TimestampInMicroseconds animationMaxJitter=core->getConfigStore()->getDoubleValue("Graphic","navigationPointAnimationMaxJitter",__FILE__,__LINE__);
  this->animationJitter=rand()%animationMaxJitter;
  animationDuration=core->getConfigStore()->getIntValue("Graphic","animationDuration", __FILE__, __LINE__);
}

// Destructor
NavigationPointVisualization::~NavigationPointVisualization() {
}

// Updates the visualization
void NavigationPointVisualization::updateVisualization(TimestampInMicroseconds t, MapPosition mapPos, MapArea displayArea, bool debug) {

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
      case NavigationPointVisualizationTypePointCandidate:
        if (!(graphicRectangle=new GraphicRectangle(*core->getDefaultGraphicEngine()->getNavigationPointCandidateIcon()))) {
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
      graphicRectangle->setReference(this);
      graphicPrimitiveKey=visualizationObject->addPrimitive((GraphicPrimitive*)graphicRectangle);
    }

    // Update the position
    if (t<createTime+animationJitter+animationDuration) {
      graphicRectangle->setX(visPosX);
      graphicRectangle->setY(visPosY+core->getDefaultScreen()->getHeight());
      graphicRectangle->setTranslateAnimation(createTime+animationJitter,graphicRectangle->getX(),graphicRectangle->getY(),visPosX,visPosY,false,animationDuration,GraphicTranslateAnimationTypeAccelerated);
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


// Stores the content into a file
void NavigationPointVisualization::store(std::ofstream *ofs) {

  // Store the fields
  Storage::storeByte(ofs,visualizationType);
  Storage::storeString(ofs,name);
  Storage::storeAlignment(ofs,sizeof(double));
  Storage::storeDouble(ofs,pos.getLng());
  Storage::storeDouble(ofs,pos.getLat());
}

// Recreates the content from a binary file
void NavigationPointVisualization::retrieve(char *&data, Int &size) {

  // Read the fields
  Byte type;
  Storage::retrieveByte(data,size,type);
  //DEBUG("type=%d",type);
  this->visualizationType=(NavigationPointVisualizationType)type;
  char *name;
  Storage::retrieveString(data,size,&name);
  this->name=std::string(name);
  Storage::retrieveAlignment(data,size,sizeof(double));
  double t;
  Storage::retrieveDouble(data,size,t);
  pos.setLng(t);
  Storage::retrieveDouble(data,size,t);
  pos.setLat(t);
}

// Indicates that textures and buffers shall be created
void NavigationPointVisualization::createGraphic() {
  if (graphicPrimitiveKey!=0) {
    GraphicRectangle *graphicRectangle=(GraphicRectangle*)visualizationObject->getPrimitive(graphicPrimitiveKey);
    if (graphicRectangle) {
      switch(visualizationType) {
      case NavigationPointVisualizationTypePoint:
        graphicRectangle->setTexture(core->getDefaultGraphicEngine()->getNavigationPointIcon()->getTexture());
        break;
      case NavigationPointVisualizationTypePointCandidate:
        graphicRectangle->setTexture(core->getDefaultGraphicEngine()->getNavigationPointIcon()->getTexture());
        break;
      case NavigationPointVisualizationTypeStartFlag:
        graphicRectangle->setTexture(core->getDefaultGraphicEngine()->getPathStartFlagIcon()->getTexture());
        break;
      case NavigationPointVisualizationTypeEndFlag:
        graphicRectangle->setTexture(core->getDefaultGraphicEngine()->getPathEndFlagIcon()->getTexture());
        break;
      default:
        FATAL("unknown navigation point visualization type",NULL);
      }
    }
  }
}

// Indicates that textures and buffers have been cleared
void NavigationPointVisualization::destroyGraphic() {
  if (graphicPrimitiveKey!=0) {
    GraphicRectangle *graphicRectangle=(GraphicRectangle*)visualizationObject->getPrimitive(graphicPrimitiveKey);
    if (graphicRectangle) {
      graphicRectangle->invalidate();
    }
  }
}

} /* namespace GEODISCOVERER */
