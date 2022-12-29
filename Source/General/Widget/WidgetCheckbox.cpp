//============================================================================
// Name        : WidgetCheckbox.cpp
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
#include <WidgetCheckbox.h>
#include <Commander.h>
#include <WidgetEngine.h>
#include <WidgetContainer.h>
#include <GraphicEngine.h>

namespace GEODISCOVERER {

WidgetCheckbox::WidgetCheckbox(WidgetContainer *widgetContainer) : WidgetPrimitive(widgetContainer) {
  widgetType=WidgetTypeCheckbox;
  checkedTexture=Screen::getTextureNotDefined();
  uncheckedTexture=Screen::getTextureNotDefined();
  nextUpdateTime=0;
}

WidgetCheckbox::~WidgetCheckbox() {
  texture=Screen::getTextureNotDefined();
  if ((checkedTexture!=Screen::getTextureNotDefined())&&(destroyTexture)) {
    screen->destroyTextureInfo(checkedTexture,"WidgetCheckbox (checked texture)");
    checkedTexture=Screen::getTextureNotDefined();
  }
  if ((uncheckedTexture!=Screen::getTextureNotDefined())&&(destroyTexture)) {
    screen->destroyTextureInfo(uncheckedTexture,"WidgetCheckbox (unchecked texture)");
    uncheckedTexture=Screen::getTextureNotDefined();
  }
}

// Executed if the widget is currently touched
void WidgetCheckbox::onTouchDown(TimestampInMicroseconds t, Int x, Int y) {
  WidgetPrimitive::onTouchDown(t,x,y);
}

// Executed every time the graphic engine needs to draw
bool WidgetCheckbox::work(TimestampInMicroseconds t) {
  bool changed=WidgetPrimitive::work(t);
  if (t>nextUpdateTime) {
    Int checked=core->getConfigStore()->getIntValue(stateConfigPath,stateConfigName,__FILE__, __LINE__);
    //DEBUG("%s=%d",stateConfigName.c_str(),checked);
    update(checked,false);
    nextUpdateTime=t+updateInterval;
  }
  return changed;
}

// Updates the state of the check box
void WidgetCheckbox::update(bool checked, bool executeCommand) {
  if (!checked) {
    if (executeCommand) {
      widgetContainer->getWidgetEngine()->queueCommand(uncheckedCommand,this);
    } else {
      texture=uncheckedTexture;
    }
    //DEBUG("executing unchecked command",NULL);
  } else {
    //DEBUG("executing checked command",NULL);
    if (executeCommand) {
      widgetContainer->getWidgetEngine()->queueCommand(checkedCommand,this);
    } else {
      texture=checkedTexture;
    }
  }
  isUpdated=true;
}

// Executed if the widget has been untouched
void WidgetCheckbox::onTouchUp(TimestampInMicroseconds t, Int x, Int y, bool cancel) {
  WidgetPrimitive::onTouchUp(t,x,y,cancel);
  if (getIsHit()) {
    Int checked=core->getConfigStore()->getIntValue(stateConfigPath,stateConfigName,__FILE__, __LINE__);
    update(1-checked,true);
  }
}

// Executed every time the graphic engine needs to draw
void WidgetCheckbox::draw(TimestampInMicroseconds t) {

  // Get the fade scale depending on ambiet mode
  double fadeScale=widgetContainer->getWidgetEngine()->getGraphicEngine()->getAmbientFadeScale();
  if (fadeScale!=1.0)
    screen->setAlphaScale(fadeScale);
  if (fadeScale>0) {
    WidgetPrimitive::draw(t);
  }
  if (fadeScale!=1.0)
    screen->setAlphaScale(1.0);
}

}
