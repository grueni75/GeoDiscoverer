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
  debugMode=core->getConfigStore()->getIntValue("Graphic","debugMode");
  posMutex=core->getThread()->createMutex();
  pathAnimatorsMutex=core->getThread()->createMutex();
  noChangeFrameCount=0;
  locationAccuracyBackgroundColor=core->getConfigStore()->getGraphicColorValue("Graphic/LocationAccuracyBackgroundColor");
  locationAccuracyCircleColor=core->getConfigStore()->getGraphicColorValue("Graphic/LocationAccuracyCircleColor");
  locationAccuracyCircleLineWidth=core->getConfigStore()->getIntValue("Graphic","locationAccuracyCircleLineWidth");
  locationAccuracyRadiusX=0;
  locationAccuracyRadiusY=0;
  locationIconMutex=core->getThread()->createMutex();
  targetIconMutex=core->getThread()->createMutex();
  centerIconTimeout=core->getConfigStore()->getIntValue("Graphic","centerIconTimeout");
  centerIcon.setColor(GraphicColor(255,255,255,0));
  locationIcon.setColor(GraphicColor(255,255,255,0));
  compassConeIcon.setColor(core->getConfigStore()->getGraphicColorValue("Graphic/CompassConeColor"));
  compassConeIcon.setAngle(std::numeric_limits<double>::max());
  compassConeIconMutex=core->getThread()->createMutex();
  targetIcon.setColor(GraphicColor(255,255,255,0));
  lastCenterIconFadeStartTime=0;
  isDrawing=false;
  lastDrawingStartTime=0;
  minDrawingTime=+std::numeric_limits<double>::max();
  maxDrawingTime=-std::numeric_limits<double>::max();
  totalDrawingTime=0;
  minIdleTime=+std::numeric_limits<double>::max();
  maxIdleTime=-std::numeric_limits<double>::max();
  totalIdleTime=0;
  statsMutex=core->getThread()->createMutex();
  frameCount=0;

  // Init the dynamic data
  init();
}

// Inits dynamic data
void GraphicEngine::init() {
}

// Recreates all graphic
void GraphicEngine::recreateGraphic() {
  deinit();
  centerIcon.setTextureFromIcon(core->getConfigStore()->getStringValue("Graphic","centerIconFilename"));
  lockLocationIcon();
  locationIcon.setTextureFromIcon(core->getConfigStore()->getStringValue("Graphic","locationIconFilename"));
  unlockLocationIcon();
  pathDirectionIcon.setTextureFromIcon(core->getConfigStore()->getStringValue("Graphic","pathDirectionIconFilename"));
  lockCompassConeIcon();
  compassConeIcon.setTextureFromIcon(core->getConfigStore()->getStringValue("Graphic","compassConeFilename"));
  unlockCompassConeIcon();
  lockTargetIcon();
  targetIcon.setTextureFromIcon(core->getConfigStore()->getStringValue("Graphic","targetIconFilename"));
  unlockTargetIcon();
}

// Deinits dynamic data
void GraphicEngine::deinit() {
  centerIcon.deinit();
  lockLocationIcon();
  locationIcon.deinit();
  unlockLocationIcon();
  pathDirectionIcon.deinit();
  targetIcon.deinit();
  lockCompassConeIcon();
  compassConeIcon.deinit();
  unlockCompassConeIcon();
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
  pos=*(lockPos());
  unlockPos();

  // Force redraw if requested externally
  if (forceRedraw) {
    DEBUG("forcing redraw due to external request",NULL);
    redrawScene=true;
  }

  // Let the map primitives work
  if (map) {
    map->lockAccess();
    if (map->work(currentTime)) {
      //DEBUG("requesting scene redraw due to map work result",NULL);
      redrawScene=true;
    }
    map->unlockAccess();
  }

  // Let the path animators primitives work
  lockPathAnimators();
  if (pathAnimators.work(currentTime)) {
    //DEBUG("requesting scene redraw due to paths work result",NULL);
    redrawScene=true;
  }
  unlockPathAnimators();

  // Let the widget primitives work
  if ((widgetPage)&&(widgetPage->getGraphicObject()->work(currentTime))) {
    //DEBUG("requesting scene redraw due to widget page work result",NULL);
    redrawScene=true;
  }

  // Handle the hiding of the center icon
  TimestampInMicroseconds fadeStartTime=pos.getLastUserModification()+centerIconTimeout;
  if (fadeStartTime!=lastCenterIconFadeStartTime) {
    centerIcon.setColor(GraphicColor(255,255,255,255));
    centerIcon.setFadeAnimation(fadeStartTime,centerIcon.getColor(),GraphicColor(255,255,255,0));
  }
  lastCenterIconFadeStartTime=fadeStartTime;

  // Let the center primitive work
  if (centerIcon.work(currentTime)) {
    //DEBUG("requesting scene redraw due to center icon work result",NULL);
    redrawScene=true;
  }

  // Let the target primitive work
  lockTargetIcon();
  if (targetIcon.work(currentTime)) {
    //DEBUG("requesting scene redraw due to target icon work result",NULL);
    redrawScene=true;
  }
  unlockTargetIcon();

  // Did the pos change?
  if (pos!=previousPosition) {

    // Scene must be redrawn
    //DEBUG("requesting scene redraw due to pos change",NULL);
    redrawScene=true;

  }

  // Did the location icon change?
  lockLocationIcon();
  if (locationIcon.getIsUpdated()) {
    //DEBUG("requesting scene redraw due to changed location icon",NULL);
    redrawScene=true;
    locationIcon.setIsUpdated(false);
  }
  unlockLocationIcon();

  // Did the target icon change?
  lockTargetIcon();
  if (targetIcon.getIsUpdated()) {
    //DEBUG("requesting scene redraw due to changed location icon",NULL);
    redrawScene=true;
    targetIcon.setIsUpdated(false);
  }
  unlockTargetIcon();

  // Did the compass cone icon change?
  lockCompassConeIcon();
  if (compassConeIcon.getIsUpdated()) {
    //DEBUG("requesting scene redraw due to changed compass cone icon",NULL);
    redrawScene=true;
    compassConeIcon.setIsUpdated(false);
  }
  unlockCompassConeIcon();

  // Check if the map object has changed
  if (map)  {
    map->lockAccess();
    if (map->getIsUpdated()) {
      //DEBUG("requesting scene redraw due to changed map object",NULL);
      redrawScene=true;
      map->setIsUpdated(false);
    }
    map->unlockAccess();
  }

  // Check if the paths object has changed
  lockPathAnimators();
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

      map->lockAccess();
      std::list<GraphicPrimitive*> *mapDrawList=map->getDrawList();
      for(std::list<GraphicPrimitive *>::const_iterator i=mapDrawList->begin(); i != mapDrawList->end(); i++) {

        //DEBUG("inside primitive draw",NULL);
        switch((*i)->getType()) {

          case GraphicTypeObject:
          {
            // Get graphic object
            GraphicObject *tileVisualization;
            tileVisualization=(GraphicObject*)*i;
            tileVisualization->lockAccess();

            // Skip drawing if primitive is invisible
            UByte alpha=tileVisualization->getColor().getAlpha();
            if (alpha!=0) {

              // Position the object
              screen->startObject();
              screen->translate(tileVisualization->getX(),tileVisualization->getY(),tileVisualization->getZ());

              // Go through all objects of the tile visualization
              std::list<GraphicPrimitive*> *tileDrawList=tileVisualization->getDrawList();
              for(std::list<GraphicPrimitive *>::const_iterator j=tileDrawList->begin(); j != tileDrawList->end(); j++) {

                // Which type of primitive?
                GraphicPrimitive *primitive=*j;
                switch(primitive->getType()) {

                  // Rectangle primitive?
                  case GraphicTypeRectangle:
                  {
                    GraphicRectangle *rectangle=(GraphicRectangle*)primitive;

                    // Set color
                    screen->setColor(rectangle->getColor().getRed(),rectangle->getColor().getGreen(),rectangle->getColor().getBlue(),rectangle->getColor().getAlpha());
                    /*if (rectangle->getColor().getRed()==0) {
                      DEBUG("red component is 0",NULL);
                    }
                    if (rectangle->getColor().getGreen()==0) {
                      DEBUG("green component is 0",NULL);
                    }
                    if (rectangle->getColor().getBlue()==0) {
                      DEBUG("blue component is 0",NULL);
                    }
                    if (rectangle->getColor().getAlpha()==0) {
                      DEBUG("alpha component is 0",NULL);
                    }*/

                    // Dimm the color in debug mode
                    // This allows to differntiate the tiles
                    if ((debugMode)&&(primitive->getName().size()!=0))
                      screen->setColor(rectangle->getColor().getRed()/2,rectangle->getColor().getGreen()/2,rectangle->getColor().getBlue()/2,rectangle->getColor().getAlpha());

                    // Draw a rectangle if primitive matches
                    x1=rectangle->getX();
                    y1=rectangle->getY();
                    x2=(GLfloat)(rectangle->getWidth()+x1);
                    y2=(GLfloat)(rectangle->getHeight()+y1);
                    //DEBUG("x1=%d y1=%d x2=%d y2=%d",x1,y1,x2,y2);
                    screen->drawRectangle(x1,y1,x2,y2,rectangle->getTexture(),rectangle->getFilled());

                    //  DEBUG("rectangle->getTexture()=%d screen->getTextureNotDefined()=%d",rectangle->getTexture(),screen->getTextureNotDefined());

                    // If the texture is not defined, draw a box around it and it's name inside the box
                    if ((primitive->getName().size()!=0)&&(rectangle->getTexture()==screen->getTextureNotDefined())||(debugMode)) {

                      // Draw the name of the tile
                      //std::string name=".";
                      core->getFontEngine()->setFont("sansSmall");
                      std::list<std::string> name=rectangle->getName();
                      FontEngine *fontEngine=core->getFontEngine();
                      Int nameHeight=name.size()*fontEngine->getLineHeight();
                      Int lineNr=name.size()-1;
                      for(std::list<std::string>::iterator i=name.begin();i!=name.end();i++) {
                        FontString *fontString=fontEngine->createString(*i);
                        fontString->setX(x1+(rectangle->getWidth()-fontString->getIconWidth())/2);
                        fontString->setY(y1+(rectangle->getHeight()-nameHeight)/2+lineNr*fontEngine->getLineHeight());
                        fontString->draw(screen,currentTime);
                        fontEngine->destroyString(fontString);
                        lineNr--;
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
                    line->draw(screen);
                    screen->setColorModeAlpha();
                    break;
                  }

                  // Rectangle list primitive?
                  case GraphicTypeRectangleList:
                  {
                    GraphicRectangleList *rectangleList=(GraphicRectangleList*)primitive;
                    GraphicColor color=rectangleList->getAnimator()->getColor();
                    screen->setColor(color.getRed(),color.getGreen(),color.getBlue(),color.getAlpha());
                    rectangleList->draw(screen);
                    break;
                  }

                  default:
                    FATAL("unknown primitive type",NULL);
                    break;
                }
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
            x1=r->getX();
            y1=r->getY();
            x2=(GLfloat)(r->getWidth()+x1);
            y2=(GLfloat)(r->getHeight()+y1);
            screen->startObject();
            screen->setColor(r->getColor().getRed(),r->getColor().getGreen(),r->getColor().getBlue(),r->getColor().getAlpha());
            screen->setLineWidth(1);
            screen->drawRectangle(x1,y1,x2,y2,screen->getTextureNotDefined(),false);
            screen->endObject();
            break;
          }

          default:
            FATAL("unknown primitive type",NULL);
            break;
        }
      }
      map->unlockAccess();
    }

    PROFILE_ADD("map drawing");

    //DEBUG("after path draw",NULL);

    // Draw the location icon and the compass cone
    //DEBUG("locationIcon.getColor().getAlpha()=%d locationIcon.getX()=%d locationIcon.getY()=%d",locationIcon.getColor().getAlpha(),locationIcon.getX(),locationIcon.getY());
    lockLocationIcon();
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
      x1=-locationIcon.getWidth()/2;
      y1=-locationIcon.getHeight()/2;
      x2=x1+locationIcon.getWidth();
      y2=y1+locationIcon.getHeight();
      screen->rotate(locationIcon.getAngle(),0,0,1);
      screen->setColor(locationIcon.getColor().getRed(),locationIcon.getColor().getGreen(),locationIcon.getColor().getBlue(),locationIcon.getColor().getAlpha());
      screen->drawRectangle(x1,y1,x2,y2,locationIcon.getTexture(),true);
      screen->endObject();
      unlockLocationIcon();

      // Draw the compass cone
      lockCompassConeIcon();
      if (compassConeIcon.getAngle()!=std::numeric_limits<double>::max()) {
        screen->startObject();
        screen->setColor(compassConeIcon.getColor().getRed(),compassConeIcon.getColor().getGreen(),compassConeIcon.getColor().getBlue(),compassConeIcon.getColor().getAlpha());
        x1=-compassConeIcon.getWidth()/2;
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
    lockTargetIcon();
    if (targetIcon.getColor().getAlpha()>0) {

      // Translate to the target location
      screen->startObject();
      screen->translate(targetIcon.getX(),targetIcon.getY(),0);
      screen->scale(backScale,backScale,1.0);

      // Draw the target icon
      screen->startObject();
      screen->scale(targetIcon.getScale(),targetIcon.getScale(),1.0);
      screen->rotate(targetIcon.getAngle(),0,0,1);
      x1=-targetIcon.getWidth()/2;
      y1=-targetIcon.getHeight()/2;
      x2=x1+targetIcon.getWidth();
      y2=y1+targetIcon.getHeight();
      screen->rotate(targetIcon.getAngle(),0,0,1);
      screen->setColor(targetIcon.getColor().getRed(),targetIcon.getColor().getGreen(),targetIcon.getColor().getBlue(),targetIcon.getColor().getAlpha());
      screen->drawRectangle(x1,y1,x2,y2,targetIcon.getTexture(),true);
      screen->endObject();
      screen->endObject();
    }
    unlockTargetIcon();

    PROFILE_ADD("target drawing");

    // End the map object
    screen->endObject();

    //DEBUG("after location icon draw",NULL);

    // Draw the cursor
    screen->startObject();
    screen->setColor(centerIcon.getColor().getRed(),centerIcon.getColor().getGreen(),centerIcon.getColor().getBlue(),centerIcon.getColor().getAlpha());
    x1=-centerIcon.getWidth()/2;
    x2=x1+centerIcon.getWidth();
    y1=-centerIcon.getHeight()/2;
    y2=y1+centerIcon.getHeight();
    screen->drawRectangle(x1,y1,x2,y2,centerIcon.getTexture(),true);
    screen->endObject();

    PROFILE_ADD("cursor drawing");

    //DEBUG("after cursor icon draw",NULL);

    // Draw all widgets
    if (widgetPage) {
      screen->startObject();
      std::list<GraphicPrimitive*> *widgetDrawList=widgetPage->getGraphicObject()->getDrawList();
      for(std::list<GraphicPrimitive *>::const_iterator i=widgetDrawList->begin(); i != widgetDrawList->end(); i++) {

        // Draw the widget
        WidgetPrimitive *widget;
        widget=(WidgetPrimitive*)*i;
        widget->draw(screen,currentTime);

      }
      screen->endObject();
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

  //PROFILE_END;
}

// Destructor
GraphicEngine::~GraphicEngine() {
  core->getThread()->destroyMutex(posMutex);
  core->getThread()->destroyMutex(pathAnimatorsMutex);
  core->getThread()->destroyMutex(locationIconMutex);
  core->getThread()->destroyMutex(targetIconMutex);
  core->getThread()->destroyMutex(compassConeIconMutex);
  core->getThread()->destroyMutex(statsMutex);
}

// Outputs statistical infos
void GraphicEngine::outputStats() {
  core->getThread()->lockMutex(statsMutex);
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
