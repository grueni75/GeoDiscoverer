//============================================================================
// Name        : GDCore.java
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

package com.untouchableapps.android.geodiscoverer;

import java.io.File;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import android.content.res.Configuration;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.location.Location;
import android.location.LocationListener;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Message;
import android.util.DisplayMetrics;
import android.widget.FrameLayout;
import android.widget.TextView;
import android.os.Process;

/** Interfaces with the C++ part */
public class GDCore implements GLSurfaceView.Renderer, LocationListener, SensorEventListener {

  //
  // Variables
  //
  
  /** Parent activity */
  public ViewMap activity = null;

  /** Current opengl context */
  GL10 currentGL = null;
  
  /** Path to the home directory */
  String homePath;
  
  /** DPI of the screen */
  int screenDPI = 0;
  
  /** Indicates that the core is stopped */
  boolean coreStopped = false;

  /** Indicates that the core shall not update its frames */
  boolean suspendCore = false;

  /** Indicates if the core is initialized */
  boolean coreInitialized = false;
    
  /** Indicates that home dir is available */
  boolean homeDirAvailable = false;

  /** Indicates that the graphic must be re-created */
  boolean createGraphic = false;

  /** Indicates that the graphic has been invalidated */
  boolean graphicInvalidated = false;

  /** Indicates that the sreen properties have changed */
  boolean changeScreen = false;
  
  /** Indicates that the screen should be redrawn */
  boolean forceRedraw = false;
  
  /** Indicates that the last frame has been handled by the core */
  boolean lastFrameDrawnByCore = true;
  
  /** Indicates that the splash is currently shown */
  boolean splashIsVisible = false;
  
  /** Command to execute for changing the screen */
  String changeScreenCommand = "";
  
  /** Queued commands to execute if core is initialized */
  LinkedList<String> queuedCoreCommands = new LinkedList<String>();
  
  // Sensor readings
  float[] lastAcceleration = null;
  float[] lastMagneticField = null;

  // Arrays used to compute the orientation
  float[] R = new float[16];
  float[] correctedR = new float[16];
  float[] I = new float[16];
  float[] orientation = new float[3];

  // Thread signaling
  final Lock lock = new ReentrantLock();
    
  //
  // Constructor and destructor
  //
  
  /** Load the required libraries */
  static {
    System.loadLibrary("gdcore");
  }
  
  /** Constructor 
   * @param screenDPI */
  GDCore(String homePath) {

    // Copy variables
    this.homePath=homePath;
  }
  
  /** Sets the view map activity */
  public void setActivity(ViewMap activity) {
    lock.lock();
    this.activity = activity;
    this.coreStopped = false;
    if (activity!=null) {
      DisplayMetrics metrics = new DisplayMetrics();
      activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);
      this.screenDPI=metrics.densityDpi;
    }
    lock.unlock();
  }
  
  /** Destructor */
  protected void finalize() throws Throwable
  {
    // Clean up the C++ part
    stop();
  } 

  /** Indicates that the home dir is available */
  public void homeDirAvailable()
  {
    lock.lock();
    homeDirAvailable=true;
    lock.unlock();
  } 

  /** Starts the core */
  protected void start(boolean innerCall)
  {
    if (!innerCall) lock.lock();
    boolean isInitialized=coreInitialized;
    if (!innerCall) lock.unlock();
    if (!isInitialized) {
  
      // Init the core
      boolean initialized=false;
      if (homeDirAvailable) {
        init(homePath,screenDPI);    
        initialized=true;
      } else {
        GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp","home dir not available");
      }
  
      // Ensure that the screen is recreated
      if (!innerCall) lock.lock();    
      if (initialized) {
        coreInitialized=true;
        executeAppCommand("updateWakeLock()");
      }
      coreStopped=false;
      if (!changeScreenCommand.equals("")) {        
        changeScreen=true;
      }
      createGraphic=true;
      if (!innerCall) lock.unlock();

    }
  } 

  /** Deinits the core */
  protected void stop()
  {
    lock.lock();
    boolean isInitialized=coreInitialized;
    lock.unlock();
    if (isInitialized) {
  
      // Update flags
      lock.lock();
      coreInitialized=false;
      coreStopped=true;
      lock.unlock();
      
      // Deinit the core
      deinit();
    
    }
  } 

  /** Deinits the core and restarts it */
  public synchronized void restart(boolean resetConfig)
  {
    // Clean up the C++ part
    stop();

    // Remove the config if requested
    if (resetConfig) {
      File configFile = new File(homePath + "/config.xml");
      configFile.delete();
    }

    // Start the core
    start(false);
  } 
  
  //
  // Functions implemented by the native core
  //
  
  /** Starts the C++ part */
  protected native void init(String homePath, int DPI);
  
  /** Stops the C++ part */
  protected native void deinit();
  
  /** Draw a frame by the C++ part */
  protected native void updateScreen(boolean forceRedraw);
  
  /** Send a command to the core */
  protected native String executeCoreCommandInt(String cmd);
  
  /** Sets a string value in the config */
  native void configStoreSetStringValue(String path, String name, String value);

  /** Gets a string value from the config */
  native String configStoreGetStringValue(String path, String name);

  /** Lists all elements for the given path in the config */
  native String[] configStoreGetNodeNames(String path);

  /** Checks if the path exists in the config */
  native boolean configStorePathExists(String path);

  /** Removes the path from the config */
  native void configStoreRemovePath(String path);

  /** Lists all values of the given attribute in the config */
  native String[] configStoreGetAttributeValues(String path, String attributeName);

  /** Returns information about the given node in the config */
  native Bundle configStoreGetNodeInfo(String path);
  
  /** Sends an command to the core after checking if it it is ready */
  public String executeCoreCommand(String cmd)
  {
    lock.lock();
    String result;
    if (coreInitialized) {
      result = executeCoreCommandInt(cmd);
    } else {
      result = "";
    }
    lock.unlock();
    return result;
  }

  /** Sends an command to the core if it is initialized or remembers them for execution after core is initialized */
  public void scheduleCoreCommand(String cmd)
  {
    lock.lock();
    if (coreInitialized) {
      executeCoreCommandInt(cmd);
    } else {
      queuedCoreCommands.add(cmd);
    }
    lock.unlock();
  }

  //
  // Functions that are called by the native core
  //
  
  /** Execute an command */
  public void executeAppCommand(String cmd)
  {
    if (cmd.equals("initComplete()")) {
      for (String queuedCmd : queuedCoreCommands) {
        executeCoreCommandInt(queuedCmd);
      }
      queuedCoreCommands.clear();      
    } else {
      if (activity!=null) {
        Message m=Message.obtain(activity.coreMessageHandler);
        m.what = ViewMap.EXECUTE_COMMAND;
        Bundle b = new Bundle();
        b.putString("command", cmd);
        m.setData(b);    
        activity.coreMessageHandler.sendMessage(m);
      }
    }
  }

  /** Sets the thread priority */
  public void setThreadPriority(int priority)
  {
    switch(priority) {
      case 0:
        Process.setThreadPriority(Process.THREAD_PRIORITY_FOREGROUND);
        break;
      case 1:
        Process.setThreadPriority(Process.THREAD_PRIORITY_BACKGROUND);
        break;
      case 2:
        Process.setThreadPriority(Process.THREAD_PRIORITY_BACKGROUND+Process.THREAD_PRIORITY_LESS_FAVORABLE);
        break;
      default:
        GDApplication.addMessage(GDApplication.FATAL_MSG, "GDapp", "unsupported thread priority");
    }    
  }

  //
  // GLSurfaceView.Renderer implementation
  //
  
  /** Called when a frame needs to be drawn */
  public void onDrawFrame(GL10 gl) {
    lock.lock();
    boolean blankScreen=false;
    if (gl==currentGL) {
      if ((!coreStopped)&&(homeDirAvailable)) {
        if (!suspendCore) {
          if (!coreInitialized) {
            start(true);
          }
          if (graphicInvalidated) {        
            executeCoreCommand("graphicInvalidated()");
            graphicInvalidated=false;
            createGraphic=false;
          }
          if (createGraphic) {        
            executeCoreCommand("createGraphic()");
            createGraphic=false;
          }
          if (changeScreen) {
            executeCoreCommand(changeScreenCommand);
            changeScreen=false;
            forceRedraw=true;
          }
          updateScreen(forceRedraw);
          forceRedraw=false;
          if ((!lastFrameDrawnByCore)||(splashIsVisible)) {
            splashIsVisible = false;
            executeAppCommand("setSplashVisibility(0)");
          }
          lastFrameDrawnByCore=true;
        }
      } else {
        blankScreen=true;
      }
    } else {
      blankScreen=true;
    }
    if (blankScreen) {
      gl.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      gl.glClear(GL10.GL_COLOR_BUFFER_BIT);
      if (lastFrameDrawnByCore) {
        executeAppCommand("setSplashVisibility(1)");
      }
      lastFrameDrawnByCore=false;
    }    
    lock.unlock();
  }
  
  /** Updates the splash visibility flag */
  public void setSplashIsVisible(boolean splashIsVisible) {
    lock.lock();
    this.splashIsVisible = splashIsVisible;
    lock.unlock();
  }
  
  /** Called when the surface changes */
  public void onSurfaceChanged(GL10 gl, int width, int height) {
    lock.lock();
    if (activity==null) {
      GDApplication.addMessage(GDApplication.ERROR_MSG, "GDApp", "onSurfaceChanged called without activity");
    } else {
      if (gl!=currentGL) {
        GDApplication.addMessage(GDApplication.ERROR_MSG, "GDApp", "opengl context change is not supported");          
      }
      int orientationValue = activity.getResources().getConfiguration().orientation;
      String orientationString="portrait";
      if (orientationValue==Configuration.ORIENTATION_LANDSCAPE)
        orientationString="landscape";
      changeScreenCommand="screenChanged(" + orientationString + "," + width + "," + height + ")";
      changeScreen=true;
    }
    lock.unlock();
  }

  /** Called when the surface is created */
  public void onSurfaceCreated(GL10 gl, EGLConfig config) {
    lock.lock();
    // Context is lost, so tell the core to recreate any textures
    if (activity==null)
      GDApplication.addMessage(GDApplication.ERROR_MSG, "GDApp", "onSurfaceCreated called without activity");
    graphicInvalidated=true;
    createGraphic=true;
    currentGL=gl;
    lock.unlock();
  }

  /** Called when a new fix is available */  
  public void onLocationChanged(Location location) {
    if (location!=null) {
      String cmd = "locationChanged(" + location.getProvider() + "," + location.getTime();
      cmd += "," + location.getLongitude() + "," + location.getLatitude();
      int t=0;
      if (location.hasAltitude()) {
        t=1;
      } 
      cmd += "," + t + "," + location.getAltitude() + ",1";
      t=0;
      if (location.hasBearing()) {
        t=1;
      }
      cmd += "," + t + "," + location.getBearing();
      t=0;
      if (location.hasSpeed()) {
        t=1;
      }
      cmd += "," + t + "," + location.getSpeed();
      t=0;
      if (location.hasAccuracy()) {
        t=1;
      }
      cmd += "," + t + "," + location.getAccuracy();
      cmd += ")";
      executeCoreCommand(cmd);
    }
  }

  // Other call backs from the location manager
  public void onProviderDisabled(String provider) {
  }
  public void onProviderEnabled(String provider) {
  }
  public void onStatusChanged(String provider, int status, Bundle extras) {
  }

  /** Called when a the orientation sensor has changed */
  public void onSensorChanged(SensorEvent event) {
    
    // Drop the event if it is unreliable
    if (event.accuracy==SensorManager.SENSOR_STATUS_UNRELIABLE)
      return;

    // Remember the last sensor readings
    switch (event.sensor.getType()) {
      case Sensor.TYPE_MAGNETIC_FIELD:
        lastMagneticField = event.values.clone();
        break;
      case Sensor.TYPE_ACCELEROMETER:
        lastAcceleration = event.values.clone();
        break;
    }
    
    // Compute the orientation corrected by the screen rotation
    if ((lastMagneticField!=null) && (lastAcceleration!=null) && (coreInitialized)) {
      SensorManager.getRotationMatrix(R, I, lastAcceleration, lastMagneticField);
      SensorManager.remapCoordinateSystem(R,SensorManager.AXIS_X,SensorManager.AXIS_Z, correctedR);
      SensorManager.getOrientation(correctedR, orientation);
      orientation[0]=(float) (orientation[0]*180.0/Math.PI);
      if (orientation[0]<0)
        orientation[0]=360+orientation[0];
      executeCoreCommand("compassBearingChanged(" + orientation[0] + ")");
    }
    
  }

  // Other calls back from the sensor manager
  public void onAccuracyChanged(Sensor sensor, int accuracy) {
  }
  
  
}
