//============================================================================
// Name        : WidgetNavigation.cpp
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
WidgetNavigation::WidgetNavigation() : WidgetPrimitive(), turnArrowPointBuffer(24) {
  widgetType=WidgetTypeNavigation;
  updateInterval=1000000;
  nextUpdateTime=0;
  distanceLabelFontString=NULL;
  durationLabelFontString=NULL;
  distanceValueFontString=NULL;
  durationValueFontString=NULL;
  locationBearingActual=0;
  locationBearingActual=0;
  directionChangeDuration=0;
  FontEngine *fontEngine=core->getFontEngine();
  fontEngine->lockFont("sansBoldTiny",__FILE__, __LINE__);
  fontEngine->updateString(&distanceLabelFontString,"Distance");
  fontEngine->updateString(&durationLabelFontString,"Duration");
  fontEngine->unlockFont();
  fontEngine->lockFont("sansTiny",__FILE__, __LINE__);
  for(int i=0;i<4;i++)
    orientationLabelFontStrings[i]=NULL;
  fontEngine->updateString(&orientationLabelFontStrings[0],"N");
  fontEngine->updateString(&orientationLabelFontStrings[1],"E");
  fontEngine->updateString(&orientationLabelFontStrings[2],"S");
  fontEngine->updateString(&orientationLabelFontStrings[3],"W");
  fontEngine->unlockFont();
  targetIcon.setRotateAnimation(0,0,360,true,core->getNavigationEngine()->getTargetRotateDuration(),GraphicRotateAnimationTypeLinear);
  targetObject.addPrimitive(&targetIcon);
  compassObject.addPrimitive(&directionIcon);
  compassObject.addPrimitive(&targetObject);
  hideCompass=true;
  hideTarget=true;
  showTurn=false;
  active=false;
}

// Destructor
WidgetNavigation::~WidgetNavigation() {
  FontEngine *fontEngine=core->getFontEngine();
  fontEngine->lockFont("sansBoldTiny",__FILE__, __LINE__);
  if (distanceLabelFontString) core->getFontEngine()->destroyString(distanceLabelFontString);
  if (durationLabelFontString) core->getFontEngine()->destroyString(durationLabelFontString);
  fontEngine->unlockFont();
  fontEngine->lockFont("sansSmall",__FILE__, __LINE__);
  if (distanceValueFontString) core->getFontEngine()->destroyString(distanceValueFontString);
  if (durationValueFontString) core->getFontEngine()->destroyString(durationValueFontString);
  fontEngine->unlockFont();
  fontEngine->lockFont("sansTiny",__FILE__, __LINE__);
  for(int i=0;i<4;i++) {
    if (orientationLabelFontStrings[i]) core->getFontEngine()->destroyString(orientationLabelFontStrings[i]);
  }
  fontEngine->unlockFont();
}

// Executed every time the graphic engine checks if drawing is required
bool WidgetNavigation::work(TimestampInMicroseconds t) {

  bool update;
  FontEngine *fontEngine=core->getFontEngine();
  NavigationEngine *navigationEngine=core->getNavigationEngine();
  UnitConverter *unitConverter=core->getUnitConverter();
  std::string value, unit;
  std::stringstream infos;

  // Do the inherited stuff
  bool changed=WidgetPrimitive::work(t);

  // Only update the info at given update interval
  if (t>=nextUpdateTime) {

    // Is a turn coming?
    bool activateWidget = false;
    NavigationInfo *navigationInfo=navigationEngine->lockNavigationInfo(__FILE__, __LINE__);
    fontEngine->lockFont("sansSmall",__FILE__, __LINE__);
    if (navigationInfo->getTurnDistance()!=NavigationInfo::getUnknownDistance()) {

      // Get distance to turn
      showTurn=true;
      activateWidget=true;
      unitConverter->formatMeters(navigationInfo->getTurnDistance(),value,unit);
      infos.str("");
      infos << value << " " << unit;
      fontEngine->updateString(&distanceValueFontString,infos.str());

      // Set the arrow to show
      turnArrowPointBuffer.reset();
      Int mirror=1;
      double turnAngle = navigationInfo->getTurnAngle();
      if (turnAngle<0) {
        turnAngle=-turnAngle;
        mirror=-1;
      }
      double sinOfTurnAngle = (double)sin(FloatingPoint::degree2rad(turnAngle));
      double cosOfTurnAngle = (double)cos(FloatingPoint::degree2rad(turnAngle));
      GraphicPoint p1 = GraphicPoint(turnLineStartX-turnLineWidth/2,turnLineStartY);
      GraphicPoint p2 = GraphicPoint(turnLineStartX+turnLineWidth/2,turnLineStartY);
      GraphicPoint p3 = GraphicPoint(p2.getX(),p2.getY()+turnLineStartHeight);
      GraphicPoint p4 = GraphicPoint(p1.getX(),p1.getY()+turnLineStartHeight);
      GraphicPoint pm;
      if (mirror>0) {
        pm=p4;
      } else {
        pm=p3;
      }
      GraphicPoint p5 = GraphicPoint(
          pm.getX()+mirror*(Short)(turnLineWidth*cosOfTurnAngle),
          pm.getY()+(Short)(turnLineWidth*sinOfTurnAngle));
      GraphicPoint p10 = GraphicPoint(
          pm.getX()+mirror*(Short)(turnLineWidth/2*cosOfTurnAngle),
          pm.getY()+(Short)(turnLineWidth/2*sinOfTurnAngle));
      p10.setX(p10.getX()-mirror*(Short)((turnLineMiddleHeight+turnLineArrowHeight)*sinOfTurnAngle));
      p10.setY(p10.getY()+(Short)((turnLineMiddleHeight+turnLineArrowHeight)*cosOfTurnAngle));
      GraphicPoint p6 = GraphicPoint(
          p5.getX()-mirror*(Short)(turnLineMiddleHeight*sinOfTurnAngle),
          p5.getY()+(Short)(turnLineMiddleHeight*cosOfTurnAngle));
      GraphicPoint p7 = GraphicPoint(
          p6.getX()-mirror*(Short)(turnLineWidth*cosOfTurnAngle),
          p6.getY()-(Short)(turnLineWidth*sinOfTurnAngle));
      GraphicPoint p8 = GraphicPoint(
          p7.getX()-mirror*(Short)(turnLineArrowOverhang*cosOfTurnAngle),
          p7.getY()-(Short)(turnLineArrowOverhang*sinOfTurnAngle));
      GraphicPoint p9 = GraphicPoint(
          p6.getX()+mirror*(Short)(turnLineArrowOverhang*cosOfTurnAngle),
          p6.getY()+(Short)(turnLineArrowOverhang*sinOfTurnAngle));
      turnArrowPointBuffer.addPoint(p1);
      turnArrowPointBuffer.addPoint(p2);
      turnArrowPointBuffer.addPoint(p3);
      turnArrowPointBuffer.addPoint(p3);
      turnArrowPointBuffer.addPoint(p1);
      turnArrowPointBuffer.addPoint(p4);
      turnArrowPointBuffer.addPoint(p4);
      turnArrowPointBuffer.addPoint(p3);
      turnArrowPointBuffer.addPoint(p5);
      turnArrowPointBuffer.addPoint(pm);
      turnArrowPointBuffer.addPoint(p5);
      turnArrowPointBuffer.addPoint(p6);
      turnArrowPointBuffer.addPoint(pm);
      turnArrowPointBuffer.addPoint(p6);
      turnArrowPointBuffer.addPoint(p7);
      turnArrowPointBuffer.addPoint(p8);
      turnArrowPointBuffer.addPoint(p7);
      turnArrowPointBuffer.addPoint(p10);
      turnArrowPointBuffer.addPoint(p7);
      turnArrowPointBuffer.addPoint(p6);
      turnArrowPointBuffer.addPoint(p10);
      turnArrowPointBuffer.addPoint(p6);
      turnArrowPointBuffer.addPoint(p9);
      turnArrowPointBuffer.addPoint(p10);

    } else {

      // Only activate widget if target is shown or if off route
      // Get the current duration and distance to the target
      showTurn=false;
      if (navigationInfo->getTargetDistance()!=NavigationInfo::getUnknownDistance()) {
        unitConverter->formatMeters(navigationInfo->getTargetDistance(),value,unit);
        infos.str("");
        infos << value << " " << unit;
        fontEngine->updateString(&distanceValueFontString,infos.str());
        activateWidget=true;
      } else {
        fontEngine->updateString(&distanceValueFontString,"infinite");
      }
      if (navigationInfo->getOffRoute()) {
        fontEngine->updateString(&durationValueFontString,"off route!");
      } else if (navigationInfo->getTargetDuration()!=NavigationInfo::getUnknownDuration()) {
        unitConverter->formatTime(navigationInfo->getTargetDuration(),value,unit);
        infos.str("");
        infos << value << " " << unit;
        fontEngine->updateString(&durationValueFontString,infos.str());
        activateWidget=true;
      } else {
        fontEngine->updateString(&durationValueFontString,"move!");
      }
    }
    fontEngine->unlockFont();
    changed=true;

    // Set positions
    separatorIcon.setX(x);
    separatorIcon.setY(y);
    if (distanceLabelFontString) {
      distanceLabelFontString->setX(x+(iconWidth-distanceLabelFontString->getIconWidth())/2);
      distanceLabelFontString->setY(y+targetDistanceLabelOffsetY);
    }
    if (distanceValueFontString) {
      distanceValueFontString->setX(x+(iconWidth-distanceValueFontString->getIconWidth())/2);
      if (showTurn)
        distanceValueFontString->setY(y+turnDistanceValueOffsetY);
      else
        distanceValueFontString->setY(y+targetDistanceValueOffsetY);
    }
    if (durationLabelFontString) {
      durationLabelFontString->setX(x+(iconWidth-durationLabelFontString->getIconWidth())/2);
      durationLabelFontString->setY(y+durationLabelOffsetY);
    }
    if (durationValueFontString) {
      durationValueFontString->setX(x+(iconWidth-durationValueFontString->getIconWidth())/2);
      durationValueFontString->setY(y+durationValueOffsetY);
    }
    if (prevNavigationInfo.getLocationBearing()!=navigationInfo->getLocationBearing()) {
      if (navigationInfo->getLocationBearing()==NavigationInfo::getUnknownAngle()) {
        hideCompass=true;
      } else {
        GraphicRotateAnimationParameter rotateParameter;
        rotateParameter.setStartTime(t);
        rotateParameter.setStartAngle(compassObject.getAngle());
        rotateParameter.setEndAngle(navigationInfo->getLocationBearing());
        rotateParameter.setDuration(directionChangeDuration);
        rotateParameter.setInfinite(false);
        rotateParameter.setAnimationType(GraphicRotateAnimationTypeAccelerated);
        std::list<GraphicRotateAnimationParameter> rotateAnimationSequence = compassObject.getRotateAnimationSequence();
        rotateAnimationSequence.push_back(rotateParameter);
        compassObject.setRotateAnimationSequence(rotateAnimationSequence);
        //directionIcon.setAngle(navigationInfos->locationBearing);
        activateWidget=true;
        hideCompass=false;
      }
    }
    if (prevNavigationInfo.getTargetBearing()!=navigationInfo->getTargetBearing()) {
      if (navigationInfo->getTargetBearing()==NavigationInfo::getUnknownAngle()) {
        hideTarget=true;
      } else {
        GraphicRotateAnimationParameter rotateParameter;
        rotateParameter.setStartTime(t);
        rotateParameter.setStartAngle(targetObject.getAngle());
        rotateParameter.setEndAngle(navigationInfo->getTargetBearing());
        rotateParameter.setDuration(directionChangeDuration);
        rotateParameter.setInfinite(false);
        rotateParameter.setAnimationType(GraphicRotateAnimationTypeAccelerated);
        std::list<GraphicRotateAnimationParameter> rotateAnimationSequence = targetObject.getRotateAnimationSequence();
        rotateAnimationSequence.push_back(rotateParameter);
        targetObject.setRotateAnimationSequence(rotateAnimationSequence);
        //directionIcon.setAngle(navigationInfos->locationBearing);
        hideTarget=false;
        activateWidget=true;
      }
    }
    prevNavigationInfo=*navigationInfo;
    navigationEngine->unlockNavigationInfo();

    // Depending on the navigation type, widget may or may not be activated
    switch(navigationInfo->getType()) {
      case NavigationInfoTypeTarget:
        // Always show widget
        break;
      case NavigationInfoTypeRoute:
        // Only show widget if off route or turn is shown
        if ((!navigationInfo->getOffRoute()&&(!showTurn)))
          activateWidget=false;
        break;
      case NavigationInfoTypeUnknown:
        activateWidget=false;
        break;
      default:
        FATAL("unsupported navigation info type",NULL);
    }

    // Activate widget if not already
    if (!core->getWidgetEngine()->getWidgetsActive()) {
      if (activateWidget!=active) {
        if (activateWidget) {
          setFadeAnimation(t,getColor(),getActiveColor(),false,core->getGraphicEngine()->getFadeDuration());
        } else {
          setFadeAnimation(t,getColor(),getInactiveColor(),false,core->getGraphicEngine()->getFadeDuration());
        }
        active=activateWidget;
      }
    } else {
      active=false;
    }

    // Set the next update time
    nextUpdateTime=t+updateInterval;

  }

  // Let the directionIcon work
  changed |= compassObject.work(t);

  // Return result
  return changed;
}

// Executed every time the graphic engine needs to draw
void WidgetNavigation::draw(Screen *screen, TimestampInMicroseconds t) {

  // Let the primitive draw the background
  WidgetPrimitive::draw(screen,t);

  // Draw the compass
  if (!hideCompass) {
    screen->startObject();
    screen->translate(getX()+getIconWidth()/2,getY()+getIconHeight()/2,getZ());
    screen->rotate(compassObject.getAngle(),0,0,1);
    directionIcon.setColor(color);
    directionIcon.draw(screen,t);
    screen->endObject();
    screen->startObject();
    screen->translate(getX()+getIconWidth()/2,getY()+getIconHeight()/2,getZ());
    for (Int i=0;i<4;i++) {
      double x=orientationLabelRadius*sin((i+4)*M_PI/2-FloatingPoint::degree2rad(compassObject.getAngle()));
      double y=orientationLabelRadius*cos((i+4)*M_PI/2-FloatingPoint::degree2rad(compassObject.getAngle()));
      x-=((double)orientationLabelFontStrings[i]->getIconWidth())/2;
      y-=((double)orientationLabelFontStrings[i]->getIconHeight())/2;
      y-=orientationLabelFontStrings[i]->getBaselineOffsetY();
      orientationLabelFontStrings[i]->setX(round(x));
      orientationLabelFontStrings[i]->setY(round(y));
      orientationLabelFontStrings[i]->setColor(color);
      orientationLabelFontStrings[i]->draw(screen,t);
    }
    screen->endObject();
    if (!hideTarget) {
      screen->startObject();
      screen->translate(getX()+getIconWidth()/2,getY()+getIconHeight()/2,getZ());
      screen->rotate(compassObject.getAngle(),0,0,1);
      screen->translate(targetRadius*sin(FloatingPoint::degree2rad(targetObject.getAngle())),targetRadius*cos(FloatingPoint::degree2rad(targetObject.getAngle())),getZ());
      targetIcon.setColor(color);
      screen->rotate(targetIcon.getAngle(),0,0,1);
      targetIcon.draw(screen,t);
      screen->endObject();
    }
  }

  // Is a turn coming?
  if (showTurn) {

    // Draw the turn
    screen->startObject();
    screen->translate(getX(),getY(),getZ());
    screen->setColor(turnColor.getRed(),turnColor.getGreen(),turnColor.getBlue(),color.getAlpha());
    turnArrowPointBuffer.drawAsTriangles(screen);
    screen->endObject();

  } else {

    // Draw the information about the target / route
    separatorIcon.setColor(color);
    separatorIcon.draw(screen,t);
    distanceLabelFontString->setColor(color);
    distanceLabelFontString->draw(screen,t);
    durationLabelFontString->setColor(color);
    durationLabelFontString->draw(screen,t);
    durationValueFontString->setColor(color);
    durationValueFontString->draw(screen,t);
  }

  // Draw the distance text (independent if a turn is coming or not)
  distanceValueFontString->setColor(color);
  distanceValueFontString->draw(screen,t);
}

// Called when the widget has changed its position
void WidgetNavigation::updatePosition(Int x, Int y, Int z) {
  WidgetPrimitive::updatePosition(x,y,z);
  nextUpdateTime=0;
}

}
