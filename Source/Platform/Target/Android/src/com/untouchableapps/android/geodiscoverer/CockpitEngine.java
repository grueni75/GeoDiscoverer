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

import java.util.Calendar;
import java.util.LinkedList;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import com.untouchableapps.android.geodiscoverer.CockpitAppInterface.AlertType;

import android.content.Context;
import android.hardware.Camera.PreviewCallback;

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
  
  // List of registered apps
  LinkedList<CockpitAppInterface> apps = new LinkedList<CockpitAppInterface>();

  /** Constructor */
  public CockpitEngine(Context context) {
    super();
    
    // Remember context
    this.context = context;
    
    // Init parameters
    minUpdatePeriodNormal = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit", "minUpdatePeriodNormal"));
    minUpdatePeriodTurn = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit", "minUpdatePeriodTurn"));
    waitTimeBeforeAlert = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit", "waitTimeBeforeAlert"));
    offRouteAlertFastPeriod = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit", "offRouteAlertFastPeriod"));
    offRouteAlertFastCount = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit", "offRouteAlertFastCount"));
    offRouteAlertSlowPeriod = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit", "offRouteAlertSlowPeriod"));

    // Add all activated apps
    if (Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit/App/MetaWatch", "active"))>0) {
      apps.add(new CockpitAppMetaWatch(context));
    }
    if (Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit/App/Voice", "active"))>0) {
      apps.add(new CockpitAppVoice(context));
    }
    
    // Start the engine
    start();
  }
  
  /** Starts the cockpit apps */
  public void start() {

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

              // Skip vibrate if we are not off route anymore and this is 
              // is not the first vibrate
              if ((!currentOffRoute)&&(fastVibrateCount>1))
                break;
              
              // Alert the user of all registered cockpit apps
              GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "vibrateThread: currentOffRoute=" + Boolean.toString(currentOffRoute));
              for (CockpitAppInterface app : apps) {
                if (currentOffRoute)
                  app.alert(AlertType.offRoute);
                else
                  app.alert(AlertType.newTurn);
              }
              
              // Repeat if off route 
              // Vibrate fast at the beginning, slow afterwards
              // Quit if a new vibrate is requested or we are on route again
              if (currentOffRoute) {
                int offRouteVibratePeriod;
                if (fastVibrateCount>offRouteAlertFastCount) {
                  offRouteVibratePeriod=offRouteAlertSlowPeriod;
                } else {
                  offRouteVibratePeriod=offRouteAlertFastPeriod;
                  fastVibrateCount++;
                }
                for (int i=0;i<offRouteVibratePeriod/1000;i++) {
                  Thread.sleep(1000);
                  if ((!currentOffRoute)||(currentVibrateCount<expectedAlertCount-1))
                    break;
                }
              }
              if (quitVibrateThread)
                return;
            }
            while ((currentOffRoute)&&(currentVibrateCount==expectedAlertCount-1));
            currentVibrateCount++;
            if (!currentOffRoute) 
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
    cockpitInfos.targetBearing = infosAsArray[1];
    cockpitInfos.targetDistance = infosAsArray[2];
    cockpitInfos.targetDuration = infosAsArray[3];
    cockpitInfos.turnAngle = infosAsArray[4];
    cockpitInfos.turnDistance = infosAsArray[5];
    cockpitInfos.offRoute = infosAsArray[6].equals("off route");
    currentTurnDistance = cockpitInfos.turnDistance;
    currentOffRoute = cockpitInfos.offRoute;
    GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "update: currentOffRoute=" + Boolean.toString(currentOffRoute));
    
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
            
      // Inform the vibrate thread to do the work
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
  }
  
  /** Checks if at least one cockpit app is active */
  public boolean isActive() {
    if (apps.size()>0)
      return true;
    else
      return false;
  }

}
