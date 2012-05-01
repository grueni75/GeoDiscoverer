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
import java.io.IOException;
import java.util.LinkedList;
import java.util.Vector;

import android.app.AlertDialog;
import android.app.Application;
import android.app.ProgressDialog;
import android.content.*;
import android.content.res.Configuration;
import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.location.LocationManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.preference.PreferenceScreen;

import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Toast;

public class ViewMap extends GDActivity {
  
  // Variables for monitoring the state of the external storage
  BroadcastReceiver externalStorageReceiver;
  boolean externalStorageAvailable;
  boolean externalStorageWriteable;
  ProgressDialog externalStorageWaitDialog=null;
  
  // Request codes for calling other activities
  static final int SHOW_PREFERENCE_REQUEST = 0;

  // GUI components
  ProgressDialog progressDialog;
  int progressMax;
  int progressCurrent;
  String progressMessage;
  GDMapSurfaceView mapSurfaceView = null;
  
  // Managers
  LocationManager locationManager;
  SensorManager sensorManager;
  PowerManager powerManager;
  
  // Wake lock
  WakeLock wakeLock = null;
  
  // Flags
  boolean locationWatchStarted = false;
  boolean compassWatchStarted = false;

  /** Updates the progress dialog */
  protected void updateProgressDialog(String message, int progress) {
    progressCurrent=progress;
    progressMessage=message;
    if (progressDialog==null) {
      progressDialog = new ProgressDialog(this);
      if (progressMax==0)
        progressDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
      else
        progressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
      progressDialog.setMessage(progressMessage);    
      progressDialog.setMax(progressMax);
      progressDialog.setCancelable(false);
      progressDialog.show();
    }
    //Log.d("GDApp","updating progress dialog (" + message + "," + String.valueOf(progress) + ")");
    progressDialog.setMessage(progressMessage);    
    progressDialog.setProgress(progressCurrent);
  }

  // Communication with the native core
  public static final int EXECUTE_COMMAND = 0;
  
  public Handler coreMessageHandler = new Handler() {

      /** Called when the core has a message */
      @Override
      public void handleMessage(Message msg) {  
          Bundle b=msg.getData();
          switch(msg.what) {
            case EXECUTE_COMMAND:
              
              // Extract the command
              String command=b.getString("command");
              int args_start=command.indexOf("(");
              int args_end=command.lastIndexOf(")");
              String commandFunction=command.substring(0, args_start);
              String t=command.substring(args_start+1,args_end);
              Vector<String> commandArgs = new Vector<String>();
              boolean stringStarted=false;
              int startPos=0;
              for(int i=0;i<t.length();i++) {
                if (t.substring(i,i+1).equals("\"")) {
                  if (stringStarted)
                    stringStarted=false;
                  else
                    stringStarted=true;                    
                }
                if (!stringStarted) {
                  if ((t.substring(i,i+1).equals(","))||(i==t.length()-1)) {
                    String arg;
                    if (i==t.length()-1) 
                      arg=t.substring(startPos,i+1);
                    else
                      arg=t.substring(startPos,i);
                    if (arg.startsWith("\"")) {
                      arg=arg.substring(1);
                    }
                    if (arg.endsWith("\"")) {
                      arg=arg.substring(0,arg.length()-1);
                    }
                    commandArgs.add(arg);
                    startPos=i+1;
                  }
                }
              }
              
              // Execute command
              boolean commandExecuted=false;
              if (commandFunction.equals("fatalDialog")) {
                fatalDialog(commandArgs.get(0));
                commandExecuted=true;
              }
              if (commandFunction.equals("errorDialog")) {
                errorDialog(commandArgs.get(0));
                commandExecuted=true;
              }
              if (commandFunction.equals("warningDialog")) {
                warningDialog(commandArgs.get(0));
                commandExecuted=true;
              }
              if (commandFunction.equals("infoDialog")) {
                infoDialog(commandArgs.get(0));
                commandExecuted=true;
              }
              if (commandFunction.equals("createProgressDialog")) {
                progressMax=Integer.parseInt(commandArgs.get(1));
                updateProgressDialog(commandArgs.get(0),0);
                commandExecuted=true;
              }
              if (commandFunction.equals("updateProgressDialog")) {
                updateProgressDialog(commandArgs.get(0),Integer.parseInt(commandArgs.get(1)));
                commandExecuted=true;
              }
              if (commandFunction.equals("closeProgressDialog")) {
                if (progressDialog!=null) {
                  progressDialog.dismiss();
                  progressDialog=null;
                }
                commandExecuted=true;
              }
              if (commandFunction.equals("getLastKnownLocation")) {
                if (mapSurfaceView!=null) {
                  mapSurfaceView.coreObject.onLocationChanged(locationManager.getLastKnownLocation(LocationManager.NETWORK_PROVIDER));
                  mapSurfaceView.coreObject.onLocationChanged(locationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER));
                }
                commandExecuted=true;
              }
              if (commandFunction.equals("updateWakeLock")) {
                updateWakeLock();
                commandExecuted=true;
              }
              if (!commandExecuted) {
                Log.e("GDApp","unknown command " + command + "received");
              }
              break;
          }
      }
  };  

  /** Sets the screen time out */
  void updateWakeLock() {
    if (mapSurfaceView!=null) {
      String state=mapSurfaceView.coreObject.executeCoreCommand("getWakeLock()");
      if (state.equals("true")) {
        Log.d("GDApp","wake lock enabled");
        if (!wakeLock.isHeld())
          wakeLock.acquire();
      } else {
        Log.d("GDApp","wake lock disabled");
        if (wakeLock.isHeld())
          wakeLock.release();
      }
    }
  }
  
  /** Called when the external storage state changes */
  synchronized void handleExternalStorageState(boolean externalStorageAvailable, boolean externalStorageWritable) {

    // If the external storage is not available, display a busy dialog
    if ((!externalStorageAvailable)||(!externalStorageWritable)) {
      if (externalStorageWaitDialog==null)
        externalStorageWaitDialog=ProgressDialog.show(this, getTitle(), getString(R.string.busy_waiting_for_external_storage), true);      
    }
    
    // If the external storage is available, start the native core
    if ((externalStorageAvailable)&&(externalStorageWritable)) {
      if (externalStorageWaitDialog!=null)
        externalStorageWaitDialog.dismiss();
      
      // Is the core not running yet?
      if (mapSurfaceView==null) {
                
        // Check if the core object is already initialized
        GDApplication app=(GDApplication)getApplication();
        if (app.coreObject==null) {
        
          // Create the home directory
          //File homeDir=Environment.getExternalStorageDirectory(getString(R.string.home_directory));
          File externalStorageDirectory=Environment.getExternalStorageDirectory(); 
          String homeDirPath = externalStorageDirectory.getAbsolutePath() + "/GeoDiscoverer";
          File homeDir=new File(homeDirPath);
          if (homeDir == null) {
            fatalDialog(getString(R.string.fatal_home_directory_creation));
            return;
          }
          if (!homeDir.exists()) {
            try {
              homeDir.mkdir();
            }
            catch (Exception e){
              fatalDialog(getString(R.string.fatal_home_directory_creation));
              return;
            }
          }
          homeDirPath=homeDir.getAbsolutePath();
          
          // Check if the core object is already initialized
          app.coreObject=new GDCore(ViewMap.this,homeDirPath);
        }
        
        // Assign the opengl surface   
        Log.d("GDApp", "creating new mapSurfaceView");
        mapSurfaceView = new GDMapSurfaceView(ViewMap.this,app.coreObject);
        setContentView(mapSurfaceView);
        
        // Start listening for location and compass updates 
        startWatchingLocation();
        startWatchingCompass();
      }
      
    }
    Log.w("GDApp","handling the external storage state not completly implemented!");
  }
  
  /** Sets the current state of the external media */
  void updateExternalStorageState() {
    String state = Environment.getExternalStorageState();
    if (Environment.MEDIA_MOUNTED.equals(state)) {
      externalStorageAvailable = externalStorageWriteable = true;
    } else if (Environment.MEDIA_MOUNTED_READ_ONLY.equals(state)) {
      externalStorageAvailable = true;
      externalStorageWriteable = false;
    } else {
      externalStorageAvailable = externalStorageWriteable = false;
    }
    handleExternalStorageState(externalStorageAvailable,externalStorageWriteable);
  }

  /** Initiates the watching of the external storage */
  void startWatchingExternalStorage() {
    externalStorageReceiver = new BroadcastReceiver() {
      @Override
      public void onReceive(Context context, Intent intent) {
        Log.i("GDApp", "Storage: " + intent.getData());
        updateExternalStorageState();
      }
    };
    IntentFilter filter = new IntentFilter();
    filter.addAction(Intent.ACTION_MEDIA_MOUNTED);
    filter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
    registerReceiver(externalStorageReceiver, filter);
    updateExternalStorageState();
  }

  /** Stops the watching of the external storage */
  void stopWatchingExternalStorage() {
    unregisterReceiver(externalStorageReceiver);
    externalStorageReceiver=null;
  }
  
  /** Start listening for location fixes */
  synchronized void startWatchingLocation() {
    if ((mapSurfaceView!=null)&&(!locationWatchStarted)) {      
      locationManager.requestLocationUpdates(LocationManager.NETWORK_PROVIDER, 0, 0, mapSurfaceView.coreObject);
      locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 0, 0, mapSurfaceView.coreObject);
      locationWatchStarted=true;
      Log.d("GDApp","location updates from gps and network started.");
    }
  }
  
  /** Stop listening for location fixes */
  synchronized void stopWatchingLocation() {
    if (locationWatchStarted) {
      locationManager.removeUpdates(mapSurfaceView.coreObject);
      locationWatchStarted=false;
      Log.d("GDApp","location updates from gps and network stopped.");
    }
  }

  /** Start listening for compass bearing */
  synchronized void startWatchingCompass() {
    if ((mapSurfaceView!=null)&&(!compassWatchStarted)) {
      sensorManager.registerListener(mapSurfaceView.coreObject,sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD),SensorManager.SENSOR_DELAY_NORMAL);
      sensorManager.registerListener(mapSurfaceView.coreObject,sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER),SensorManager.SENSOR_DELAY_NORMAL);
      compassWatchStarted=true;
      Log.d("GDApp","compass bearing updates started.");
    }
  }
  
  /** Stop listening for location fixes */
  synchronized void stopWatchingCompass() {
    if (compassWatchStarted) {
      sensorManager.unregisterListener(mapSurfaceView.coreObject);
      compassWatchStarted=false;
      Log.d("GDApp","compass bearing updates stopped.");
    }
  }

  /** Called when the activity is first created. */
  @Override
  public void onCreate(Bundle savedInstanceState) {
    
    super.onCreate(savedInstanceState);

    // Get important handles
    locationManager = (LocationManager) this.getSystemService(Context.LOCATION_SERVICE);
    sensorManager = (SensorManager) this.getSystemService(Context.SENSOR_SERVICE);
    powerManager = (PowerManager) this.getSystemService(Context.POWER_SERVICE);
    
    // Setup the GUI
    requestWindowFeature(Window.FEATURE_NO_TITLE);    

    // Get a wake lock
    wakeLock=powerManager.newWakeLock(PowerManager.FULL_WAKE_LOCK, "DoNotDimScreen");
    if (wakeLock==null) {
      fatalDialog("Can not obtain wake lock!");
    }

  }
  
  /** Called when the app suspends */
  @Override
  public void onPause() {
    super.onPause();
    Log.d("GDApp","onPause called by " + Thread.currentThread().getName());
    stopWatchingCompass();
    stopWatchingLocation();
    stopWatchingExternalStorage();
    mapSurfaceView.coreObject.executeCoreCommand("maintenance()");
    mapSurfaceView.coreObject.parentActivity=null;
  }
  
  /** Called when the app resumes */
  @Override
  public void onResume() {
    super.onResume();    
    Log.d("GDApp","onResume called by " + Thread.currentThread().getName());
    startWatchingExternalStorage();
    startWatchingLocation();
    startWatchingCompass();
    updateWakeLock();
    if (mapSurfaceView!=null) {
      mapSurfaceView.coreObject.parentActivity=this;
      mapSurfaceView.coreObject.forceRedraw=true;
    }
  }

  /** Called when the activity is destroyed */
  @Override
  public void onDestroy() {    
    super.onDestroy();
    Log.d("GDApp","onDestroy called by " + Thread.currentThread().getName());
    if (wakeLock.isHeld())
      wakeLock.release();
  }

  /** Called when a configuration change (e.g., caused by a screen rotation) has occured */
  public void onConfigurationChanged(Configuration newConfig) {
    
    super.onConfigurationChanged(newConfig);
    
    /* Recreate the progress dialog
    if (progressDialog!=null) {
      progressDialog.dismiss();
      progressDialog=null;
      updateProgressDialog(progressMessage, progressCurrent);
    }*/
          
  }
  
  /** Called when a option menu shall be created */  
  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
      MenuInflater inflater = getMenuInflater();
      inflater.inflate(R.menu.view_map, menu);
      return true;
  }

  /** Called when an option menu item has been selected */
  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
      switch (item.getItemId()) {
          case R.id.preferences:
              if (mapSurfaceView!=null) {
                Intent myIntent = new Intent(getApplicationContext(), Preferences.class);
                startActivityForResult(myIntent, SHOW_PREFERENCE_REQUEST);
              }
              return true;
          case R.id.reset:
            if (mapSurfaceView!=null) {
              AlertDialog.Builder builder = new AlertDialog.Builder(this);
              builder.setTitle(getTitle());
              builder.setMessage(R.string.reset_question);
              builder.setCancelable(true);
              builder.setPositiveButton(R.string.yes,
                  new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                      RestartCoreObjectTask task = new RestartCoreObjectTask();
                      task.resetConfig=true;
                      task.execute();
                    }
                  });
              builder.setNegativeButton(R.string.no, null);
              AlertDialog alert = builder.create();
              alert.show();
            }
            return true;
          default:
              return super.onOptionsItemSelected(item);
      }
  }  

  /** Restarts the core object */
  private class RestartCoreObjectTask extends AsyncTask<Void, Void, Void> {

    ProgressDialog progressDialog;
    public boolean resetConfig = false;

    protected void onPreExecute() {
      progressDialog = new ProgressDialog(ViewMap.this);
      progressDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
      progressDialog.setMessage(getString(R.string.restarting_core_object));
      progressDialog.setCancelable(false);
      progressDialog.show();
    }

    protected Void doInBackground(Void... params) {
      mapSurfaceView.coreObject.restart(resetConfig);      
      return null;
    }

    protected void onPostExecute(Void result) {
      progressDialog.dismiss();
    }
  }
  
  
  /** Called when a called activity finishes */
  protected void onActivityResult(int requestCode, int resultCode, Intent data) {
      
    // Did the activity changes prefs?
    if (requestCode == SHOW_PREFERENCE_REQUEST) {
      if (resultCode==1) {
        
        // Restart the core object
        if (mapSurfaceView!=null) {
          new RestartCoreObjectTask().execute();
        }
        
      }
    }
  }

}