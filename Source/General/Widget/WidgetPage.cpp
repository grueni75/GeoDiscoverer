//============================================================================
// Name        : WidgetPage.cpp
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
#include <WidgetPage.h>
#include <WidgetEngine.h>
#include <GraphicEngine.h>

namespace GEODISCOVERER {

// Constructor
WidgetPage::WidgetPage(WidgetEngine *widgetEngine, std::string name) : 
  WidgetContainer(widgetEngine)
{
  this->name=name;
  widgetsActive=false;
  hiddenAnimationDuration=core->getConfigStore()->getIntValue("Graphic/Widget","hiddenAnimationDuration",__FILE__, __LINE__);
}

// Destructor
WidgetPage::~WidgetPage() {
  deinit();
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
          if ((primitive->getXHidden()!=0)||(primitive->getYHidden()!=0)) {
            widgetEngine->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
            primitive->setTranslateAnimation(t,primitive->getXHidden(),primitive->getYHidden(),primitive->getXOriginal(),primitive->getYOriginal(),false,hiddenAnimationDuration,GraphicTranslateAnimationTypeAccelerated);
            widgetEngine->getGraphicEngine()->unlockDrawing();
          }
        } else {
          widgetEngine->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
          primitive->setFadeAnimation(t,primitive->getColor(),primitive->getInactiveColor(),false,widgetEngine->getGraphicEngine()->getFadeDuration());
          widgetEngine->getGraphicEngine()->unlockDrawing();
          //primitive->setColor(primitive->getInactiveColor());
          if ((primitive->getXHidden()!=0)||(primitive->getYHidden()!=0)) {
            widgetEngine->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
            primitive->setTranslateAnimation(t,primitive->getX(),primitive->getY(),primitive->getXHidden(),primitive->getYHidden(),false,hiddenAnimationDuration,GraphicTranslateAnimationTypeAccelerated);
            widgetEngine->getGraphicEngine()->unlockDrawing();
          }
        }
      }
    }
    this->widgetsActive=widgetsActive;
  }
  if (widgetsActive) {
    touchEndTime=t;
  }
}

// Called when the page is touched
bool WidgetPage::onTouchDown(TimestampInMicroseconds t, Int x, Int y) {

  // Call the inhertied function
  if (!WidgetContainer::onTouchDown(t,x,y))
    return false;

  // Do the fade in animation
  if ((selectedWidget)&&(!widgetsActive)) {
    setWidgetsActive(t,true);
  }

  return true;
}

// Let the page work
bool WidgetPage::work(TimestampInMicroseconds t) {
  if ((widgetsActive)&&(firstTouch)&&(t>touchEndTime+getWidgetEngine()->getWidgetsActiveTimeout())) {
    setWidgetsActive(t,false);
    return true;
  }
  return false;
}

}
