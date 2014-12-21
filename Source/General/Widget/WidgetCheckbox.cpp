//============================================================================
// Name        : WidgetCheckbox.cpp
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
void WidgetCheckbox::onTouchUp(TimestampInMicroseconds t, Int x, Int y) {
  WidgetPrimitive::onTouchUp(t,x,y);
  if (getIsHit()) {
    Int checked=core->getConfigStore()->getIntValue(stateConfigPath,stateConfigName,__FILE__, __LINE__);
    if (update(1-checked,true))
      checked=1-checked;
  }
}


}
