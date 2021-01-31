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
    update(checked,false);
    nextUpdateTime=t+updateInterval;
  }
  return changed;
}

// Updates the state of the check box
bool WidgetCheckbox::update(bool checked, bool executeCommand) {
  if (!checked) {
    if (executeCommand) {
      std::string result=core->getCommander()->execute(uncheckedCommand);
      if (result=="false")
        return false;
    }
    texture=uncheckedTexture;
    //DEBUG("executing unchecked command",NULL);
  } else {
    //DEBUG("executing checked command",NULL);
    if (executeCommand) {
      std::string result=core->getCommander()->execute(checkedCommand);
      if (result=="false")
        return false;
    }
    texture=checkedTexture;
  }
  isUpdated=true;
  return true;
}

// Executed if the widget has been untouched
void WidgetCheckbox::onTouchUp(TimestampInMicroseconds t, Int x, Int y, bool cancel) {
  WidgetPrimitive::onTouchUp(t,x,y,cancel);
  if (getIsHit()) {
    Int checked=core->getConfigStore()->getIntValue(stateConfigPath,stateConfigName,__FILE__, __LINE__);
    if (update(1-checked,true))
      checked=1-checked;
  }
}


}
