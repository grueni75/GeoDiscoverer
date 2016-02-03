//============================================================================
// Name        : WidgetNavigation.cpp
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
WidgetNavigation::WidgetNavigation(WidgetPage *widgetPage) :
    WidgetPrimitive(widgetPage),
    turnArrowPointBuffer(widgetPage->getScreen(),24),
    directionIcon(widgetPage->getScreen()),
    targetIcon(widgetPage->getScreen()),
    separatorIcon(widgetPage->getScreen()),
    targetObject(widgetPage->getScreen()),
    compassObject(widgetPage->getScreen())
{
  widgetType=WidgetTypeNavigation;
  updateInterval=1000000;
  nextUpdateTime=0;
  distanceLabelFontString=NULL;
  durationLabelFontString=NULL;
  distanceValueFontString=NULL;
  durationValueFontString=NULL;
  trackLengthLabelFontString=NULL;
  trackLengthValueFontString=NULL;
  altitudeLabelFontString=NULL;
  altitudeValueFontString=NULL;
  locationBearingActual=0;
  locationBearingActual=0;
  directionChangeDuration=0;
  FontEngine *fontEngine=widgetPage->getFontEngine();
  fontEngine->lockFont("sansBoldTiny",__FILE__, __LINE__);
  fontEngine->updateString(&distanceLabelFontString,"Distance");
  fontEngine->updateString(&durationLabelFontString,"Duration");
  fontEngine->updateString(&altitudeLabelFontString,"Altitude");
  fontEngine->updateString(&trackLengthLabelFontString,"Track Len");
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
  FontEngine *fontEngine=widgetPage->getFontEngine();
  fontEngine->lockFont("sansBoldTiny",__FILE__, __LINE__);
  if (distanceLabelFontString) fontEngine->destroyString(distanceLabelFontString);
  if (durationLabelFontString) fontEngine->destroyString(durationLabelFontString);
  if (altitudeLabelFontString) fontEngine->destroyString(altitudeLabelFontString);
  if (trackLengthLabelFontString) fontEngine->destroyString(trackLengthLabelFontString);
  fontEngine->unlockFont();
  fontEngine->lockFont("sansSmall",__FILE__, __LINE__);
  if (distanceValueFontString) fontEngine->destroyString(distanceValueFontString);
  if (durationValueFontString) fontEngine->destroyString(durationValueFontString);
  if (altitudeValueFontString) fontEngine->destroyString(altitudeValueFontString);
  if (trackLengthValueFontString) fontEngine->destroyString(trackLengthValueFontString);
  fontEngine->unlockFont();
  fontEngine->lockFont("sansTiny",__FILE__, __LINE__);
  for(int i=0;i<4;i++) {
    if (orientationLabelFontStrings[i]) fontEngine->destroyString(orientationLabelFontStrings[i]);
  }
  fontEngine->unlockFont();
}

// Executed every time the graphic engine checks if drawing is required
bool WidgetNavigation::work(TimestampInMicroseconds t) {

  bool update;
  FontEngine *fontEngine=widgetPage->getFontEngine();
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
      double distance;
      if (navigationInfo->getOffRoute())
        distance=navigationInfo->getRouteDistance();
      else if (navigationInfo->getTargetDistance()!=NavigationInfo::getUnknownDistance())
        distance=navigationInfo->getTargetDistance();
      else
        distance=-1;
      if (distance!=-1) {
        unitConverter->formatMeters(distance,value,unit);
        infos.str("");
        infos << value << " " << unit;
        fontEngine->updateString(&distanceValueFontString,infos.str());
        activateWidget=true;
      } else {
        fontEngine->updateString(&distanceValueFontString,"unknown");
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
        fontEngine->updateString(&durationValueFontString,"unknown");
      }
      if (textColumnCount==2) {
        if (navigationInfo->getAltitude()!=NavigationInfo::getUnknownDistance()) {
          unitConverter->formatMeters(navigationInfo->getAltitude(),value,unit);
          infos.str("");
          infos << value << " " << unit;
          fontEngine->updateString(&altitudeValueFontString,infos.str());
          activateWidget=true;
        } else {
          fontEngine->updateString(&altitudeValueFontString,"unknown");
        }
        if (navigationInfo->getTrackLength()!=NavigationInfo::getUnknownDistance()) {
          unitConverter->formatMeters(navigationInfo->getTrackLength(),value,unit);
          infos.str("");
          infos << value << " " << unit;
          fontEngine->updateString(&trackLengthValueFontString,infos.str());
          activateWidget=true;
        } else {
          fontEngine->updateString(&trackLengthValueFontString,"unknown");
        }
      }
    }
    fontEngine->unlockFont();
    changed=true;

    // Set positions
    separatorIcon.setX(x);
    separatorIcon.setY(y);
    if (distanceLabelFontString) {
      distanceLabelFontString->setX((textColumnCount==1)?
              x+(iconWidth-distanceLabelFontString->getIconWidth())/2 :
              x+iconWidth/2-textColumnOffsetX-distanceLabelFontString->getIconWidth()
      );
      distanceLabelFontString->setY(y+textRowFirstOffsetY);
    }
    if (distanceValueFontString) {
      if (showTurn)
        distanceValueFontString->setX(x+(iconWidth-distanceValueFontString->getIconWidth())/2);
      else
        distanceValueFontString->setX((textColumnCount==1)?
            x+(iconWidth-distanceValueFontString->getIconWidth())/2 :
            x+iconWidth/2-textColumnOffsetX-distanceValueFontString->getIconWidth()
        );
      if (showTurn)
        distanceValueFontString->setY(y+turnDistanceValueOffsetY);
      else
        distanceValueFontString->setY(y+textRowSecondOffsetY);
    }
    if (durationLabelFontString) {
      durationLabelFontString->setX((textColumnCount==1)?
          x+(iconWidth-durationLabelFontString->getIconWidth())/2 :
          x+iconWidth/2-textColumnOffsetX-durationLabelFontString->getIconWidth()
      );
      durationLabelFontString->setY(y+textRowFourthOffsetY);
    }
    if (durationValueFontString) {
      durationValueFontString->setX((textColumnCount==1)?
          x+(iconWidth-durationValueFontString->getIconWidth())/2 :
          x+iconWidth/2-textColumnOffsetX-durationValueFontString->getIconWidth()
      );
      durationValueFontString->setY(y+textRowThirdOffsetY);
    }
    if (trackLengthLabelFontString) {
      trackLengthLabelFontString->setX((textColumnCount==1)?
              x+(iconWidth-trackLengthLabelFontString->getIconWidth())/2 :
              x+iconWidth/2+textColumnOffsetX
      );
      trackLengthLabelFontString->setY(y+textRowFirstOffsetY);
    }
    if (trackLengthValueFontString) {
      trackLengthValueFontString->setX((textColumnCount==1)?
          x+(iconWidth-trackLengthValueFontString->getIconWidth())/2 :
          x+iconWidth/2+textColumnOffsetX
      );
      trackLengthValueFontString->setY(y+textRowSecondOffsetY);
    }
    if (altitudeLabelFontString) {
      altitudeLabelFontString->setX((textColumnCount==1)?
          x+(iconWidth-altitudeLabelFontString->getIconWidth())/2 :
          x+iconWidth/2+textColumnOffsetX
      );
      altitudeLabelFontString->setY(y+textRowFourthOffsetY);
    }
    if (altitudeValueFontString) {
      altitudeValueFontString->setX((textColumnCount==1)?
          x+(iconWidth-altitudeValueFontString->getIconWidth())/2 :
          x+iconWidth/2+textColumnOffsetX
      );
      altitudeValueFontString->setY(y+textRowThirdOffsetY);
    }
    if (prevNavigationInfo.getLocationBearing()!=navigationInfo->getLocationBearing()) {
      if (navigationInfo->getLocationBearing()==NavigationInfo::getUnknownAngle()) {
        hideCompass=true;
      } else {
        if (widgetPage->getWidgetEngine()->getDevice()->isAnimationFriendly()) {
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
        } else {
          compassObject.setAngle(navigationInfo->getLocationBearing());
        }
        activateWidget=true;
        hideCompass=false;
      }
    }
    if (prevNavigationInfo.getTargetBearing()!=navigationInfo->getTargetBearing()) {
      if (navigationInfo->getTargetBearing()==NavigationInfo::getUnknownAngle()) {
        hideTarget=true;
      } else {
        if (widgetPage->getWidgetEngine()->getDevice()->isAnimationFriendly()) {
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
        } else {
          targetObject.setAngle(navigationInfo->getTargetBearing());
        }
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
    if (!widgetPage->getWidgetEngine()->getWidgetsActive()) {
      if (activateWidget!=active) {
        if (activateWidget) {
          setFadeAnimation(t,getColor(),getActiveColor(),false,widgetPage->getGraphicEngine()->getFadeDuration());
        } else {
          setFadeAnimation(t,getColor(),getInactiveColor(),false,widgetPage->getGraphicEngine()->getFadeDuration());
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
void WidgetNavigation::draw(TimestampInMicroseconds t) {

  // Let the primitive draw the background
  WidgetPrimitive::draw(t);

  // Draw the compass
  if (!hideCompass) {
    screen->startObject();
    screen->translate(getX()+getIconWidth()/2,getY()+getIconHeight()/2,getZ());
    screen->rotate(compassObject.getAngle(),0,0,1);
    directionIcon.setColor(color);
    directionIcon.draw(t);
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
      orientationLabelFontStrings[i]->draw(t);
    }
    screen->endObject();
    if (!hideTarget) {
      screen->startObject();
      screen->translate(getX()+getIconWidth()/2,getY()+getIconHeight()/2,getZ());
      screen->rotate(compassObject.getAngle(),0,0,1);
      screen->translate(targetRadius*sin(FloatingPoint::degree2rad(targetObject.getAngle())),targetRadius*cos(FloatingPoint::degree2rad(targetObject.getAngle())),getZ());
      targetIcon.setColor(color);
      screen->rotate(targetIcon.getAngle(),0,0,1);
      targetIcon.draw(t);
      screen->endObject();
    }
  }

  // Is a turn coming?
  if (showTurn) {

    // Draw the turn
    screen->startObject();
    screen->translate(getX(),getY(),getZ());
    screen->setColor(turnColor.getRed(),turnColor.getGreen(),turnColor.getBlue(),color.getAlpha());
    turnArrowPointBuffer.drawAsTriangles();
    screen->endObject();

  } else {

    // Draw the information about the target / route
    separatorIcon.setColor(color);
    separatorIcon.draw(t);
    if (distanceLabelFontString) {
      distanceLabelFontString->setColor(color);
      distanceLabelFontString->draw(t);
    }
    if (durationLabelFontString) {
      durationLabelFontString->setColor(color);
      durationLabelFontString->draw(t);
    }
    if (durationValueFontString) {
      durationValueFontString->setColor(color);
      durationValueFontString->draw(t);
    }
    if (textColumnCount==2) {
      if (altitudeLabelFontString) {
        altitudeLabelFontString->setColor(color);
        altitudeLabelFontString->draw(t);
      }
      if (altitudeValueFontString) {
        altitudeValueFontString->setColor(color);
        altitudeValueFontString->draw(t);
      }
      if (trackLengthLabelFontString) {
        trackLengthLabelFontString->setColor(color);
        trackLengthLabelFontString->draw(t);
      }
      if (trackLengthValueFontString) {
        trackLengthValueFontString->setColor(color);
        trackLengthValueFontString->draw(t);
      }
    }
  }

  // Draw the distance text (independent if a turn is coming or not)
  if (distanceValueFontString) {
    distanceValueFontString->setColor(color);
    distanceValueFontString->draw(t);
  }
}

// Called when the widget has changed its position
void WidgetNavigation::updatePosition(Int x, Int y, Int z) {
  WidgetPrimitive::updatePosition(x,y,z);
  nextUpdateTime=0;
}


}
