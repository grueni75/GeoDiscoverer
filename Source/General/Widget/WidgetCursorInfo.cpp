//============================================================================
// Name        : WidgetCursorInfo.cpp
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

namespace GEODISCOVERER {

// Constructor
WidgetCursorInfo::WidgetCursorInfo(WidgetPage *widgetPage) : WidgetPrimitive(widgetPage) {
  widgetType=WidgetTypeCursorInfo;
  labelWidth=0;
  infoFontString=NULL;
  info="";
  updateInfo=true;
  isHidden=true; // By intention set to hidden to prevent fade out after timeout
}

// Destructor
WidgetCursorInfo::~WidgetCursorInfo() {
  widgetPage->getFontEngine()->lockFont("sansBoldLarge",__FILE__, __LINE__);
  if (infoFontString) widgetPage->getFontEngine()->destroyString(infoFontString);
  widgetPage->getFontEngine()->unlockFont();
}

void WidgetCursorInfo::updateInfoFontString() {
  if (info=="")
    return;
  FontEngine *fontEngine=widgetPage->getFontEngine();
  fontEngine->lockFont("sansBoldLarge",__FILE__, __LINE__);
  Int width = labelWidth*(double)widgetPage->getScreen()->getWidth()/100.0;
  fontEngine->updateString(&infoFontString,info,width);
  fontEngine->unlockFont();
  infoFontString->setX(getX() - infoFontString->getIconWidth() / 2);
  infoFontString->setY(getY());
  //DEBUG("width=%d infoFontString->getWidth()=%d getX()=%d getY()=%d",width,infoFontString->getIconWidth(),getX(),getY());
}

// Executed every time the graphic engine checks if drawing is required
bool WidgetCursorInfo::work(TimestampInMicroseconds t) {

  Int textX, textY;
  std::list<std::string> status;

  // Do the inherited stuff
  bool changed=WidgetPrimitive::work(t);

  // Shall we update the text?
  if (updateInfo) {
    updateInfoFontString();
    updateInfo=false;
  }
  
  // Shall we fade in the information?
  if (fadeIn) {

    // Is a fade anim already ongoing?
    if (fadeStartTime!=fadeEndTime) {

      // Is it a fade in anim?
      if (fadeEndColor==this->activeColor) {

        // No need to start it again
        //DEBUG("text being faded in", NULL);
        fadeIn=false;
      }
    } else {

      // Is the text already faded in?
      if (color==this->activeColor) {

        // No need to start a fade in anim
        fadeIn=false;
        //DEBUG("text already faded in", NULL);
      }
    }
    if (fadeIn) {
      //DEBUG("fading in", NULL);
      setFadeAnimation(t,color,this->activeColor,false,widgetPage->getGraphicEngine()->getFadeDuration());
    }
    fadeIn=false;
  }

  // Shall we fade out the information?
  if (fadeOut) {

    // Is a fade anim already ongoing?
    if (fadeStartTime!=fadeEndTime) {

      // Is it a fade out anim?
      if (fadeEndColor==GraphicColor(255,255,255,0)) {

        // No need to start it again
        fadeOut=false;
        //DEBUG("text being faded out", NULL);
      }
    } else {

      // Is the text already faded out?
      if (color==GraphicColor(255,255,255,0)) {

        // No need to start a fade out anim
        fadeOut=false;
        //DEBUG("text already faded out", NULL);
      }
    }
    if (fadeOut) {
      setFadeAnimation(t,color,GraphicColor(255,255,255,0),false,widgetPage->getGraphicEngine()->getFadeDuration());
      //DEBUG("fading out", NULL);
    }
    fadeOut=false;
  }

  // Return result
  return changed;
}

// Executed every time the graphic engine needs to draw
void WidgetCursorInfo::draw(TimestampInMicroseconds t) {

  // Let the primitive draw the background
  WidgetPrimitive::draw(t);

  // Draw the status
  if (color.getAlpha()!=0) {
    if (infoFontString) {
      infoFontString->setColor(color);
      infoFontString->draw(t);
    }
  }

}

// Called when the widget has changed its position
void WidgetCursorInfo::updatePosition(Int x, Int y, Int z) {
  WidgetPrimitive::updatePosition(x,y,z);
  updateInfoFontString();
}

// Called when the map has changed
void WidgetCursorInfo::onMapChange(bool widgetVisible, MapPosition pos) {
  onDataChange();
}


// Called when some data has changed
void WidgetCursorInfo::onDataChange() {

  // Check if an address point was hit
  GraphicPosition visPos=*(core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__));
  core->getDefaultGraphicEngine()->unlockPos();
  std::string name = core->getNavigationEngine()->getAddressPointName(visPos);
  //DEBUG("name=%s",name.c_str());
  if ((info=="")&&(name!="")) {
    info=name;
    fadeIn=true;
    fadeOut=false;
    updateInfo=true;
  }
  if ((info!="")&&(name=="")) {
    info="";
    fadeOut=true;
    fadeIn=false;
    updateInfo=true;
  }
  if (info!=name) {
    info=name;
    updateInfo=true;
  }
}


} /* namespace GEODISCOVERER */
