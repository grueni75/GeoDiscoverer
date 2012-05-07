//============================================================================
// Name        : GDGLSurfaceView.java
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

package com.perfectapp.android.geodiscoverer;

import java.lang.reflect.Field;
import java.lang.reflect.Method;

import android.opengl.GLSurfaceView;
import android.view.MotionEvent;

/** Does the rendering with the OpenGL API */
public class GDMapSurfaceView extends GLSurfaceView {

  /** Interface to the native C++ core */
  protected GDCore coreObject=null;
      
  /** ID of the pointer that touched the screen first */
  int firstPointerID=-1;
  
  /** ID of the pointer that touched the screen second */
  int secondPointerID=-1;
  
  /** Previous angle between first and second pointer */
  float prevAngle=0;
  
  /** Previous distance between first and second pointer */
  float prevDistance=0;

  // Variables for handling multi touch
  boolean multitouchAvailable = true;
  int ACTION_POINTER_UP;
  int ACTION_POINTER_DOWN;
  int ACTION_POINTER_INDEX_MASK;
  int ACTION_POINTER_INDEX_SHIFT;
  int ACTION_MASK;
  int ACTION_DOWN;
  int ACTION_MOVE;
  int ACTION_UP;
  Method findPointerIndex;
  Method getPointerId;
  Method getPointerCount;
  Method getX;
  Method getY;
  Method getAction;

  /** Constructor */
  public GDMapSurfaceView(ViewMap parent) {
    super(parent);
    
    // Set the framebuffer
    setEGLConfigChooser(5,6,5,0,0,0);
    
    // Get the core object
    GDApplication app=(GDApplication)parent.getApplication();
    coreObject=app.coreObject;
    
    // Set the renderer
    setRenderer(coreObject);
    //setRenderMode(RENDERMODE_WHEN_DIRTY);
    //requestRender();

    // Check if we are on a multitouch device
    ACTION_POINTER_UP=-1;
    ACTION_POINTER_DOWN=-1;    
    try {
      Class motionEventClass = Class.forName("android.view.MotionEvent");
      Field f;
      f = motionEventClass.getDeclaredField("ACTION_POINTER_UP");
      ACTION_POINTER_UP=f.getInt(null);
      f = motionEventClass.getDeclaredField("ACTION_POINTER_DOWN");
      ACTION_POINTER_DOWN=f.getInt(null);
      f = motionEventClass.getDeclaredField("ACTION_POINTER_INDEX_MASK");
      ACTION_POINTER_INDEX_MASK=f.getInt(null);
      f = motionEventClass.getDeclaredField("ACTION_POINTER_INDEX_SHIFT");
      ACTION_POINTER_INDEX_SHIFT=f.getInt(null);
      f = motionEventClass.getDeclaredField("ACTION_MASK");
      ACTION_MASK=f.getInt(null);
      f = motionEventClass.getDeclaredField("ACTION_DOWN");
      ACTION_DOWN=f.getInt(null);
      f = motionEventClass.getDeclaredField("ACTION_UP");
      ACTION_UP=f.getInt(null);
      f = motionEventClass.getDeclaredField("ACTION_MOVE");
      ACTION_MOVE=f.getInt(null);
      findPointerIndex=motionEventClass.getMethod("findPointerIndex", int.class);
      getPointerId=motionEventClass.getMethod("getPointerId", int.class);
      getPointerCount=motionEventClass.getMethod("getPointerCount");
      getX=motionEventClass.getMethod("getX", int.class);
      getY=motionEventClass.getMethod("getY", int.class);
      getAction=motionEventClass.getMethod("getAction");
    }
    catch (Throwable e) {
      multitouchAvailable=false;
    }
    
    if (multitouchAvailable) {
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "multitouch is available");
    } else {
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "multitouch is not available");
    }
  }

  /** Calculates the chage in angle and zoom if the pinch-to-zoom gesture is used */
  protected void updateTwoFingerGesture(MotionEvent event, boolean moveAction) {
    if (firstPointerID==-1)
      return;
    try {      
      if (secondPointerID==-1) {
  
        // Update the core object
        int pointerIndex=(Integer)findPointerIndex.invoke(event,firstPointerID);
        int x = Math.round((Float)getX.invoke(event,pointerIndex));
        int y = Math.round((Float)getY.invoke(event,pointerIndex));
        if (moveAction) {
          coreObject.executeCoreCommand("touchMove(" + x + "," + y + ")");        
        } else {
          coreObject.executeCoreCommand("touchDown(" + x + "," + y + ")");                
        }
        
      } else {
        
        // Compute new angle, distance and position
        int firstPointerIndex=(Integer)findPointerIndex.invoke(event, firstPointerID);
        float firstX = (Float)getX.invoke(event,firstPointerIndex);
        float firstY = (Float)getY.invoke(event,firstPointerIndex);
        int secondPointerIndex=(Integer)findPointerIndex.invoke(event, secondPointerID);
        float secondX = (Float)getX.invoke(event,secondPointerIndex);
        float secondY = (Float)getY.invoke(event,secondPointerIndex);
        double distX=secondX-firstX;
        double distY=secondY-firstY;
        float distance=(float)Math.sqrt(distX*distX+distY*distY);
        float angle=(float)Math.atan2(distX, distY);
        int x = (int)Math.round(firstX+distance/2*Math.cos(angle));
        int y = (int)Math.round(firstY+distance/2*Math.sin(angle));
        
        // Move or set action?
        if (moveAction) {
          
          // Compute change in angle        
          double angleDiff=angle-prevAngle;
          if (angleDiff>Math.PI)
            angleDiff-=2*Math.PI;
          if (angleDiff<-Math.PI)
            angleDiff+=2*Math.PI;
          angleDiff=(double)angleDiff/Math.PI*180.0;
          coreObject.executeCoreCommand("rotate(" + angleDiff +")");
          
          // Compute change in scale
          double scaleDiff=distance/prevDistance;
          coreObject.executeCoreCommand("zoom(" + scaleDiff +")");
          
          // Set new position
          coreObject.executeCoreCommand("touchMove(" + x + "," + y + ")");        
                  
        } else {
          
          // Set new position
          coreObject.executeCoreCommand("touchDown(" + x + "," + y + ")");        
          
        }
        
        // Update previous values      
        prevDistance=distance;
        prevAngle=angle;      
      }
    }
    catch (Throwable e) {
      GDApplication.addMessage(GDApplication.ERROR_MSG, "GDApp", "can not call multitouch related methods");
      System.exit(1);
    }    
  }
  
  /** Called if the surface is touched */
  public boolean onTouchEvent(final MotionEvent event) {
                        
    // What happened?
    if (!multitouchAvailable) {

      // Extract infos
      int action = event.getAction();
      int x = Math.round(event.getX());
      int y = Math.round(event.getY());
      
      switch(action) {
      
        case MotionEvent.ACTION_DOWN:
          coreObject.executeCoreCommand("touchDown(" + x + "," + y + ")");
          break;
          
        case MotionEvent.ACTION_MOVE:
          coreObject.executeCoreCommand("touchMove(" + x + "," + y + ")");
          break;
    
        case MotionEvent.ACTION_UP:
          coreObject.executeCoreCommand("touchUp(" + x + "," + y + ")");
          break;
        }
      
    } else {
    
      try {
        
        // Find out what happened
        int actionValue=(Integer)getAction.invoke(event);
        int action=actionValue & ACTION_MASK;
        int pointerIndex;
        int x,y;

        // First pointer touched screen?
        if (action==ACTION_DOWN) {
          firstPointerID=(Integer)getPointerId.invoke(event, 0);
          pointerIndex=(Integer)findPointerIndex.invoke(event,firstPointerID);
          x = Math.round((Float)getX.invoke(event,pointerIndex));
          y = Math.round((Float)getY.invoke(event,pointerIndex));
          coreObject.executeCoreCommand("touchDown(" + x + "," + y + ")");
        }

        // One or more pointers have moved?
        if (action==ACTION_MOVE) {
          updateTwoFingerGesture(event, true);
        }
        
        // All pointers have left?
        if (action==ACTION_UP) {
          pointerIndex=(Integer)findPointerIndex.invoke(event,firstPointerID);
          x = Math.round((Float)getX.invoke(event,pointerIndex));
          y = Math.round((Float)getY.invoke(event,pointerIndex));          
          coreObject.executeCoreCommand("touchUp(" + x + "," + y + ")");
          firstPointerID=-1;
        }
  
        // New pointer touched screen? 
        if (action==ACTION_POINTER_DOWN) {
         
          // Choose a second pointer if we don't have one yet
          if (secondPointerID==-1) {
            int newPointerIndex = (actionValue & ACTION_POINTER_INDEX_MASK) >> ACTION_POINTER_INDEX_SHIFT;
            secondPointerID=(Integer)getPointerId.invoke(event,newPointerIndex);
            updateTwoFingerGesture(event, false);            
          }

        }
        
        // Existing pointer left screen?
        if (action==ACTION_POINTER_UP) {

          // Choose a new first and second pointer
          int leavingPointerIndex = (actionValue & ACTION_POINTER_INDEX_MASK) >> ACTION_POINTER_INDEX_SHIFT;
          int numberOfPointers=(Integer)getPointerCount.invoke(event);
          int firstPointerIndex=-1;
          for (int i=0;i<numberOfPointers;i++) {
            if (leavingPointerIndex!=i) {
             firstPointerIndex=i;
            }
          }        
          firstPointerID=(Integer)getPointerId.invoke(event, firstPointerIndex);
          int secondPointerIndex=-1;            
          for (int i=0;i<numberOfPointers;i++) {
            if ((leavingPointerIndex!=i)&&(firstPointerIndex!=i)) {
              secondPointerIndex=i;
            }
          }
          if (secondPointerIndex!=-1) {
            secondPointerID=(Integer)getPointerId.invoke(event, secondPointerIndex);
          } else {
            secondPointerID=-1;
          }
          updateTwoFingerGesture(event, false);            
        }
      }
      catch (Throwable e) {
        GDApplication.addMessage(GDApplication.ERROR_MSG, "GDApp", "can not call multitouch related methods");
        System.exit(1);
      }
    }
        
    return true;
  }

}
