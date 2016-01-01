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

package com.untouchableapps.android.geodiscoverer;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Calendar;
import java.util.LinkedList;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import com.untouchableapps.android.geodiscoverer.CockpitAppInterface.AlertType;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.MediaPlayer;

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

  // GDCore
  GDCore core;

  // Context
  Context context;
  
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
            final AssetFileDescriptor afd = context.getAssets().openFd(soundFile);
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

  /** Constructor */
  public CockpitEngine(GDCore core, Context context) {
    super();
    
    // Remember context
    this.core = core;
    this.context = context;
    
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
      apps.add(new CockpitAppMetaWatch(context));
    }
    if (Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit/App/Voice", "active"))>0) {
      apps.add(new CockpitAppVoice(context,this));
    }
    if (Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit/App/Vibration", "active"))>0) {
      apps.add(new CockpitAppVibration(context));
    }
    
    // Start the engine
    start();
  }
  
  /** Starts the cockpit apps */
  public void start() {

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
    quitVibrateThread();
    for (CockpitAppInterface app : apps) {
      app.stop();
    }
    stopNetworkService();
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
        final AssetFileDescriptor afd = context.getAssets().openFd("Sound/silence.mp3");
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
