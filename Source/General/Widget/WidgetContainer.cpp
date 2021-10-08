//============================================================================
// Name        : WidgetContainer.cpp
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
#include <WidgetContainer.h>
#include <WidgetEngine.h>
#include <FontEngine.h>
#include <GraphicEngine.h>

namespace GEODISCOVERER {

// Constructor
WidgetContainer::WidgetContainer(WidgetEngine *widgetEngine, std::string name) : graphicObject(widgetEngine->getScreen()){
  touchStartedOutside=false;
  firstTouch=true;
  selectedWidget=NULL;
  touchEndTime=0;
  lastTouchStartedOutside=true;
  this->widgetEngine=widgetEngine;
  this->name=name;
}

// Destructor
WidgetContainer::~WidgetContainer() {
  deinit();
}

// Removes all widgets
void WidgetContainer::deinit(bool deleteWidgets) {
  graphicObject.deinit(deleteWidgets);
}

// Adds a widget to the page
void WidgetContainer::addWidget(WidgetPrimitive *primitive) {
  graphicObject.addPrimitive(primitive);
}

// Called when a two finger gesture is done on the page
bool WidgetContainer::onTwoFingerGesture(TimestampInMicroseconds t, Int dX, Int dY, double angleDiff, double scaleDiff) {

  // Two the two fingure gesture on the selected widget
  if (selectedWidget) {
    selectedWidget->onTwoFingerGesture(t,dX,dY,angleDiff,scaleDiff);
    return true;
  } else {
    return false;
  }
}

// Called when the page is touched
bool WidgetContainer::onTouchDown(TimestampInMicroseconds t, Int x, Int y) {

  // Skip the check if it started outside
  touchEndTime=t;
  if (touchStartedOutside)
    return false;

  // Check all widgets
  WidgetPrimitive *previousSelectedWidget=selectedWidget;
  selectedWidget=NULL;
  std::list<GraphicPrimitive*> *drawList=graphicObject.getDrawList();
  for(std::list<GraphicPrimitive*>::reverse_iterator i = drawList->rbegin(); i!=drawList->rend(); i++) {
    WidgetPrimitive *primitive=(WidgetPrimitive*)*i;
    primitive->onTouchDown(t,x,y);
    if (primitive->getIsSelected()) {
      selectedWidget=primitive;
      break;
    }
  }
  if (previousSelectedWidget!=selectedWidget) {
    if (previousSelectedWidget) {
      widgetEngine->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
      previousSelectedWidget->setFadeAnimation(t,previousSelectedWidget->getColor(),previousSelectedWidget->getActiveColor(),false,widgetEngine->getGraphicEngine()->getFadeDuration());
      widgetEngine->getGraphicEngine()->unlockDrawing();
    }
    if (selectedWidget) {
      widgetEngine->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
      selectedWidget->setFadeAnimation(t,selectedWidget->getColor(),getWidgetEngine()->getSelectedWidgetColor(),false,widgetEngine->getGraphicEngine()->getFadeDuration());
      widgetEngine->getGraphicEngine()->unlockDrawing();
    }
  }

  // If this is the first touch and no widget was hit, do not activate any widgets later
  // This allows smooth scrolling of the map
  if ((firstTouch)&&(!selectedWidget)) {
    //setWidgetsActive(t,false);
    touchStartedOutside=true;
    lastTouchStartedOutside=true;
    return false;
  } else {
    lastTouchStartedOutside=false;
  }

  // Next touch is not the first one
  firstTouch=false;

  return true;
}

// Called when the page is not touched anymore
void WidgetContainer::onTouchUp(TimestampInMicroseconds t, Int x, Int y, bool cancel) {

  // Check all widgets
  std::list<GraphicPrimitive*> *drawList=graphicObject.getDrawList();
  if (!touchStartedOutside) {
    for(std::list<GraphicPrimitive*>::iterator i = drawList->begin(); i!=drawList->end(); i++) {
      WidgetPrimitive *primitive=(WidgetPrimitive*)*i;
      primitive->onTouchUp(t,x,y,cancel);
    }
  }
  deselectWidget(t);
  touchEndTime=t;
}


// Deselects currently selected widget
void WidgetContainer::deselectWidget(TimestampInMicroseconds t) {
  if (!touchStartedOutside) {
    if (selectedWidget) {
      widgetEngine->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
      selectedWidget->setFadeAnimation(t,selectedWidget->getColor(),selectedWidget->getActiveColor(),false,widgetEngine->getGraphicEngine()->getFadeDuration());
      widgetEngine->getGraphicEngine()->unlockDrawing();
    }
    selectedWidget=NULL;
  }
  firstTouch=true;
  touchStartedOutside=false;
}

FontEngine *WidgetContainer::getFontEngine() {
  return widgetEngine->getFontEngine();
}

WidgetEngine *WidgetContainer::getWidgetEngine() {
  return widgetEngine;
}

GraphicEngine *WidgetContainer::getGraphicEngine() {
  return widgetEngine->getGraphicEngine();
}

Screen *WidgetContainer::getScreen() {
  return widgetEngine->getScreen();
}

// Called when the map has changed
void WidgetContainer::onMapChange(bool pageVisible, MapPosition pos) {

  // Check all widgets
  std::list<GraphicPrimitive*> *drawList=graphicObject.getDrawList();
  for(std::list<GraphicPrimitive*>::iterator i = drawList->begin(); i!=drawList->end(); i++) {
    WidgetPrimitive *primitive=(WidgetPrimitive*)*i;
    primitive->onMapChange(pageVisible, pos);
  }
}

// Called when the location has changed
void WidgetContainer::onLocationChange(bool pageVisible, MapPosition pos) {

  // Check all widgets
  std::list<GraphicPrimitive*> *drawList=graphicObject.getDrawList();
  for(std::list<GraphicPrimitive*>::iterator i = drawList->begin(); i!=drawList->end(); i++) {
    WidgetPrimitive *primitive=(WidgetPrimitive*)*i;
    primitive->onLocationChange(pageVisible, pos);
  }
}

// Called when a path has changed
void WidgetContainer::onPathChange(bool pageVisible, NavigationPath *path, NavigationPathChangeType changeType) {

  // Check all widgets
  std::list<GraphicPrimitive*> *drawList=graphicObject.getDrawList();
  for(std::list<GraphicPrimitive*>::iterator i = drawList->begin(); i!=drawList->end(); i++) {
    WidgetPrimitive *primitive=(WidgetPrimitive*)*i;
    primitive->onPathChange(pageVisible, path, changeType);
  }
}

// Called when some data has changed
void WidgetContainer::onDataChange() {

  // Check all widgets
  std::list<GraphicPrimitive*> *drawList=graphicObject.getDrawList();
  for(std::list<GraphicPrimitive*>::iterator i = drawList->begin(); i!=drawList->end(); i++) {
    WidgetPrimitive *primitive=(WidgetPrimitive*)*i;
    primitive->onDataChange();
  }
}

// Called when touch mode has changed
void WidgetContainer::setTouchMode(Int mode) {

  // Check all widgets
  std::list<GraphicPrimitive*> *drawList=graphicObject.getDrawList();
  for(std::list<GraphicPrimitive*>::iterator i = drawList->begin(); i!=drawList->end(); i++) {
    WidgetPrimitive *primitive=(WidgetPrimitive*)*i;
    primitive->setTouchMode(mode);
  }
}
}
