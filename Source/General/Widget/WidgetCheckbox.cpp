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

WidgetCheckbox::WidgetCheckbox() {
  widgetType=WidgetCheckboxType;
  firstTime=true;
}

WidgetCheckbox::~WidgetCheckbox() {
}

// Executed if the widget is currently touched
void WidgetCheckbox::onTouchDown(TimestampInMicroseconds t, Int x, Int y) {
  WidgetPrimitive::onTouchDown(t,x,y);
}

// Executed every time the graphic engine needs to draw
bool WidgetCheckbox::work(TimestampInMicroseconds t) {
  bool changed=WidgetPrimitive::work(t);
  if (firstTime) {
    Int checked=core->getConfigStore()->getIntValue(stateConfigPath,stateConfigName);
    update(checked,false);
    firstTime=false;
  }
  return changed;
}

// Updates the state of the check box
void WidgetCheckbox::update(bool checked, bool executeCommand) {
  if (!checked) {
    texture=uncheckedTexture;
    //DEBUG("executing unchecked command",NULL);
    if (executeCommand)
      core->getCommander()->execute(uncheckedCommand);
  } else {
    texture=checkedTexture;
    //DEBUG("executing checked command",NULL);
    if (executeCommand)
      core->getCommander()->execute(checkedCommand);
  }
  isUpdated=true;
}

// Executed if the widget has been untouched
void WidgetCheckbox::onTouchUp(TimestampInMicroseconds t, Int x, Int y) {
  WidgetPrimitive::onTouchUp(t,x,y);
  if (getIsHit()) {
    Int checked=core->getConfigStore()->getIntValue(stateConfigPath,stateConfigName);
    if (checked) {
      checked=0;
    } else {
      checked=1;
    }
    update(checked,true);
  }
}


}
