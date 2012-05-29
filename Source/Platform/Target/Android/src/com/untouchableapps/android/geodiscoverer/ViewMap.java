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

package com.untouchableapps.android.geodiscoverer;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.LinkedList;
import java.util.List;
import java.util.StringTokenizer;
import java.util.Vector;

import android.app.AlertDialog;
import android.app.Application;
import android.app.ProgressDialog;
import android.content.*;
import android.content.res.Configuration;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable.Orientation;
import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.location.Address;
import android.location.Geocoder;
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

import android.sax.TextElementListener;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.KeyEvent;
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
import android.view.animation.RotateAnimation;
import android.view.animation.TranslateAnimation;
import android.view.inputmethod.ExtractedTextRequest;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.FrameLayout.LayoutParams;
import android.widget.ImageView;
import android.widget.LinearLayout;
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
  TextView busyTextView = null;
  ImageView splashView = null;
  ImageView busyCircleView = null;
  FrameLayout rootFrameLayout = null;
  LinearLayout messageLayout = null;
  LinearLayout splashLinearLayout = null;
  
  /** Reference to the core object */
  GDCore coreObject = null;
  
  // Info about the last location entered
  String lastLocationSubject;
  String lastLocationText = "";
  
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

  // Shows the splash screen
  void setSplashVisibility(boolean isVisible) {
    if (isVisible) {
      splashLinearLayout.setVisibility(LinearLayout.VISIBLE);
      messageView.setVisibility(TextView.VISIBLE);
      coreObject.setSplashIsVisible(true);
      /*RotateAnimation animation = new RotateAnimation(0, 360, Animation.RELATIVE_TO_SELF, 0.5f, Animation.RELATIVE_TO_SELF, 0.5f);
      animation.setRepeatCount(Animation.INFINITE);
      animation.setDuration(3000);
      busyCircleView.setAnimation(animation);*/
    } else {
      splashLinearLayout.setVisibility(LinearLayout.GONE);
      //busyCircleView.setAnimation(null);                    
      messageView.setVisibility(TextView.INVISIBLE);
      busyTextView.setText(" " + getString(R.string.starting_core_object) + " ");
    }
  }
  
  // Shows the context menu
  void showContextMenu() {
    AlertDialog.Builder builder = new AlertDialog.Builder(this);
    builder.setTitle(R.string.context_menu_title);
    builder.setIcon(android.R.drawable.ic_menu_mapmode);
    String[] items = { 
      getString(R.string.set_target_at_map_center), 
      getString(R.string.set_target_at_location), 
      getString(R.string.show_target), 
      getString(R.string.hide_target), 
    };
    builder.setItems(items, 
        new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dialog, int which) {
            switch(which) {
              case 0: coreObject.executeCoreCommand("setTargetAtMapCenter()"); break;
              case 1: askForLocation(getString(R.string.manually_entered_location),lastLocationText); break;
              case 2: coreObject.executeCoreCommand("showTarget()"); break;
              case 3: coreObject.executeCoreCommand("hideTarget()"); break;
            }
          }
        }
    );
    builder.setCancelable(true);
    AlertDialog alert = builder.create();
    alert.show();    
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
              if (commandFunction.equals("setSplashVisibility")) {
                if (messageView!=null) {
                  if (commandArgs.get(0).equals("1")) {
                    setSplashVisibility(true);
                  } else {
                    setSplashVisibility(false);
                  }
                  rootFrameLayout.requestLayout();
                }
                commandExecuted=true;
              }
              if (commandFunction.equals("showContextMenu")) {
                showContextMenu();
                commandExecuted=true;
              }
              if (commandFunction.equals("exitActivity")) {
                exitRequested = true;
                stopService(new Intent(ViewMap.this, GDService.class));
                finish();                
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

  /** Updates the configuration of views */
  void updateViewConfiguration(Configuration configuration) {
    if (configuration.orientation == Configuration.ORIENTATION_LANDSCAPE)
      messageLayout.setOrientation(LinearLayout.HORIZONTAL);
    else 
      messageLayout.setOrientation(LinearLayout.VERTICAL);  
    rootFrameLayout.requestLayout();
  }
  
  /** Asks the user for confirmation of the address */
  void askForLocation(final String subject, final String text) {
    AlertDialog.Builder builder = new AlertDialog.Builder(this);
    final EditText editText = new EditText(this);
    editText.setText(text);
    builder.setTitle(R.string.location_dialog_title);
    builder.setMessage(R.string.location_dialog_message);
    builder.setIcon(android.R.drawable.ic_menu_mapmode);
    builder.setView(editText);
    builder.setPositiveButton(R.string.finished, new DialogInterface.OnClickListener() {  
      public void onClick(DialogInterface dialog, int whichButton) {  
        AddressFromLocationTask task = new AddressFromLocationTask();
        task.subject = subject;
        task.text = editText.getText().toString();
        task.execute();
      }  
    });
    builder.setNegativeButton(R.string.cancel, null);
    builder.setCancelable(true);
    AlertDialog alert = builder.create();
    alert.show();                
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
    DisplayMetrics metrics = new DisplayMetrics();
    getWindowManager().getDefaultDisplay().getMetrics(metrics);
    rootFrameLayout = new FrameLayout(this);    
    mapSurfaceView = new GDMapSurfaceView(this);
    mapSurfaceView.setLayoutParams(new FrameLayout.LayoutParams(LayoutParams.FILL_PARENT,LayoutParams.FILL_PARENT));
    messageLayout = new LinearLayout(this);
    messageLayout.setLayoutParams(new FrameLayout.LayoutParams(LayoutParams.FILL_PARENT,LayoutParams.FILL_PARENT));
    messageView = new TextView(this);
    messageView.setLayoutParams(new LinearLayout.LayoutParams(LayoutParams.FILL_PARENT,LayoutParams.FILL_PARENT));
    messageView.setTextColor(0xFFFFFFFF);
    messageView.setTypeface(Typeface.MONOSPACE);
    messageView.setTextSize(10);
    messageView.setBackgroundColor(0xA0000000);
    messageView.setVisibility(TextView.INVISIBLE);
    messageView.setGravity(Gravity.BOTTOM);
    splashLinearLayout = new LinearLayout(this);
    LinearLayout.LayoutParams linearLayoutParams = new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT,LayoutParams.WRAP_CONTENT);
    linearLayoutParams.gravity = Gravity.CENTER;
    splashLinearLayout.setLayoutParams(linearLayoutParams);
    splashLinearLayout.setOrientation(LinearLayout.VERTICAL);
    splashView = new ImageView(this);
    splashView.setImageResource(R.drawable.splash);
    linearLayoutParams = new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT,LayoutParams.WRAP_CONTENT);
    linearLayoutParams.gravity = Gravity.CENTER;
    splashView.setLayoutParams(linearLayoutParams);
    busyTextView = new TextView(this);
    linearLayoutParams = new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT,LayoutParams.WRAP_CONTENT);
    linearLayoutParams.gravity = Gravity.CENTER;
    linearLayoutParams.leftMargin = (int)(5*metrics.density);
    linearLayoutParams.rightMargin = linearLayoutParams.leftMargin;
    linearLayoutParams.bottomMargin = (int)(10*metrics.density);
    busyTextView.setLayoutParams(linearLayoutParams);
    busyTextView.setTextSize(18.0f);
    busyTextView.setTextColor(0xFFFFFFFF);
    busyTextView.setBackgroundColor(0xA0000000);
    busyTextView.setGravity(Gravity.CENTER);
    splashLinearLayout.addView(splashView,0);
    splashLinearLayout.addView(busyTextView,1);
    messageLayout.addView(splashLinearLayout, 0);
    messageLayout.addView(messageView, 1);    
    rootFrameLayout.addView(mapSurfaceView,0);
    rootFrameLayout.addView(messageLayout,1);
    splashLinearLayout.setVisibility(LinearLayout.GONE);
    updateViewConfiguration(getResources().getConfiguration());
    setSplashVisibility(true); // to get the correct busy text
    setSplashVisibility(false);
    setContentView(rootFrameLayout);   

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
    if (coreObject.coreStopped) {
      Message m=Message.obtain(coreObject.messageHandler);
      m.what = GDCore.START_CORE;
      coreObject.messageHandler.sendMessage(m);      
    }
    exitRequested=false;
    
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
        } else if (extras.containsKey(Intent.EXTRA_TEXT)) {
          askForLocation(extras.getString(Intent.EXTRA_SUBJECT), extras.getString(Intent.EXTRA_TEXT));
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
                  busyTextView.setText(" " + getString(R.string.restarting_core_object) + " ");
                  Message m=Message.obtain(coreObject.messageHandler);
                  m.what = GDCore.RESTART_CORE;
                  Bundle b = new Bundle();
                  b.putBoolean("resetConfig", false);
                  m.setData(b);    
                  coreObject.messageHandler.sendMessage(m);
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
    updateViewConfiguration(newConfig);
          
  }
  
  /** Called when a option menu shall be created */  
  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    MenuInflater inflater = getMenuInflater();
    inflater.inflate(R.menu.view_map, menu);
    return true;
  }

  /** Called when a option menu shall be displayed */  
  @Override
  public boolean onPrepareOptionsMenu(Menu menu) {
    super.onPrepareOptionsMenu(menu);
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
                      busyTextView.setText(" " + getString(R.string.restarting_core_object) + " ");
                      Message m=Message.obtain(coreObject.messageHandler);
                      m.what = GDCore.RESTART_CORE;
                      Bundle b = new Bundle();
                      b.putBoolean("resetConfig", true);
                      m.setData(b);    
                      coreObject.messageHandler.sendMessage(m);
                    }
                  });
              builder.setNegativeButton(R.string.no, null);
              builder.setIcon(android.R.drawable.ic_dialog_alert);
              AlertDialog alert = builder.create();
              alert.show();
            }
            return true;
          case R.id.exit:
            busyTextView.setText(" " + getString(R.string.stopping_core_object) + " ");
            Message m=Message.obtain(coreObject.messageHandler);
            m.what = GDCore.STOP_CORE;
            coreObject.messageHandler.sendMessage(m);
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
            rootFrameLayout.requestLayout();
            return true;
          default:
              return super.onOptionsItemSelected(item);
      }
  }  
  
  /** Finds the geographic position for the given address */
  private class AddressFromLocationTask extends AsyncTask<Void, Void, Void> {

    String subject;
    String text;
    boolean locationFound=false;
    
    protected void onPreExecute() {
    }

    protected Void doInBackground(Void... params) {

      // Go through all lines and treat each line as an address
      // If the geocoder finds the address, add it as a POI
      String[] locationLines = text.split("\n");
      Geocoder geocoder = new Geocoder(ViewMap.this);
      for (String locationLine : locationLines) { 
        try {
          List<Address> addresses = geocoder.getFromLocationName(locationLine, 1);
          if (addresses.size()>0) {
            Address address = addresses.get(0);
            locationFound=true;
            coreObject.scheduleCoreCommand("setTargetAtGeographicCoordinate(" + address.getLongitude() + "," + address.getLatitude() + ")");
          }
        }
        catch(IOException e) {
          GDApplication.addMessage(GDApplication.WARNING_MSG, "GDApp", "Geocoding not successful: " + e.getMessage());        
        }        
      }
      return null;
    }

    protected void onPostExecute(Void result) {
      if (!locationFound) {
        warningDialog(String.format(getString(R.string.location_not_found),text));
      } 
      lastLocationSubject=subject;
      lastLocationText=text;
    }
  }
  
  /** Called when a called activity finishes */
  protected void onActivityResult(int requestCode, int resultCode, Intent data) {
      
    // Did the activity changes prefs?
    if (requestCode == SHOW_PREFERENCE_REQUEST) {
      if (resultCode==1) {
        
        // Restart the core object
        busyTextView.setText(" " + getString(R.string.restarting_core_object) + " ");        
        Message m=Message.obtain(coreObject.messageHandler);
        m.what = GDCore.RESTART_CORE;
        Bundle b = new Bundle();
        b.putBoolean("resetConfig", false);
        m.setData(b);    
        coreObject.messageHandler.sendMessage(m);
        
      }
    }
  }

  /** Called when a key is pressed */ 
  public boolean onKeyUp (int keyCode, KeyEvent event) {
    if (keyCode==KeyEvent.KEYCODE_DPAD_CENTER) {
      coreObject.executeCoreCommand("showContextMenu()");
      return true;
    }
    return false;
  }

}
