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

package com.untouchableapps.android.geodiscoverer;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.ref.WeakReference;
import java.math.BigInteger;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.util.logging.ConsoleHandler;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.jmdns.JmDNS;
import javax.jmdns.ServiceListener;
import javax.jmdns.impl.JmDNSImpl;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import org.acra.ACRA;
import org.acra.ACRAConstants;
import org.acra.ReportField;

import android.annotation.SuppressLint;
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
import android.net.nsd.NsdManager.DiscoveryListener;
import android.net.nsd.NsdManager.RegistrationListener;
import android.net.nsd.NsdManager.ResolveListener;
import android.net.nsd.NsdManager;
import android.net.nsd.NsdServiceInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.MulticastLock;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.Process;
import android.util.Base64;
import android.util.DisplayMetrics;
import android.util.Log;
import android.widget.Toast;

/** Interfaces with the C++ part */
public class GDCore implements GLSurfaceView.Renderer, LocationListener, SensorEventListener, Runnable {

  //
  // Variables
  //
  
  /** Parent application */
  protected Application application = null;
  
  /** Parent activity */
  protected ViewMap activity = null;

  /** Current opengl context */
  protected GL10 currentGL = null;
  
  /** Path to the home directory */
  protected String homePath;
  
  /** DPI of the screen */
  protected int screenDPI = 0;
  
  /** Diagonal of the screen in inches */
  protected double screenDiagonal;
  
  /** Indicates that the core is stopped */
  protected boolean coreStopped = true;

  /** Indicates that the core shall not update its frames */
  boolean suspendCore = false;

  /** Indicates if the core is initialized */
  boolean coreInitialized = false;
  
  /** Indicates if the core is currently initialized or destroyed */
  boolean coreLifeCycleOngoing = false;

  /** Indicates that home dir is available */
  protected boolean homeDirAvailable = false;

  /** Indicates that the graphic must be re-created */
  protected boolean createGraphic = false;

  /** Indicates that the graphic has been invalidated */
  protected boolean graphicInvalidated = false;

  /** Indicates that the sreen properties have changed */
  protected boolean changeScreen = false;
  
  /** Indicates that the screen should be redrawn */
  protected boolean forceRedraw = false;
  
  /** Indicates that the last frame has been handled by the core */
  protected boolean lastFrameDrawnByCore = false;
  
  /** Indicates that the splash is currently shown */
  protected boolean splashIsVisible = false;
  
  /** Command to execute for changing the screen */
  protected String changeScreenCommand = "";
  
  /** Queued commands to execute if core is initialized */
  protected LinkedList<String> queuedCoreCommands = new LinkedList<String>();
  
  // Sensor readings
  protected float[] lastAcceleration = null;
  protected float[] lastMagneticField = null;

  // Arrays used to compute the orientation
  protected float[] R = new float[16];
  protected float[] correctedR = new float[16];
  protected float[] I = new float[16];
  protected float[] orientation = new float[3];

  // Thread signaling
  protected final Lock lock = new ReentrantLock();
  protected final Condition threadInitialized = lock.newCondition();
    
  // Cockpit engine
  protected CockpitEngine cockpitEngine = null;
  
  // Indicates if a replay is active
  protected boolean replayTraceActive = false; 

  // References for jmDNS
  WifiManager wifiManager;
  protected JmDNS jmDNS = null;
  protected GDDashboardServiceListener dashboardServiceListener = null;
  MulticastLock multicastLock = null;

  // References for the network discovery via ARP lookup
  Thread lookupARPCacheThread = null;
  boolean quitLookupARPCacheThread = false;

  // Last time the download status was updated
  long lastDownloadStatusUpdate = 0;

  /**
   * Returns the IP4 address of the wlan interface
   */
  @SuppressWarnings("unchecked")
  private InetAddress getWifiInetAddress() {
    int ipAddress = wifiManager.getConnectionInfo().getIpAddress();
    if (ByteOrder.nativeOrder().equals(ByteOrder.LITTLE_ENDIAN)) {
      ipAddress = Integer.reverseBytes(ipAddress);
    }
    byte[] ipByteArray = BigInteger.valueOf(ipAddress).toByteArray();
    InetAddress inetAddress=null;
    try {
      inetAddress = InetAddress.getByAddress(ipByteArray);
    } catch (UnknownHostException e) {
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", e.getMessage());
    }
    return inetAddress;
  }

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
  
  /** Constructor 
   * @param screenDPI */
  @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
  GDCore(Application application, String homePath) {

    // Copy variables
    this.application=application;
    this.homePath=homePath;
   
    // Get services
    wifiManager = (android.net.wifi.WifiManager) application.getSystemService(android.content.Context.WIFI_SERVICE);

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
  // Thread that handles the starting and stoppingg of the core
  //
  
  // Message handler
  static protected class AppMessageHandler extends Handler {
    
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
          GDApplication.addMessage(GDApplication.ERROR_MSG, "GDApp", "unknown message received");
      }
    }
  }

  protected Thread thread = null;
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
  protected boolean updateHome() {

    // Check if the GeoDiscoverer assets need to be updated
    AssetManager am = application.getAssets();
    SharedPreferences prefs = application.getSharedPreferences("assets",Context.MODE_PRIVATE);
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
  
  /** Sets the view map activity */
  public void setActivity(ViewMap activity) {
    lock.lock();
    this.activity = activity;
    lock.unlock();
  }
  
  /** Starts the core */
  @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
  protected synchronized void start()
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
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp","home dir not available");
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
      
      // Search for geo dashboard devices if configured
      if (Integer.valueOf(configStoreGetStringValue("Cockpit/App/Dashboard","active"))>0) {
        
        // Use zero conf to discover devices if configured
        if (Integer.valueOf(configStoreGetStringValue("Cockpit/App/Dashboard","useZeroConf"))!=0) {
          GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "acquiring multicast lock");
          if (wifiManager!=null) {
            multicastLock = wifiManager.createMulticastLock("Geo Discoverer lock for JmDNS");
            if (multicastLock!=null) {
              multicastLock.setReferenceCounted(true);
              multicastLock.acquire();
              GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "starting jmDNS");
              InetAddress deviceAddress = getWifiInetAddress();
              if (deviceAddress==null)
                executeAppCommand("errorDialog(\"Can not start zero conf daemon! WLAN not active?\")");
              else {
                //Logger logger = Logger.getLogger("javax.jmdns.impl.SocketListener");
                //logger.setLevel(Level.FINEST);
                try {
                  jmDNS = JmDNS.create(deviceAddress);
                  dashboardServiceListener = new GDDashboardServiceListener(jmDNS);
                  jmDNS.addServiceListener(GDApplication.dashboardNetworkServiceType, dashboardServiceListener);
                  executeAppCommand("infoDialog(\"" + deviceAddress.toString() + "\")");
                } catch (IOException e) {
                  GDApplication.addMessage(GDApplication.ERROR_MSG, "GDApp", e.getMessage());
                  executeAppCommand("errorDialog(\"Could not start zero conf daemon!\")");
                }
              }
            } else {
              executeAppCommand("errorDialog(\"Could not get multicast lock!\")");
            }
          } else {
            executeAppCommand("errorDialog(\"No WiFi manager available?\")");
          }
        }

        // Use ARP cache lookups to discover devices if configured
        if (Integer.valueOf(configStoreGetStringValue("Cockpit/App/Dashboard","useAddressCacheLookup"))!=0) {
          quitLookupARPCacheThread = false;
          GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "starting ARP cache lookup thread");
          lookupARPCacheThread = new Thread(new Runnable() {
            
            @Override
            public void run() {
              
              int sleepTime = Integer.valueOf(configStoreGetStringValue("Cockpit/App/Dashboard","addressCacheSleepTime"))*1000;
              int port = Integer.valueOf(configStoreGetStringValue("Cockpit/App/Dashboard", "port"));
              while (!quitLookupARPCacheThread) {
                
                // Go through the ARP cache
                try {
                  BufferedReader br = new BufferedReader(new FileReader("/proc/net/arp"));
                  try {
                    String line = br.readLine();
                    Pattern p = Pattern.compile("^\\s*(\\d+\\.\\d+\\.\\d+\\.\\d+)\\s*");
                    while ((line != null)&&(!quitLookupARPCacheThread)) {
                      Matcher m = p.matcher(line);
                      if (m.find()) {
                        String ip = m.group(1);
                        executeCoreCommand(String.format("addDashboardDevice(%s,%d)",ip,port));
                      }
                      line = br.readLine();
                    }
                  } finally {
                    br.close();
                  }
                }
                catch (IOException e) {
                  GDApplication.addMessage(GDApplication.ERROR_MSG, "GDApp", e.toString());
                  executeAppCommand("errorDialog(\"Could not lookup address cache!\")");
                }
                
                // Sleep for the defined time
                if (!quitLookupARPCacheThread) {
                  try {
                    Thread.sleep(sleepTime);
                  }
                  catch (InterruptedException e) {
                  }
                }
              }
            }
          });
          lookupARPCacheThread.start();
        }
      }        
    }
    coreLifeCycleOngoing=false;
    lock.unlock();
  } 

  /** Deinits the core */
  @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
  protected synchronized void stop()
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
    if (cockpitEngine!=null) { 
      cockpitEngine.stop();
      cockpitEngine=null;
    }
    
    // Stop service discovery
    if (dashboardServiceListener!=null) {
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "stopping jmDNS");
      jmDNS.removeServiceListener(GDApplication.dashboardNetworkServiceType, dashboardServiceListener);
      try {
        jmDNS.close();
      }
      catch (IOException e) {
        GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", e.getMessage());
      }
      jmDNS=null;
      dashboardServiceListener=null;
    }
    if (multicastLock!=null) {
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "releasing multicast lock");
      multicastLock.release();
      multicastLock=null;
    }
    if (lookupARPCacheThread!=null) {
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "stopping ARP cache lookup thread");
      quitLookupARPCacheThread = true;
      boolean repeat = true;
      while (repeat) {
        repeat=false;
        lookupARPCacheThread.interrupt();
        try {
          lookupARPCacheThread.join(100);
        }
        catch (InterruptedException e) {
          repeat=true;
        }
        if (lookupARPCacheThread.isAlive())
          repeat=true;
        else
          repeat=false;
      }
      lookupARPCacheThread = null;
    }

    // Deinit the core
    deinitCore();

    // Update flags
    lock.lock();
    coreLifeCycleOngoing=false;
    lock.unlock();
  } 

  /** Deinits the core and restarts it */
  protected void restart(boolean resetConfig)
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
  
  // Checks if the cockpit engine is active
  boolean cockpitEngineIsActive() {
    if (cockpitEngine!=null) {
      return cockpitEngine.isActive();
    } else {
      return false;
    }
  }
  
  //
  // Functions implemented by the native core
  //
  
  /** Prepares the java native interface */
  protected native void initJNI();
  
  /** Cleans up the java native interfacet */
  protected native void deinitJNI();

  /** Starts the C++ part */
  protected native void initCore(String homePath, int DPI, double diagonal);
  
  /** Stops the C++ part */
  protected native void deinitCore();
  
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
  
  /** Sends a native crash report */
  public void sendNativeCrashReport(String dumpBinPath, boolean quitApp) {

    // Get the content of the minidump file and format it in base64
    ACRA.getErrorReporter().putCustomData("nativeMinidumpPath", dumpBinPath);
    String dumpTxtPath = dumpBinPath.concat(".base64");
    try {
        DataInputStream dumpBinReader = new DataInputStream(new FileInputStream(dumpBinPath));
        long len = new File(dumpBinPath).length();
        if (len > Integer.MAX_VALUE) {
          dumpBinReader.close();
          throw new IOException("File "+dumpBinPath+" too large, was "+len+" bytes.");
        }
        byte[] bytes = new byte[(int) len];
        dumpBinReader.readFully(bytes);
        String dumpContents = Base64.encodeToString(bytes,Base64.DEFAULT);
        BufferedWriter dumpTextWriter = new BufferedWriter(new FileWriter(dumpTxtPath));
        dumpTextWriter.write(dumpContents);
        dumpTextWriter.close();
        String[] dumpContentsInLines = dumpContents.split("\n");
        ReportField[] customReportFields = new ReportField[ACRAConstants.DEFAULT_REPORT_FIELDS.length+1];
        System.arraycopy(ACRAConstants.DEFAULT_REPORT_FIELDS, 0, customReportFields, 0, ACRAConstants.DEFAULT_REPORT_FIELDS.length);
        customReportFields[ACRAConstants.DEFAULT_REPORT_FIELDS.length]=ReportField.APPLICATION_LOG;
        ACRA.getConfig().setCustomReportContent(customReportFields);
        ACRA.getConfig().setApplicationLogFileLines(dumpContentsInLines.length+1);
        ACRA.getConfig().setApplicationLogFile(dumpTxtPath);
        dumpBinReader.close();
    } 
    catch (Exception e) {
    }
    
    // Send report via ACRA
    Exception e = new Exception("GDCore has crashed");
    ACRA.getErrorReporter().handleException(e,quitApp);
  }
  
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
          GDApplication.appContext.startActivity(startMain);
          
          // Send the native report
          String dumpBinPath = cmd.substring(cmd.indexOf("(")+1, cmd.indexOf(")"));
          sendNativeCrashReport(dumpBinPath,true);
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
      if (cockpitEngine!=null) {
        cockpitEngine.stop();
      }
      cockpitEngine=new CockpitEngine(this,application);
      cmdExecuted=false; // forward message to activity
    }
    if (cmd.startsWith("updateNavigationInfos(")) {
      String infos = cmd.substring(cmd.indexOf("(")+1, cmd.indexOf(")"));
      if (cockpitEngine!=null)
        cockpitEngine.update(infos, false);
      cmdExecuted=true;
    }
    if (cmd.startsWith("updateMapDownloadStatus(")) {
      String infos = cmd.substring(cmd.indexOf("(") + 1, cmd.indexOf(")"));
      String[] args=infos.split(",");
      int tilesLeft = Integer.parseInt(args[1]);
      long t = System.currentTimeMillis()/1000;
      if ((System.currentTimeMillis()/1000>lastDownloadStatusUpdate)||
          (tilesLeft==0)) {
        Intent intent = new Intent(application.getApplicationContext(), GDService.class);
        intent.setAction("mapDownloadStatusUpdated");
        intent.putExtra("tilesDone", Integer.parseInt(args[0]));
        intent.putExtra("tilesLeft",tilesLeft);
        intent.putExtra("timeLeft",args[2]);
        lastDownloadStatusUpdate=System.currentTimeMillis()/1000;
        application.startService(intent);
      }
      cmdExecuted=true;
    }
    if (!cmdExecuted) {
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
        Process.setThreadPriority(Process.THREAD_PRIORITY_FOREGROUND+Process.THREAD_PRIORITY_LESS_FAVORABLE);
        break;
      case 2:
        Process.setThreadPriority(Process.THREAD_PRIORITY_BACKGROUND);
        break;
      case 3:
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
    if (activity!=null) {
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
