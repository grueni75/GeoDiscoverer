//============================================================================
// Name        : CockpitEngine.java
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

package com.untouchableapps.android.geodiscoverer.core.cockpit;

import android.content.Context;
import android.graphics.BitmapFactory;
import android.os.Message;
import android.util.Log;
import android.widget.Toast;

import com.untouchableapps.android.geodiscoverer.core.GDAppInterface;
import com.untouchableapps.android.geodiscoverer.core.GDCore;
import com.untouchableapps.android.geodiscoverer.core.cockpit.CockpitAppInterface.AlertType;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Calendar;
import java.util.LinkedList;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

public class CockpitEngine {
  
  // Minimum time that must pass between updates
  int minUpdatePeriodNormal;
  int minUpdatePeriodTurn;
  int offRouteAlertFastPeriod;
  int offRouteAlertFastCount;
  int offRouteAlertSlowPeriod;

  // Time in milliseconds to slee+p before alerting
  int waitTimeBeforeAlert;

  // Last time the navigation info was updated
  long lastUpdate;
  
  // Last infos used for updating
  String lastInfosAsSSV = "";

  // Current cockpit infos
  CockpitInfos cockpitInfos=null;
  
  // Distance to turn
  String currentTurnDistance="-";
  String lastTurnDistance="-";
  
  // Off route indication
  boolean currentOffRoute=false;
  boolean lastOffRoute=false;

  // References to the outside world
  public GDCore coreObject;
  protected Context context;

  // Thread that manages vibrations
  final Lock lock = new ReentrantLock();
  final Condition triggerAlert = lock.newCondition();
  int expectedAlertCount = 0;
  int currentVibrateCount = 0;
  int fastVibrateCount = 1;
  boolean quitVibrateThread = false;
  Thread vibrateThread = null;

  // Minimum speed required to trigger an alert
  float minSpeedToAlert;

  // Current speed in m/s
  float locationSpeed;

  // List of registered apps
  protected LinkedList<CockpitAppInterface> apps = new LinkedList<CockpitAppInterface>();

  /** Converts a distance string into meters */
  public float textDistanceToMeters(String text) {
    if (text.equals(""))
      return 0;
    float number = Float.valueOf(text.substring(0, text.indexOf(" ")));
    String unit = text.substring(text.indexOf(" ") + 1);
    if (unit.equals("m")) {
      return (float) number;
    }
    if (unit.equals("km")) {
      return (float) number*(float)1e3;
    }
    if (unit.equals("Mm")) {
      return (float) number*(float)1e6;
    }
    if (unit.equals("mi")) {
      return (float) number*(float)1609.34;
    }
    if (unit.equals("yd")) {
      return (float) number*(float)0.9144;
    }
    coreObject.executeAppCommand("fatalDialog(\"Distance unit not supported\")");
    return 0;
  }

  /** Converts a speed string into meters per second */
  public float textSpeedToMetersPerSecond(String text) {
    if (text.equals(""))
      return 0;
    float number = Float.valueOf(text.substring(0, text.indexOf(" ")));
    String unit = text.substring(text.indexOf(" ")+1);
    if (unit.equals("km/h")) {
      return (float) (number/3.6);
    }
    if (unit.equals("mph")) {
      return (float) (number*0.44704);
    }
    coreObject.executeAppCommand("fatalDialog(\"Speed unit not supported\")");
    return 0;
  }

  /** Constructor */
  public CockpitEngine(Context context, GDCore coreObject) {
    super();
    
    // Remember context
    this.context = context;
    this.coreObject = coreObject;

    // Init parameters
    minUpdatePeriodNormal = Integer.parseInt(coreObject.configStoreGetStringValue("Cockpit", "minUpdatePeriodNormal"));
    minUpdatePeriodTurn = Integer.parseInt(coreObject.configStoreGetStringValue("Cockpit", "minUpdatePeriodTurn"));
    waitTimeBeforeAlert = Integer.parseInt(coreObject.configStoreGetStringValue("Cockpit", "waitTimeBeforeAlert"));
    offRouteAlertFastPeriod = Integer.parseInt(coreObject.configStoreGetStringValue("Cockpit", "offRouteAlertFastPeriod"));
    offRouteAlertFastCount = Integer.parseInt(coreObject.configStoreGetStringValue("Cockpit", "offRouteAlertFastCount"));
    offRouteAlertSlowPeriod = Integer.parseInt(coreObject.configStoreGetStringValue("Cockpit", "offRouteAlertSlowPeriod"));
    minSpeedToAlert = Float.parseFloat(coreObject.configStoreGetStringValue("Cockpit", "minSpeedToAlert"));

    // Add all activated apps
    if (Integer.parseInt(coreObject.configStoreGetStringValue("Cockpit/App/Vibration", "active"))>0) {
      apps.add(new CockpitAppVibration(context));
    }
  }
  
  /** Starts the cockpit apps */
  public void start() {

    // Start the vibrate thread
    startVibrateThread();

    // Inform metawatch about this app
    for (CockpitAppInterface app : apps) {
      app.start();
    }
    lastUpdate = 0;
    update(null,true);
    lastUpdate = 0;
  }

  /** Starts the vibrate thread */
  public void startVibrateThread() {

    // Start the vibrate thread
    if ((vibrateThread!=null)&&(vibrateThread.isAlive())) {
      quitVibrateThread();
    }
    quitVibrateThread=false;
    vibrateThread = new Thread(new Runnable() {
      public void run() {
        coreObject.setThreadPriority(2);
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

              // Skip the vibrate if the current speed is below the threshold
              //coreObject.appIf.addAppMessage(GDAppInterface.DEBUG_MSG, "GDApp", String.format("locationSpeed=%f",locationSpeed));
              if (locationSpeed<minSpeedToAlert) {
                coreObject.appIf.addAppMessage(GDAppInterface.DEBUG_MSG, "GDApp", String.format("Skipping alert because user is not moving (locationSpeed=%f)",locationSpeed));
                break;
              }

              // Skip vibrate if we are not off route anymore
              // and this is is not the first vibrate
              if (((!currentOffRoute)||(!currentTurnDistance.equals("-")))&&(fastVibrateCount>1)) {
                coreObject.appIf.addAppMessage(GDAppInterface.DEBUG_MSG, "GDApp", String.format("Skipping alert (currentOffRoute=%s, currentTurnDistance=%s, fastVibrateCount=%d)",Boolean.toString(currentOffRoute),currentTurnDistance,fastVibrateCount));
                break;
              }

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
                coreObject.appIf.addAppMessage(GDAppInterface.DEBUG_MSG, "GDApp", "repeating alert");
                int offRouteVibratePeriod;
                if (fastVibrateCount>offRouteAlertFastCount) {
                  offRouteVibratePeriod=offRouteAlertSlowPeriod;
                } else {
                  offRouteVibratePeriod=offRouteAlertFastPeriod;
                  fastVibrateCount++;
                }
                for (int i=0;i<offRouteVibratePeriod/1000;i++) {
                  Thread.sleep(1000);
                  if (locationSpeed<minSpeedToAlert) {
                    coreObject.appIf.addAppMessage(GDAppInterface.DEBUG_MSG, "GDApp", String.format("Skipping repeat because user is not moving (locationSpeed=%f)",locationSpeed));
                    break;
                  }
                  if ((!currentOffRoute)||(!currentTurnDistance.equals("-"))||(currentVibrateCount<expectedAlertCount-1)) {
                    coreObject.appIf.addAppMessage(GDAppInterface.DEBUG_MSG, "GDApp", String.format("Skipping repeat (currentOffRoute=%s, currentTurnDistance=%s, currentVibrateCount=%d, expectedAlertCount=%d)",Boolean.toString(currentOffRoute),currentTurnDistance,currentVibrateCount,expectedAlertCount));
                    break;
                  }
                }
              }
              if (quitVibrateThread)
                return;
            }
            while ((currentOffRoute)&&(currentTurnDistance.equals("-"))&&(currentVibrateCount==expectedAlertCount-1));
            currentVibrateCount++;
            coreObject.appIf.addAppMessage(GDAppInterface.DEBUG_MSG, "GDApp", String.format("currentVibrateCount=%d expectedVibrateCount=%d",currentVibrateCount,expectedAlertCount));
            fastVibrateCount=1;
          }
          catch(InterruptedException e) {
            ; // repeat
          }
        }
      }
    });
    vibrateThread.start();
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

  /** Creates cockpit infos from a string */
  static public CockpitInfos createCockpitInfo(String infos) {
    CockpitInfos cockpitInfos = new CockpitInfos();
    String[] infosAsArray = infos.split(";");
    cockpitInfos.locationBearing = infosAsArray[0];
    cockpitInfos.locationSpeed = infosAsArray[1];
    cockpitInfos.trackLength = infosAsArray[2];
    cockpitInfos.targetBearing = infosAsArray[3];
    cockpitInfos.targetDistance = infosAsArray[4];
    cockpitInfos.targetDuration = infosAsArray[5];
    cockpitInfos.turnAngle = infosAsArray[6];
    cockpitInfos.turnDistance = infosAsArray[7];
    cockpitInfos.offRoute = infosAsArray[8].equals("off route");
    cockpitInfos.routeDistance = infosAsArray[9];
    cockpitInfos.nearestNavigationPointBearing = infosAsArray[10];
    cockpitInfos.nearestNavigationPointDistance = infosAsArray[11];
    return cockpitInfos;
  }

  /** Updates the cockpit apps with new infos */
  public void update(String infosAsSSV, boolean forceUpdate) {
    
    // Use the last infos if no infos given
    if (infosAsSSV == null) {
      infosAsSSV = lastInfosAsSSV;
    }
    lastInfosAsSSV = infosAsSSV;

    // Obtain the dashboard infos
    if (infosAsSSV.equals(""))
      return;
    cockpitInfos = createCockpitInfo(infosAsSSV);
    /*coreObject.appIf.addAppMessage(GDAppInterface.DEBUG_MSG,"GDApp","locationBearing="+cockpitInfos.locationBearing);
    coreObject.appIf.addAppMessage(GDAppInterface.DEBUG_MSG,"GDApp","locationSpeed="+cockpitInfos.locationSpeed);
    coreObject.appIf.addAppMessage(GDAppInterface.DEBUG_MSG,"GDApp","trackLength="+cockpitInfos.trackLength);
    coreObject.appIf.addAppMessage(GDAppInterface.DEBUG_MSG,"GDApp","targetBearing="+cockpitInfos.targetBearing);
    coreObject.appIf.addAppMessage(GDAppInterface.DEBUG_MSG,"GDApp","targetDistance="+cockpitInfos.targetDistance);
    coreObject.appIf.addAppMessage(GDAppInterface.DEBUG_MSG,"GDApp","targetDuration="+cockpitInfos.targetDuration);
    coreObject.appIf.addAppMessage(GDAppInterface.DEBUG_MSG,"GDApp","turnAngle="+cockpitInfos.turnAngle);
    coreObject.appIf.addAppMessage(GDAppInterface.DEBUG_MSG,"GDApp","turnDistance="+cockpitInfos.turnDistance);
    coreObject.appIf.addAppMessage(GDAppInterface.DEBUG_MSG,"GDApp","offRoute="+cockpitInfos.offRoute);
    coreObject.appIf.addAppMessage(GDAppInterface.DEBUG_MSG,"GDApp","routeDistance="+cockpitInfos.routeDistance);
    coreObject.appIf.addAppMessage(GDAppInterface.DEBUG_MSG,"GDApp","nearestNavigationPointBearing="+cockpitInfos.nearestNavigationPointBearing);
    coreObject.appIf.addAppMessage(GDAppInterface.DEBUG_MSG,"GDApp","nearestNavigationPointDistance="+cockpitInfos.nearestNavigationPointDistance);*/
    currentTurnDistance = cockpitInfos.turnDistance;
    currentOffRoute = cockpitInfos.offRoute;
    //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "update: currentOffRoute=" + Boolean.toString(currentOffRoute));

    // Remember the current speed
    if (!cockpitInfos.locationSpeed.equals("-")) {
      locationSpeed = textSpeedToMetersPerSecond(cockpitInfos.locationSpeed);
    } else {
      locationSpeed = -1;
    }
    //coreObject.appIf.addAppMessage(GDAppInterface.DEBUG_MSG,"GDApp","locationSpeed updated to " + locationSpeed);

    // If the turn has appeared or disappears or we are off route, force an update
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

    // Stop the vibration thread
    quitVibrateThread();

    // Stop all apps
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
