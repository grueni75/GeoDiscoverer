//============================================================================
// Name        : WidgetCheckbox.cpp
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

WidgetCheckbox::WidgetCheckbox() : WidgetPrimitive() {
  widgetType=WidgetTypeCheckbox;
  checkedTexture=core->getScreen()->getTextureNotDefined();
  uncheckedTexture=core->getScreen()->getTextureNotDefined();
  nextUpdateTime=0;
}

WidgetCheckbox::~WidgetCheckbox() {
  deinit();
}

// Deinits the checkbox
void WidgetCheckbox::deinit() {
  texture=core->getScreen()->getTextureNotDefined();
  if ((checkedTexture!=core->getScreen()->getTextureNotDefined())&&(destroyTexture)) {
    core->getScreen()->destroyTextureInfo(checkedTexture,"WidgetCheckbox (checked texture)");
    checkedTexture=core->getScreen()->getTextureNotDefined();
  }
  if ((uncheckedTexture!=core->getScreen()->getTextureNotDefined())&&(destroyTexture)) {
    core->getScreen()->destroyTextureInfo(uncheckedTexture,"WidgetCheckbox (unchecked texture)");
    uncheckedTexture=core->getScreen()->getTextureNotDefined();
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
    Int checked=core->getConfigStore()->getIntValue(stateConfigPath,stateConfigName);
    update(checked,false);
    nextUpdateTime=t+updateInterval;
  }
  return changed;
}

// Updates the state of the check box
bool WidgetCheckbox::update(bool checked, bool executeCommand) {
  if (!checked) {
    if (executeCommand) {
      std::string result=core->getCommander()->execute(uncheckedCommand,true);
      if (result=="false")
        return false;
    }
    texture=uncheckedTexture;
    //DEBUG("executing unchecked command",NULL);
  } else {
    //DEBUG("executing checked command",NULL);
    if (executeCommand) {
      std::string result=core->getCommander()->execute(checkedCommand,true);
      if (result=="false")
        return false;
    }
    texture=checkedTexture;
  }
  isUpdated=true;
  return true;
}

// Executed if the widget has been untouched
void WidgetCheckbox::onTouchUp(TimestampInMicroseconds t, Int x, Int y) {
  WidgetPrimitive::onTouchUp(t,x,y);
  if (getIsHit()) {
    Int checked=core->getConfigStore()->getIntValue(stateConfigPath,stateConfigName);
    if (update(1-checked,true))
      checked=1-checked;
  }
}


}
