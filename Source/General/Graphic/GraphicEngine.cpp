//============================================================================
// Name        : GraphicEngine.cpp
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

GraphicEngine::GraphicEngine() {

  // Init variables
  map=NULL;
  pos=GraphicPosition();
  debugMode=core->getConfigStore()->getIntValue("Graphic","debugMode",__FILE__, __LINE__);
  posMutex=core->getThread()->createMutex("graphic engine pos mutex");
  pathAnimatorsMutex=core->getThread()->createMutex("graphic engine path animators mutex");
  noChangeFrameCount=0;
  locationAccuracyBackgroundColor=core->getConfigStore()->getGraphicColorValue("Graphic/LocationAccuracyBackgroundColor",__FILE__, __LINE__);
  locationAccuracyCircleColor=core->getConfigStore()->getGraphicColorValue("Graphic/LocationAccuracyCircleColor",__FILE__, __LINE__);
  locationAccuracyRadiusX=0;
  locationAccuracyRadiusY=0;
  locationIconMutex=core->getThread()->createMutex("graphic engine location icon mutex");
  targetIconMutex=core->getThread()->createMutex("graphic engine target icon mutex");
  centerIconTimeout=core->getConfigStore()->getIntValue("Graphic","centerIconTimeout",__FILE__, __LINE__);
  centerIcon.setColor(GraphicColor(255,255,255,0));
  locationIcon.setColor(GraphicColor(255,255,255,0));
  compassConeIcon.setColor(core->getConfigStore()->getGraphicColorValue("Graphic/CompassConeColor",__FILE__, __LINE__));
  compassConeIcon.setAngle(std::numeric_limits<double>::max());
  compassConeIconMutex=core->getThread()->createMutex("graphic engine compass cone icon mutex");
  targetIcon.setColor(GraphicColor(255,255,255,0));
  arrowIconMutex=core->getThread()->createMutex("graphic engine arrow icon mutex");
  arrowIcon.setColor(GraphicColor(255,255,255,0));
  lastCenterIconFadeStartTime=0;
  isDrawing=false;
  lastDrawingStartTime=0;
  minDrawingTime=+std::numeric_limits<double>::max();
  maxDrawingTime=-std::numeric_limits<double>::max();
  totalDrawingTime=0;
  minIdleTime=+std::numeric_limits<double>::max();
  maxIdleTime=-std::numeric_limits<double>::max();
  totalIdleTime=0;
  statsMutex=core->getThread()->createMutex("graphic engine status mutex");
  frameCount=0;
  tileImageNotCachedImage.setColor(GraphicColor(255,255,255,0));
  tileImageNotDownloadedFilename.setColor(GraphicColor(255,255,255,0));
  fadeDuration=core->getConfigStore()->getIntValue("Graphic","fadeDuration",__FILE__, __LINE__);
  blinkDuration=core->getConfigStore()->getIntValue("Graphic","blinkDuration",__FILE__, __LINE__);
  widgetGraphicObject=NULL;

  // Init the dynamic data
  init();
}

// Inits dynamic data
void GraphicEngine::init() {
}

// Clears all graphic
void GraphicEngine::destroyGraphic() {
  deinit();
}

// Creates all graphic
void GraphicEngine::createGraphic() {
  centerIcon.setTextureFromIcon(core->getConfigStore()->getStringValue("Graphic","centerIconFilename",__FILE__, __LINE__));
  lockLocationIcon(__FILE__, __LINE__);
  locationIcon.setTextureFromIcon(core->getConfigStore()->getStringValue("Graphic","locationIconFilename",__FILE__, __LINE__));
  unlockLocationIcon();
  pathDirectionIcon.setTextureFromIcon(core->getConfigStore()->getStringValue("Graphic","pathDirectionIconFilename",__FILE__, __LINE__));
  pathStartFlagIcon.setTextureFromIcon(core->getConfigStore()->getStringValue("Graphic","pathStartFlagIconFilename",__FILE__, __LINE__));
  pathEndFlagIcon.setTextureFromIcon(core->getConfigStore()->getStringValue("Graphic","pathEndFlagIconFilename",__FILE__, __LINE__));
  lockCompassConeIcon(__FILE__, __LINE__);
  compassConeIcon.setTextureFromIcon(core->getConfigStore()->getStringValue("Graphic","compassConeFilename",__FILE__, __LINE__));
  unlockCompassConeIcon();
  lockTargetIcon(__FILE__, __LINE__);
  targetIcon.setTextureFromIcon(core->getConfigStore()->getStringValue("Graphic","targetIconFilename",__FILE__, __LINE__));
  unlockTargetIcon();
  lockArrowIcon(__FILE__, __LINE__);
  arrowIcon.setTextureFromIcon(core->getConfigStore()->getStringValue("Graphic","arrowIconFilename",__FILE__, __LINE__));
  unlockArrowIcon();
  tileImageNotCachedImage.setTextureFromIcon(core->getConfigStore()->getStringValue("Graphic","tileImageNotCachedFilename",__FILE__, __LINE__));
  tileImageNotDownloadedFilename.setTextureFromIcon(core->getConfigStore()->getStringValue("Graphic","tileImageNotDownloadedFilename",__FILE__, __LINE__));
}

// Deinits dynamic data
void GraphicEngine::deinit() {
  centerIcon.deinit();
  lockLocationIcon(__FILE__, __LINE__);
  locationIcon.deinit();
  unlockLocationIcon();
  pathDirectionIcon.deinit();
  pathStartFlagIcon.deinit();
  pathEndFlagIcon.deinit();
  lockTargetIcon(__FILE__, __LINE__);
  targetIcon.deinit();
  unlockTargetIcon();
  lockArrowIcon(__FILE__, __LINE__);
  arrowIcon.deinit();
  unlockArrowIcon();
  lockCompassConeIcon(__FILE__, __LINE__);
  compassConeIcon.deinit();
  unlockCompassConeIcon();
  tileImageNotCachedImage.deinit();
  tileImageNotDownloadedFilename.deinit();
  GraphicPointBuffer::destroyBuffers();
}

// Does the drawing
void GraphicEngine::draw(bool forceRedraw) {

  GraphicPosition pos;
  TimestampInMicroseconds currentTime,idleTime,drawingTime;
  bool redrawScene=false;
  Int x1,y1,x2,y2,x,y;
  Screen *screen = core->getScreen();
  double scale,backScale;

  PROFILE_START;

  // Start measuring of drawing time and utilization
  currentTime=core->getClock()->getMicrosecondsSinceStart();
#ifdef PROFILING_ENABLED
  idleTime=currentTime-lastDrawingStartTime;
  lastDrawingStartTime=currentTime;
#endif

  // Indicate that the engine is drawing
  isDrawing=true;

  // Copy the current position
  //lockPos()->setAngle(this->pos.getAngle()+0.1);
  //unlockPos();
  pos=*(lockPos(__FILE__, __LINE__));
  unlockPos();

  // Force redraw if requested externally
  if (forceRedraw) {
    DEBUG("forcing redraw due to external request",NULL);
    redrawScene=true;
  }

  // Let the map primitives work
  if (map) {
    map->lockAccess(__FILE__, __LINE__);
    if (map->work(currentTime)) {
      //DEBUG("requesting scene redraw due to map work result",NULL);
      redrawScene=true;
    }
    map->unlockAccess();
  }

  // Let the path animators primitives work
  lockPathAnimators(__FILE__, __LINE__);
  if (pathAnimators.work(currentTime)) {
    //DEBUG("requesting scene redraw due to paths work result",NULL);
    redrawScene=true;
  }
  unlockPathAnimators();

  // Let the widget primitives work
  widgetGraphicObject->lockAccess(__FILE__, __LINE__);
  if ((widgetGraphicObject)&&(widgetGraphicObject->work(currentTime))) {
    //DEBUG("requesting scene redraw due to widget page work result",NULL);
    redrawScene=true;
  }
  core->getWidgetEngine()->work(currentTime);
  widgetGraphicObject->unlockAccess();

  // Handle the hiding of the center icon
  TimestampInMicroseconds fadeStartTime=pos.getLastUserModification()+centerIconTimeout;
  if (fadeStartTime!=lastCenterIconFadeStartTime) {
    centerIcon.setColor(GraphicColor(255,255,255,255));
    centerIcon.setFadeAnimation(fadeStartTime,centerIcon.getColor(),GraphicColor(255,255,255,0),false,core->getGraphicEngine()->getFadeDuration());
  }
  lastCenterIconFadeStartTime=fadeStartTime;

  // Let the center primitive work
  if (centerIcon.work(currentTime)) {
    //DEBUG("requesting scene redraw due to center icon work result",NULL);
    redrawScene=true;
  }

  // Let the target primitive work
  lockTargetIcon(__FILE__, __LINE__);
  if (targetIcon.work(currentTime)) {
    //DEBUG("requesting scene redraw due to target icon work result",NULL);
    redrawScene=true;
  }
  unlockTargetIcon();

  // Let the arrow primitive work
  lockArrowIcon(__FILE__, __LINE__);
  if (arrowIcon.work(currentTime)) {
    //DEBUG("requesting scene redraw due to arrow icon work result",NULL);
    redrawScene=true;
  }
  unlockArrowIcon();

  // Did the pos change?
  if (pos!=previousPosition) {

    // Scene must be redrawn
    //DEBUG("requesting scene redraw due to pos change",NULL);
    redrawScene=true;

  }

  // Did the location icon change?
  lockLocationIcon(__FILE__, __LINE__);
  if (locationIcon.getIsUpdated()) {
    //DEBUG("requesting scene redraw due to changed location icon",NULL);
    redrawScene=true;
    locationIcon.setIsUpdated(false);
  }
  unlockLocationIcon();

  // Did the target icon change?
  lockTargetIcon(__FILE__, __LINE__);
  if (targetIcon.getIsUpdated()) {
    //DEBUG("requesting scene redraw due to changed location icon",NULL);
    redrawScene=true;
    targetIcon.setIsUpdated(false);
  }
  unlockTargetIcon();

  // Did the arrow icon change?
  lockArrowIcon(__FILE__, __LINE__);
  if (arrowIcon.getIsUpdated()) {
    //DEBUG("requesting scene redraw due to changed location icon",NULL);
    redrawScene=true;
    arrowIcon.setIsUpdated(false);
  }
  unlockArrowIcon();

  // Did the compass cone icon change?
  lockCompassConeIcon(__FILE__, __LINE__);
  if (compassConeIcon.getIsUpdated()) {
    //DEBUG("requesting scene redraw due to changed compass cone icon",NULL);
    redrawScene=true;
    compassConeIcon.setIsUpdated(false);
  }
  unlockCompassConeIcon();

  // Check if the map object has changed
  if (map)  {
    map->lockAccess(__FILE__, __LINE__);
    if (map->getIsUpdated()) {
      //DEBUG("requesting scene redraw due to changed map object",NULL);
      redrawScene=true;
      map->setIsUpdated(false);
    }
    map->unlockAccess();
  }

  // Check if the paths object has changed
  lockPathAnimators(__FILE__, __LINE__);
  if (pathAnimators.getIsUpdated()) {
    //DEBUG("requesting scene redraw due to changed paths object",NULL);
    redrawScene=true;
    pathAnimators.setIsUpdated(false);
  }
  unlockPathAnimators();

  // Check if two frames without any change have been drawn
  if (!redrawScene) {
    if (noChangeFrameCount<2) {
      //DEBUG("requesting scene redraw due to not enough unchanged frames",NULL);
      redrawScene=true;
      noChangeFrameCount++;
    }
  } else {
    noChangeFrameCount=0;
  }

  PROFILE_ADD("drawing check");

  // Redraw required?
  if (redrawScene) {

    // Clear the scene
    screen->clear();

    // Set default line width
    screen->setLineWidth(1);

    // Start the map object
    screen->startObject();

    // Set rotation factor
    //DEBUG("pos.getAngle()=%f",pos.getAngle());
    screen->rotate(pos.getAngle(),0,0,1);

    // Set scaling factor
    scale=pos.getZoom();
    backScale=1.0/pos.getZoom();
    //std::cout << "scale=" << scale << std::endl;
    //scale=scale/2;
    screen->scale(scale,scale,1.0);

    // Set translation factors
    screen->translate(-pos.getX(),-pos.getY(),0);

    PROFILE_ADD("drawing init");

    // Draw all primitives of the map object
    if (map) {

      //PROFILE_START;

      map->lockAccess(__FILE__, __LINE__);
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
              tileVisualization->lockAccess(__FILE__, __LINE__);

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
                      rectangle->draw(screen,currentTime);

                      // Restore the color
                      if ((debugMode)&&(primitive->getName().size()!=0)) {
                        rectangle->setColor(originalColor);
                      }

                      //DEBUG("rectangle->getTexture()=%d screen->getTextureNotDefined()=%d",rectangle->getTexture(),screen->getTextureNotDefined());

                      // If the texture is not defined, draw a box around it and it's name inside the box
                      //PROFILE_START;
                      if ((primitive->getName().size()!=0)&&(debugMode)) {

                        // Draw the name of the tile
                        //std::string name=".";
                        if (debugMode) {
                          std::list<std::string> name=rectangle->getName();
                          FontEngine *fontEngine=core->getFontEngine();
                          fontEngine->lockFont("sansSmall", __FILE__, __LINE__);
                          Int nameHeight=name.size()*fontEngine->getLineHeight();
                          Int lineNr=name.size()-1;
                          x1=rectangle->getX();
                          y1=rectangle->getY();
                          x2=x1+rectangle->getWidth();
                          y2=y1+rectangle->getHeight();
                          //PROFILE_START;
                          for(std::list<std::string>::iterator i=name.begin();i!=name.end();i++) {
                            //DEBUG("text=%s",(*i).c_str());
                            FontString *fontString=fontEngine->createString(*i);
                            //PROFILE_ADD("create string");
                            fontString->setX(x1+(rectangle->getWidth()-fontString->getIconWidth())/2);
                            fontString->setY(y1+(rectangle->getHeight()-nameHeight)/2+lineNr*fontEngine->getLineHeight());
                            //PROFILE_ADD("set string coordinate");
                            fontString->draw(screen,currentTime);
                            //PROFILE_ADD("draw string");
                            fontEngine->destroyString(fontString);
                            //PROFILE_ADD("destroy string");
                            lineNr--;
                          }
                          fontEngine->unlockFont();
                          //PROFILE_END;
                        }
                        //PROFILE_ADD("map tile name drawing");

                        // Draw the borders of the tile
                        screen->setColor(255,255,255,255);
                        screen->setLineWidth(1);
                        screen->drawRectangle(x1,y1,x2,y2,screen->getTextureNotDefined(),false);
                        //PROFILE_ADD("map tile border drawing");
                      }
                      //PROFILE_ADD("map tile name drawing");
                      //PROFILE_END;
                      break;
                    }

                    // Line primitive?
                    case GraphicTypeLine:
                    {
                      GraphicLine *line=(GraphicLine*)primitive;
                      screen->setColorModeMultiply();
                      GraphicColor color=line->getAnimator()->getColor();
                      screen->setColor(color.getRed(),color.getGreen(),color.getBlue(),color.getAlpha());
                      line->draw(screen);
                      screen->setColorModeAlpha();
                      //PROFILE_ADD("map overlay line drawing");
                      break;
                    }

                    // Rectangle list primitive?
                    case GraphicTypeRectangleList:
                    {
                      GraphicRectangleList *rectangleList=(GraphicRectangleList*)primitive;
                      GraphicColor color=rectangleList->getAnimator()->getColor();
                      screen->setColor(color.getRed(),color.getGreen(),color.getBlue(),color.getAlpha());
                      rectangleList->draw(screen);
                      //PROFILE_ADD("map overlay icon drawing");
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
              tileVisualization->unlockAccess();
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
                //PROFILE_ADD("map rectangle drawing");
              }
              break;
            }

            default:
              FATAL("unknown primitive type",NULL);
              break;
          }
        }
      }
      map->unlockAccess();
    }

    //PROFILE_END;

    PROFILE_ADD("map drawing");

    //DEBUG("after path draw",NULL);

    // Draw the location icon and the compass cone
    //DEBUG("locationIcon.getColor().getAlpha()=%d locationIcon.getX()=%d locationIcon.getY()=%d",locationIcon.getColor().getAlpha(),locationIcon.getX(),locationIcon.getY());
    lockLocationIcon(__FILE__, __LINE__);
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
      unlockLocationIcon();

      // Draw the compass cone
      lockCompassConeIcon(__FILE__, __LINE__);
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
      unlockCompassConeIcon();
      screen->endObject();
    } else {
      unlockLocationIcon();
    }
    //WARNING("enable location icon",NULL);
    //DEBUG("locationAccuradyRadiusX=%d locationAccuracyRadiusY=%d",locationAccuracyRadiusX,locationAccuracyRadiusY);

    PROFILE_ADD("location drawing");

    // Draw the target icon
    lockTargetIcon(__FILE__, __LINE__);
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
    unlockTargetIcon();

    // Draw the arrow icon
    lockArrowIcon(__FILE__, __LINE__);
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
    unlockArrowIcon();

    PROFILE_ADD("arrow drawing");

    // End the map object
    screen->endObject();

    //DEBUG("after location icon draw",NULL);

    // Draw the cursor
    screen->startObject();
    screen->setColor(centerIcon.getColor().getRed(),centerIcon.getColor().getGreen(),centerIcon.getColor().getBlue(),centerIcon.getColor().getAlpha());
    x1=-centerIcon.getIconWidth()/2;
    x2=x1+centerIcon.getWidth();
    y1=-centerIcon.getIconHeight()/2;
    y2=y1+centerIcon.getHeight();
    screen->drawRectangle(x1,y1,x2,y2,centerIcon.getTexture(),true);
    screen->endObject();

    PROFILE_ADD("cursor drawing");

    //DEBUG("after cursor icon draw",NULL);

    // Draw all widgets
    if (widgetGraphicObject) {
      widgetGraphicObject->lockAccess(__FILE__, __LINE__);
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
          widget->draw(screen,currentTime);

        }
        screen->endObject();
      }
      widgetGraphicObject->unlockAccess();
    }
    //DEBUG("after widget draw",NULL);

    PROFILE_ADD("widget drawing");

    // Finish the drawing
    screen->endScene();

  } else {
    //DEBUG("scene not drawn",NULL);
  }

  // Remember the last position
  previousPosition=pos;

  // Update the time measurement
#ifdef PROFILING_ENABLED
  core->getThread()->lockMutex(statsMutex);
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
  core->getThread()->unlockMutex(statsMutex);
#endif

  // Everything done
  isDrawing=false;

  PROFILE_END;
}

// Destructor
GraphicEngine::~GraphicEngine() {
  core->getThread()->destroyMutex(posMutex);
  core->getThread()->destroyMutex(pathAnimatorsMutex);
  core->getThread()->destroyMutex(locationIconMutex);
  core->getThread()->destroyMutex(targetIconMutex);
  core->getThread()->destroyMutex(arrowIconMutex);
  core->getThread()->destroyMutex(compassConeIconMutex);
  core->getThread()->destroyMutex(statsMutex);
}

// Outputs statistical infos
void GraphicEngine::outputStats() {
  core->getThread()->lockMutex(statsMutex,__FILE__, __LINE__);
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
  core->getThread()->unlockMutex(statsMutex);
}

}
