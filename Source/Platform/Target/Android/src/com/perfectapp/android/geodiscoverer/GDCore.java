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

package com.perfectapp.android.geodiscoverer;

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
import android.util.Log;
import android.widget.FrameLayout;

/** Interfaces with the C++ part */
public class GDCore implements GLSurfaceView.Renderer, LocationListener, SensorEventListener {

  //
  // Variables
  //
  
  /** Parent activity */
  public ViewMap parentActivity = null;
    
  /** Path to the home directory */
  String homePath;
  
  /** DPI of the screen */
  int screenDPI;
  
  /** Indicates if the core is initialized */
  boolean coreInitialized = false;
  
  /** Indicates that the first frame has been drawn */
  boolean firstFrameDrawn = false;
  
  /** Indicates that the screen must be re-created */
  boolean createScreen = false;

  /** Indicates that the sreen properties have changed */
  boolean changeScreen = false;
  
  /** Indicates that the screen should be redrawn */
  boolean forceRedraw = false;
  
  /** Command to execute for changing the screen */
  String changeScreenCommand = "";
  
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
  final Condition screenUpdated  = lock.newCondition(); 
    
  //
  // Constructor and destructor
  //
  
  /** Load the required libraries */
  static {
    System.loadLibrary("gdcore");
  }
  
  /** Constructor 
   * @param screenDPI */
  GDCore(ViewMap parent, String homePath) {

    // Copy variables
    this.parentActivity=parent;
    this.homePath=homePath;

    // Get the dpi of the screen
    DisplayMetrics metrics = new DisplayMetrics();
    parentActivity.getWindowManager().getDefaultDisplay().getMetrics(metrics);
    this.screenDPI=metrics.densityDpi;    
  }
  
  /** Destructor */
  protected void finalize() throws Throwable
  {
    // Clean up the C++ part
    if (coreInitialized) {
      deinit();
    }
  } 

  /** Deinits the core */
  public void restart(boolean resetConfig)
  {
    lock.lock();
    
    // Clean up the C++ part
    coreInitialized=false;
    if (coreInitialized) {
      deinit();
    }
    
    // Remove the config if requested
    if (resetConfig) {
      File configFile = new File(homePath + "/config.xml");
      configFile.delete();
    }
    
    // Ensure that the screen is recreated
    if (!changeScreenCommand.equals("")) {
      changeScreen=true;
    }
    
    // Wait until the first frame is drawn
    boolean repeat = true;
    while (repeat) {
      try {
        repeat = false;
        screenUpdated.await();
      }
      catch (InterruptedException e) {
        repeat = true;
      }
    }
    
    lock.unlock();
  } 
  
  //
  // Functions implemented by the native core
  //
  
  /** Starts the C++ part */
  protected native void init(String homePath, int DPI);
  
  /** Stops the C++ part */
  protected native void deinit();
  
  /** (Re)creates the screen */
  protected native boolean createScreen();

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

  //
  // Functions that are called by the native core
  //
  
  /** Execute an command */
  public void executeAppCommand(String cmd)
  {
    if (parentActivity!=null) {
      Message m=Message.obtain(parentActivity.coreMessageHandler);
      m.what = ViewMap.EXECUTE_COMMAND;
      Bundle b = new Bundle();
      b.putString("command", cmd);
      m.setData(b);    
      parentActivity.coreMessageHandler.sendMessage(m);
    }
  }

  //
  // GLSurfaceView.Renderer implementation
  //
  
  /** Called when a frame needs to be drawn */
  public void onDrawFrame(GL10 gl) {
    lock.lock();
    if (firstFrameDrawn) {
      if (!coreInitialized) {
        init(homePath,screenDPI);
        createScreen=false;
        coreInitialized=true;
        executeAppCommand("updateWakeLock()");
      }
      if (createScreen) {        
        if (createScreen()) {
          createScreen=false;
        }
      }
      if (changeScreen) {
        executeCoreCommand(changeScreenCommand);
        changeScreen=false;
        forceRedraw=true;
      }
      updateScreen(forceRedraw);
      forceRedraw=false;
      screenUpdated.signal();
    } else {
      firstFrameDrawn=true;
    }
    lock.unlock();
  }

  /** Called when the surface changes */
  public void onSurfaceChanged(GL10 gl, int width, int height) {
    lock.lock();
    int orientationValue = parentActivity.getResources().getConfiguration().orientation;
    String orientationString="portrait";
    if (orientationValue==Configuration.ORIENTATION_LANDSCAPE)
      orientationString="landscape";
    changeScreenCommand="screenChanged(" + orientationString + "," + width + "," + height + ")";
    changeScreen=true;
    lock.unlock();
  }

  /** Called when the surface is created */
  public void onSurfaceCreated(GL10 gl, EGLConfig config) {
    lock.lock();
    // Context is lost, so tell the core to recreate any textures
    createScreen=true;
    lock.unlock();
  }

  /** Called when a new fix is available */  
  public void onLocationChanged(Location location) {
    if (location!=null) {
      //Log.d("GDApp","location update received from provider <" + location.getProvider() + ">.");
      if (coreInitialized) {
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
      //Log.d("GDApp","azimuth is " + orientation[0] + " degrees");
      executeCoreCommand("compassBearingChanged(" + orientation[0] + ")");
    }
    
  }

  // Other calls back from the sensor manager
  public void onAccuracyChanged(Sensor sensor, int accuracy) {
  }
  
  
}
