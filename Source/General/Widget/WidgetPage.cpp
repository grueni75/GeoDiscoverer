//============================================================================
// Name        : WidgetPage.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#include <Core.h>

namespace GEODISCOVERER {

// Constructor
WidgetPage::WidgetPage(WidgetEngine *widgetEngine, std::string name) : graphicObject(widgetEngine->getScreen()){
  this->name=name;
  widgetsActive=false;
  touchStartedOutside=false;
  firstTouch=true;
  selectedWidget=NULL;
  touchEndTime=0;
  lastTouchStartedOutside=true;
  this->widgetEngine=widgetEngine;
}

// Destructor
WidgetPage::~WidgetPage() {
  deinit();
}

// Removes all widgets
void WidgetPage::deinit(bool deleteWidgets) {
  graphicObject.deinit(deleteWidgets);
}

// Adds a widget to the page
void WidgetPage::addWidget(WidgetPrimitive *primitive) {
  graphicObject.addPrimitive(primitive);
}

// Sets the active state of the widgets
void WidgetPage::setWidgetsActive(TimestampInMicroseconds t, bool widgetsActive) {
  if (this->widgetsActive!=widgetsActive) {
    GraphicPrimitiveMap *primitiveMap=graphicObject.getPrimitiveMap();
    GraphicPrimitiveMap::iterator i;
    for(i = primitiveMap->begin(); i!=primitiveMap->end(); i++) {
      WidgetPrimitive *primitive=(WidgetPrimitive*)i->second;
      if (!primitive->getIsHidden()) {
        if (widgetsActive) {
          if (primitive==selectedWidget) {
            widgetEngine->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
            primitive->setFadeAnimation(t,primitive->getColor(),getWidgetEngine()->getSelectedWidgetColor(),false,widgetEngine->getGraphicEngine()->getFadeDuration());
            widgetEngine->getGraphicEngine()->unlockDrawing();
          } else {
            widgetEngine->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
            primitive->setFadeAnimation(t,primitive->getColor(),primitive->getActiveColor(),false,widgetEngine->getGraphicEngine()->getFadeDuration());
            //primitive->setColor(primitive->getActiveColor());
            widgetEngine->getGraphicEngine()->unlockDrawing();
          }
        } else {
          widgetEngine->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
          primitive->setFadeAnimation(t,primitive->getColor(),primitive->getInactiveColor(),false,widgetEngine->getGraphicEngine()->getFadeDuration());
          widgetEngine->getGraphicEngine()->unlockDrawing();
          //primitive->setColor(primitive->getInactiveColor());
        }
      }
    }
    this->widgetsActive=widgetsActive;
  }
  if (widgetsActive) {
    touchEndTime=t;
  }
}

// Called when a two finger gesture is done on the page
bool WidgetPage::onTwoFingerGesture(TimestampInMicroseconds t, Int dX, Int dY, double angleDiff, double scaleDiff) {

  // Two the two fingure gesture on the selected widget
  if (selectedWidget) {
    selectedWidget->onTwoFingerGesture(t,dX,dY,angleDiff,scaleDiff);
    return true;
  } else {
    return false;
  }
}

// Called when the page is touched
bool WidgetPage::onTouchDown(TimestampInMicroseconds t, Int x, Int y) {

  // Skip the check if it started outside
  touchEndTime=t;
  if (touchStartedOutside)
    return false;

  // Check all widgets
  WidgetPrimitive *previousSelectedWidget=selectedWidget;
  selectedWidget=NULL;
  std::list<GraphicPrimitive*> *drawList=graphicObject.getDrawList();
  for(std::list<GraphicPrimitive*>::iterator i = drawList->begin(); i!=drawList->end(); i++) {
    WidgetPrimitive *primitive=(WidgetPrimitive*)*i;
    primitive->onTouchDown(t,x,y);
    if (primitive->getIsSelected()) {
      selectedWidget=primitive;
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

  // Do the fade in animation
  if ((selectedWidget)&&(!widgetsActive)) {
    setWidgetsActive(t,true);
  }

  // Next touch is not the first one
  firstTouch=false;

  return true;
}

// Called when the page is not touched anymore
void WidgetPage::onTouchUp(TimestampInMicroseconds t, Int x, Int y) {

  // Check all widgets
  std::list<GraphicPrimitive*> *drawList=graphicObject.getDrawList();
  if (!touchStartedOutside) {
    for(std::list<GraphicPrimitive*>::iterator i = drawList->begin(); i!=drawList->end(); i++) {
      WidgetPrimitive *primitive=(WidgetPrimitive*)*i;
      primitive->onTouchUp(t,x,y);
    }
  }
  deselectWidget(t);
  touchEndTime=t;
}

// Called when the map has changed
void WidgetPage::onMapChange(bool pageVisible, MapPosition pos) {

  // Check all widgets
  std::list<GraphicPrimitive*> *drawList=graphicObject.getDrawList();
  for(std::list<GraphicPrimitive*>::iterator i = drawList->begin(); i!=drawList->end(); i++) {
    WidgetPrimitive *primitive=(WidgetPrimitive*)*i;
    primitive->onMapChange(pageVisible, pos);
  }
}

// Called when the location has changed
void WidgetPage::onLocationChange(bool pageVisible, MapPosition pos) {

  // Check all widgets
  std::list<GraphicPrimitive*> *drawList=graphicObject.getDrawList();
  for(std::list<GraphicPrimitive*>::iterator i = drawList->begin(); i!=drawList->end(); i++) {
    WidgetPrimitive *primitive=(WidgetPrimitive*)*i;
    primitive->onLocationChange(pageVisible, pos);
  }
}

// Called when a path has changed
void WidgetPage::onPathChange(bool pageVisible, NavigationPath *path, NavigationPathChangeType changeType) {

  // Check all widgets
  std::list<GraphicPrimitive*> *drawList=graphicObject.getDrawList();
  for(std::list<GraphicPrimitive*>::iterator i = drawList->begin(); i!=drawList->end(); i++) {
    WidgetPrimitive *primitive=(WidgetPrimitive*)*i;
    primitive->onPathChange(pageVisible, path, changeType);
  }
}

// Deselects currently selected widget
void WidgetPage::deselectWidget(TimestampInMicroseconds t) {
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

// Let the page work
bool WidgetPage::work(TimestampInMicroseconds t) {
  if ((widgetsActive)&&(firstTouch)&&(t>touchEndTime+getWidgetEngine()->getWidgetsActiveTimeout())) {
    setWidgetsActive(t,false);
    return true;
  }
  return false;
}

FontEngine *WidgetPage::getFontEngine() {
  return widgetEngine->getFontEngine();
}

WidgetEngine *WidgetPage::getWidgetEngine() {
  return widgetEngine;
}

GraphicEngine *WidgetPage::getGraphicEngine() {
  return widgetEngine->getGraphicEngine();
}

Screen *WidgetPage::getScreen() {
  return widgetEngine->getScreen();
}

}
