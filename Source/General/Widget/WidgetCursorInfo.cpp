//============================================================================
// Name        : WidgetCursorInfo.cpp
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
WidgetCursorInfo::WidgetCursorInfo(WidgetPage *widgetPage) : WidgetPrimitive(widgetPage) {
  widgetType=WidgetTypeCursorInfo;
  labelWidth=0;
  infoFontString=NULL;
  info="";
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

  // Shall we fade in the information?
  if (fadeIn) {

    // Update the text
    //DEBUG("setting text to <%s>", info.c_str());
    updateInfoFontString();

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

  // Check if an address point was hit
  GraphicPosition visPos=*(core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__));
  core->getDefaultGraphicEngine()->unlockPos();
  std::string name = core->getNavigationEngine()->getAddressPointName(visPos);
  if ((info=="")&&(name!="")) {
    info=name;
    fadeIn=true;
    fadeOut=false;
  }
  if ((info!="")&&(name=="")) {
    info="";
    fadeOut=true;
    fadeIn=false;
  }
}

} /* namespace GEODISCOVERER */
