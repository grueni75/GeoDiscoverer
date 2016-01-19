//============================================================================
// Name        : CockpitEngine.java
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

package com.untouchableapps.android.geodiscoverer.cockpit;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.math.BigInteger;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.ByteOrder;
import java.util.Calendar;
import java.util.LinkedList;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.untouchableapps.android.geodiscoverer.cockpit.CockpitAppInterface.AlertType;
import com.untouchableapps.android.geodiscoverer.GDApplication;
import com.untouchableapps.android.geodiscoverer.core.GDCore;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.MediaPlayer;
import android.net.wifi.WifiManager;

import javax.jmdns.JmDNS;

public class CockpitEngine {
  
  // Minimum time that must pass between updates
  int minUpdatePeriodNormal;
  int minUpdatePeriodTurn;
  int offRouteAlertFastPeriod;
  int offRouteAlertFastCount;
  int offRouteAlertSlowPeriod;

  // Time in milliseconds to sleep before alerting
  int waitTimeBeforeAlert;

  // Last time the navigation info was updated
  long lastUpdate;
  
  // Last infos used for updating
  String lastInfosAsSSV = "";
  
  // Distance to turn
  String currentTurnDistance="-";
  String lastTurnDistance="-";
  
  // Off route indication
  boolean currentOffRoute=false;
  boolean lastOffRoute=false;

  // Time info for deciding if silence needs to be played to wake up bluetooth device
  long audioWakeupLastTrigger = 0;
  long audioWakeupDelay = 0;
  long audioWakeupDuration = 0;

  // GDApp
  GDApplication app;

  // Thread that manages vibrations
  final Lock lock = new ReentrantLock();
  final Condition triggerAlert = lock.newCondition();
  int expectedAlertCount = 0;
  int currentVibrateCount = 0;
  int fastVibrateCount = 1;
  boolean quitVibrateThread = false;
  Thread vibrateThread = null;

  // Thread that handles network requests
  static final int NET_CMD_PLAY_SOUND_DOG = 3;
  static final int NET_CMD_PLAY_SOUND_SHIP = 4;
  static final int NET_CMD_PLAY_SOUND_CAR = 5;
  boolean quitNetworkServerThread=false;
  Thread networkServerThread=null;
  ServerSocket networkServerSocket=null;
  int networkServerPort = 11111;
  boolean networkServiceActive=false;
  void startNetworkService() {

    quitNetworkServerThread=false;
    networkServerThread = new Thread(new Runnable() {

      @Override
      public void run() {

        // Open server socket
        if (networkServerSocket==null) {
          try {
            networkServerSocket = new ServerSocket();
            networkServerSocket.setReuseAddress(true);
            networkServerSocket.bind(new InetSocketAddress(networkServerPort+1));
          }
          catch (IOException e) {
            GDApplication.addMessage(
                GDApplication.DEBUG_MSG,
                "GDApp",
                e.getMessage()
            );
            networkServerSocket = null;
            return;
          }
        }

        // Set bitmap to indicate "wait for first connection"
        networkServiceActive = true;

        // Handle network communication
        while (!quitNetworkServerThread) {
          try {
            Socket client = networkServerSocket.accept();
            int cmd = client.getInputStream().read();
            String soundFile = "";
            switch(cmd) {
              case NET_CMD_PLAY_SOUND_SHIP:
                soundFile = "Sound/ship.mp3";
                break;
              case NET_CMD_PLAY_SOUND_CAR:
                soundFile = "Sound/car.mp3";
                break;
              case NET_CMD_PLAY_SOUND_DOG:
                soundFile = "Sound/dog.mp3";
                break;
            }
            client.close();
            audioWakeup();
            final AssetFileDescriptor afd = app.getContext().getAssets().openFd(soundFile);
            final MediaPlayer player = new MediaPlayer();
            player.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
            player.prepare();
            player.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
              public void onCompletion(MediaPlayer mp) {
                try {
                  afd.close();
                  player.release();
                } catch (IOException e) {
                  GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", e.getMessage());
                }
              }
            });
            player.start();
          }
          catch (IOException e) {
            GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", e.getMessage());
          }
        }
      }

    });
    networkServerThread.start();
  }
  void stopNetworkService() {
    quitNetworkServerThread=true;
    Thread stopThread = new Thread(new Runnable() {
      @Override
      public void run() {
        try {
          Socket clientSocket = new Socket("localhost",networkServerSocket.getLocalPort());
          clientSocket.getOutputStream().write(255);
          clientSocket.close();
          networkServerThread.join();
        }
        catch (Exception e) {
        }
        networkServiceActive=false;
      }
    });
    stopThread.start();
    boolean repeat=true;
    while (repeat) {
      try {
        repeat=false;
        stopThread.join();
      }
      catch (InterruptedException e) {
        repeat=true;
      }
    }
  }

  // List of registered apps
  LinkedList<CockpitAppInterface> apps = new LinkedList<CockpitAppInterface>();

  // References for jmDNS
  WifiManager wifiManager;
  protected JmDNS jmDNS = null;
  protected GDDashboardServiceListener dashboardServiceListener = null;
  WifiManager.MulticastLock multicastLock = null;

  // References for the network discovery via ARP lookup
  Thread lookupARPCacheThread = null;
  boolean quitLookupARPCacheThread = false;

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
      app.addMessage(app.DEBUG_MSG, "GDApp", e.getMessage());
    }
    return inetAddress;
  }

  /** Constructor */
  public CockpitEngine(GDApplication app) {
    super();
    
    // Remember context
    this.app = app;

    // Get services
    wifiManager = (android.net.wifi.WifiManager) app.getContext().getSystemService(android.content.Context.WIFI_SERVICE);

    // Init parameters
    minUpdatePeriodNormal = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit", "minUpdatePeriodNormal"));
    minUpdatePeriodTurn = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit", "minUpdatePeriodTurn"));
    waitTimeBeforeAlert = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit", "waitTimeBeforeAlert"));
    offRouteAlertFastPeriod = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit", "offRouteAlertFastPeriod"));
    offRouteAlertFastCount = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit", "offRouteAlertFastCount"));
    offRouteAlertSlowPeriod = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit", "offRouteAlertSlowPeriod"));
    audioWakeupDuration = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit", "audioWakeupDuration"))*1000;
    audioWakeupDelay = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit", "audioWakeupDelay"))*1000;

    // Add all activated apps
    if (Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit/App/MetaWatch", "active"))>0) {
      apps.add(new CockpitAppMetaWatch(app.getContext()));
    }
    if (Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit/App/Voice", "active"))>0) {
      apps.add(new CockpitAppVoice(app.getContext(),this));
    }
    if (Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit/App/Vibration", "active"))>0) {
      apps.add(new CockpitAppVibration(app.getContext()));
    }
    
    // Start the engine
    start();
  }
  
  /** Starts the cockpit apps */
  public void start() {

    // Search for geo dashboard devices if configured
    if (Integer.valueOf(app.coreObject.configStoreGetStringValue("Cockpit/App/Dashboard", "active"))>0) {

      // Use zero conf to discover devices if configured
      if (Integer.valueOf(app.coreObject.configStoreGetStringValue("Cockpit/App/Dashboard", "useZeroConf"))!=0) {
        app.addMessage(app.DEBUG_MSG, "GDApp", "acquiring multicast lock");
        if (wifiManager!=null) {
          multicastLock = wifiManager.createMulticastLock("Geo Discoverer lock for JmDNS");
          if (multicastLock!=null) {
            multicastLock.setReferenceCounted(true);
            multicastLock.acquire();
            app.addMessage(app.DEBUG_MSG, "GDApp", "starting jmDNS");
            InetAddress deviceAddress = getWifiInetAddress();
            if (deviceAddress==null)
              app.executeAppCommand("errorDialog(\"Can not start zero conf daemon! WLAN not active?\")");
            else {
              //Logger logger = Logger.getLogger("javax.jmdns.impl.SocketListener");
              //logger.setLevel(Level.FINEST);
              try {
                jmDNS = JmDNS.create(deviceAddress);
                dashboardServiceListener = new GDDashboardServiceListener(jmDNS);
                jmDNS.addServiceListener(app.dashboardNetworkServiceType, dashboardServiceListener);
                app.executeAppCommand("infoDialog(\"" + deviceAddress.toString() + "\")");
              } catch (IOException e) {
                app.addMessage(app.ERROR_MSG, "GDApp", e.getMessage());
                app.executeAppCommand("errorDialog(\"Could not start zero conf daemon!\")");
              }
            }
          } else {
            app.executeAppCommand("errorDialog(\"Could not get multicast lock!\")");
          }
        } else {
          app.executeAppCommand("errorDialog(\"No WiFi manager available?\")");
        }
      }

      // Use ARP cache lookups to discover devices if configured
      if (Integer.valueOf(app.coreObject.configStoreGetStringValue("Cockpit/App/Dashboard", "useAddressCacheLookup"))!=0) {
        quitLookupARPCacheThread = false;
        app.addMessage(app.DEBUG_MSG, "GDApp", "starting ARP cache lookup thread");
        lookupARPCacheThread = new Thread(new Runnable() {

          @Override
          public void run() {

            int sleepTime = Integer.valueOf(app.coreObject.configStoreGetStringValue("Cockpit/App/Dashboard", "addressCacheSleepTime"))*1000;
            int port = Integer.valueOf(app.coreObject.configStoreGetStringValue("Cockpit/App/Dashboard", "port"));
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
                      app.coreObject.executeCoreCommand(String.format("addDashboardDevice(%s,%d)",ip,port));
                    }
                    line = br.readLine();
                  }
                } finally {
                  br.close();
                }
              }
              catch (IOException e) {
                app.addMessage(app.ERROR_MSG, "GDApp", e.toString());
                app.executeAppCommand("errorDialog(\"Could not lookup address cache!\")");
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

    // Start the network thread
    startNetworkService();

    // Start the vibrate thread
    if ((vibrateThread!=null)&&(vibrateThread.isAlive())) {
      quitVibrateThread();
    }
    quitVibrateThread=false;
    vibrateThread = new Thread(new Runnable() {
      public void run() {
        while (!quitVibrateThread) {
          try {
            lock.lock();
            if (currentVibrateCount==expectedAlertCount)
              triggerAlert.await();
            lock.unlock();
            if (quitVibrateThread)
              return;
            do {

              // Focus the screen of all registered cockpit apps
              for (CockpitAppInterface app : apps) {
                app.inform();
                app.focus();
              }
                            
              // Wait a little bit before vibrating
              Thread.sleep(waitTimeBeforeAlert);
              if (quitVibrateThread)
                return;
              
              // Skip vibrate if we are not off route anymore 
              // and this is is not the first vibrate
              if (((!currentOffRoute)||(!currentTurnDistance.equals("-")))&&(fastVibrateCount>1))
                break;
              
              // Alert the user of all registered cockpit apps
              //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "vibrateThread: currentOffRoute=" + Boolean.toString(currentOffRoute));
              for (CockpitAppInterface app : apps) {
                if (!currentTurnDistance.equals("-"))
                  app.alert(AlertType.newTurn,fastVibrateCount>1);
                else
                  app.alert(AlertType.offRoute,fastVibrateCount>1);
              }
              
              // Repeat if off route 
              // Vibrate fast at the beginning, slow afterwards
              // Quit if a new vibrate is requested or we are on route again
              if ((currentOffRoute)&&(currentTurnDistance.equals("-"))) {
                //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "repeating alert");
                int offRouteVibratePeriod;
                if (fastVibrateCount>offRouteAlertFastCount) {
                  offRouteVibratePeriod=offRouteAlertSlowPeriod;
                } else {
                  offRouteVibratePeriod=offRouteAlertFastPeriod;
                  fastVibrateCount++;
                }
                for (int i=0;i<offRouteVibratePeriod/1000;i++) {
                  Thread.sleep(1000);
                  if ((!currentOffRoute)||(!currentTurnDistance.equals("-"))||(currentVibrateCount<expectedAlertCount-1))
                    break;
                }
              }
              if (quitVibrateThread)
                return;
            }
            while ((currentOffRoute)&&(currentTurnDistance.equals("-"))&&(currentVibrateCount==expectedAlertCount-1));
            currentVibrateCount++;
            //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", String.format("currentVibrateCount=%d expectedVibrateCount=%d",currentVibrateCount,expectedAlertCount));
            fastVibrateCount=1;
          }
          catch(InterruptedException e) {
            ; // repeat
          }
        }
      }
    });   
    vibrateThread.start();
    
    // Inform metawatch about this app
    for (CockpitAppInterface app : apps) {
      app.start();
    }
    lastUpdate = 0;
    update(null,true);
    lastUpdate = 0;
  }

  /** Stops the vibrate thread */
  void quitVibrateThread() {
    boolean repeat=true;
    while(repeat) {
      repeat=false;
      try {
        lock.lock();
        quitVibrateThread=true;
        triggerAlert.signal();
        lock.unlock();
        vibrateThread.interrupt();
        vibrateThread.join();
      }
      catch(InterruptedException e) {
        repeat=true;
      }
    }
    vibrateThread=null;
  }
  
  /** Updates the cockpit apps with new infos */
  public void update(String infosAsSSV, boolean forceUpdate) {
    
    // Use the last infos if no infos given
    if (infosAsSSV == null) {
      infosAsSSV = lastInfosAsSSV;
    }
    lastInfosAsSSV = infosAsSSV;

    // Obtain the dashboard infos
    CockpitInfos cockpitInfos = new CockpitInfos();
    if (infosAsSSV.equals(""))
      return;
    String[] infosAsArray = infosAsSSV.split(";");
    cockpitInfos.locationBearing = infosAsArray[0];
    cockpitInfos.locationSpeed = infosAsArray[1];
    cockpitInfos.targetBearing = infosAsArray[2];
    cockpitInfos.targetDistance = infosAsArray[3];
    cockpitInfos.targetDuration = infosAsArray[4];
    cockpitInfos.turnAngle = infosAsArray[5];
    cockpitInfos.turnDistance = infosAsArray[6];
    cockpitInfos.offRoute = infosAsArray[7].equals("off route");
    cockpitInfos.routeDistance = infosAsArray[8];
    currentTurnDistance = cockpitInfos.turnDistance;
    currentOffRoute = cockpitInfos.offRoute;
    //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "update: currentOffRoute=" + Boolean.toString(currentOffRoute));
    
    // If the turn has appeared or disappears, force an update
    if ((currentTurnDistance.equals("-"))&&(!lastTurnDistance.equals("-")))
      forceUpdate=true;
    if ((!currentTurnDistance.equals("-"))&&(lastTurnDistance.equals("-")))
      forceUpdate=true;
    if (lastOffRoute!=currentOffRoute) 
      forceUpdate=true;
    
    // Check if the info is updated too fast
    long minUpdatePeriod;
    if (currentTurnDistance.equals("-"))
      minUpdatePeriod = minUpdatePeriodNormal;
    else
      minUpdatePeriod = minUpdatePeriodTurn;
    long t = Calendar.getInstance().getTimeInMillis();
    long diffToLastUpdate = t - lastUpdate;
    if ((!forceUpdate)&&(diffToLastUpdate < minUpdatePeriod)) {
      //GDApplication.addMessage(GDApplication.DEBUG_MSG, "CockpitEngine", "Skipped update because last update was too recent");
      return;
    }
        
    // Redraw all registered apps
    for (CockpitAppInterface app : apps) {
      app.update(cockpitInfos);
    }
    
    // Alert if the turn appears the first time or if off route
    boolean alert=false;
    if (((!lastOffRoute)&&currentOffRoute)) {
      alert=true;
    }
    if ((!currentTurnDistance.equals("-"))&&(lastTurnDistance.equals("-"))) {
      alert=true;
    }
    if (alert) {
            
      // Inform the alert thread to do the work
      lock.lock();
      expectedAlertCount++;
      triggerAlert.signal();
      lock.unlock();
      
    } else {

      // Send the bitmap directly
      for (CockpitAppInterface app : apps) {
        app.inform();
      }
      
    }

    // Remember when was updated
    lastUpdate = Calendar.getInstance().getTimeInMillis();
    lastTurnDistance=currentTurnDistance;
    lastOffRoute=currentOffRoute;
  }
  
  /** Stops the cockpit apps */
  public void stop() {

    // Stop the network service
    stopNetworkService();

    // Stop service discovery
    if (dashboardServiceListener!=null) {
      app.addMessage(app.DEBUG_MSG, "GDApp", "stopping jmDNS");
      jmDNS.removeServiceListener(app.dashboardNetworkServiceType, dashboardServiceListener);
      try {
        jmDNS.close();
      }
      catch (IOException e) {
        app.addMessage(app.DEBUG_MSG, "GDApp", e.getMessage());
      }
      jmDNS=null;
      dashboardServiceListener=null;
    }
    if (multicastLock!=null) {
      app.addMessage(app.DEBUG_MSG, "GDApp", "releasing multicast lock");
      multicastLock.release();
      multicastLock=null;
    }
    if (lookupARPCacheThread!=null) {
      app.addMessage(app.DEBUG_MSG, "GDApp", "stopping ARP cache lookup thread");
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

    // Stop the vibration thread
    quitVibrateThread();

    // Stop all apps
    for (CockpitAppInterface app : apps) {
      app.stop();
    }

  }
  
  /** Checks if at least one cockpit app is active */
  public boolean isActive() {
    if ((apps.size()>0)||(Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit/App/Dashboard", "active"))>0))
      return true;
    else
      return false;
  }

  /** Wakeups the audio device if required */
  public synchronized void audioWakeup() {
    long t = System.currentTimeMillis();
    if (audioIsAsleep()) {
      audioWakeupLastTrigger = t + audioWakeupDelay;
      try {
        final AssetFileDescriptor afd = app.getContext().getAssets().openFd("Sound/silence.mp3");
        final MediaPlayer player = new MediaPlayer();
        player.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
        player.prepare();
        player.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
          public void onCompletion(MediaPlayer mp) {
            try {
              afd.close();
              player.release();
            } catch (IOException e) {
              GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", e.getMessage());
            }
          }
        });
        player.start();
      }
      catch (IOException e) {
        GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", e.getMessage());
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
    //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", String.format("lastTrigger=%d",audioWakeupLastTrigger));
  }

  /** Indicates if audio device is asleep */
  public boolean audioIsAsleep() {
    long t = System.currentTimeMillis();
    return (t>audioWakeupLastTrigger+audioWakeupDuration);
  }
}
