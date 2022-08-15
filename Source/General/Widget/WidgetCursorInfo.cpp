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
#include <WidgetCursorInfo.h>
#include <WidgetContainer.h>
#include <FontEngine.h>
#include <GraphicEngine.h>
#include <MapPosition.h>
#include <NavigationPath.h>
#include <WidgetEngine.h>
#include <NavigationEngine.h>
#include <MapEngine.h>
#include <UnitConverter.h>

namespace GEODISCOVERER {

// Constructor
WidgetCursorInfo::WidgetCursorInfo(WidgetContainer *widgetContainer) : WidgetPrimitive(widgetContainer) {
  widgetType=WidgetTypeCursorInfo;
  infoFontString=NULL;
  info="";
  updateInfo=true;
  isHidden=true; // By intention set to hidden to prevent fade out after timeout
  fadeIn=false;
  fadeOut=false;
  updateInfo=false;
  permanentVisible=false;
  maxPathDistance=core->getConfigStore()->getDoubleValue("Graphic/Widget","maxPathDistance",__FILE__, __LINE__)*widgetContainer->getScreen()->getDPI();
  pathNearby=false;
  addressPointNearby=false;
  infoKeepEndCharCount=-1;
}

// Destructor
WidgetCursorInfo::~WidgetCursorInfo() {
  widgetContainer->getFontEngine()->lockFont("sansBoldLarge",__FILE__, __LINE__);
  if (infoFontString) widgetContainer->getFontEngine()->destroyString(infoFontString);
  widgetContainer->getFontEngine()->unlockFont();
}

void WidgetCursorInfo::updateInfoFontString() {
  if (info=="")
    return;
  FontEngine *fontEngine=widgetContainer->getFontEngine();
  fontEngine->lockFont("sansBoldLarge",__FILE__, __LINE__);
  fontEngine->updateString(&infoFontString,info,width,infoKeepEndCharCount);
  fontEngine->unlockFont();
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
      setFadeAnimation(t,color,this->activeColor,false,widgetContainer->getGraphicEngine()->getAnimDuration());
      setScaleAnimation(t,scale,1.0,false,widgetContainer->getGraphicEngine()->getAnimDuration());
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
      //DEBUG("fading out", NULL);
      setFadeAnimation(t,color,GraphicColor(255,255,255,0),false,widgetContainer->getGraphicEngine()->getAnimDuration());
      setScaleAnimation(t,scale,0.0,false,widgetContainer->getGraphicEngine()->getAnimDuration());
    }
    fadeOut=false;
  }

  // Update the coordinates
  if (infoFontString) {
    infoFontString->setX(-infoFontString->getIconWidth() / 2);
    infoFontString->setY(-infoFontString->getBaselineOffsetY() + (height-infoFontString->getIconHeight())/2 -height/2);
  }

  // Change hidden state
  if (getIsHidden()) {
    if (scale!=0) {
      setIsHidden(false);
    }
  } else {
    if (scale==0) {
      setIsHidden(true);
    }
  }

  // Return result
  return changed;
}

// Executed every time the graphic engine needs to draw
void WidgetCursorInfo::draw(TimestampInMicroseconds t) {

  // Let the primitive draw the background
  //WidgetPrimitive::draw(t);

  // Do not draw if primitive is hidden
  if (getIsHidden())
    return;

 // Get the fade scale depending on ambiet mode
  double fadeScale=widgetContainer->getWidgetEngine()->getGraphicEngine()->getAmbientFadeScale();
  if (fadeScale!=1.0)
    screen->setAlphaScale(fadeScale);
  if (fadeScale>0) {

    // Draw the status
    if (color.getAlpha()!=0) {
      if (infoFontString) {
        screen->startObject();
        screen->translate(x,y,0);
        screen->scale(scale,scale,1.0);
        Int w;
        double widthScale;
        if ((t>=translateStartTime)&&(t<translateEndTime)&&(translateEndY!=translateStartY)) {
          widthScale = 1.0-fabs(((double)(translateEndY-y))/((double)(translateEndY-translateStartY)));
          //DEBUG("info=%s fadeIn=%d fadeOut=%d updateInfo=%d permanentVisible=%d maxPathDistance=%f pathNearby=%d infoKeepEndCharCount=%d infoFontString=0x%08x",info.c_str(),fadeIn,fadeOut,updateInfo,permanentVisible,maxPathDistance,pathNearby,infoKeepEndCharCount,infoFontString);
        } else
          widthScale = 1.0;
        if (permanentVisible) {
          w=infoFontString->getIconWidth()+widthScale*(width-infoFontString->getIconWidth());        
        } else {
          w=width-widthScale*(width-infoFontString->getIconWidth());
        }
        screen->setColor(getColor().getRed(),getColor().getGreen(),getColor().getBlue(),color.getAlpha()*getInactiveColor().getAlpha()/255);
        screen->drawRoundedRectangle(w,height);
        infoFontString->setColor(color);
        infoFontString->draw(t);
        screen->endObject();
      }
    }
  }
  if (fadeScale!=1.0)
    screen->setAlphaScale(1.0);
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
  infoKeepEndCharCount=-1;
  NavigationPoint addressPoint;
  std::string name;
  if (core->getNavigationEngine()->getAddressPoint(visPos,addressPoint)) {

    // Address point found
    name=addressPoint.getName();
    addressPointNearby=true;
    
  } else {

    // No address point nearby
    addressPointNearby=false;

    // Now check if an navigation path is hit
    MapPosition nearestPathMapPos;
    NavigationPath *nearestPath = widgetContainer->getWidgetEngine()->getNearestPath(NULL,&nearestPathMapPos);
    //DEBUG("nearestPath=0x%08x",nearestPath);
    if (nearestPath!=NULL) {
      /*MapPosition mapCenterMapPos = *(core->getMapEngine()->lockMapPos(__FILE__,__LINE__));
      core->getMapEngine()->unlockMapPos();
      double distance;
      bool success=core->getMapEngine()->calculateDistanceInScreenPixels(mapCenterMapPos,nearestPathMapPos,distance);
      //DEBUG("success=%d distance=%f",success,distance);
      if ((success)&&(distance<maxPathDistance)) {*/

      // Compute the overlap in meters for the given map state
      double overlapInMeters;
      if (!core->getMapEngine()->calculateMaxDistanceInMeters(maxPathDistance,overlapInMeters)) {
        overlapInMeters=0;
      }

      // Compute the distance on the path
      MapPosition normalPos;
      std::stringstream postfix;
      double distance=nearestPath->computeDistance(nearestPathMapPos,overlapInMeters,normalPos);
      if (distance>0) {
        /*double t;
        success=core->getMapEngine()->calculateDistanceInScreenPixels(mapCenterMapPos,normalPos,t);
        DEBUG("normalPosDistance=%f success=%d distance=%f t=%f",mapCenterMapPos.computeDistance(normalPos),success,distance,t);
        if ((success)&&(t<maxPathDistance)) {
        }*/
        std::string value;
        std::string unit;
        core->getUnitConverter()->formatMeters(distance,value,unit,1,"km");
        postfix << ": " << value << " " << unit;
      }

      // Compute the distance to the selected point
      infoKeepEndCharCount=postfix.str().length();
      if (infoKeepEndCharCount==0)
        infoKeepEndCharCount=-1;
      name=nearestPath->getGpxFilename() + postfix.str();        
      pathNearby=true;

    } else {
      pathNearby=false;
    }
  }
  if (permanentVisible) {
    if (name=="") 
      name="Nothing nearby";
  }
  //DEBUG("info=%s name=%s",info.c_str(),name.c_str());
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

// Changes the state of the widget
void WidgetCursorInfo::changeState(Int x, Int y, bool permanentVisible, TimestampInMicroseconds animationDuration) {

  // Ensure that we are not drawing

  // Setup the animations
  widgetContainer->getGraphicEngine()->lockDrawing(__FILE__,__LINE__);
  this->permanentVisible=permanentVisible;
  TimestampInMicroseconds t=core->getClock()->getMicrosecondsSinceStart();
  setTranslateAnimation(t,getX(),getY(),x,y,false,animationDuration,GraphicTranslateAnimationTypeAccelerated);
  widgetContainer->getGraphicEngine()->unlockDrawing();

  // Update the fields
  onDataChange();
  updateInfo=true;
}


} /* namespace GEODISCOVERER */
