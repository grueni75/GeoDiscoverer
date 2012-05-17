//============================================================================
// Name        : WidgetPage.cpp
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
WidgetPage::WidgetPage(std::string name) {
  this->name=name;
  widgetsActive=false;
  touchStartedOutside=false;
  firstTouch=true;
  selectedWidget=NULL;
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
          if (primitive==selectedWidget)
            primitive->setFadeAnimation(t,primitive->getColor(),core->getWidgetEngine()->getSelectedWidgetColor());
          else
            primitive->setFadeAnimation(t,primitive->getColor(),primitive->getActiveColor());
          //primitive->setColor(primitive->getActiveColor());
        } else {
          primitive->setFadeAnimation(t,primitive->getColor(),primitive->getInactiveColor());
          //primitive->setColor(primitive->getInactiveColor());
        }
      }
    }
    this->widgetsActive=widgetsActive;
  }
}

// Called when the page is touched
bool WidgetPage::onTouchDown(TimestampInMicroseconds t, Int x, Int y) {

  // Skip the check if it started outside
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
      previousSelectedWidget->setFadeAnimation(t,previousSelectedWidget->getColor(),previousSelectedWidget->getActiveColor());
    }
    if (selectedWidget) {
      selectedWidget->setFadeAnimation(t,selectedWidget->getColor(),core->getWidgetEngine()->getSelectedWidgetColor());
    }
  }

  // If this is the first touch and no widget was hit, do not activate any widgets later
  // This allows smooth scrolling of the map
  if ((firstTouch)&&(!selectedWidget)) {
    setWidgetsActive(t,false);
    touchStartedOutside=true;
    return false;
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
bool WidgetPage::onTouchUp(TimestampInMicroseconds t, Int x, Int y) {

  // Check all widgets
  std::list<GraphicPrimitive*> *drawList=graphicObject.getDrawList();
  if (!touchStartedOutside) {
    for(std::list<GraphicPrimitive*>::iterator i = drawList->begin(); i!=drawList->end(); i++) {
      WidgetPrimitive *primitive=(WidgetPrimitive*)*i;
      primitive->onTouchUp(t,x,y);
    }
    if (selectedWidget) {
      selectedWidget->setFadeAnimation(t,selectedWidget->getColor(),selectedWidget->getActiveColor());
    }
    selectedWidget=NULL;
  }

  // Reset some variables
  firstTouch=true;
  touchStartedOutside=false;
}



}
