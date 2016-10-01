//============================================================================
// Name        : GDGLSurfaceView.java
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

package com.untouchableapps.android.geodiscoverer.core;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.util.AttributeSet;
import android.view.MotionEvent;

import com.untouchableapps.android.geodiscoverer.core.GDCore;

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

  /** Constructor */
  @SuppressLint("NewApi")
  public GDMapSurfaceView(Context context, AttributeSet attrs) {
    super(context, attrs);

    // Use OpenGL ES 2.0
    setEGLContextClientVersion(2);

    // Set the framebuffer
    setEGLConfigChooser(8,8,8,8,16,0);

    // Preserve the context if possible
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
      setPreserveEGLContextOnPause(true);
    }
  }

  /** Sets the core object */
  public void setCoreObject(GDCore coreObject) {
    this.coreObject=coreObject;
    setRenderer(coreObject);
    //setRenderMode(RENDERMODE_WHEN_DIRTY);
    //requestRender();
  }
  
  /** Calculates the chage in angle and zoom if the pinch-to-zoom gesture is used */
  protected void updateTwoFingerGesture(MotionEvent event, boolean moveAction) {
    if (firstPointerID==-1)
      return;
    try {
      if (secondPointerID==-1) {
  
        // Update the core object
        int pointerIndex=event.findPointerIndex(firstPointerID);
        if (pointerIndex==-1) {
          return;
        }
        int x = Math.round(event.getX(pointerIndex));
        int y = Math.round(event.getY(pointerIndex));
        if (moveAction) {
          coreObject.executeCoreCommand("touchMove(" + x + "," + y + ")");        
        } else {
          coreObject.executeCoreCommand("touchDown(" + x + "," + y + ")");                
        }
        
      } else {
        
        // Compute new angle, distance and position
        int firstPointerIndex=event.findPointerIndex(firstPointerID);
        if (firstPointerIndex==-1) {
          return;
        }
        float firstX = Math.round(event.getX(firstPointerIndex));
        float firstY = Math.round(event.getY(firstPointerIndex));
        int secondPointerIndex=event.findPointerIndex(secondPointerID);
        if (secondPointerIndex==-1) {
          return;
        }
        float secondX = Math.round(event.getX(secondPointerIndex));
        float secondY = Math.round(event.getY(secondPointerIndex));
        double distX=secondX-firstX;
        double distY=secondY-firstY;
        float distance=(float)Math.sqrt(distX*distX+distY*distY);
        float angle=(float)Math.atan2(distX, distY);
        int x = (int)Math.round(firstX+distance/2*Math.sin(angle));
        int y = (int)Math.round(firstY+distance/2*Math.cos(angle));
        
        // Move or set action?
        if (moveAction) {
          
          // Compute change in angle        
          double angleDiff=angle-prevAngle;
          if (angleDiff>=Math.PI)
            angleDiff-=2*Math.PI;
          if (angleDiff<=-Math.PI)
            angleDiff+=2*Math.PI;
          angleDiff=(double)angleDiff/Math.PI*180.0;
          //Log.d("GDApp","angleDiff=" + angleDiff);
          //coreObject.executeCoreCommand("rotate(" + angleDiff +")");
          /*if (angleDiff>0) {
            for (int i=0;i<100;i++) {
              coreObject.executeCoreCommand("rotate(0.5)");
              Thread.sleep(10);
            }          
          }*/
          
          // Compute change in scale
          double scaleDiff=distance/prevDistance;
          //coreObject.executeCoreCommand("zoom(" + scaleDiff +")");
          
          // Set new position
          //coreObject.executeCoreCommand("touchMove(" + x + "," + y + ")");    
          
          // Set new position
          coreObject.executeCoreCommand("twoFingerGesture(" + x + "," + y + "," + angleDiff + "," + scaleDiff + ")");    
          
                  
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
      coreObject.appIf.addAppMessage(coreObject.appIf.WARNING_MSG, "GDApp", "can not call multitouch related methods");
      System.exit(1);
    }   
  }
  
  /** Called if the surface is touched */
  @SuppressLint("ClickableViewAccessibility")
  @TargetApi(Build.VERSION_CODES.ECLAIR)
  public boolean onTouchEvent(final MotionEvent event) {
                        
    // What happened?
    if (VERSION.SDK_INT < VERSION_CODES.ECLAIR) {

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
            
      // Find out what happened
      int actionValue=event.getAction();
      int action=actionValue & MotionEvent.ACTION_MASK;
      int pointerIndex;
      int x,y;

      // First pointer touched screen?
      if (action==MotionEvent.ACTION_DOWN) {
        firstPointerID=event.getPointerId(0);
        pointerIndex=event.findPointerIndex(firstPointerID);
        if (pointerIndex!=-1) {
          x = Math.round(event.getX(pointerIndex));
          y = Math.round(event.getY(pointerIndex));
          coreObject.executeCoreCommand("touchDown(" + x + "," + y + ")");
        }
      }

      // One or more pointers have moved?
      if (action==MotionEvent.ACTION_MOVE) {
        updateTwoFingerGesture(event, true);
      }
      
      // All pointers have left?
      if ((action==MotionEvent.ACTION_UP)||(action==MotionEvent.ACTION_CANCEL)) {
        pointerIndex=event.findPointerIndex(firstPointerID);
        if (pointerIndex!=-1) {
          x = Math.round(event.getX(pointerIndex));
          y = Math.round(event.getY(pointerIndex));
          coreObject.executeCoreCommand("touchUp(" + x + "," + y + ")");
          firstPointerID=-1;
          secondPointerID=-1;
        }
      }

      // New pointer touched screen? 
      if (action==MotionEvent.ACTION_POINTER_DOWN) {
       
        // Choose a second pointer if we don't have one yet
        if (secondPointerID==-1) {
          int newPointerIndex = (actionValue & MotionEvent.ACTION_POINTER_INDEX_MASK) >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
          secondPointerID=event.getPointerId(newPointerIndex);
          updateTwoFingerGesture(event, false);            
        }

      }
      
      // Existing pointer left screen?
      if (action==MotionEvent.ACTION_POINTER_UP) {

        // Choose a new first and second pointer
        int leavingPointerIndex = (actionValue & MotionEvent.ACTION_POINTER_INDEX_MASK) >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
        int numberOfPointers=event.getPointerCount();
        int firstPointerIndex=-1;
        for (int i=0;i<numberOfPointers;i++) {
          if (leavingPointerIndex!=i) {
           firstPointerIndex=i;
          }
        }        
        firstPointerID=event.getPointerId(firstPointerIndex);
        int secondPointerIndex=-1;            
        for (int i=0;i<numberOfPointers;i++) {
          if ((leavingPointerIndex!=i)&&(firstPointerIndex!=i)) {
            secondPointerIndex=i;
          }
        }
        if (secondPointerIndex!=-1) {
          secondPointerID=event.getPointerId(secondPointerIndex);
        } else {
          secondPointerID=-1;
        }
        updateTwoFingerGesture(event, false);            
      }
    }
     
    return true;
  }
  
}
