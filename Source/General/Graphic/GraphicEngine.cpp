//============================================================================
// Name        : GraphicEngine.cpp
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
#include <Commander.h>
#include <GraphicEngine.h>
#include <Device.h>
#include <ProfileEngine.h>
#include <WidgetEngine.h>
#include <FontEngine.h>
#include <GraphicLine.h>
#include <GraphicRectangleList.h>
#include <MapEngine.h>

namespace GEODISCOVERER {

GraphicEngine::GraphicEngine(Device *device) :
    pathAnimators(device->getScreen()),
    centerIcon(device->getScreen()),
    locationIcon(device->getScreen()),
    targetIcon(device->getScreen()),
    navigationPointIcon(device->getScreen()),
    arrowIcon(device->getScreen()),
    pathDirectionIcon(device->getScreen()),
    pathStartFlagIcon(device->getScreen()),
    pathEndFlagIcon(device->getScreen()),
    compassConeIcon(device->getScreen()),
    tileImageNotCached(device->getScreen()),
    tileImageNotDownloaded(device->getScreen()),
    tileImageDownloadErrorOccured(device->getScreen())
{

  // Init variables
  this->device=device;
  map=NULL;
  navigationPoints=NULL;
  drawingMutex=core->getThread()->createMutex("graphic engine drawing mutex");
  posMutex=core->getThread()->createMutex("graphic engine pos mutex");
  debugMode=core->getConfigStore()->getIntValue("Graphic","debugMode",__FILE__, __LINE__);
  locationAccuracyBackgroundColor=core->getConfigStore()->getGraphicColorValue("Graphic/LocationAccuracyBackgroundColor",__FILE__, __LINE__);
  locationAccuracyCircleColor=core->getConfigStore()->getGraphicColorValue("Graphic/LocationAccuracyCircleColor",__FILE__, __LINE__);
  locationAccuracyRadiusX=0;
  locationAccuracyRadiusY=0;
  centerIconTimeout=core->getConfigStore()->getIntValue("Graphic","centerIconTimeout",__FILE__, __LINE__);
  centerIcon.setColor(GraphicColor(255,255,255,0));
  locationIcon.setColor(GraphicColor(255,255,255,0));
  compassConeIcon.setColor(core->getConfigStore()->getGraphicColorValue("Graphic/CompassConeColor",__FILE__, __LINE__));
  compassConeIcon.setAngle(std::numeric_limits<double>::max());
  targetIcon.setColor(GraphicColor(255,255,255,0));
  arrowIcon.setColor(GraphicColor(255,255,255,0));
  navigationPointIcon.setColor(GraphicColor(255,255,255,255));
  lastCenterIconFadeStartTime=0;
  isDrawing=false;
  lastDrawingStartTime=0;
  minDrawingTime=+std::numeric_limits<double>::max();
  maxDrawingTime=-std::numeric_limits<double>::max();
  totalDrawingTime=0;
  minIdleTime=+std::numeric_limits<double>::max();
  maxIdleTime=-std::numeric_limits<double>::max();
  totalIdleTime=0;
  frameCount=0;
  tileImageNotCached.setColor(GraphicColor(255,255,255,0));
  tileImageNotDownloaded.setColor(GraphicColor(255,255,255,0));
  tileImageDownloadErrorOccured.setColor(GraphicColor(255,255,255,0));
  animDuration=core->getConfigStore()->getIntValue("Graphic","animDuration",__FILE__, __LINE__);
  ambientModeTransitionDuration=core->getConfigStore()->getIntValue("Graphic","ambientModeTransitionDuration",__FILE__, __LINE__);
  blinkDuration=core->getConfigStore()->getIntValue("Graphic","blinkDuration",__FILE__, __LINE__);
  mapReferenceDPI=core->getConfigStore()->getIntValue("Graphic","mapReferenceDotsPerInch",__FILE__,__LINE__);
  timeOffsetPeriod=core->getConfigStore()->getIntValue("Graphic","timeOffsetPeriod",__FILE__,__LINE__);
  targetDrawingTime=(TimestampInMicroseconds) 1.0/((double)core->getConfigStore()->getIntValue("Graphic","refreshRate",__FILE__,__LINE__))*1.0e6; 
  drawingTooSlow=false;
  ambientModeStartTime=0;
  interactiveModeStartTime=0;
  widgetlessModeStartTime=0;
  widgetfullModeStartTime=0;
  currentTime=0;
  ambientTransitionActive=false;
  
  // Init the dynamic data
  init();
}

// Inits dynamic data
void GraphicEngine::init() {
  if (device->getIsWatch()) {
    WARNING("Disable ambient leave delay once Samsung fixes the WearOS 4.0 bug",NULL);
  }
}

// Clears all graphic
void GraphicEngine::destroyGraphic() {
  deinit();
}

// Creates all graphic
void GraphicEngine::createGraphic() {
  lockDrawing(__FILE__, __LINE__);
  centerIcon.setTextureFromIcon(device->getScreen(),core->getConfigStore()->getStringValue("Graphic","centerIconFilename",__FILE__, __LINE__));
  locationIcon.setTextureFromIcon(device->getScreen(),core->getConfigStore()->getStringValue("Graphic","locationIconFilename",__FILE__, __LINE__));
  pathDirectionIcon.setTextureFromIcon(device->getScreen(),core->getConfigStore()->getStringValue("Graphic","pathDirectionIconFilename",__FILE__, __LINE__));
  //DEBUG("pathDirectionIcon.texture=%u",pathDirectionIcon.getTexture())
  pathStartFlagIcon.setTextureFromIcon(device->getScreen(),core->getConfigStore()->getStringValue("Graphic","pathStartFlagIconFilename",__FILE__, __LINE__));
  pathEndFlagIcon.setTextureFromIcon(device->getScreen(),core->getConfigStore()->getStringValue("Graphic","pathEndFlagIconFilename",__FILE__, __LINE__));
  compassConeIcon.setTextureFromIcon(device->getScreen(),core->getConfigStore()->getStringValue("Graphic","compassConeFilename",__FILE__, __LINE__));
  targetIcon.setTextureFromIcon(device->getScreen(),core->getConfigStore()->getStringValue("Graphic","targetIconFilename",__FILE__, __LINE__));
  navigationPointIcon.setTextureFromIcon(device->getScreen(),core->getConfigStore()->getStringValue("Graphic","navigationPointIconFilename",__FILE__, __LINE__));
  arrowIcon.setTextureFromIcon(device->getScreen(),core->getConfigStore()->getStringValue("Graphic","arrowIconFilename",__FILE__, __LINE__));
  tileImageNotCached.setTextureFromIcon(device->getScreen(),core->getConfigStore()->getStringValue("Graphic","tileImageNotCachedFilename",__FILE__, __LINE__));
  tileImageNotDownloaded.setTextureFromIcon(device->getScreen(),core->getConfigStore()->getStringValue("Graphic","tileImageNotDownloadedFilename",__FILE__, __LINE__));
  tileImageDownloadErrorOccured.setTextureFromIcon(device->getScreen(),core->getConfigStore()->getStringValue("Graphic","tileImageDownloadErrorOccuredFilename",__FILE__, __LINE__));
  unlockDrawing();
}

// Deinits dynamic data
void GraphicEngine::deinit() {
  lockDrawing(__FILE__, __LINE__);
  centerIcon.deinit();
  locationIcon.deinit();
  pathDirectionIcon.deinit();
  pathStartFlagIcon.deinit();
  pathEndFlagIcon.deinit();
  targetIcon.deinit();
  navigationPointIcon.deinit();
  arrowIcon.deinit();
  compassConeIcon.deinit();
  tileImageNotCached.deinit();
  tileImageNotDownloaded.deinit();
  tileImageDownloadErrorOccured.deinit();
  unlockDrawing();
}

// Does the drawing
bool GraphicEngine::draw(bool forceRedraw) {

  bool result=true;
  Screen *screen = device->getScreen();
  GraphicPosition pos;
  TimestampInMicroseconds idleTime,drawingTime;
  bool redrawScene=false;
  bool dataHasChanged=false;
  Int x1,y1,x2,y2,x,y;
  bool isDefaultScreen = core->getDefaultScreen() == screen;
  double scale,backScale;
  GraphicObject *widgetGraphicObject = device->getVisibleWidgetPages();
  GraphicObject *widgetFingerMenu = device->getFingerMenu();

  /* Activate the screen
  screen->startScene();

  // Clear the scene
  screen->clear();

  // Set default line width
  screen->setLineWidth(1);

  // Testing
  screen->setColor(255,0,255,255);
  screen->setLineWidth(10);
  screen->drawRectangle(-100,-100,+100,+100,Screen::getTextureNotDefined(),false);
  screen->startObject();
  screen->setLineWidth(10);
  screen->setColor(255,0,255,255);
  screen->scale(100,100,1.0);
  screen->drawEllipse(false);
  screen->endObject();

  screen->endScene();

  // Write a png if this is not the default screen
  if (!isDefaultScreen) {
    screen->createScreenShot();
  }

  return;*/

  // Get the time
  this->currentTime=core->getClock()->getMicrosecondsSinceStart();

#ifdef PROFILING_ENABLED
  core->getProfileEngine()->clearResult(__PRETTY_FUNCTION__);
#endif
  PROFILE_START;

  // Let the position work and copy the result
  lockPos(__FILE__,__LINE__);
  if (this->pos.work(currentTime)) {
    redrawScene=true;
  }
  pos=this->pos;
  unlockPos();
  //PROFILE_ADD("position copy");

  // Drawing starts
  lockDrawing(__FILE__,__LINE__);
  PROFILE_ADD("drawing lock");

  // Start measuring of drawing time and utilization
#ifdef PROFILING_ENABLED
  idleTime=currentTime-lastDrawingStartTime;
  lastDrawingStartTime=currentTime;
#endif

  // Indicate that the engine is drawing
  isDrawing=true;

  // Set the time offset
  screen->setTimeOffset(((double)(currentTime%timeOffsetPeriod))/(double)timeOffsetPeriod);

  // Force redraw if requested externally
  if (forceRedraw) {
    DEBUG("forcing redraw due to external request",NULL);
    redrawScene=true;
  }
  //PROFILE_ADD("init");

  // Do stuff for the default screen
  if (isDefaultScreen) {

    // Let the map primitives work
    if (map) {
      if (map->work(currentTime)) {
        //DEBUG("requesting scene redraw due to map work result",NULL);
        redrawScene=true;
      }
      if (map->getIsUpdated()) {
        //DEBUG("requesting scene redraw due to changed map object",NULL);
        redrawScene=true;
        map->setIsUpdated(false);
      }
    }

    // Let the path animators primitives work
    if (pathAnimators.work(currentTime)) {
      //DEBUG("requesting scene redraw due to paths work result",NULL);
      redrawScene=true;
    }
    if (pathAnimators.getIsUpdated()) {
      //DEBUG("requesting scene redraw due to changed paths object",NULL);
      redrawScene=true;
      pathAnimators.setIsUpdated(false);
    }

    // Handle the hiding of the center icon
    TimestampInMicroseconds fadeStartTime=pos.getLastUserModification()+centerIconTimeout;
    if (fadeStartTime!=lastCenterIconFadeStartTime) {
      centerIcon.setColor(GraphicColor(255,255,255,255));
      centerIcon.setFadeAnimation(fadeStartTime,centerIcon.getColor(),GraphicColor(255,255,255,0),false,getAnimDuration());
    }
    lastCenterIconFadeStartTime=fadeStartTime;

    // Let the center primitive work
    if (centerIcon.work(currentTime)) {
      //DEBUG("requesting scene redraw due to center icon work result",NULL);
      redrawScene=true;
    }

    // Let the target primitive work
    if (targetIcon.work(currentTime)) {
      //DEBUG("requesting scene redraw due to target icon work result",NULL);
      redrawScene=true;
    }
    if (targetIcon.getIsUpdated()) {
      //DEBUG("requesting scene redraw due to changed location icon",NULL);
      redrawScene=true;
      targetIcon.setIsUpdated(false);
    }

    // Let the arrow primitive work
    if (arrowIcon.work(currentTime)) {
      //DEBUG("requesting scene redraw due to arrow icon work result",NULL);
      redrawScene=true;
    }
    if (arrowIcon.getIsUpdated()) {
      //DEBUG("requesting scene redraw due to changed location icon",NULL);
      redrawScene=true;
      arrowIcon.setIsUpdated(false);
    }

    // Let the navigation point primitives work
    if (navigationPoints) {
      if (navigationPoints->work(currentTime)) {
        //DEBUG("requesting scene redraw due to navigation points work result",NULL);
        redrawScene=true;
        dataHasChanged=true;
      }
    }

    // Did the pos change?
    if (pos!=previousPosition) {

      // Scene must be redrawn
      //DEBUG("requesting scene redraw due to pos change",NULL);
      redrawScene=true;

    }

    // Did the location icon change?
    if (locationIcon.getIsUpdated()) {
      //DEBUG("requesting scene redraw due to changed location icon",NULL);
      redrawScene=true;
      locationIcon.setIsUpdated(false);
    }

    // Did the compass cone icon change?
    if (compassConeIcon.getIsUpdated()) {
      //DEBUG("requesting scene redraw due to changed compass cone icon",NULL);
      redrawScene=true;
      compassConeIcon.setIsUpdated(false);
    }
  }

  // Let the widget primitives work
  if ((widgetGraphicObject)&&(widgetGraphicObject->work(currentTime))) {
    //DEBUG("requesting scene redraw due to widget page work result",NULL);
    redrawScene=true;
  }
  if ((widgetFingerMenu)&&(widgetFingerMenu->work(currentTime))) {
    //DEBUG("requesting scene redraw due to widget finger menu work result",NULL);
    redrawScene=true;
  }
  device->getWidgetEngine()->work(currentTime);

  // Check if two frames without any change have been drawn
  if (!redrawScene) {
    if (device->getNoChangeFrameCount()<2) {
      //DEBUG("requesting scene redraw due to not enough unchanged frames",NULL);
      redrawScene=true;
      device->setNoChangeFrameCount(device->getNoChangeFrameCount()+1);
    }
  } else {
    device->setNoChangeFrameCount(0);
  }

  PROFILE_ADD("drawing work and update check");

  // Redraw required?
  if (redrawScene) {

    // Activate the screen
    screen->startScene();

    // Clear the scene
    screen->clear();

    // Set default line width
    screen->setLineWidth(1);

    /* Testing
    screen->setColor(255,255,255,255);
    screen->setLineWidth(10);
    screen->drawRectangle(-100,-100,+100,+100,Screen::getTextureNotDefined(),false);
    screen->startObject();
    screen->setLineWidth(10);
    screen->setColor(255,0,255,255);
    screen->scale(100,100,1.0);
    screen->drawEllipse(false);
    screen->endObject();
*/

    //PROFILE_ADD("drawing init");

    // Do the stuff for the default screen
    if (isDefaultScreen) {

      // Check if we are in ambient mode
      double fadeScale=getAmbientFadeScale();
      if (fadeScale>0.0) {
        ambientTransitionActive=true;

        // Set the map window position
        screen->startObject();
        if (map) screen->translate(map->getX(),map->getY(),0);

        // Start the map object
        screen->setAlphaScale(fadeScale);
        screen->startObject();

        // Set rotation factor
        //DEBUG("pos.getAngle()=%f",pos.getAngle());
        screen->rotate(pos.getAngle(),0,0,1);

        // Set scaling factor
        //DEBUG("pos.getZoom()=%f",pos.getZoom());
        double screenScale = getMapTileToScreenScale(screen);
        scale=pos.getZoom()*screenScale;
        backScale=1.0/scale;
        //std::cout << "scale=" << scale << std::endl;
        //scale=scale/2;
        screen->scale(scale,scale,1.0);

        // Set translation factors
        //DEBUG("pos.getX()=%d pos.getY()=%d",pos.getX(),pos.getY());
        screen->translate(-pos.getX(),-pos.getY(),0);

        // Draw all primitives of the map object
        if (map) {

          // Scale the map tiles according to the screen dpi

          // Draw the tiles
          for(Int z=0;z<=4;z++) {
            std::list<GraphicPrimitive*> *mapDrawList=map->getDrawList();
            //DEBUG("tile count = %d",mapDrawList->size());
            for(std::list<GraphicPrimitive *>::iterator i=mapDrawList->begin(); i != mapDrawList->end(); i++) {

              //DEBUG("inside primitive draw",NULL);
              switch((*i)->getType()) {

                case GraphicTypeObject:
                {
                  // Get graphic object
                  GraphicObject *tileVisualization;
                  tileVisualization=(GraphicObject*)*i;

                  // Skip drawing if primitive is invisible
                  UByte alpha=tileVisualization->getColor().getAlpha();
                  if (alpha!=0) {

                    // Position the object
                    screen->startObject();
                    screen->translate(tileVisualization->getX(),tileVisualization->getY(),tileVisualization->getZ());

                    // Go through all objects of the tile visualization for the requested z
                    std::list<GraphicPrimitive*>::iterator j=tileVisualization->getFirstElementInDrawList(z);
                    //DEBUG("tile count = %d",mapDrawList->size());
                    while(j!=tileVisualization->getDrawList()->end()&&((*j)->getZ()==z)) {

                      // Which type of primitive?
                      GraphicPrimitive *primitive=*j;
                      switch(primitive->getType()) {

                        // Rectangle primitive?
                        case GraphicTypeRectangle:
                        {
                          GraphicRectangle *rectangle=(GraphicRectangle*)primitive;

                          // Set color
                          screen->setColor(rectangle->getColor().getRed(),rectangle->getColor().getGreen(),rectangle->getColor().getBlue(),rectangle->getColor().getAlpha());

                          // Dimm the color in debug mode
                          // This allows to differntiate the tiles
                          GraphicColor originalColor;
                          if ((debugMode)&&(primitive->getName().size()!=0)) {
                            originalColor=rectangle->getColor();
                            GraphicColor modifiedColor=originalColor;
                            modifiedColor.setRed(originalColor.getRed()/2);
                            modifiedColor.setGreen(originalColor.getGreen()/2);
                            modifiedColor.setBlue(originalColor.getBlue()/2);
                            rectangle->setColor(modifiedColor);
                          }

                          // Draw the rectangle
                          rectangle->draw(currentTime);

                          // Restore the color
                          if ((debugMode)&&(primitive->getName().size()!=0)) {
                            rectangle->setColor(originalColor);
                          }

                          //DEBUG("rectangle->getTexture()=%d screen->getTextureNotDefined()=%d",rectangle->getTexture(),screen->getTextureNotDefined());

                          // If the texture is not defined, draw a box around it and it's name inside the box
                          if ((primitive->getName().size()!=0)&&(debugMode)) {

                            // Draw the name of the tile
                            //std::string name=".";
                            if (debugMode) {
                              std::list<std::string> name=rectangle->getName();
                              FontEngine *fontEngine=device->getFontEngine();
                              fontEngine->lockFont("sansTiny", __FILE__, __LINE__);
                              Int nameHeight=name.size()*fontEngine->getLineHeight();
                              Int lineNr=name.size()-1;
                              x1=rectangle->getX();
                              y1=rectangle->getY();
                              x2=x1+rectangle->getWidth();
                              y2=y1+rectangle->getHeight();
                              for(std::list<std::string>::iterator i=name.begin();i!=name.end();i++) {
                                //DEBUG("text=%s",(*i).c_str());
                                FontString *fontString=fontEngine->createString(*i);
                                //fontString->setX(x1+(rectangle->getWidth()-fontString->getIconWidth())/2);
                                //fontString->setY(y1+(rectangle->getHeight()-nameHeight)/2+lineNr*fontEngine->getLineHeight());
                                fontString->setX(-fontString->getIconWidth()/2);
                                fontString->setY(-fontString->getIconHeight()/2);
                                screen->startObject();
                                screen->translate(x1+rectangle->getWidth()/2,y1+(rectangle->getHeight()-nameHeight/2)/2+lineNr*fontEngine->getLineHeight(),0);
                                screen->startObject();
                                screen->scale(0.5,0.5,1.0);
                                fontString->draw(currentTime);
                                screen->endObject();
                                screen->endObject();
                                fontEngine->destroyString(fontString);
                                lineNr--;
                              }
                              fontEngine->unlockFont();
                            }

                            // Draw the borders of the tile
                            screen->setColor(255,255,255,255);
                            screen->setLineWidth(1);
                            screen->drawRectangle(x1,y1,x2,y2,screen->getTextureNotDefined(),false);
                          }
                          break;
                        }

                        // Line primitive?
                        case GraphicTypeLine:
                        {
                          GraphicLine *line=(GraphicLine*)primitive;
                          screen->setColorModeMultiply();
                          GraphicColor color=line->getAnimator()->getColor();
                          screen->setColor(color.getRed(),color.getGreen(),color.getBlue(),color.getAlpha());
                          line->draw();
                          screen->setColorModeAlpha();
                          break;
                        }

                        // Rectangle list primitive?
                        case GraphicTypeRectangleList:
                        {
                          GraphicRectangleList *rectangleList=(GraphicRectangleList*)primitive;
                          GraphicColor color=rectangleList->getAnimator()->getColor();
                          screen->setColor(color.getRed(),color.getGreen(),color.getBlue(),color.getAlpha());
                          rectangleList->draw();
                          break;
                        }

                        default:
                          FATAL("unknown primitive type",NULL);
                          break;
                      }
                      j++;
                    }

                    // That's it
                    screen->endObject();
                  }

                  // Unlock the visualization
                  break;
                }

                case GraphicTypeRectangle:
                {
                  GraphicRectangle *r=(GraphicRectangle*)*i;
                  if (r->getZ()==z) {
                    x1=r->getX();
                    y1=r->getY();
                    x2=(GLfloat)(r->getWidth()+x1);
                    y2=(GLfloat)(r->getHeight()+y1);
                    screen->startObject();
                    screen->setColor(r->getColor().getRed(),r->getColor().getGreen(),r->getColor().getBlue(),r->getColor().getAlpha());
                    screen->setLineWidth(1);
                    screen->drawRectangle(x1,y1,x2,y2,screen->getTextureNotDefined(),false);
                    screen->endObject();
                  }
                  break;
                }

                default:
                  FATAL("unknown primitive type",NULL);
                  break;
              }
            }
          }

        }

        //PROFILE_ADD("map drawing");

        // Draw all navigation points
        if (navigationPoints) {
          std::list<GraphicPrimitive*> *drawList=navigationPoints->getDrawList();
          std::list<GraphicPrimitive*> eraseList;
          screen->startObject();
          screen->setColor(navigationPointIcon.getColor().getRed(),navigationPointIcon.getColor().getGreen(),navigationPointIcon.getColor().getBlue(),navigationPointIcon.getColor().getAlpha());
          for(std::list<GraphicPrimitive *>::const_iterator i=drawList->begin(); i != drawList->end(); i++) {
            GraphicRectangle *r = (GraphicRectangle*) *i;

            // If point is scaled down to 0, remove it from the drawing
            if (r->getScale()==0.0) {
              eraseList.push_back(*i);
            } else {

              // Draw the point
              screen->startObject();
              screen->translate(r->getX(),r->getY(),0);
              screen->rotate(-pos.getAngle(),0,0,1);
              screen->scale(backScale,backScale,1.0);
              screen->scale(r->getScale(),r->getScale(),1.0);
              x1=-r->getIconWidth()/2;
              y1=-r->getIconHeight()/2;
              x2=x1+r->getWidth();
              y2=y1+r->getHeight();
              screen->drawRectangle(x1,y1,x2,y2,r->getTexture(),true);
              screen->endObject();
            }
          }
          screen->endObject();

          // Execute the removal of invisible points
          for(std::list<GraphicPrimitive *>::const_iterator i=eraseList.begin(); i != eraseList.end(); i++) {
            navigationPoints->removePrimitive(navigationPoints->getPrimitiveKey(*i),true);
          }
        }

        //PROFILE_ADD("address points drawing");

        // Draw the location icon and the compass cone
        //DEBUG("locationIcon.getColor().getAlpha()=%d locationIcon.getX()=%d locationIcon.getY()=%d",locationIcon.getColor().getAlpha(),locationIcon.getX(),locationIcon.getY());
        if (locationIcon.getColor().getAlpha()>0) {

          // Translate to the current location
          screen->startObject();
          screen->translate(locationIcon.getX(),locationIcon.getY(),0);
          screen->scale(backScale,backScale,1.0);

          // Draw the accuracy circle
          screen->startObject();
          screen->setColor(locationAccuracyBackgroundColor.getRed(),locationAccuracyBackgroundColor.getGreen(),locationAccuracyBackgroundColor.getBlue(),locationAccuracyBackgroundColor.getAlpha());
          screen->scale(locationAccuracyRadiusX*scale,locationAccuracyRadiusY*scale,1.0);
          screen->drawEllipse(true);
          screen->endObject();

          // Draw the location icon
          screen->startObject();
          x1=-locationIcon.getIconWidth()/2;
          y1=-locationIcon.getIconHeight()/2;
          x2=x1+locationIcon.getWidth();
          y2=y1+locationIcon.getHeight();
          screen->rotate(locationIcon.getAngle(),0,0,1);
          screen->setColor(locationIcon.getColor().getRed(),locationIcon.getColor().getGreen(),locationIcon.getColor().getBlue(),locationIcon.getColor().getAlpha());
          screen->drawRectangle(x1,y1,x2,y2,locationIcon.getTexture(),true);
          screen->endObject();

          // Draw the compass cone
          if (compassConeIcon.getAngle()!=std::numeric_limits<double>::max()) {
            screen->startObject();
            screen->setColor(compassConeIcon.getColor().getRed(),compassConeIcon.getColor().getGreen(),compassConeIcon.getColor().getBlue(),compassConeIcon.getColor().getAlpha());
            x1=-compassConeIcon.getIconWidth()/2;
            y1=0;
            x2=x1+compassConeIcon.getWidth();
            y2=y1+compassConeIcon.getHeight();
            screen->rotate(compassConeIcon.getAngle(),0,0,1);
            screen->drawRectangle(x1,y1,x2,y2,compassConeIcon.getTexture(),true);
            screen->endObject();
          }
          screen->endObject();
        }
        //WARNING("enable location icon",NULL);
        //DEBUG("locationAccuradyRadiusX=%d locationAccuracyRadiusY=%d",locationAccuracyRadiusX,locationAccuracyRadiusY);
        //PROFILE_ADD("location drawing");

        // Draw the target icon
        if (targetIcon.getColor().getAlpha()>0) {

          // Translate to the target location
          screen->startObject();
          screen->translate(targetIcon.getX(),targetIcon.getY(),0);
          screen->scale(backScale,backScale,1.0);

          // Draw the target icon
          screen->startObject();
          screen->scale(targetIcon.getScale(),targetIcon.getScale(),1.0);
          screen->rotate(targetIcon.getAngle(),0,0,1);
          x1=-targetIcon.getIconWidth()/2;
          y1=-targetIcon.getIconHeight()/2;
          x2=x1+targetIcon.getWidth();
          y2=y1+targetIcon.getHeight();
          screen->setColor(targetIcon.getColor().getRed(),targetIcon.getColor().getGreen(),targetIcon.getColor().getBlue(),targetIcon.getColor().getAlpha());
          screen->drawRectangle(x1,y1,x2,y2,targetIcon.getTexture(),true);
          screen->endObject();
          screen->endObject();
        }
        //PROFILE_ADD("target icon drawing");

        // Draw the arrow icon
        if (arrowIcon.getColor().getAlpha()>0) {

          // Translate to the target location
          screen->startObject();
          screen->translate(arrowIcon.getX(),arrowIcon.getY(),0);
          screen->scale(backScale,backScale,1.0);

          // Draw the target icon
          screen->startObject();
          screen->scale(arrowIcon.getScale(),arrowIcon.getScale(),1.0);
          screen->rotate(arrowIcon.getAngle(),0,0,1);
          x1=-arrowIcon.getIconWidth()/2;
          y1=-arrowIcon.getIconHeight()/2;
          x2=x1+arrowIcon.getWidth();
          y2=y1+arrowIcon.getHeight();
          screen->setColor(arrowIcon.getColor().getRed(),arrowIcon.getColor().getGreen(),arrowIcon.getColor().getBlue(),arrowIcon.getColor().getAlpha());
          screen->drawRectangle(x1,y1,x2,y2,arrowIcon.getTexture(),true);
          screen->endObject();
          screen->endObject();
        }
        //PROFILE_ADD("arrow drawing");

        // End the map object
        screen->endObject();

        // Draw the cursor
        //DEBUG("cursor drawing!",NULL);
        screen->startObject();
        screen->setColor(centerIcon.getColor().getRed(),centerIcon.getColor().getGreen(),centerIcon.getColor().getBlue(),centerIcon.getColor().getAlpha());
        //DEBUG("texture=%d",centerIcon.getTexture());
        x1=-centerIcon.getIconWidth()/2;
        x2=x1+centerIcon.getWidth();
        y1=-centerIcon.getIconHeight()/2;
        y2=y1+centerIcon.getHeight();
        //DEBUG("x1=%d y1=%d x2=%d y2=%d",x1,y1,x2,y2);
        screen->drawRectangle(x1,y1,x2,y2,centerIcon.getTexture(),true);
        screen->endObject();
        screen->setAlphaScale(1.0);
        //PROFILE_ADD("cursor drawing");

        // Finish the map window translation
        screen->endObject();
        //PROFILE_ADD("overlay drawing");
      } else {
        if (ambientTransitionActive) {
          //DEBUG("ambient transition over",NULL);
          core->getCommander()->dispatch("ambientTransitionFinished()");
          ambientTransitionActive=false;
        }
      }
    }

    // Get the fade scale depending on ambiet mode
    double fadeScale=getWidgetlessFadeScale();
    if (fadeScale!=1.0)
      screen->setAlphaScale(fadeScale);
    if (fadeScale>0) {

      // Draw all widgets
      if (widgetGraphicObject) {

        std::list<GraphicPrimitive*> *pageDrawList=widgetGraphicObject->getDrawList();
        for(std::list<GraphicPrimitive *>::const_iterator i=pageDrawList->begin(); i != pageDrawList->end(); i++) {
          GraphicObject *page = (GraphicObject*) *i;
          std::list<GraphicPrimitive*> *widgetDrawList=page->getDrawList();
          screen->startObject();
          screen->translate(page->getX(),page->getY(),0);
          for(std::list<GraphicPrimitive *>::const_iterator i=widgetDrawList->begin(); i != widgetDrawList->end(); i++) {

            // Draw the widget
            WidgetPrimitive *widget;
            widget=(WidgetPrimitive*)*i;
            widget->draw(currentTime);
          }
          screen->endObject();
        }
      }
      //PROFILE_ADD("widget drawing");

      // Draw the finger menu
      if (widgetFingerMenu) {
        std::list<GraphicPrimitive*> *fingerMenuDrawList=widgetFingerMenu->getDrawList();
        for(std::list<GraphicPrimitive *>::const_iterator i=fingerMenuDrawList->begin(); i != fingerMenuDrawList->end(); i++) {
          WidgetPrimitive *widget;
          widget=(WidgetPrimitive*)*i;
          widget->draw(currentTime);
        }
      }      
    }
    if (fadeScale!=1.0)
      screen->setAlphaScale(1.0);

    // Finish the drawing
    screen->endScene();

    // Write a png if this is not the default screen
    if (!isDefaultScreen) {
      result = screen->createScreenShot();
    }
  }

  // Remember the last position
  previousPosition=pos;

  // Update the time measurement
#ifdef PROFILING_ENABLED
  drawingTime=core->getClock()->getMicrosecondsSinceStart()-lastDrawingStartTime;
  if (idleTime>maxIdleTime) {
    maxIdleTime=idleTime;
  }
  if (idleTime<minIdleTime) {
    minIdleTime=idleTime;
  }
  totalIdleTime+=idleTime;
  if (drawingTime>maxDrawingTime) {
    maxDrawingTime=drawingTime;
  }
  if (drawingTime<minDrawingTime) {
    minDrawingTime=drawingTime;
  }
  totalDrawingTime+=drawingTime;
  frameCount++;
  if (drawingTime>targetDrawingTime) {
    drawingTooSlow=true;
  } else {
    drawingTooSlow=false;
  }
#endif

  // Everything done
  isDrawing=false;

  // Drawing ends
  unlockDrawing();

  //PROFILE_ADD("cleanup");
  PROFILE_ADD("drawing itself");
  //PROFILE_END;
#ifdef PROFILING_ENABLED
  if (drawingTooSlow) {
    core->getProfileEngine()->outputResult(__PRETTY_FUNCTION__,true);
  }
#endif

  // Has data changed?
  if (dataHasChanged) {
    core->onDataChange();
  }

  // Executes commands (if any)
  device->getWidgetEngine()->executeCommands();

  // Enforce a frame rate by waiting the rest of the time
  TimestampInMicroseconds currentDrawingTime = core->getClock()->getMicrosecondsSinceStart() - currentTime;
  if (targetDrawingTime>currentDrawingTime) {
    usleep(targetDrawingTime-currentDrawingTime);
  }
  
  return result;
}

// Destructor
GraphicEngine::~GraphicEngine() {
  core->getThread()->destroyMutex(drawingMutex);
  core->getThread()->destroyMutex(posMutex);
}

// Outputs statistical infos
void GraphicEngine::outputStats() {
  lockDrawing(__FILE__,__LINE__);
  double avgIdleTime=(double)totalIdleTime/(double)frameCount;
  double avgDrawingTime=(double)totalDrawingTime/(double)frameCount;
  DEBUG("printing drawing statistics",NULL);
  DEBUG("minIdleTime=   %8.2fms * avgIdleTime=   %8.2fms * maxIdleTime=   %8.2fms * ",
         minIdleTime/1000.0,avgIdleTime/1000.0,maxIdleTime/1000.0);
  DEBUG("minDrawingTime=%8.2fms * avgDrawingTime=%8.2fms * maxDrawingTime=%8.2fms * ",
         minDrawingTime/1000.0,avgDrawingTime/1000.0,maxDrawingTime/1000.0);
  DEBUG("avgLoad=%6.2f%%",avgDrawingTime/(avgIdleTime+avgDrawingTime)*100.0);
  frameCount=0;
  minDrawingTime=+std::numeric_limits<double>::max();
  maxDrawingTime=-std::numeric_limits<double>::max();
  totalDrawingTime=0;
  minIdleTime=+std::numeric_limits<double>::max();
  maxIdleTime=-std::numeric_limits<double>::max();
  totalIdleTime=0;
  unlockDrawing();
}

// Returns the additional scale to match the scale the map tiles have been made for
double GraphicEngine::getMapTileToScreenScale(Screen *screen) {
  return ((double)screen->getDPI()) / ((double)mapReferenceDPI);
}

// Checks if display is in ambient mode
bool GraphicEngine::isAmbientMode(TimestampInMicroseconds &duration) {
  duration=0;
  //DEBUG("currentTime=%ld",currentTime);
  //DEBUG("ambientModeStartTime=%ld currentTime=%ld",ambientModeStartTime,currentTime);
  if ((ambientModeStartTime!=0)&&(currentTime>=ambientModeStartTime)) {
    duration=currentTime-ambientModeStartTime;
    //DEBUG("duration=%ld",duration);
    return true;
  } else {
    duration=currentTime-interactiveModeStartTime;
    return false;
  }
}

// Sets the stat time of the ambient mode
void GraphicEngine::setAmbientModeStartTime(TimestampInMicroseconds offset) {
  TimestampInMicroseconds t=core->getClock()->getMicrosecondsSinceStart();
  if ((ambientModeStartTime!=0)&&(t>=ambientModeStartTime)) 
    interactiveModeStartTime=t;
  ambientModeStartTime=t+offset-ambientModeTransitionDuration;
}

// Enables or disables the ambient mode
void GraphicEngine::setAmbientMode(boolean enabled) {
  TimestampInMicroseconds t=core->getClock()->getMicrosecondsSinceStart();
  if (enabled) 
    ambientModeStartTime=t;
  else {
    ambientModeStartTime=0;
    interactiveModeStartTime=t+200000; // bug in current Samsung WearOS 4.0 requires delayed exit
  }
}

// Returns the fade scale for the ambient transition
double GraphicEngine::getAmbientFadeScale() {
  double fadeScale=1.0;
  TimestampInMicroseconds duration;
  if (isAmbientMode(duration)) {
    if (duration<animDuration) {
      fadeScale=((double)(animDuration-duration))/((double)animDuration);
      //DEBUG("fadeScale=%f",fadeScale);
    } else {
      fadeScale=0.0;
    }
  } else {
    if (duration<animDuration) {
      fadeScale=((double)(duration))/((double)animDuration);
      //DEBUG("fadeScale=%f",fadeScale);
    }
  }
  return fadeScale;
}

// Checks if display is in no widgets mode
bool GraphicEngine::isWidgetlessMode(TimestampInMicroseconds &duration) {
  duration=0;
  //DEBUG("ambientModeStartTime=%ld currentTime=%ld",ambientModeStartTime,currentTime);
  if ((widgetlessModeStartTime!=0)&&(currentTime>=widgetlessModeStartTime)) {
    duration=currentTime-widgetlessModeStartTime;
    //DEBUG("Yes: duration=%ld",duration);
    return true;
  } else {
    duration=currentTime-widgetfullModeStartTime;
    //DEBUG("No: duration=%ld",duration);
    return false;
  }
}

// Enables or disables the widgetless mode
void GraphicEngine::setWidgetlessMode(boolean mode) {
  TimestampInMicroseconds t=core->getClock()->getMicrosecondsSinceStart();
  if (mode) {
    widgetlessModeStartTime=t;
  } else {
    widgetfullModeStartTime=t;
    widgetlessModeStartTime=0;
  }
}

// Returns the fade scale for the ambient transition
double GraphicEngine::getWidgetlessFadeScale() {
  double fadeScale=1.0;
  TimestampInMicroseconds duration;
  if (isWidgetlessMode(duration)) {
    if (duration<animDuration) {
      fadeScale=((double)(animDuration-duration))/((double)animDuration);
    } else {
      fadeScale=0.0;
    }
  } else {
    if (duration<animDuration) {
      fadeScale=((double)(duration))/((double)animDuration);
    }
  }
  return fadeScale;
}


}
