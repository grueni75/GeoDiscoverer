//============================================================================
// Name        : WidgetFingerMenu.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2021 Matthias Gruenewald
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
#include <WidgetFingerMenu.h>
#include <FloatingPoint.h>
#include <WidgetEngine.h>
#include <FontEngine.h>

namespace GEODISCOVERER {

// Constructor
WidgetFingerMenu::WidgetFingerMenu(WidgetEngine *widgetEngine) : 
  WidgetContainer(widgetEngine,"Finger Menu")
{
  circleRadius=core->getConfigStore()->getDoubleValue("Graphic/Widget/FingerMenu","radiusCircle",__FILE__,__LINE__)*widgetEngine->getScreen()->getDPI();
  angleOffset=FloatingPoint::degree2rad(core->getConfigStore()->getDoubleValue("Graphic/Widget/FingerMenu","angleOffset",__FILE__,__LINE__));
  rowDistance=core->getConfigStore()->getDoubleValue("Graphic/Widget/FingerMenu","distanceRow",__FILE__,__LINE__)*widgetEngine->getScreen()->getDPI();
  rowOffsetY=core->getConfigStore()->getDoubleValue("Graphic/Widget/FingerMenu","rowOffsetY",__FILE__,__LINE__)*widgetEngine->getScreen()->getDPI();
  animationDuration=core->getConfigStore()->getIntValue("Graphic/Widget/FingerMenu","animationDuration",__FILE__,__LINE__);
  cursorInfoClosedOffsetY=core->getConfigStore()->getDoubleValue("Graphic/Widget/FingerMenu","cursorInfoClosedOffsetY",__FILE__,__LINE__)*widgetEngine->getScreen()->getDPI();
  cursorInfoOpenedOffsetY=core->getConfigStore()->getDoubleValue("Graphic/Widget/FingerMenu","cursorInfoOpenedOffsetY",__FILE__,__LINE__)*widgetEngine->getScreen()->getDPI();
  closeTimeout=core->getConfigStore()->getIntValue("Graphic/Widget/FingerMenu","closeTimeout",__FILE__,__LINE__);
  closeTimestamp=0;
  pathNameFontString=NULL;
  cursorInfo=NULL;
  opened=false;
  pathNearby=false;
  addressPointNearby=false;
  stateChanged=true;
}

// Destructor
WidgetFingerMenu::~WidgetFingerMenu() {
  if (pathNameFontString) widgetEngine->getFontEngine()->destroyString(pathNameFontString);  
  deinit();
}

// Removes all widgets
void WidgetFingerMenu::deinit(bool deleteWidgets) {
  WidgetContainer::deinit(deleteWidgets);
}

// Adds a widget to the given list
void WidgetFingerMenu::addWidgetToList(WidgetPrimitive *primitive, std::list<WidgetPrimitive*> *list) {

  // Configure the primitive
  primitive->setScale(0);
  primitive->setColor(primitive->getActiveColor());

  // Call the inhertied function
  WidgetContainer::addWidget(primitive);
  list->push_back(primitive);
}

// Adds a widget to the circle of the menu
void WidgetFingerMenu::addWidgetToCircle(WidgetPrimitive *primitive) {
  addWidgetToList(primitive,&circleWidgets);
}

// Adds a widget to the path row of the menu
void WidgetFingerMenu::addWidgetToPathRow(WidgetPrimitive *primitive) {
  addWidgetToList(primitive,&pathRowWidgets);
}

// Adds a widget to the address point row of the menu
void WidgetFingerMenu::addWidgetToAddressPointRow(WidgetPrimitive *primitive) {
  addWidgetToList(primitive,&addressPointRowWidgets);
}

// Sets the cursor info 
void WidgetFingerMenu::setCursorInfoWidget(WidgetCursorInfo *primitive) {
  if (cursorInfo==NULL) {
    cursorInfo=primitive;
    WidgetContainer::addWidget(primitive);
    cursorInfo->onDataChange();
  }
}

// Puts the widgets on a circle
void WidgetFingerMenu::positionWidgetOnCircle(TimestampInMicroseconds t, Int x, Int y, double radius) {
  double angle=2*M_PI-M_PI/2-angleOffset;
  double angleStep=(2*M_PI-2*angleOffset)/(circleWidgets.size()-1);
  for(std::list<WidgetPrimitive*>::iterator i=circleWidgets.begin();i!=circleWidgets.end();i++) {
    WidgetPrimitive *primitive = (WidgetPrimitive*) *i;
    //DEBUG("name=%s",primitive->getName().front().c_str());
    primitive->setTranslateAnimation(t,
      primitive->getX(),
      primitive->getY(),
      x+radius*cos(angle)-primitive->getIconWidth()/2,
      y+radius*sin(angle)-primitive->getIconHeight()/2,
      false,
      animationDuration,
      GraphicTranslateAnimationTypeAccelerated);
    primitive->setScaleAnimation(t,
      primitive->getScale(),
      1.0,
      false,
      animationDuration);
    primitive->setIsHidden(false);    
    angle-=angleStep;
  }
}

// Puts the widgets on a row
void WidgetFingerMenu::positionWidgetOnRow(TimestampInMicroseconds t, std::list<WidgetPrimitive*> *list, Int x, Int y, double distance) {
  Int tx=x-(list->size()-1)*distance/2;
  //DEBUG("tx=%d",tx);
  for(std::list<WidgetPrimitive*>::iterator i=list->begin();i!=list->end();i++) {
    WidgetPrimitive *primitive = (WidgetPrimitive*) *i;
    //DEBUG("name=%s x=%d y=%d",primitive->getName().front().c_str(),primitive->getX(),primitive->getY());
    primitive->setTranslateAnimation(t,
      primitive->getX(),
      primitive->getY(),
      tx-primitive->getIconWidth()/2,
      y-primitive->getIconHeight()/2,
      false,
      animationDuration,
      GraphicTranslateAnimationTypeAccelerated);
    primitive->setIsHidden(false);
    tx+=distance;
  }
}

// Opens the finger menu
void WidgetFingerMenu::open() {
  TimestampInMicroseconds t=core->getClock()->getMicrosecondsSinceStart();
  positionWidgetOnCircle(t, 0, 0, circleRadius);
  positionWidgetOnRow(t, &pathRowWidgets, 0, rowOffsetY, rowDistance);
  positionWidgetOnRow(t, &addressPointRowWidgets, 0, rowOffsetY, rowDistance);
  if (cursorInfo) {
    cursorInfo->setX(0);
    cursorInfo->changeState(cursorInfoOpenedOffsetY,true,animationDuration);
  }
  closeTimestamp=core->getClock()->getMicrosecondsSinceStart()+closeTimeout;
  opened=true;
  stateChanged=true;
}

// Closes the finger menu
void WidgetFingerMenu::close() {
  TimestampInMicroseconds t=core->getClock()->getMicrosecondsSinceStart();
  std::list<WidgetPrimitive*> drawingList = pathRowWidgets;
  drawingList.insert(drawingList.end(), circleWidgets.begin(), circleWidgets.end());
  drawingList.insert(drawingList.end(), addressPointRowWidgets.begin(), addressPointRowWidgets.end());
  for(std::list<WidgetPrimitive*>::iterator i=drawingList.begin();i!=drawingList.end();i++) {
    WidgetPrimitive *primitive = (WidgetPrimitive*) *i;
    primitive->setTranslateAnimation(t,
      primitive->getX(),
      primitive->getY(),
      -primitive->getIconWidth()/2,
      -primitive->getIconHeight()/2,
      false,
      animationDuration,
      GraphicTranslateAnimationTypeAccelerated);
    primitive->setScaleAnimation(t,
      primitive->getScale(),
      0.0,
      false,
      animationDuration);
  }
  if (cursorInfo) {
    cursorInfo->changeState(cursorInfoClosedOffsetY,false,animationDuration);
  }
  opened=false;
}

// Called when the page is touched
bool WidgetFingerMenu::onTouchDown(TimestampInMicroseconds t, Int x, Int y) {
  closeTimestamp=t+closeTimeout;
  bool hit = WidgetContainer::onTouchDown(t,x,y);
  return hit;
}

// Let the page work
bool WidgetFingerMenu::work(TimestampInMicroseconds t) {

  // Only work if opened
  if (opened) {

    // Check if we need to take action
    if ((stateChanged)||(addressPointNearby!=cursorInfo->getAddressPointNearby())||(pathNearby!=cursorInfo->getPathNearby())) {
      //DEBUG("stateChanged=%d addressPointNearby=%d pathNearby=%d",stateChanged,addressPointNearby,pathNearby);
      addressPointNearby=cursorInfo->getAddressPointNearby();
      pathNearby=cursorInfo->getPathNearby();
      //DEBUG("stateChanged=%d addressPointNearby=%d pathNearby=%d",stateChanged,addressPointNearby,pathNearby);

      // First scale the address point nearby widgets
      double scaleTarget;
      if (addressPointNearby)
        scaleTarget=1.0;
      else
        scaleTarget=0.0;
      //DEBUG("scaleTarget=%f animationDuration=%ld",scaleTarget,animationDuration);
      for(std::list<WidgetPrimitive*>::iterator i=addressPointRowWidgets.begin();i!=addressPointRowWidgets.end();i++) {
        WidgetPrimitive *primitive = (WidgetPrimitive*) *i;
        primitive->setScaleAnimation(t,
          primitive->getScale(),
          scaleTarget,
          false,
          animationDuration);
      }

      // Second scale the path nearby widgets
      if ((pathNearby)&&(!addressPointNearby))
        scaleTarget=1.0;
      else
        scaleTarget=0.0;
      //DEBUG("scaleTarget=%f animationDuration=%ld",scaleTarget,animationDuration);
      for(std::list<WidgetPrimitive*>::iterator i=pathRowWidgets.begin();i!=pathRowWidgets.end();i++) {
        WidgetPrimitive *primitive = (WidgetPrimitive*) *i;
        primitive->setScaleAnimation(t,
          primitive->getScale(),
          scaleTarget,
          false,
          animationDuration);
      }
      stateChanged=false;
    }

    // If timeout is reached, close menu
    if (t>=closeTimestamp) 
      close();

  } 

  // Handle the visibility of the widget
  std::list<GraphicPrimitive*> *drawingList = graphicObject.getDrawList();
  for(std::list<GraphicPrimitive*>::iterator i=drawingList->begin();i!=drawingList->end();i++) {
    WidgetPrimitive *primitive = (WidgetPrimitive*) *i;
    if (primitive->getWidgetType()!=WidgetTypeCursorInfo) {
      if ((primitive->getScale()==0)&&(!primitive->getIsHidden()))
        primitive->setIsHidden(true);
      if ((primitive->getScale()!=0)&&(primitive->getIsHidden()))
        primitive->setIsHidden(false);
    }
  }

  return false;
}

}
