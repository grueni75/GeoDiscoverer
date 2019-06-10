//============================================================================
// Name        : WidgetNavigation.cpp
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
WidgetNavigation::WidgetNavigation(WidgetPage *widgetPage) :
    WidgetPrimitive(widgetPage),
    turnArrowPointBuffer(widgetPage->getScreen(),24),
    directionIcon(widgetPage->getScreen()),
    targetIcon(widgetPage->getScreen()),
    arrowIcon(widgetPage->getScreen()),
    separatorIcon(widgetPage->getScreen()),
    blindIcon(widgetPage->getScreen()),
    statusIcon(widgetPage->getScreen()),
    batteryIconCharging(widgetPage->getScreen()),
    batteryIconEmpty(widgetPage->getScreen()),
    batteryIconFull(widgetPage->getScreen()),
    targetObject(widgetPage->getScreen()),
    compassObject(widgetPage->getScreen()),
    clockCircularStrip(widgetPage->getScreen()),
    busyColor(),
    normalColor()
{
  widgetType=WidgetTypeNavigation;
  updateInterval=1000000;
  distanceLabelFontString=NULL;
  durationLabelFontString=NULL;
  distanceValueFontString=NULL;
  durationValueFontString=NULL;
  clockFontString=NULL;
  turnFontString=NULL;
  altitudeLabelFontString=NULL;
  altitudeValueFontString=NULL;
  trackLengthLabelFontString=NULL;
  trackLengthValueFontString=NULL;
  speedLabelFontString=NULL;
  speedValueFontString=NULL;
  locationBearingActual=0;
  locationBearingActual=0;
  directionChangeDuration=0;
  textColumnCount=1;
  northButtonHit=false;
  statusTextAbove=false;
  orientationLabelRadius=0;
  batteryIconRadius=0;
  isWatch=widgetPage->getWidgetEngine()->getDevice()->getIsWatch();
  FontEngine *fontEngine=widgetPage->getFontEngine();
  if (!isWatch) {
    fontEngine->lockFont("sansBoldTiny",__FILE__, __LINE__);
    fontEngine->updateString(&durationLabelFontString,"Duration");
    fontEngine->updateString(&altitudeLabelFontString,"Altitude");
    fontEngine->updateString(&trackLengthLabelFontString,"Track");
    fontEngine->updateString(&speedLabelFontString,"Speed");
    fontEngine->unlockFont();
  }
  fontEngine->lockFont("sansTiny",__FILE__, __LINE__);
  for(int i=0;i<8;i++)
    orientationLabelFontStrings[i]=NULL;
  fontEngine->updateString(&orientationLabelFontStrings[0],"N");
  fontEngine->updateString(&orientationLabelFontStrings[1],"E");
  fontEngine->updateString(&orientationLabelFontStrings[2],"S");
  fontEngine->updateString(&orientationLabelFontStrings[3],"W");
  if (isWatch) {
    fontEngine->updateString(&orientationLabelFontStrings[4],"NE");
    fontEngine->updateString(&orientationLabelFontStrings[5],"SE");
    fontEngine->updateString(&orientationLabelFontStrings[6],"SW");
    fontEngine->updateString(&orientationLabelFontStrings[7],"NW");
  }
  fontEngine->unlockFont();
  targetIcon.setRotateAnimation(0,0,360,true,core->getNavigationEngine()->getTargetRotateDuration(),GraphicRotateAnimationTypeLinear);
  targetObject.addPrimitive(&targetIcon);
  compassObject.addPrimitive(&directionIcon);
  compassObject.addPrimitive(&targetObject);
  hideCompass=true;
  hideTarget=true;
  hideArrow=true;
  showTurn=false;
  skipTurn=false;
  active=false;
  firstRun=true;
  lastClockUpdate=0;
  secondRowState=0;
  firstTouchAfterInactive=true;
  remoteServerActive=false;
  statusTextWidthLimit=-1;
  statusTextAngleOffset=0;
  statusTextRadius=0;
  batteryDotCount=6;
  batteryTotalAngle=30;
  directionIcon.setColor(GraphicColor(255,255,255,255));
  targetIcon.setColor(GraphicColor(255,255,255,255));
  arrowIcon.setColor(GraphicColor(255,255,255,0));
  separatorIcon.setColor(GraphicColor(255,255,255,255));
  blindIcon.setColor(GraphicColor(255,255,255,255));
  statusIcon.setColor(GraphicColor(255,255,255,255));
  batteryIconCharging.setColor(GraphicColor(255,255,255,255));
  batteryIconEmpty.setColor(GraphicColor(255,255,255,255));
  batteryIconFull.setColor(GraphicColor(255,255,255,255));
  targetObject.setColor(GraphicColor(255,255,255,255));
  compassObject.setColor(GraphicColor(255,255,255,255));
  for (int i=0;i<4;i++) {
    circularStrip[i]=NULL;
  }
}

// Destructor
WidgetNavigation::~WidgetNavigation() {
  FontEngine *fontEngine=widgetPage->getFontEngine();
  if (textColumnCount==2)
    fontEngine->lockFont("sansBoldSmall",__FILE__, __LINE__);
  else
    fontEngine->lockFont("sansBoldTiny",__FILE__, __LINE__);
  if (distanceLabelFontString) fontEngine->destroyString(distanceLabelFontString);
  fontEngine->unlockFont();
  fontEngine->lockFont("sansBoldTiny",__FILE__, __LINE__);
  if (durationLabelFontString) fontEngine->destroyString(durationLabelFontString);
  if (altitudeLabelFontString) fontEngine->destroyString(altitudeLabelFontString);
  if (trackLengthLabelFontString) fontEngine->destroyString(trackLengthLabelFontString);
  if (speedLabelFontString) fontEngine->destroyString(speedLabelFontString);
  fontEngine->unlockFont();
  if (textColumnCount==2) {
    fontEngine->lockFont("sansLarge",__FILE__, __LINE__);
  } else {
    fontEngine->lockFont("sansSmall",__FILE__, __LINE__);
  }
  if (distanceValueFontString) fontEngine->destroyString(distanceValueFontString);
  fontEngine->unlockFont();
  if (textColumnCount==2)
    fontEngine->lockFont("sansNormal",__FILE__, __LINE__);
  else
    fontEngine->lockFont("sansSmall",__FILE__, __LINE__);
  if (turnFontString) fontEngine->destroyString(turnFontString);
  fontEngine->unlockFont();
  fontEngine->lockFont("sansSmall",__FILE__, __LINE__);
  if (durationValueFontString) fontEngine->destroyString(durationValueFontString);
  if (altitudeValueFontString) fontEngine->destroyString(altitudeValueFontString);
  if (trackLengthValueFontString) fontEngine->destroyString(trackLengthValueFontString);
  if (speedValueFontString) fontEngine->destroyString(speedValueFontString);
  fontEngine->unlockFont();
  fontEngine->lockFont("sansTiny",__FILE__, __LINE__);
  for(int i=0;i<8;i++) {
    if (orientationLabelFontStrings[i]) fontEngine->destroyString(orientationLabelFontStrings[i]);
  }
  fontEngine->unlockFont();
  fontEngine->lockFont("sansNormal",__FILE__,__LINE__);
  if (clockFontString) fontEngine->destroyString(clockFontString);
  fontEngine->unlockFont();
  for (int i=0;i<4;i++) {
    if (circularStrip[i]) {
      delete circularStrip[i];
      circularStrip[i]=NULL;
    }
  }
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

  // If the widgets are not active anymore, reset the first touch indicator
  if (!widgetPage->getWidgetsActive()) {
    firstTouchAfterInactive=true;
  }

  // Indicate if a download is ongoing
  if (core->getRemoteServerActive()!=remoteServerActive) {
    if (core->getRemoteServerActive()) {
      activeColor=busyColor;
      inactiveColorBackup=inactiveColor;
      inactiveColor=busyColor;
    } else {
      activeColor=normalColor;
      inactiveColor=inactiveColorBackup;
    }
    setFadeAnimation(t,getColor(),getActiveColor(),false,widgetPage->getGraphicEngine()->getFadeDuration());
    remoteServerActive=core->getRemoteServerActive();
    //DEBUG("remoteServerActive=%d",remoteServerActive);
  }

  // Update the circle strips (if not already)
  if ((isWatch)&&(circularStrip[0]==NULL)) {
    double offset=-statusTextAngleOffset;
    for (int i=0;i<4;i++) {
      circularStrip[i]=new GraphicCircularStrip(widgetPage->getScreen());
      if (!circularStrip[i]) {
        FATAL("can not create graphic circular strip object",NULL);
        return false;
      }
      circularStrip[i]->setAngle(45.0+offset+i*90.0);
      offset=-offset;
      circularStrip[i]->setColor(GraphicColor(255,255,255,255));
      circularStrip[i]->setDestroyTexture(false);
      if (i>=2) {
        circularStrip[i]->setInverse(true);
      }
    }
    clockCircularStrip.setAngle(270.0);
    clockCircularStrip.setRadius(clockRadius);
    clockCircularStrip.setColor(GraphicColor(255,255,255,255));
    clockCircularStrip.setDestroyTexture(false);
    clockCircularStrip.setInverse(true);
  }

  // Update the font strings (if not already)
  if ((distanceLabelFontString==NULL)&&(!isWatch)) {
    if (textColumnCount==2)
      fontEngine->lockFont("sansBoldSmall",__FILE__, __LINE__);
    else
      fontEngine->lockFont("sansBoldTiny",__FILE__, __LINE__);
    fontEngine->updateString(&distanceLabelFontString,"Distance");
    fontEngine->unlockFont();
    changed=true;
  }

  // Update the clock
  TimestampInSeconds t2=core->getClock()->getSecondsSinceEpoch();
  if (((textColumnCount==2)||(isWatch))&&((t2/60!=lastClockUpdate/60)||(firstRun))) {
    fontEngine->lockFont("sansNormal",__FILE__, __LINE__);
    fontEngine->updateString(&clockFontString,core->getClock()->getFormattedDate(t2,"%H:%M",true));
    fontEngine->unlockFont();
    lastClockUpdate=t2;
    clockFontString->setX(x+(iconWidth-clockFontString->getIconWidth())/2);
    clockFontString->setY(y+clockOffsetY);
    if (isWatch) {
      clockCircularStrip.setWidth(clockFontString->getWidth());
      clockCircularStrip.setHeight(clockFontString->getHeight());
      clockCircularStrip.setIconWidth(clockFontString->getIconWidth());
      clockCircularStrip.setIconHeight(clockFontString->getIconHeight());
    }
    changed=true;
  }

  // Update the arrow
  if (isWatch) {
    bool arrowVisible;
    double arrowAngle;
    core->getNavigationEngine()->getArrowInfo(arrowVisible,arrowAngle);
    if (arrowVisible) {
      arrowIcon.setAngle(arrowAngle);
      if (hideArrow) {
        GraphicColor endColor=arrowIcon.getColor();
        endColor.setAlpha(255);
        arrowIcon.setFadeAnimation(t,arrowIcon.getColor(),endColor,false,core->getDefaultGraphicEngine()->getFadeDuration());
      }
      hideArrow=false;
    } else {
      if (!hideArrow) {
        GraphicColor endColor=arrowIcon.getColor();
        endColor.setAlpha(0);
        arrowIcon.setFadeAnimation(t,arrowIcon.getColor(),endColor,false,core->getDefaultGraphicEngine()->getFadeDuration());
      }
      hideArrow=true;
    }
    changed|=arrowIcon.work(t);
  }

  // Only update the info if it has changed
  NavigationInfo *navigationInfo=navigationEngine->lockNavigationInfo(__FILE__, __LINE__);
  if ((*navigationInfo!=prevNavigationInfo)||(firstRun)) {

    // Is a turn coming?
    bool activateWidget = false;
    if (navigationInfo->getTurnDistance()!=NavigationInfo::getUnknownDistance()) {

      // Get distance to turn
      if ((textColumnCount==2)||(isWatch))
        fontEngine->lockFont("sansNormal",__FILE__, __LINE__);
      else
        fontEngine->lockFont("sansSmall",__FILE__, __LINE__);
      showTurn=true;
      activateWidget=true;
      unitConverter->formatMeters(navigationInfo->getTurnDistance(),value,unit);
      infos.str("");
      infos << value << " " << unit;
      fontEngine->updateString(&turnFontString,infos.str());
      fontEngine->unlockFont();

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
      fontEngine->lockFont("sansSmall",__FILE__, __LINE__);
      showTurn=false;
      double distance;
      if (navigationInfo->getOffRoute())
        distance=navigationInfo->getRouteDistance();
      else if (navigationInfo->getTargetDistance()!=NavigationInfo::getUnknownDistance())
        distance=navigationInfo->getTargetDistance();
      else
        distance=-1;
      if (navigationInfo->getOffRoute()) {
        fontEngine->updateString(&durationValueFontString,"off route!",statusTextWidthLimit);
      } else if (navigationInfo->getTargetDuration()==NavigationInfo::getUnknownDuration()) {
        fontEngine->updateString(&durationValueFontString,"unknown",statusTextWidthLimit);
      } else if (std::isinf(navigationInfo->getTargetDuration())) {
        fontEngine->updateString(&durationValueFontString,"move!",statusTextWidthLimit);
      } else {
        unitConverter->formatTime(navigationInfo->getTargetDuration(),value,unit,isWatch ? 1 : 2);
        infos.str("");
        infos << value << " " << unit;
        fontEngine->updateString(&durationValueFontString,infos.str(),statusTextWidthLimit);
        activateWidget=true;
      }
      if (navigationInfo->getAltitude()!=NavigationInfo::getUnknownDistance()) {
        std::string lockedUnit = unitConverter->getUnitSystem()==ImperialSystem ? "mi" : "m";
        unitConverter->formatMeters(navigationInfo->getAltitude(),value,unit,isWatch ? 1 : 2,lockedUnit);
        infos.str("");
        infos << value << " " << unit;
        fontEngine->updateString(&altitudeValueFontString,infos.str(),statusTextWidthLimit);
        activateWidget=true;
      } else {
        fontEngine->updateString(&altitudeValueFontString,"unknown",statusTextWidthLimit);
      }
      if (navigationInfo->getTrackLength()!=NavigationInfo::getUnknownDistance()) {
        unitConverter->formatMeters(navigationInfo->getTrackLength(),value,unit,isWatch ? 1 : 2);
        infos.str("");
        infos << value << " " << unit;
        fontEngine->updateString(&trackLengthValueFontString,infos.str(),statusTextWidthLimit);
        activateWidget=true;
      } else {
        fontEngine->updateString(&trackLengthValueFontString,"unknown",statusTextWidthLimit);
      }
      if (navigationInfo->getLocationSpeed()!=NavigationInfo::getUnknownSpeed()) {
        unitConverter->formatMetersPerSecond(navigationInfo->getLocationSpeed(),value,unit,isWatch ? 1 : 1);
        infos.str("");
        infos << value << " " << unit;
        fontEngine->updateString(&speedValueFontString,infos.str(),statusTextWidthLimit);
        activateWidget=true;
      } else {
        fontEngine->updateString(&speedValueFontString,"unknown",statusTextWidthLimit);
      }
      fontEngine->unlockFont();
      if (textColumnCount==2) {
        fontEngine->lockFont("sansLarge",__FILE__, __LINE__);
      } else {
        fontEngine->lockFont("sansSmall",__FILE__, __LINE__);
      }
      if (distance!=-1) {
        unitConverter->formatMeters(distance,value,unit,isWatch ? 1 : 2);
        infos.str("");
        infos << value << " " << unit;
        fontEngine->updateString(&distanceValueFontString,infos.str(),statusTextWidthLimit);
        activateWidget=true;
      } else {
        fontEngine->updateString(&distanceValueFontString,"unknown",statusTextWidthLimit);
      }
      fontEngine->unlockFont();
      if (isWatch) {
        for (Int i=0;i<4;i++) {
          FontString *s=getQuadrantFontString(i);
          circularStrip[i]->setWidth(s->getWidth());
          circularStrip[i]->setHeight(s->getHeight());
          circularStrip[i]->setIconWidth(s->getIconWidth());
          circularStrip[i]->setIconHeight(s->getIconHeight());
        }
      }
    }
    changed=true;

    // Set positions
    separatorIcon.setX(x);
    separatorIcon.setY(y);
    if (distanceLabelFontString) {
      distanceLabelFontString->setX(x+(iconWidth-distanceLabelFontString->getIconWidth())/2);
      distanceLabelFontString->setY(y+textRowFirstOffsetY);
    }
    if (distanceValueFontString) {
      distanceValueFontString->setX(x+(iconWidth-distanceValueFontString->getIconWidth())/2);
      distanceValueFontString->setY(y+textRowSecondOffsetY);
    }
    if (turnFontString) {
      turnFontString->setX(x+(iconWidth-turnFontString->getIconWidth())/2);
      turnFontString->setY(y+turnDistanceValueOffsetY);
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
    if (trackLengthLabelFontString) {
      trackLengthLabelFontString->setX((textColumnCount==1)?
          x+(iconWidth-trackLengthLabelFontString->getIconWidth())/2 :
          x+iconWidth/2-textColumnOffsetX-trackLengthLabelFontString->getIconWidth()
      );
      trackLengthLabelFontString->setY(y+textRowFourthOffsetY);
    }
    if (trackLengthValueFontString) {
      trackLengthValueFontString->setX((textColumnCount==1)?
          x+(iconWidth-trackLengthValueFontString->getIconWidth())/2 :
          x+iconWidth/2-textColumnOffsetX-trackLengthValueFontString->getIconWidth()
      );
      trackLengthValueFontString->setY(y+textRowThirdOffsetY);
    }
    if (speedLabelFontString) {
      speedLabelFontString->setX((textColumnCount==1)?
          x+(iconWidth-speedLabelFontString->getIconWidth())/2 :
          x+iconWidth/2+textColumnOffsetX
      );
      speedLabelFontString->setY(y+textRowFourthOffsetY);
    }
    if (speedValueFontString) {
      speedValueFontString->setX((textColumnCount==1)?
          x+(iconWidth-speedValueFontString->getIconWidth())/2 :
          x+iconWidth/2+textColumnOffsetX
      );
      speedValueFontString->setY(y+textRowThirdOffsetY);
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

  } else {
    navigationEngine->unlockNavigationInfo();
  }
  firstRun=false;

  // Let the directionIcon work
  changed |= compassObject.work(t);

  // Update the circular strips
  if (isWatch) {
    for (Int i=0;i<4;i++)
      changed |= circularStrip[i]->work(t);
    changed |= clockCircularStrip.work(t);
  }

  // Return result
  return changed;
}

// Returns the font string to show in the given quadrant
FontString* WidgetNavigation::getQuadrantFontString(Int i) {
  FontString *s=NULL;
  switch (i) {
  case 0:
    s = durationValueFontString;
    break;
  case 1:
    s = distanceValueFontString;
    break;
  case 2:
    s = altitudeValueFontString;
    break;
  case 3:
    s = trackLengthValueFontString;
    break;
  }
  return s;
}

// Draws the status texts on a watch
void WidgetNavigation::drawStatus(TimestampInMicroseconds t) {
  GraphicColor c = statusIcon.getColor();
  c.setAlpha(color.getAlpha());
  statusIcon.setColor(c);
  statusIcon.draw(t);
  screen->startObject();
  screen->translate(getX() + getIconWidth() / 2, getY() + getIconHeight() / 2, getZ());
  for (Int i = 0; i < 4; i++) {
    FontString* s = getQuadrantFontString(i);
    if ((s) && (circularStrip[i])) {
      c = circularStrip[i]->getColor();
      c.setAlpha(color.getAlpha());
      circularStrip[i]->setColor(c);
      if (circularStrip[i]->isInverse())
        circularStrip[i]->setRadius(statusTextRadius+s->getIconHeight()/2-s->getBaselineOffsetY());
      else
        circularStrip[i]->setRadius(statusTextRadius+s->getIconHeight()/2-s->getBaselineOffsetY());
      s->updateTexture();
      circularStrip[i]->setTexture(s->getTexture());
      circularStrip[i]->draw(t);
    }
  }
  screen->endObject();
}

// Draws a compass marker
void WidgetNavigation::drawCompassMarker(TimestampInMicroseconds t, FontString *marker, double x, double y) {
  x-=((double)marker->getIconWidth())/2;
  y-=((double)marker->getIconHeight())/2;
  y-=marker->getBaselineOffsetY();
  marker->setX(round(x));
  marker->setY(round(y));
  GraphicColor c=marker->getColor();
  c.setAlpha(color.getAlpha());
  marker->setColor(c);
  marker->draw(t);
}

// Executed every time the graphic engine needs to draw
void WidgetNavigation::draw(TimestampInMicroseconds t) {

  GraphicColor c;

  // Let the primitive draw the background
  WidgetPrimitive::draw(t);

  // Draw the battery status
  if (isWatch) {
    screen->startObject();
    screen->translate(getX()+getIconWidth()/2,getY()+getIconHeight()/2,getZ());
    for (Int kind=0;kind<2;kind++) {
      Int batteryLevel;
      bool batteryCharging;
      if (kind==0) {
        batteryLevel=core->getBatteryLevel();
        batteryCharging=core->getBatteryCharging();
      } else {
        batteryLevel=core->getRemoteBatteryLevel();
        batteryCharging=core->getRemoteBatteryCharging();
      }
      for (Int i=0;i<batteryDotCount;i++) {
        double angle;
        if (kind==0)
          angle=FloatingPoint::degree2rad(270-batteryTotalAngle/2+i*batteryTotalAngle/(batteryDotCount-1));
        else
          angle=FloatingPoint::degree2rad(90+batteryTotalAngle/2-i*batteryTotalAngle/(batteryDotCount-1));
        double x=batteryIconRadius*sin(angle);
        double y=batteryIconRadius*cos(angle);
        GraphicRectangle *dot;
        if (batteryLevel>=100*i/batteryDotCount) {
          if (batteryCharging)
            dot=&batteryIconCharging;
          else
            dot=&batteryIconFull;
        } else {
          dot=&batteryIconEmpty;
        }
        GraphicColor c=dot->getColor();
        c.setAlpha(color.getAlpha());
        dot->setColor(c);
        dot->setX(round(x-dot->getIconWidth()/2));
        dot->setY(round(y-dot->getIconHeight()/2));
        dot->draw(t);
      }
    }
    screen->endObject();
  }

  // Draw the blind if necessary
  if ((isWatch)&&(showTurn)&&(!skipTurn)) {
    c=blindIcon.getColor();
    c.setAlpha(color.getAlpha());
    blindIcon.setColor(c);
    blindIcon.draw(t);
  }

  // Draw the clock
  if ((isWatch)&&(clockFontString)&&(!((showTurn)&&(!skipTurn)))) {
    c=clockCircularStrip.getColor();
    c.setAlpha(color.getAlpha());
    clockCircularStrip.setColor(c);
    clockFontString->updateTexture();
    clockCircularStrip.setTexture(clockFontString->getTexture());
    clockCircularStrip.draw(t);
  }

  // Draw the compass
  if (!hideCompass) {
    screen->startObject();
    screen->translate(getX()+getIconWidth()/2,getY()+getIconHeight()/2,getZ());
    screen->rotate(compassObject.getAngle(),0,0,1);
    c=directionIcon.getColor();
    c.setAlpha(color.getAlpha());
    directionIcon.setColor(c);
    directionIcon.draw(t);
    screen->endObject();
    screen->startObject();
    screen->translate(getX()+getIconWidth()/2,getY()+getIconHeight()/2,getZ());
    for (Int i=0;i<4;i++) {
      double x=orientationLabelRadius*sin((i+4)*M_PI/2-FloatingPoint::degree2rad(compassObject.getAngle()));
      double y=orientationLabelRadius*cos((i+4)*M_PI/2-FloatingPoint::degree2rad(compassObject.getAngle()));
      drawCompassMarker(t,orientationLabelFontStrings[i],x,y);
    }
    if (isWatch) {
      for (Int i=0;i<4;i++) {
        double x=orientationLabelRadius*sin(M_PI/4+(i+4)*M_PI/2-FloatingPoint::degree2rad(compassObject.getAngle()));
        double y=orientationLabelRadius*cos(M_PI/4+(i+4)*M_PI/2-FloatingPoint::degree2rad(compassObject.getAngle()));
        drawCompassMarker(t,orientationLabelFontStrings[i+4],x,y);
      }
    }
    screen->endObject();
  }

  // Draw the status below the arrow and the target if widget is not active
  if ((isWatch)&&(!statusTextAbove)) {
    drawStatus(t);
  }

  // Draw the arrow
  if (arrowIcon.getColor().getAlpha()>0) {
    //DEBUG("drawing arrow",NULL);
    screen->startObject();
    screen->translate(getX()+getIconWidth()/2,getY()+getIconHeight()/2,getZ());
    screen->rotate(arrowIcon.getAngle(),0,0,1);
    c=arrowIcon.getColor();
    c.setAlpha(c.getAlpha()*color.getAlpha()/255);
    arrowIcon.setColor(c);
    arrowIcon.draw(t);
    screen->endObject();
  }

  // Draw the target
  if (!hideCompass) {
    if (!hideTarget) {
      screen->startObject();
      screen->translate(getX()+getIconWidth()/2,getY()+getIconHeight()/2,getZ());
      screen->rotate(compassObject.getAngle(),0,0,1);
      screen->translate(targetRadius*sin(FloatingPoint::degree2rad(targetObject.getAngle())),targetRadius*cos(FloatingPoint::degree2rad(targetObject.getAngle())),getZ());
      c=targetIcon.getColor();
      c.setAlpha(color.getAlpha());
      targetIcon.setColor(c);
      screen->rotate(targetIcon.getAngle(),0,0,1);
      targetIcon.draw(t);
      screen->endObject();
    }
  }

  // Draw the status above the arrow and the target if widget is active
  if ((isWatch)&&(statusTextAbove)) {
    drawStatus(t);
  }

  // Is a turn coming?
  if ((showTurn)&&(!skipTurn)) {

    // Draw the turn
    screen->startObject();
    screen->translate(getX(),getY(),getZ());
    screen->setColor(turnColor.getRed(),turnColor.getGreen(),turnColor.getBlue(),color.getAlpha());
    turnArrowPointBuffer.drawAsTriangles();
    screen->endObject();
    if (turnFontString) {
      c=turnFontString->getColor();
      c.setAlpha(color.getAlpha());
      turnFontString->setColor(c);
      turnFontString->draw(t);
    }

  } else {

    // Draw the information about the target / route
    if (!showTurn)
      skipTurn=false;
    if (!isWatch) {
      c=separatorIcon.getColor();
      c.setAlpha(color.getAlpha());
      separatorIcon.setColor(c);
      separatorIcon.draw(t);
      if (distanceLabelFontString) {
        c=distanceLabelFontString->getColor();
        c.setAlpha(color.getAlpha());
        distanceLabelFontString->setColor(c);
        distanceLabelFontString->draw(t);
      }
      if (distanceValueFontString) {
        c=distanceValueFontString->getColor();
        c.setAlpha(color.getAlpha());
        distanceValueFontString->setColor(c);
        distanceValueFontString->draw(t);
      }
      FontString *leftLabelFontString = NULL;
      FontString *leftValueFontString = NULL;
      FontString *rightLabelFontString = NULL;
      FontString *rightValueFontString = NULL;
      if (textColumnCount==2) {
        switch (secondRowState) {
        case 0:
          leftLabelFontString=durationLabelFontString;
          leftValueFontString=durationValueFontString;
          rightLabelFontString=altitudeLabelFontString;
          rightValueFontString=altitudeValueFontString;
          break;
        case 1:
          leftLabelFontString=trackLengthLabelFontString;
          leftValueFontString=trackLengthValueFontString;
          rightLabelFontString=speedLabelFontString;
          rightValueFontString=speedValueFontString;
          break;
        }
      } else {
        switch (secondRowState) {
        case 0:
          leftLabelFontString=durationLabelFontString;
          leftValueFontString=durationValueFontString;
          break;
        case 1:
          leftLabelFontString=altitudeLabelFontString;
          leftValueFontString=altitudeValueFontString;
          break;
        case 2:
          leftLabelFontString=trackLengthLabelFontString;
          leftValueFontString=trackLengthValueFontString;
          break;
        case 3:
          leftLabelFontString=speedLabelFontString;
          leftValueFontString=speedValueFontString;
          break;
        }
      }
      if (leftLabelFontString) {
        c=leftLabelFontString->getColor();
        c.setAlpha(color.getAlpha());
        leftLabelFontString->setColor(c);
        leftLabelFontString->draw(t);
        c=leftValueFontString->getColor();
        c.setAlpha(color.getAlpha());
        leftValueFontString->setColor(c);
        leftValueFontString->draw(t);
      }
      if (rightLabelFontString) {
        c=rightLabelFontString->getColor();
        c.setAlpha(color.getAlpha());
        rightLabelFontString->setColor(c);
        rightLabelFontString->draw(t);
        c=rightValueFontString->getColor();
        c.setAlpha(color.getAlpha());
        rightValueFontString->setColor(c);
        rightValueFontString->draw(t);
      }
      if (clockFontString) {
        c=clockFontString->getColor();
        c.setAlpha(color.getAlpha());
        clockFontString->setColor(c);
        clockFontString->draw(t);
      }
    }
  }
}

// Called when the widget has changed its position
void WidgetNavigation::updatePosition(Int x, Int y, Int z) {
  WidgetPrimitive::updatePosition(x,y,z);
  firstRun=true;
}

// Called when the widget is touched
void WidgetNavigation::onTouchDown(TimestampInMicroseconds t, Int x, Int y) {
  WidgetPrimitive::onTouchDown(t,x,y);
}

// Executed if the widget has been untouched
void WidgetNavigation::onTouchUp(TimestampInMicroseconds t, Int x, Int y, bool cancel) {
  WidgetPrimitive::onTouchUp(t,x,y,cancel);

  // Are we hit?
  if (getIsHit()) {

    // Turn currently active?
    if (showTurn) {

      // Skip the current turn
      skipTurn=true;

    } else {

      // On a watch?
      if (isWatch) {

        // Deactivate swipe if north button is hit
        if (northButtonHit) {
          core->getCommander()->dispatch("deactivateSwipes()");
          widgetPage->setWidgetsActive(t,false);
        } else {

          // Deactivate widgets after second touch up
          if (!firstTouchAfterInactive) {
            widgetPage->setWidgetsActive(t,false);
          } else {
            firstTouchAfterInactive=false;
          }
        }

      } else {

        // Rotate the infos
        secondRowState++;
        if (textColumnCount==2) {
          if (secondRowState>1)
            secondRowState=0;
        } else {
          if (secondRowState>3)
            secondRowState=0;
        }
      }
    }
  }
}

// Updates various flags
void WidgetNavigation::updateFlags(Int x, Int y) {
  WidgetPrimitive::updateFlags(x,y);
  if ((isWatch)&&(isHit)) {
    double dx=x-(getX()+getIconWidth()/2);
    double dy=y-(getY()+getIconHeight()/2);
    double radius = sqrt(dx*dx+dy*dy);
    if (radius<minTouchDetectionRadius) {
      isHit=false;
      if (showTurn) {
        skipTurn=true;
      }
    } else {
      double angle = FloatingPoint::rad2degree(FloatingPoint::computeAngle(dx,dy));
      //DEBUG("angle=%f",angle);
      if ((angle >= 90.0-circularButtonAngle/2)&&(angle <= 90.0+circularButtonAngle/2)) {
        northButtonHit=true;
      } else {
        northButtonHit=false;
        if ((angle <= 270.0-circularButtonAngle/2)||(angle >= 270.0+circularButtonAngle/2)) {
          isHit=false;
          statusTextAbove=!statusTextAbove;
        }
      }
    }
  }
}

}
