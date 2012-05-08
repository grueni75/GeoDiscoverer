//============================================================================
// Name        : ViewMap.java
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
import android.graphics.Typeface;
import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.location.LocationManager;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.preference.PreferenceScreen;

import android.view.Gravity;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.AnimationSet;
import android.view.animation.LayoutAnimationController;
import android.view.animation.TranslateAnimation;
import android.widget.FrameLayout;
import android.widget.FrameLayout.LayoutParams;
import android.widget.TextView;
import android.widget.Toast;

public class ViewMap extends GDActivity {
    
  // Request codes for calling other activities
  static final int SHOW_PREFERENCE_REQUEST = 0;

  // GUI components
  ProgressDialog progressDialog;
  int progressMax;
  int progressCurrent;
  String progressMessage;
  GDMapSurfaceView mapSurfaceView = null;
  TextView messageView = null;
  FrameLayout frameLayout = null;

  /** Reference to the core object */
  GDCore coreObject = null;

  // Managers
  LocationManager locationManager;
  SensorManager sensorManager;
  PowerManager powerManager;
  
  // Wake lock
  WakeLock wakeLock = null;
  
  // Flags
  boolean compassWatchStarted = false;
  boolean exitRequested = false;
  
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
      progressDialog.setCancelable(true);
      progressDialog.show();
    }
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
                if (coreObject!=null) {
                  coreObject.onLocationChanged(locationManager.getLastKnownLocation(LocationManager.NETWORK_PROVIDER));
                  coreObject.onLocationChanged(locationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER));
                }
                commandExecuted=true;
              }
              if (commandFunction.equals("updateWakeLock")) {
                updateWakeLock();
                commandExecuted=true;
              }
              if (commandFunction.equals("updateMessages")) {
                if (messageView!=null) {
                  if (messageView.getVisibility()==TextView.VISIBLE) {
                    messageView.setText(GDApplication.messages);
                    messageView.invalidate();
                  }
                }
                commandExecuted=true;
              }
              if (commandFunction.equals("setMessageVisibility")) {
                if (messageView!=null) {
                  if (commandArgs.get(0).equals("1")) {
                    messageView.setVisibility(TextView.VISIBLE);
                  } else {
                    messageView.setVisibility(TextView.INVISIBLE);
                  }
                  messageView.invalidate();
                }
                commandExecuted=true;
              }
              if (!commandExecuted) {
                GDApplication.addMessage(GDApplication.ERROR_MSG, "GDApp", "unknown command " + command + "received");
              }
              break;
          }
      }
  };  

  /** Sets the screen time out */
  void updateWakeLock() {
    if (mapSurfaceView!=null) {
      String state=coreObject.executeCoreCommand("getWakeLock()");
      if (state.equals("true")) {
        GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp","wake lock enabled");
        if (!wakeLock.isHeld())
          wakeLock.acquire();
      } else {
        GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp","wake lock disabled");
        if (wakeLock.isHeld())
          wakeLock.release();
      }
    }
  }
      
  /** Start listening for compass bearing */
  synchronized void startWatchingCompass() {
    if ((mapSurfaceView!=null)&&(!compassWatchStarted)) {
      sensorManager.registerListener(coreObject,sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD),SensorManager.SENSOR_DELAY_NORMAL);
      sensorManager.registerListener(coreObject,sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER),SensorManager.SENSOR_DELAY_NORMAL);
      compassWatchStarted=true;
    }
  }
  
  /** Stop listening for location fixes */
  synchronized void stopWatchingCompass() {
    if (compassWatchStarted) {
      sensorManager.unregisterListener(coreObject);
      compassWatchStarted=false;
    }
  }

  /** Called when the activity is first created. */
  @Override
  public void onCreate(Bundle savedInstanceState) {
    
    super.onCreate(savedInstanceState);

    // Get the core object
    coreObject=GDApplication.coreObject;    
    coreObject.setActivity(this);

    // Get important handles
    locationManager = (LocationManager) this.getSystemService(Context.LOCATION_SERVICE);
    sensorManager = (SensorManager) this.getSystemService(Context.SENSOR_SERVICE);
    powerManager = (PowerManager) this.getSystemService(Context.POWER_SERVICE);
    
    // Setup the GUI
    requestWindowFeature(Window.FEATURE_NO_TITLE);    

    // Prepare the window contents
    frameLayout = new FrameLayout(this);    
    mapSurfaceView = new GDMapSurfaceView(this);
    mapSurfaceView.setLayoutParams(new FrameLayout.LayoutParams(LayoutParams.FILL_PARENT,LayoutParams.FILL_PARENT));
    messageView = new TextView(this);
    messageView.setLayoutParams(new FrameLayout.LayoutParams(LayoutParams.FILL_PARENT,LayoutParams.FILL_PARENT));
    messageView.setTextColor(0xFFFFFFFF);
    messageView.setBackgroundColor(0xA0000000);
    messageView.setTypeface(Typeface.MONOSPACE);
    messageView.setTextSize(10);
    messageView.setVisibility(TextView.INVISIBLE);
    messageView.setGravity(Gravity.BOTTOM);
    frameLayout.addView(mapSurfaceView,0);
    frameLayout.addView(messageView,1);
    setContentView(frameLayout);    
    
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
    GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "onPause called by " + Thread.currentThread().getName());
    stopWatchingCompass();
    coreObject.executeCoreCommand("maintenance()");
    if (!exitRequested) {
      Intent intent = new Intent(this, GDService.class);
      intent.setAction("activityPaused");
      startService(intent);
    }
  }
  
  /** Called when the app resumes */
  @Override
  public void onResume() {
    super.onResume();
    
    // Resume all components
    GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "onResume called by " + Thread.currentThread().getName());
    startWatchingCompass();
    updateWakeLock();
    Intent intent = new Intent(this, GDService.class);
    intent.setAction("activityResumed");
    startService(intent);
    if (exitRequested) {
      coreObject.coreStopped=false;
      exitRequested=false;
    }
    
    // Extract the file path from the intent
    intent = getIntent();
    String srcFilename = "";
    if (intent!=null) {
      if (Intent.ACTION_SEND.equals(intent.getAction())) {
        Bundle extras = intent.getExtras();
        if (extras.containsKey(Intent.EXTRA_STREAM)) {
          Uri uri = (Uri) extras.getParcelable(Intent.EXTRA_STREAM);
          if (uri.getScheme().equals("file")) {
            srcFilename = uri.getPath();
          } else {
            warningDialog(String.format(getString(R.string.unsupported_scheme),uri.getScheme()));
          }
        } else {
          warningDialog(getString(R.string.unsupported_intent));        
        }
      }
      if (Intent.ACTION_VIEW.equals(intent.getAction())) {
        Uri uri = intent.getData();
        if (uri.getScheme().equals("file")) {
          srcFilename = uri.getPath();
        } else {
          warningDialog(String.format(getString(R.string.unsupported_scheme),uri.getScheme()));        
        }
      }
    }
    
    // Handle the intent
    GDApplication app=(GDApplication)getApplication();
    if (!srcFilename.equals("")) {
            
      // Ask the user if the file should be copied to the route directory
      final File srcFile = new File(srcFilename);
      if (srcFile.exists()) {

        // Ask the user if the file should be copied
        String dstFilename = app.getHomeDirPath() + "/Route/" + srcFile.getName();
        final File dstFile = new File(dstFilename);
        if (!srcFile.equals(dstFile)) {
          AlertDialog.Builder builder = new AlertDialog.Builder(this);
          builder.setTitle(getTitle());
          String message; 
          if (dstFile.exists())
            message=getString(R.string.overwrite_route_question);              
          else
            message=getString(R.string.copy_route_question);
          message = String.format(message, srcFile.getName());
          builder.setMessage(message);              
          builder.setCancelable(true);
          builder.setPositiveButton(R.string.yes,
              new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                  ProgressDialog progressDialog = new ProgressDialog(ViewMap.this);
                  progressDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
                  String message = getString(R.string.copying_file);
                  message = String.format(message, srcFile.getName());
                  progressDialog.setMessage(message);
                  progressDialog.setCancelable(false);
                  progressDialog.show();
                  try {
                    GDApplication.copyFile(srcFile.getPath(), dstFile.getPath());
                  }
                  catch (IOException e) {
                    errorDialog(String.format(getString(R.string.cannot_copy_file), srcFile.getPath(), dstFile.getPath()));
                  }
                  progressDialog.dismiss();
                  new RestartCoreObjectTask().execute();
                }
              });
          builder.setNegativeButton(R.string.no, null);
          builder.setIcon(android.R.drawable.ic_dialog_info);
          AlertDialog alert = builder.create();
          alert.show();
        }
      } else {
        errorDialog(getString(R.string.file_does_not_exist));
      }
    }
    
    // Reset intent
    setIntent(null);
   
  }
  
  /** Called when a new intent is available */
  @Override
  public void onNewIntent(Intent intent) {
    setIntent(intent);
  }

  /** Called when the activity is destroyed */
  @Override
  public void onDestroy() {    
    super.onDestroy();
    GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "onDestroy called by " + Thread.currentThread().getName());
    coreObject.setActivity(null);
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
      if (messageView.getVisibility()==TextView.VISIBLE) {
        menu.findItem(R.id.toggle_messages).setTitle(R.string.hide_messages);
      } else {
        menu.findItem(R.id.toggle_messages).setTitle(R.string.show_messages);
      }
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
              builder.setIcon(android.R.drawable.ic_dialog_alert);
              AlertDialog alert = builder.create();
              alert.show();
            }
            return true;
          case R.id.exit:
            RestartCoreObjectTask task = new RestartCoreObjectTask();
            task.exitOnly=true;
            task.execute();
            return true;
          case R.id.toggle_messages:
            if (messageView.getVisibility()==TextView.VISIBLE) {
              messageView.setVisibility(TextView.INVISIBLE);
              item.setTitle(R.string.show_messages);
            } else {
              messageView.setText(GDApplication.messages);
              messageView.setVisibility(TextView.VISIBLE);
              item.setTitle(R.string.hide_messages);
            }
            messageView.invalidate();
            return true;
          default:
              return super.onOptionsItemSelected(item);
      }
  }  

  /** Restarts the core object */
  private class RestartCoreObjectTask extends AsyncTask<Void, Void, Void> {

    ProgressDialog progressDialog;
    public boolean resetConfig = false;
    public boolean exitOnly = false;

    protected void onPreExecute() {
      progressDialog = new ProgressDialog(ViewMap.this);
      progressDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
      if (exitOnly) 
        progressDialog.setMessage(getString(R.string.stopping_core_object));
      else
        progressDialog.setMessage(getString(R.string.restarting_core_object));        
      progressDialog.setCancelable(false);
      progressDialog.show();
    }

    protected Void doInBackground(Void... params) {
      if (exitOnly) {
        coreObject.stop();
      } else {
        coreObject.restart(resetConfig);              
      }
      return null;
    }

    protected void onPostExecute(Void result) {
      progressDialog.dismiss();
      if (exitOnly) {
        exitRequested = true;
        stopService(new Intent(ViewMap.this, GDService.class));
        finish();                
      }
    }
  }
  
  
  /** Called when a called activity finishes */
  protected void onActivityResult(int requestCode, int resultCode, Intent data) {
      
    // Did the activity changes prefs?
    if (requestCode == SHOW_PREFERENCE_REQUEST) {
      if (resultCode==1) {
        
        // Restart the core object
        new RestartCoreObjectTask().execute();
        
      }
    }
  }

}