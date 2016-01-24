//============================================================================
// Name        : GDCore.java
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

import android.annotation.TargetApi;
import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.location.Location;
import android.location.LocationListener;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.Process;
import android.util.DisplayMetrics;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/** Interfaces with the C++ part */
public class GDCore implements GLSurfaceView.Renderer, LocationListener, SensorEventListener, Runnable {

  //
  // Variables
  //

  /** Current opengl context */
  GL10 currentGL = null;
  
  /** Path to the home directory */
  public String homePath;
  
  /** DPI of the screen */
  int screenDPI = 0;
  
  /** Diagonal of the screen in inches */
  double screenDiagonal;
  
  /** Indicates that the core is stopped */
  public boolean coreStopped = true;

  /** Indicates that the core shall not update its frames */
  boolean suspendCore = false;

  /** Indicates if the core is initialized */
  public boolean coreInitialized = false;
  
  /** Indicates if the core is currently initialized or destroyed */
  boolean coreLifeCycleOngoing = false;

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
  boolean lastFrameDrawnByCore = false;
  
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
  final Condition threadInitialized = lock.newCondition();

  // Indicates if a replay is active
  boolean replayTraceActive = false;

  // Last time the download status was updated
  long lastDownloadStatusUpdate = 0;

  // Reference to the object that implements the interface functions
  GDAppInterface appIf = null;

  //
  // Constructor and destructor
  //
  
  /** Load the required libraries */
  static {
    System.loadLibrary("gdzip");
    System.loadLibrary("gdiconv");
    System.loadLibrary("gdxml");
    System.loadLibrary("gdfreetype");
    System.loadLibrary("gdjpeg");
    System.loadLibrary("gdpng");
    System.loadLibrary("gdcurl");
    System.loadLibrary("gdproj4");
    System.loadLibrary("gdcore");
  }
  
  /** Constructor */
  @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
  public GDCore(GDAppInterface appIf, String homePath) {

    // Copy variables
    this.homePath=homePath;
    this.appIf=appIf;

    // Prepare the JNI part
    initJNI();

    // Start the thread that handles the starting and stopping
    thread = new Thread(this);
    thread.start(); 
    
    // Wait until the thread is initialized
    lock.lock();
    try {
      while (messageHandler==null)
      {           
        threadInitialized.await();
      }
    } catch (InterruptedException e) {
      e.printStackTrace();
    } finally {
      lock.unlock();
    }    
  }
  
  /** Destructor */
  protected void finalize() throws Throwable
  {
    // Clean up the C++ part
    stop();
    
    // Clean up the JNI part
    deinitJNI();
  } 

  //
  // Thread that handles the starting and stopping of the core
  //
  
  // Message handler
  public static class AppMessageHandler extends Handler {
    
    protected final WeakReference<GDCore> weakCoreObject; 
    
    AppMessageHandler(GDCore coreObject) {
      this.weakCoreObject = new WeakReference<GDCore>(coreObject);
    }
    
    /** Called when the core has a message */
    @Override
    public void handleMessage(Message msg) {  
      GDCore coreObject = weakCoreObject.get();
      if (coreObject==null)
        return;
      Bundle b=msg.getData();
      switch(msg.what) {
        case START_CORE:
          coreObject.start();
          break;
        case RESTART_CORE:
          coreObject.restart(b.getBoolean("resetConfig"));
          break;
        case STOP_CORE:
          coreObject.stop();
          coreObject.executeAppCommand("exitActivity()");
          break;
        case HOME_DIR_AVAILABLE:
          coreObject.homeDirAvailable=true;
          coreObject.start();
          break;
        case HOME_DIR_NOT_AVAILABLE:
          coreObject.homeDirAvailable=false;
          coreObject.stop();
          break;
        default:
          coreObject.appIf.addAppMessage(GDAppInterface.ERROR_MSG, "GDApp", "unknown message received");
      }
    }
  }

  Thread thread = null;
  public AppMessageHandler messageHandler = null;

  // Types of messages
  public static final int START_CORE = 0;  
  public static final int STOP_CORE = 1;  
  public static final int RESTART_CORE = 2;  
  public static final int HOME_DIR_AVAILABLE = 3;  
  public static final int HOME_DIR_NOT_AVAILABLE = 4;  

  // Handler thread
  public void run() {
    
    lock.lock();

    // This is a background thread
    Process.setThreadPriority(Process.THREAD_PRIORITY_BACKGROUND);
        
    // Process messages
    Looper.prepare();
    messageHandler = new AppMessageHandler(this);    
    threadInitialized.signal();
    lock.unlock();
    Looper.loop();
  }
  
  //
  // Support functions
  //
  
  /** Updates files on the sdcard with the latest ones */
  boolean updateHome() {

    // Check if the GeoDiscoverer assets need to be updated
    AssetManager am = appIf.getApplication().getAssets();
    SharedPreferences prefs = appIf.getApplication().getSharedPreferences("assets", Context.MODE_PRIVATE);
    String installedMD5Sum = prefs.getString("installedMD5Sum", "unknown");
    String packagedMD5Sum = "none";
    InputStream is;
    BufferedReader br;
    InputStreamReader isr;
    try {
      is = am.open("GeoDiscoverer.list");
      isr = new InputStreamReader(is);
      br = new BufferedReader(isr);
      packagedMD5Sum = br.readLine();
    }
    catch (IOException e) {
      executeAppCommand("fatalDialog(\"Could not read asset list! APK damaged?\")");
      return false;
    }
    if (installedMD5Sum.equals(packagedMD5Sum)) {
      return true;
    }
    
    // Get all files to copy
    List<String> files = new ArrayList<String>();
    while(true) {
      try {
        String file = br.readLine();
        if (file == null)
          break;
        files.add(file);
      } 
      catch (IOException e) {
        break;
      }
    }
    try {
      br.close();
      isr.close();
      is.close();
    }
    catch (IOException e) { } ;
    
    // Inform the user of the copy information
    String title = "Updating home directory";
    executeAppCommand("createProgressDialog(\"" + title + "\"," + files.size() + ")");
    
    // Copy all files
    int i = 0;
    for (String path : files) {
      String externalPath = homePath + "/" + path;
      String internalPath = "GeoDiscoverer/" + path;
      File f = new File(externalPath);
      try {
        f.getParentFile().mkdirs();
        FileOutputStream os = new FileOutputStream(f);
        is = am.open(internalPath);
        byte[] buf = new byte[1024];
        int len;
        while ((len = is.read(buf)) > 0) {
            os.write(buf, 0, len);
        }
        os.close();
        is.close();
      } 
      catch (IOException e) {
        executeAppCommand("fatalDialog(\"Could not copy file to home directory: " + e.getMessage() + "!\")");
        return false;
      }
      i++;
      executeAppCommand("updateProgressDialog(\"" + title + "\"," + i + ")");
    }
    
    // Create the .nomedia file
    File f = new File(homePath + "/.nomedia");
    try {
      FileOutputStream os = new FileOutputStream(f);
      os.close();
    }
    catch (IOException e) {
      executeAppCommand("fatalDialog(\"Could not create .nomedia file in home directory: " + e.getMessage() + "!\")");
      return false;
    }
    
    // Update the shared prefs
    SharedPreferences.Editor prefsEditor = prefs.edit();
    prefsEditor.putString("installedMD5Sum", packagedMD5Sum);
    prefsEditor.commit();
    
    // That's it!
    executeAppCommand("closeProgressDialog()");
    return true;
  }
  
  /** Sets the display metrics */
  public void setDisplayMetrics(DisplayMetrics metrics) {
    lock.lock();
    this.screenDPI=metrics.densityDpi;
    double a=metrics.widthPixels/metrics.xdpi;
    double b=metrics.heightPixels/metrics.ydpi;
    this.screenDiagonal=Math.sqrt(a*a+b*b);
    lock.unlock();
  }

  /** Starts the core */
  @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
  synchronized void start()
  {
    lock.lock();
    if (coreInitialized) {
      lock.unlock();
      return;
    }
    if (coreLifeCycleOngoing) {
      lock.unlock();
      return;
    }
    coreLifeCycleOngoing=true;
    lock.unlock();
    
    // Check if home dir is available
    boolean initialized=false;
    if (!homeDirAvailable) {
      appIf.addAppMessage(appIf.DEBUG_MSG, "GDApp","home dir not available");
    } else {
      
      // Copy the assets files
      if (!updateHome()) {
        lock.lock();
        coreLifeCycleOngoing=false;
        lock.unlock();
        return;
      }
      
      // Init the core
      initCore(homePath,screenDPI,screenDiagonal);    
      initialized=true;

    }

    // Ensure that the screen is recreated
    lock.lock();    
    if (initialized) {
      coreInitialized=true;
      executeAppCommand("coreInitialized()");
      executeAppCommand("updateWakeLock()");
      coreStopped=false;
      if (!changeScreenCommand.equals("")) {        
        changeScreen=true;
      }
      createGraphic=true;
    }
    coreLifeCycleOngoing=false;
    lock.unlock();
  } 

  /** Deinits the core */
  @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
  synchronized void stop()
  {
    lock.lock();
    if (!coreInitialized) {
      lock.unlock();
      return;
    }
    if (coreLifeCycleOngoing) {
      lock.unlock();
      return;
    }
    coreLifeCycleOngoing=true;
    coreStopped=true;
    coreInitialized=false;
    lock.unlock();

    // Stop the cockpit apps
    appIf.cockpitEngineStop();

    // Deinit the core
    deinitCore();

    // Update flags
    lock.lock();
    coreLifeCycleOngoing=false;
    lock.unlock();
  } 

  /** Deinits the core and restarts it */
  void restart(boolean resetConfig)
  {
    // Clean up the C++ part
    stop();

    // Remove the config if requested
    if (resetConfig) {
      File configFile = new File(homePath + "/config.xml");
      configFile.delete();
    }

    // Request a restart of the activity
    executeAppCommand("restartActivity()");
  } 

  //
  // Functions implemented by the native core
  //
  
  /** Prepares the java native interface */
  native void initJNI();
  
  /** Cleans up the java native interfacet */
  native void deinitJNI();

  /** Starts the C++ part */
  native void initCore(String homePath, int DPI, double diagonal);
  
  /** Stops the C++ part */
  native void deinitCore();
  
  /** Draw a frame by the C++ part */
  native void updateScreen(boolean forceRedraw);
  
  /** Send a command to the core */
  native String executeCoreCommandInt(String cmd);
  
  /** Sets a string value in the config */
  public native void configStoreSetStringValue(String path, String name, String value);

  /** Gets a string value from the config */
  public native String configStoreGetStringValue(String path, String name);

  /** Lists all elements for the given path in the config */
  public native String[] configStoreGetNodeNames(String path);

  /** Checks if the path exists in the config */
  public native boolean configStorePathExists(String path);

  /** Removes the path from the config */
  public native void configStoreRemovePath(String path);

  /** Lists all values of the given attribute in the config */
  public native String[] configStoreGetAttributeValues(String path, String attributeName);

  /** Returns information about the given node in the config */
  public native Bundle configStoreGetNodeInfo(String path);
  
  /** Sends an command to the core after checking if it it is ready */
  public String executeCoreCommand(String cmd)
  {
    String result;
    if (coreInitialized) {
      if (cmd.startsWith("replayTrace(")) {
        replayTraceActive=true;
      }
      result = executeCoreCommandInt(cmd);
    } else {
      result = "";
    }
    return result;
  }

  /** Sends an command to the core if it is initialized or remembers them for execution after core is initialized */
  public void scheduleCoreCommand(String cmd)
  {
    if (coreInitialized) {
      executeCoreCommandInt(cmd);
    } else {
      lock.lock();
      queuedCoreCommands.add(cmd);
      lock.unlock();
    }
  }

  //
  // Functions that are called by the native core
  //

  /** Execute an command */
  @SuppressWarnings("resource")
  public void executeAppCommand(final String cmd)
  {
    boolean cmdExecuted = false;
    if (cmd.startsWith("sendNativeCrashReport(")) {

      // Start the reporting thread
      Thread reportThread = new Thread() {
        public void run() {
          
          // Go to home 
          Intent startMain = new Intent(Intent.ACTION_MAIN);
          startMain.addCategory(Intent.CATEGORY_HOME);
          startMain.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
          appIf.getContext().startActivity(startMain);
          
          // Send the native report
          String dumpBinPath = cmd.substring(cmd.indexOf("(")+1, cmd.indexOf(")"));
          appIf.sendNativeCrashReport(dumpBinPath, true);
        }
      };
      reportThread.start();
      
      // Wait until app exits
      while (true) {
        try {
          Thread.sleep(1000);
        }
        catch (Exception e) {
        }
      }
    }
    if (cmd.equals("initComplete()")) {
      for (String queuedCmd : queuedCoreCommands) {
        executeCoreCommandInt(queuedCmd);
      }
      queuedCoreCommands.clear();
      appIf.cockpitEngineStart();
      cmdExecuted=false; // forward message to activity
    }
    if (cmd.startsWith("updateNavigationInfos(")) {
      String infos = cmd.substring(cmd.indexOf("(")+1, cmd.indexOf(")"));
      appIf.cockputEngineUpdate(infos);
      cmdExecuted=true;
    }
    if (cmd.startsWith("updateMapDownloadStatus(")) {
      String infos = cmd.substring(cmd.indexOf("(") + 1, cmd.indexOf(")"));
      String[] args=infos.split(",");
      int tilesLeft = Integer.parseInt(args[1]);
      long t = System.currentTimeMillis()/1000;
      if ((System.currentTimeMillis()/1000>lastDownloadStatusUpdate)||
          (tilesLeft==0)) {
        Intent intent = appIf.createServiceIntent();
        if (intent!=null) {
          intent.setAction("mapDownloadStatusUpdated");
          intent.putExtra("tilesDone", Integer.parseInt(args[0]));
          intent.putExtra("tilesLeft", tilesLeft);
          intent.putExtra("timeLeft", args[2]);
          lastDownloadStatusUpdate = System.currentTimeMillis() / 1000;
          appIf.getApplication().startService(intent);
        }
      }
      cmdExecuted=true;
    }
    if (!cmdExecuted) {
      appIf.executeAppCommand(cmd);
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
        Process.setThreadPriority(Process.THREAD_PRIORITY_FOREGROUND+Process.THREAD_PRIORITY_LESS_FAVORABLE);
        break;
      case 2:
        Process.setThreadPriority(Process.THREAD_PRIORITY_BACKGROUND);
        break;
      case 3:
        Process.setThreadPriority(Process.THREAD_PRIORITY_BACKGROUND+Process.THREAD_PRIORITY_LESS_FAVORABLE);
        break;
      default:
        appIf.addAppMessage(appIf.FATAL_MSG, "GDApp", "unsupported thread priority");
    }    
  }

  /** Returns the path of the home dir */
  public static String getHomeDirPath() {

    // Create the home directory if necessary
    //File homeDir=Environment.getExternalStorageDirectory(getString(R.string.home_directory));
    File externalStorageDirectory= Environment.getExternalStorageDirectory();
    String homeDirPath = externalStorageDirectory.getAbsolutePath() + "/GeoDiscoverer";
    File homeDir=new File(homeDirPath);
    if (!homeDir.exists()) {
      try {
        homeDir.mkdir();
      }
      catch (Exception e){
        return "";
      }
    }
    return homeDir.getAbsolutePath();

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
          if (changeScreen) {
            executeCoreCommand(changeScreenCommand);
            changeScreen=false;
            forceRedraw=true;
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
    int orientationValue = appIf.getActivityOrientation();
    if (orientationValue!= Configuration.ORIENTATION_UNDEFINED) {
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
    graphicInvalidated=true;
    createGraphic=true;
    currentGL=gl;
    lock.unlock();
  }

  /** Called when a new fix is available */  
  public void onLocationChanged(Location location) {
    if (replayTraceActive)
      return;
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
    
    if (replayTraceActive)
      return;

    // Drop the event if it is unreliable
    //if (event.accuracy==SensorManager.SENSOR_STATUS_UNRELIABLE)
    //  return;

    // Remember the last sensor readings
    switch (event.sensor.getType()) {
      case Sensor.TYPE_MAGNETIC_FIELD:
        lastMagneticField = event.values.clone();
        break;
      case Sensor.TYPE_ACCELEROMETER:
        lastAcceleration = event.values.clone();
        break;
      default:
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
