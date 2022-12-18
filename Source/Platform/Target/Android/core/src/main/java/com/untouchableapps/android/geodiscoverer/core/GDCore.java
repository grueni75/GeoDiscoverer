//============================================================================
// Name        : GDCore.java
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

package com.untouchableapps.android.geodiscoverer.core;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.location.Location;
import android.location.LocationListener;
import android.media.MediaPlayer;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.Process;
import android.util.DisplayMetrics;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Hashtable;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/** Interfaces with the C++ part */
public class GDCore implements
    GLSurfaceView.Renderer,
    LocationListener,
    SensorEventListener,
    Runnable,
    GestureDetector.OnGestureListener,
    GestureDetector.OnDoubleTapListener
{

  //
  // Variables
  //

  // Time info for deciding if silence needs to be played to wake up bluetooth device
  long audioWakeupLastTrigger = 0;
  public long audioWakeupDelay = -1;
  public long audioWakeupDuration = -1;

  /** Current opengl context */
  GL10 currentGL = null;

  /** Current opengl thread */
  Thread renderThread = null;

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

  /** Indicates if the core has finished its early initialization */
  public boolean coreEarlyInitComplete = false;

  /** Indicates if the core has finished its late initialization */
  public boolean coreLateInitComplete = false;

  /** Indicates if the core is currently initialized or destroyed */
  boolean coreLifeCycleOngoing = false;

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
  String [] changeScreenCommandArgs = null;
  
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
  final Lock coreLock = new ReentrantLock();
  final Lock audioWakeupLock = new ReentrantLock();
  final Condition threadInitialized = coreLock.newCondition();

  // Indicates if a replay is active
  boolean replayTraceActive = false;

  // Last time the download status was updated
  long lastDownloadStatusUpdate = 0;

  // Reference to the object that implements the interface functions
  public GDAppInterface appIf = null;

  // Indicates if core is running on a watch
  boolean isWatch = false;

  // Stores the mapping of channel names to file names
  public Hashtable<String, Bundle> channelPathToFilePath = new Hashtable<String, Bundle>();

  /** ID of the pointer that touched the screen first */
  int firstPointerID=-1;

  /** ID of the pointer that touched the screen second */
  int secondPointerID=-1;

  /** Previous angle between first and second pointer */
  float prevAngle=0;

  /** Previous distance between first and second pointer */
  float prevDistance=0;

  // Items for receiving battery changes
  BatteryManager batteryManager;
  public String batteryStatus;

  /** Timestamp after the compass sensor is used again */
  long ignoreCompassSensorEnd=0;
  long ignoreCompassSensorTimeout=0;

  /** Gesture detectors */
  GestureDetector gestureDetector;
  ScaleGestureDetector scaleGestureDetector;
  private class ScaleListener extends ScaleGestureDetector.SimpleOnScaleGestureListener {
    @Override
    public boolean onScaleBegin(ScaleGestureDetector detector) {
      //appIf.addAppMessage(appIf.DEBUG_MSG,"GDApp","scale begins");
      scaleGestureOccured=true;
      return true;
    }

    @Override
    public boolean onScale(ScaleGestureDetector detector) {
      //if (detector.getScaleFactor()!=1.0) appIf.addAppMessage(appIf.DEBUG_MSG,"GDApp",String.format("scale=%f",detector.getScaleFactor()));
      if (zoomMode) executeCoreCommand("zoom(" + String.valueOf(detector.getScaleFactor() + ")"));
      return true;
    }

    @Override
    public void onScaleEnd(ScaleGestureDetector detector) {
      super.onScaleEnd(detector);
      //appIf.addAppMessage(appIf.DEBUG_MSG,"GDApp","scale ends");
      scaleGestureOccured=false;
    }
  }
  boolean zoomMode=false;
  boolean scaleGestureOccured=false;
  /*float zoomStartX;
  float zoomStartY;*/

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
    System.loadLibrary("gdopenssl");
    System.loadLibrary("gdcurl");
    System.loadLibrary("gdproj");
    System.loadLibrary("gdsqlite3");
    System.loadLibrary("gdgdal");
    System.loadLibrary("gdcore");
  }

  /** Updates the battery status */
  public void updateBatteryStatus(boolean force) {
    Intent intent  = appIf.getContext().registerReceiver(null, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
    int status = intent.getIntExtra(BatteryManager.EXTRA_STATUS, -1);
    boolean isCharging = status == BatteryManager.BATTERY_STATUS_CHARGING || status == BatteryManager.BATTERY_STATUS_FULL;
    int percentage = batteryManager.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY);
    String args = String.valueOf(percentage) + "," + (isCharging ? "1" : "0");
    //appIf.addAppMessage(GDAppInterface.DEBUG_MSG,"GDApp",args);
    if (coreInitialized&&(force||(!args.equals(batteryStatus)))) {
      batteryStatus = args;
      executeCoreCommandRaw("setBattery("+batteryStatus+")");
    }
  }
  
  /** Constructor */
  @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
  public GDCore(GDAppInterface appIf, String homePath) {

    // Copy variables
    this.homePath=homePath;
    this.appIf=appIf;

    // Find out if this is a watch
    Configuration config = appIf.getContext().getResources().getConfiguration();
    isWatch = (config.uiMode & Configuration.UI_MODE_TYPE_MASK) == Configuration.UI_MODE_TYPE_WATCH;

    // Prepare the JNI part
    initJNI();

    // Get infos about the screen
    setDisplayMetrics(appIf.getApplication().getResources().getDisplayMetrics());

    // Start the thread that handles the starting and stopping
    thread = new Thread(this);
    thread.start(); 

    // Wait until the thread is initialized
    coreLock.lock();
    try {
      while (messageHandler==null)
      {           
        threadInitialized.await();
      }
    } catch (InterruptedException e) {
      e.printStackTrace();
    } finally {
      coreLock.unlock();
    }

    // Read the battery status
    batteryManager = (BatteryManager) appIf.getContext().getSystemService(Context.BATTERY_SERVICE);

    // Install gesture detectors
    gestureDetector = new GestureDetector(appIf.getContext(),this);
    scaleGestureDetector = new ScaleGestureDetector(appIf.getContext(), new ScaleListener());
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

    AppMessageHandler(Looper looper, GDCore coreObject) {
      super(looper);
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
          if (coreObject.isWatch) {
            coreObject.appIf.addAppMessage(coreObject.appIf.DEBUG_MSG, "GDApp", "Core stop: setWearDeviceAlive(0)");
            coreObject.appIf.sendWearCommand("setWearDeviceAlive(0)");
          }
          coreObject.executeAppCommand("exitActivity()");
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

  // Handler thread
  public void run() {
    
    coreLock.lock();

    // This is a background thread
    setThreadPriority(2);

    // Process messages
    Looper.prepare();
    messageHandler = new AppMessageHandler(Looper.myLooper(),this);
    threadInitialized.signal();
    coreLock.unlock();
    Looper.loop();
  }
  
  //
  // Support functions
  //
  
  /** Updates files on the sdcard with the latest ones */
  boolean updateHome() {

    // Remove any temp downloaded files
    File dir = new File(homePath + "/Map");
    File[] directoryListing = dir.listFiles();
    if (directoryListing != null) {
      String title = "Cleaning home directory";
      executeAppCommand("createProgressDialog(\"" + title + "\"," + directoryListing.length + ")");
      int i=0;
      for (File child : directoryListing) {
        if (child.getName().endsWith(".gda")) {
          child.delete();
        }
        if (child.getName().endsWith(".gdo")) {
          child.delete();
        }
        i++;
        executeAppCommand("updateProgressDialog(\"" + title + "\"," + i + ")");
      }
      executeAppCommand("closeProgressDialog()");
    }

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
    
    // Copy all files (only if changed)
    int i = 0;
    for (String path : files) {
      String externalPath = homePath + "/" + path;
      String internalPath = "GeoDiscoverer/" + path;
      boolean copyFile=true;
      File f = new File(externalPath);
      if (f.exists()) {
        try {
          InputStream is1 = new FileInputStream(f);
          InputStream is2 = am.open(internalPath);
          byte[] buf1 = new byte[1024];
          byte[] buf2 = new byte[1024];
          int len1,len2;
          boolean mismatch=false;
          while ((len1 = is1.read(buf1)) > 0) {
            len2 = is2.read(buf2);
            if (len1!=len2) {
              mismatch=true;
              break;
            }
            if (!Arrays.equals(buf1,buf2)) {
              mismatch = true;
              break;
            }
          }
          is1.close();
          is2.close();
          if (!mismatch)
            copyFile=false;
          //appIf.addAppMessage(GDAppInterface.DEBUG_MSG,"GDApp",f.getPath() + " mismatch=" + String.valueOf(mismatch));
        }
        catch (IOException e) {
          appIf.addAppMessage(GDAppInterface.DEBUG_MSG,"GDApp",e.getMessage());
        }
      }
      if (copyFile) {
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
        } catch (IOException e) {
          executeAppCommand("fatalDialog(\"Could not copy file to home directory: " + e.getMessage() + "!\")");
          return false;
        }
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
  void setDisplayMetrics(DisplayMetrics metrics) {
    coreLock.lock();
    this.screenDPI=metrics.densityDpi;
    double a=metrics.widthPixels/metrics.xdpi;
    double b=metrics.heightPixels/metrics.ydpi;
    this.screenDiagonal=Math.sqrt(a*a+b*b);
    coreLock.unlock();
  }

  /** Starts the core */
  @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
  synchronized void start()
  {
    coreLock.lock();
    if (coreInitialized) {
      coreLock.unlock();
      return;
    }
    if (coreLifeCycleOngoing) {
      coreLock.unlock();
      return;
    }
    coreLifeCycleOngoing=true;
    coreLock.unlock();

    // Check if home dir is available
    boolean initialized=false;

    // Copy the assets files
    if (!updateHome()) {
      coreLock.lock();
      coreLifeCycleOngoing=false;
      coreLock.unlock();
      return;
    }

    // Init the core
    initCore(homePath,screenDPI,screenDiagonal);
    initialized=true;

    // Ensure that the screen is recreated
    coreLock.lock();
    if (initialized) {
      coreInitialized=true;
      executeAppCommand("coreInitialized()");
      executeAppCommand("updateWakeLock()");
      coreStopped=false;
      if (changeScreenCommandArgs!=null) {
        changeScreen=true;
      }
      createGraphic=true;
    }
    coreLifeCycleOngoing=false;
    coreLock.unlock();

    // Update battery
    if (initialized) {
      updateBatteryStatus(true);
    }
  }

  /** Deinits the core */
  @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
  synchronized void stop()
  {
    coreLock.lock();
    if (!coreInitialized) {
      coreLock.unlock();
      return;
    }
    if (coreLifeCycleOngoing) {
      coreLock.unlock();
      return;
    }
    coreLifeCycleOngoing=true;
    coreStopped=true;
    coreInitialized=false;
    coreEarlyInitComplete=false;
    coreLateInitComplete=false;
    coreLock.unlock();

    // Stop the cockpit apps
    //appIf.addAppMessage(GDAppInterface.DEBUG_MSG,"GDApp","before cockpit engine stop");
    appIf.cockpitEngineStop();

    // Deinit the core
    //appIf.addAppMessage(GDAppInterface.DEBUG_MSG,"GDApp","before deinit core");
    deinitCore();

    // Update flags
    coreLock.lock();
    coreLifeCycleOngoing=false;
    coreLock.unlock();
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

  /** Fetches map tile and returns the resulting image */
  public native byte[] fetchMapTile(int z, int x, int y, float saturationOffset, float brightnessOffset);

  /** Renders a hillshade tile and returns the resulting image */
  public native byte[] renderHillshadeTile(int z, int x, int y);

  /** Sets the category path in the config */
  public void configStoreSetStringList(String path, List<String> value) {
    configStoreRemovePath(path);
    for (int i=0;i<value.size();i++) {
     String p=path+"[@index='"+i+"']";
     //appIf.addAppMessage(appIf.DEBUG_MSG,"GDApp","p="+path);
     configStoreSetStringValue(p,"name",value.get(i));
    }
  }

  /** Returns the category path in the config */
  public List<String> configStoreGetStringList(String path) {
    LinkedList<String> result=new LinkedList<String>();
    String[] indices=configStoreGetAttributeValues(path,"index");
    int i=0;
    while (true) {
      boolean found=false;
      for (int j=0;j<indices.length;j++) {
        if (indices[j].equals(String.valueOf(i))) {
          found=true;
          break;
        }
      }
      if (!found)
        break;
      String p=path+"[@index='"+i+"']";
      result.push(configStoreGetStringValue(p,"name"));
      //appIf.addAppMessage(appIf.DEBUG_MSG,"GDApp","i="+i+" name="+result.get(i));
      i++;
    }
    return result;
  }

  /** Constructs a command from the given arguments */
  private String constructCoreCommand(String cmd, String [] args) {
    String cmdInt = cmd + "(";
    boolean firstArg=true;
    for (String arg : args) {
      arg=arg.replace("\\", "\\\\");
      arg=arg.replace("\"", "\\\"");
      if (firstArg) {
        firstArg=false;
      } else {
        cmdInt+=",";
      }
      cmdInt+='"' + arg + '"';
    }
    cmdInt+=")";
    return cmdInt;
  }

  /** Sends an command to the core after checking if it it is ready */
  public String executeCoreCommandRaw(String cmdRaw)
  {
    String result;
    if (coreInitialized) {
      if (cmdRaw.startsWith("replayTrace(")) {
        replayTraceActive=true;
      }
      result = executeCoreCommandInt(cmdRaw);
    } else {
      result = "";
    }
    return result;
  }
  /** Sends an command to the core after checking if it it is ready */
  public String executeCoreCommand(String cmd, String ... args)
  {
    String cmdRaw = constructCoreCommand(cmd, args);
    return executeCoreCommandRaw(cmdRaw);
  }

  /** Sends an command to the core if it is initialized or remembers them for execution after core is initialized */
  public void scheduleCoreCommand(String cmd, String ... args)
  {
    String cmdInt = constructCoreCommand(cmd, args);
    if (coreLateInitComplete) {
      executeCoreCommandInt(cmdInt);
    } else {
      coreLock.lock();
      queuedCoreCommands.add(cmdInt);
      coreLock.unlock();
    }
  }

  //
  // Functions that are called by the native core
  //

  /** Execute an command */
  @SuppressWarnings("resource")
  public String executeAppCommand(final String cmd)
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
    if (cmd.startsWith("scheduleRestart(")) {
      appIf.scheduleRestart();
      cmdExecuted=true;
    }
    if (cmd.equals("lateInitComplete()")) {
      coreLock.lock();
      for (String queuedCmd : queuedCoreCommands) {
        executeCoreCommandInt(queuedCmd);
      }
      queuedCoreCommands.clear();
      coreLock.unlock();
      audioWakeupDuration = Integer.parseInt(configStoreGetStringValue("General", "audioWakeupDuration"))*1000;
      audioWakeupDelay = Integer.parseInt(configStoreGetStringValue("General", "audioWakeupDelay"))*1000;
      appIf.cockpitEngineStart();
      coreLock.lock();
      coreLateInitComplete=true;
      coreLock.unlock();
      if (isWatch) {
        appIf.addAppMessage(appIf.DEBUG_MSG, "GDApp", "earlyInitComplete: setWearDeviceAlive(1)");
        appIf.sendWearCommand("setWearDeviceAlive(1)");
      }
      cmdExecuted=false; // forward message to activity
    }
    if (cmd.equals("getWearDeviceAlive()")) {
      if (coreLateInitComplete) {
        appIf.addAppMessage(appIf.DEBUG_MSG, "GDApp", "getWearDeviceAlive(): setWearDeviceAlive(1)");
        appIf.sendWearCommand("setWearDeviceAlive(1)");
      } else {
        appIf.addAppMessage(appIf.DEBUG_MSG, "GDApp", "getWearDeviceAlive(): setWearDeviceAlive(0)");
        appIf.sendWearCommand("setWearDeviceAlive(0)");
      }
    }
    if (cmd.equals("earlyInitComplete()")) {

      // Update flags
      coreLock.lock();
      coreEarlyInitComplete=true;
      coreLock.unlock();
      ignoreCompassSensorTimeout = Integer.parseInt(configStoreGetStringValue("General", "ignoreCompassSensorTimeout"))*1000;
      cmdExecuted=false; // forward message to activity
    }
    if (cmd.startsWith("setFormattedNavigationInfo(")) {
      String infos = cmd.substring(cmd.indexOf("(")+1, cmd.indexOf(")"));
      appIf.cockputEngineUpdate(infos);
      cmdExecuted=true;
    }
    if (cmd.startsWith("setWearDeviceSleeping(")) {
      if (isWatch)
        appIf.sendWearCommand(cmd);
      cmdExecuted=true;
    }
    if (cmd.startsWith("setWearDeviceAlive(")) {
      if (isWatch)
        appIf.sendWearCommand(cmd);
      cmdExecuted = true;
    }
    if (cmd.startsWith("setNewRemoteMap(")) {
      if (!isWatch)
        appIf.sendWearCommand(cmd);
      cmdExecuted=true;
    }
    if (cmd.startsWith("findRemoteMapTileByGeographicCoordinate(")) {
      if (isWatch)
        appIf.sendWearCommand(cmd);
      cmdExecuted=true;
    }
    if (cmd.startsWith("fillGeographicAreaWithRemoteTiles(")) {
      if (isWatch)
        appIf.sendWearCommand(cmd);
      cmdExecuted=true;
    }
    if (cmd.startsWith("serveRemoteMapArchive(")) {
      if (!isWatch)
        appIf.sendWearCommand(cmd);
      cmdExecuted=true;
    }
    if (cmd.startsWith("serveRemoteOverlayArchive(")) {
      if (!isWatch)
        appIf.sendWearCommand(cmd);
      cmdExecuted=true;
    }
    if (cmd.equals("forceRemoteMapUpdate()")) {
      if (!isWatch)
        appIf.sendWearCommand(cmd);
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
    if (cmd.startsWith("getDeviceName()")) {
      if (isWatch)
        return "Watch";
      else
        return "Default";
    }
    if (cmd.startsWith("setAllNavigationInfo(")) {
      if (!isWatch)
        appIf.sendWearCommand(cmd);
      cmdExecuted=true;
    }
    if (cmd.startsWith("playNewNearestAddressPointAlarm()")) {
      playSound("nearAddressPointAlarm.wav",3, 100);
      cmdExecuted=true;
    }
    if (cmd.startsWith("startMapTileServer()")) {
      Intent intent = appIf.createServiceIntent();
      if (intent!=null) {
        intent.setAction("startMapTileServer");
        appIf.getApplication().startService(intent);
      }
      cmdExecuted=true;
    }
    if (cmd.startsWith("getLastKnownLocation(")) {
      Intent intent = appIf.createServiceIntent();
      if (intent!=null) {
        intent.setAction("getLastKnownLocation");
        appIf.getApplication().startService(intent);
      }
      cmdExecuted=true;
    }
    if (cmd.startsWith("updateNearestPOI(")) {
      String infos = cmd.substring(cmd.indexOf("(") + 1, cmd.indexOf(")"));
      String[] args=infos.split(",");
      Intent intent = appIf.createServiceIntent();
      if (intent!=null) {
        intent.setAction("updateNearestPOI");
        intent.putExtra("lat",Double.valueOf(args[0]));
        intent.putExtra("lng",Double.valueOf(args[1]));
        appIf.getApplication().startService(intent);
      }
      cmdExecuted=true;
    }

    if (!cmdExecuted) {
      appIf.executeAppCommand(cmd);
    }
    return "";
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
  @SuppressWarnings("deprecation")
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
    //appIf.addAppMessage(appIf.DEBUG_MSG, "GDApp", "draw start");
    coreLock.lock();
    boolean blankScreen=false;
    if (gl==currentGL) {
      if (!coreStopped) {
        if (!suspendCore) {
          if (changeScreen) {
            executeCoreCommand("screenChanged", changeScreenCommandArgs);
            changeScreen=false;
            forceRedraw=true;
          }
          if (graphicInvalidated) {        
            executeCoreCommand("graphicInvalidated");
            graphicInvalidated=false;
            createGraphic=false;
          }
          if (createGraphic) {        
            executeCoreCommand("createGraphic");
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
      GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
      if (lastFrameDrawnByCore) {
        executeAppCommand("setSplashVisibility(1)");
      }
      lastFrameDrawnByCore=false;
    }
    coreLock.unlock();
    //appIf.addAppMessage(appIf.DEBUG_MSG, "GDApp", "draw end");
  }
  
  /** Updates the splash visibility flag */
  public void setSplashIsVisible(boolean splashIsVisible) {
    coreLock.lock();
    this.splashIsVisible = splashIsVisible;
    coreLock.unlock();
  }
  
  /** Called when the surface changes */
  public void onSurfaceChanged(GL10 gl, int width, int height) {
    appIf.addAppMessage(GDAppInterface.DEBUG_MSG, "GDApp", "onSurfaceChanged called");
    coreLock.lock();
    int orientationValue = appIf.getActivityOrientation();
    if (orientationValue!= Configuration.ORIENTATION_UNDEFINED) {
      String orientationString="portrait";
      if (orientationValue==Configuration.ORIENTATION_LANDSCAPE)
        orientationString="landscape";
      if (changeScreenCommandArgs==null) {
        changeScreenCommandArgs=new String[3];
      }
      changeScreenCommandArgs[0]=orientationString;
      changeScreenCommandArgs[1]=String.valueOf(width);
      changeScreenCommandArgs[2]=String.valueOf(height);
      changeScreen=true;
    }
    coreLock.unlock();
  }

  /** Called when the surface is created */
  public void onSurfaceCreated(GL10 gl, EGLConfig config) {
    appIf.addAppMessage(GDAppInterface.DEBUG_MSG, "GDApp", "onSurfaceCreated called");
    coreLock.lock();
    // Context is lost, so tell the core to recreate any textures
    graphicInvalidated=true;
    createGraphic=true;
    currentGL=gl;
    renderThread=Thread.currentThread();
    coreLock.unlock();
  }

  /** Called when a new fix is available */  
  public void onLocationChanged(Location location) {
    updateBatteryStatus(false);
    if (replayTraceActive)
      return;
    if (location!=null) {
      String[] args = new String[13];
      args[0]=location.getProvider();
      args[1]=String.valueOf(location.getTime());
      args[2]=String.valueOf(location.getLongitude());
      args[3]=String.valueOf(location.getLatitude());
      int t=0;
      if (location.hasAltitude()) {
        t=1;
      }
      args[4]=String.valueOf(t);
      args[5]=String.valueOf(location.getAltitude());
      args[6]=String.valueOf(1);
      t=0;
      if (location.hasBearing()) {
        t=1;
      }
      args[7]=String.valueOf(t);
      args[8]=String.valueOf(location.getBearing());
      t=0;
      if (location.hasSpeed()) {
        t=1;
      }
      args[9]=String.valueOf(t);
      args[10]=String.valueOf(location.getSpeed());
      t=0;
      if (location.hasAccuracy()) {
        t=1;
      }
      args[11]=String.valueOf(t);
      args[12]=String.valueOf(location.getAccuracy());
      StringBuffer sb = new StringBuffer();
      for(int i = 0; i < args.length; i++) {
        sb.append(args[i]);
      }
      //appIf.addAppMessage(appIf.DEBUG_MSG,"GDApp","locationChanged: hasSpeed=" + args[9] + " speed=" + args[10]);
      executeCoreCommand("locationChanged",args);
    }
  }

  // Other call backs from the location manager
  public void onProviderDisabled(String provider) {
  }
  public void onProviderEnabled(String provider) {
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
    if ((lastMagneticField!=null) && (lastAcceleration!=null) && (coreInitialized) && (System.currentTimeMillis()>=ignoreCompassSensorEnd)) {
      SensorManager.getRotationMatrix(R, I, lastAcceleration, lastMagneticField);
      SensorManager.remapCoordinateSystem(R,SensorManager.AXIS_X,SensorManager.AXIS_Z, correctedR);
      SensorManager.getOrientation(correctedR, orientation);
      orientation[0]=(float) (orientation[0]*180.0/Math.PI);
      if (orientation[0]<0)
        orientation[0]=360+orientation[0];
      executeCoreCommand("compassBearingChanged",String.valueOf(orientation[0]));
    }
    
  }

  /** Called when a the orientation sensor has changed */
  public void overrideCompassBearing(float value) {
    ignoreCompassSensorEnd=System.currentTimeMillis()+ignoreCompassSensorTimeout;
    if (coreInitialized) {
      executeCoreCommand("compassBearingChanged", String.valueOf(value));
    }
  }

  // Other calls back from the sensor manager
  public void onAccuracyChanged(Sensor sensor, int accuracy) {
  }

  /** Wakeups the audio device if required */
  public void audioWakeup() {

    audioWakeupLock.lock();
    if (audioWakeupDelay==-1) {
      audioWakeupLock.unlock();
      return;
    }
    //appIf.addAppMessage(GDAppInterface.DEBUG_MSG, "GDApp", String.format("audioWakeup called"));
    long t = System.currentTimeMillis();
    if (audioIsAsleep()) {
      audioWakeupLastTrigger = t + audioWakeupDelay;
      try {
        final AssetFileDescriptor afd = appIf.getContext().getAssets().openFd("Sound/silence.mp3");
        final MediaPlayer player = new MediaPlayer();
        player.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
        player.prepare();
        player.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
          public void onCompletion(MediaPlayer mp) {
            try {
              afd.close();
              player.release();
            } catch (IOException e) {
              appIf.addAppMessage(GDAppInterface.DEBUG_MSG, "GDApp", e.getMessage());
            }
          }
        });
        player.start();
      }
      catch (IOException e) {
        appIf.addAppMessage(GDAppInterface.DEBUG_MSG, "GDApp", e.getMessage());
      }
      boolean repeat=true;
      while (repeat) {
        try {
          Thread.sleep(audioWakeupLastTrigger-t);
          repeat=false;
        } catch (InterruptedException e) {
          t = System.currentTimeMillis();
          if (t>=audioWakeupLastTrigger)
            repeat=false;
        }
      }
    }
    audioWakeupLastTrigger = System.currentTimeMillis();
    //appIf.addAppMessage(GDAppInterface.DEBUG_MSG, "GDApp", String.format("lastTrigger=%d",audioWakeupLastTrigger));
    audioWakeupLock.unlock();
  }

  /** Indicates if audio device is asleep */
  public boolean audioIsAsleep() {
    if (audioWakeupDuration==-1)
      return true;
    long t = System.currentTimeMillis();
    return (t>audioWakeupLastTrigger+audioWakeupDuration);
  }

  /** Calculates the chage in angle and zoom if the pinch-to-zoom gesture is used */
  void updateTwoFingerGesture(MotionEvent event, boolean moveAction) {
    if (zoomMode)
      return;
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
          executeCoreCommand("touchMove", String.valueOf(x), String.valueOf(y));
        } else {
          executeCoreCommand("touchDown", String.valueOf(x), String.valueOf(y));
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
          executeCoreCommand("twoFingerGesture", String.valueOf(x), String.valueOf(y), String.valueOf(angleDiff), String.valueOf(scaleDiff));


        } else {

          // Set new position
          executeCoreCommand("touchDown", String.valueOf(x), String.valueOf(y));

        }

        // Update previous values
        prevDistance=distance;
        prevAngle=angle;
      }
    }
    catch (Throwable e) {
      appIf.addAppMessage(appIf.WARNING_MSG, "GDApp", "can not call multitouch related methods");
      System.exit(1);
    }
  }

  /** Called if the map is touched */
  public boolean onTouchEvent(final MotionEvent event) {

    // Call the gesture detectors
    gestureDetector.onTouchEvent(event);
    scaleGestureDetector.onTouchEvent(event);
    //appIf.addAppMessage(appIf.DEBUG_MSG,"GDApp",String.format("zoomMode=%d",zoomMode ? 1 : 0));

    /* If a zoom is detected, then handle it exclusively until finger is removed
    if (zoomMode) {
      float distY=event.getY()-zoomStartY;
      double scaleDiff;
      appIf.addAppMessage(appIf.DEBUG_MSG,"GDApp",String.format("distY=%f",distY));
      if (prevDistance!=0) {
        if (distY>0)
          scaleDiff=distY/prevDistance;
        else
          scaleDiff=-prevDistance/distY;
        executeCoreCommand("zoom(" + String.valueOf(scaleDiff) + ")");
      }
      prevDistance=distY;
    }*/

    // What happened?
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.ECLAIR) {

      // Extract infos
      int action = event.getAction();
      int x = Math.round(event.getX());
      int y = Math.round(event.getY());

      switch(action) {

        case MotionEvent.ACTION_DOWN:
          if (!zoomMode) executeCoreCommand("touchDown", String.valueOf(x), String.valueOf(y));
          break;

        case MotionEvent.ACTION_MOVE:
          if (!zoomMode) executeCoreCommand("touchMove", String.valueOf(x), String.valueOf(y));
          break;

        case MotionEvent.ACTION_UP:
          if (!zoomMode) executeCoreCommand("touchUp", String.valueOf(x), String.valueOf(y));
          zoomMode=false;
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
          if (!zoomMode) executeCoreCommand("touchDown",String.valueOf(x), String.valueOf(y));
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
          if (!zoomMode) executeCoreCommand("touchUp", String.valueOf(x), String.valueOf(y));
          firstPointerID=-1;
          secondPointerID=-1;
          zoomMode=false;
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

  /** Plays a sound from the asset directory */
  public void playSound(String filename, int repeat, int volume) {
    Thread playThread = new Thread(new Runnable() {
      int remainingRepeats=repeat;
      @Override
      public void run() {
        setThreadPriority(2);
        audioWakeup();
        try {
          final AssetFileDescriptor afd = appIf.getContext().getAssets().openFd("Sound/" + filename);
          MediaPlayer player = new MediaPlayer();
          player.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
          player.prepare();
          //appIf.addAppMessage(appIf.DEBUG_MSG,"GDApp",String.format("volume=%d",volume));
          float log1=1.0f-(float)(Math.log(100-volume)/Math.log(100));
          player.setVolume(log1,log1);
          player.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
            @Override
            public void onCompletion(MediaPlayer mp) {
              remainingRepeats--;
              if (remainingRepeats>0) {
                player.reset();
                try {
                  player.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
                  player.prepare();
                } catch (IOException e) {
                  appIf.addAppMessage(appIf.DEBUG_MSG, "GDApp", e.getMessage());
                }
                player.start();
              } else {
                player.release();
                try {
                  afd.close();
                } catch (IOException e) {
                  appIf.addAppMessage(appIf.DEBUG_MSG, "GDApp", e.getMessage());
                }
              }
            }
          });
          player.start();
        }
        catch (IOException e) {
          appIf.addAppMessage(appIf.DEBUG_MSG, "GDApp", e.getMessage());
        }
      }
    });
    playThread.start();
  }

  //
  // Various callbacks for detected gestures
  //

  @Override
  public boolean onSingleTapConfirmed(MotionEvent e) {
    return false;
  }

  @Override
  public boolean onDoubleTap(MotionEvent e) {
    //appIf.addAppMessage(appIf.DEBUG_MSG,"GDApp",String.format("double tap!"));
    zoomMode=true;
    /*zoomStartX=e.getX();
    zoomStartY=e.getY();
    prevDistance=0;*/
    return false;
  }

  @Override
  public boolean onDoubleTapEvent(MotionEvent e) {
    if (e.getAction()==MotionEvent.ACTION_UP) {
      if (!scaleGestureOccured) {
        //appIf.addAppMessage(appIf.DEBUG_MSG, "GDApp", String.format("double tap event!"));
        executeCoreCommand("toggleZoomLevelLock()(");
      }
      return true;
    } else {
      return false;
    }
  }

  @Override
  public boolean onDown(MotionEvent e) {
    return false;
  }

  @Override
  public void onShowPress(MotionEvent e) {

  }

  @Override
  public boolean onSingleTapUp(MotionEvent e) {
    return false;
  }

  @Override
  public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
    return false;
  }

  @Override
  public void onLongPress(MotionEvent e) {
    //appIf.addAppMessage(appIf.DEBUG_MSG, "GDApp", String.format("on long press!"));
    //executeCoreCommand("toggleFingerMenu()(");
  }

  @Override
  public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
    return false;
  }
}
