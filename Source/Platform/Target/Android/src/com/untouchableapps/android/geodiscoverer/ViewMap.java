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
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintWriter;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedList;
import java.util.List;
import java.util.Vector;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.AlertDialog;
import android.app.DownloadManager;
import android.app.ProgressDialog;
import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.database.Cursor;
import android.graphics.Color;
import android.graphics.Path.FillType;
import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.location.Address;
import android.location.Geocoder;
import android.location.LocationManager;
import android.net.Uri;
import android.opengl.Visibility;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.support.v4.widget.DrawerLayout;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ListView;
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
  DrawerLayout viewMapRootLayout = null;
  LinearLayout messageLayout = null;
  LinearLayout splashLayout = null;
  ListView navDrawerList = null;
  
  /** Reference to the core object */
  GDCore coreObject = null;
  
  // Info about the last address entered
  String lastAddressSubject;
  String lastAddressText = "";
  
  // Info about the current gpx file
  String gpxName = "";
  
  // Managers
  LocationManager locationManager;
  SensorManager sensorManager;
  PowerManager powerManager;
  DevicePolicyManager devicePolicyManager;
  DownloadManager downloadManager;
  
  // Wake lock
  WakeLock wakeLock = null;
  
  // Flags
  boolean compassWatchStarted = false;
  boolean exitRequested = false;
  
  // Handles finished queued downloads
  LinkedList<Bundle> downloads = new LinkedList<Bundle>();
  BroadcastReceiver downloadCompleteReceiver = new BroadcastReceiver() {
    
    @TargetApi(Build.VERSION_CODES.GINGERBREAD)
    @Override
    public void onReceive(Context context, Intent intent) {
      
      // If one of the requested download finished, restart the core
      for (Bundle download : downloads) {
        DownloadManager.Query query = new DownloadManager.Query();
        query.setFilterById(download.getLong("ID"));
        Cursor cur = downloadManager.query(query);
        int col = cur.getColumnIndex(DownloadManager.COLUMN_STATUS);
        if (cur.moveToFirst()) {
          if (cur.getInt(col)==DownloadManager.STATUS_SUCCESSFUL) {
            restartCore(false);
          } else {
            errorDialog(getString(R.string.download_failed,download.getLong("Name")));
          }
        }
        downloads.remove(download);
        break;
      }
    }
  };
  
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
    progressDialog.setMessage(progressMessage);    
    progressDialog.setProgress(progressCurrent);
  }

  // Shows the splash screen
  void setSplashVisibility(boolean isVisible) {
    if (isVisible) {
      splashLayout.setVisibility(LinearLayout.VISIBLE);
      messageLayout.setVisibility(LinearLayout.VISIBLE);
      coreObject.setSplashIsVisible(true);
      /*RotateAnimation animation = new RotateAnimation(0, 360, Animation.RELATIVE_TO_SELF, 0.5f, Animation.RELATIVE_TO_SELF, 0.5f);
      animation.setRepeatCount(Animation.INFINITE);
      animation.setDuration(3000);
      busyCircleView.setAnimation(animation);*/
    } else {
      splashLayout.setVisibility(LinearLayout.GONE);
      //busyCircleView.setAnimation(null);                    
      messageLayout.setVisibility(LinearLayout.INVISIBLE);
      busyTextView.setText(" " + getString(R.string.starting_core_object) + " ");
    }
  }
  
  // Shows the context menu
  void showContextMenu() {
    AlertDialog.Builder builder = new AlertDialog.Builder(this);
    builder.setTitle(R.string.context_menu_title);
    builder.setIcon(android.R.drawable.ic_dialog_info);
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
              case 1: askForAddress(getString(R.string.manually_entered_address),lastAddressText); break;
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
  static protected class CoreMessageHandler extends Handler {
    
    protected final WeakReference<ViewMap> weakViewMap; 
    
    CoreMessageHandler(ViewMap viewMap) {
      this.weakViewMap = new WeakReference<ViewMap>(viewMap);
    }
    
    /** Called when the core has a message */
    @Override
    public void handleMessage(Message msg) {  
      
      // Abort if the object is not available anymore
      ViewMap viewMap = weakViewMap.get();
      if (viewMap==null)
        return;
    
      // Handle the message
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
            viewMap.fatalDialog(commandArgs.get(0));
            commandExecuted=true;
          }
          if (commandFunction.equals("errorDialog")) {
            viewMap.errorDialog(commandArgs.get(0));
            commandExecuted=true;
          }
          if (commandFunction.equals("warningDialog")) {
            viewMap.warningDialog(commandArgs.get(0));
            commandExecuted=true;
          }
          if (commandFunction.equals("infoDialog")) {
            viewMap.infoDialog(commandArgs.get(0));
            commandExecuted=true;
          }
          if (commandFunction.equals("createProgressDialog")) {
            viewMap.progressMax=Integer.parseInt(commandArgs.get(1));
            viewMap.updateProgressDialog(commandArgs.get(0),0);
            commandExecuted=true;
          }
          if (commandFunction.equals("updateProgressDialog")) {
            viewMap.updateProgressDialog(commandArgs.get(0),Integer.parseInt(commandArgs.get(1)));
            commandExecuted=true;
          }
          if (commandFunction.equals("closeProgressDialog")) {
            if (viewMap.progressDialog!=null) {
              viewMap.progressDialog.dismiss();
              viewMap.progressDialog=null;
            }
            commandExecuted=true;
          }
          if (commandFunction.equals("getLastKnownLocation")) {
            if (viewMap.coreObject!=null) {
              viewMap.coreObject.onLocationChanged(viewMap.locationManager.getLastKnownLocation(LocationManager.NETWORK_PROVIDER));
              viewMap.coreObject.onLocationChanged(viewMap.locationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER));
            }
            commandExecuted=true;
          }
          if (commandFunction.equals("coreInitialized")) {
            Intent intent = new Intent(viewMap, GDService.class);
            intent.setAction("coreInitialized");
            viewMap.startService(intent);             
            commandExecuted=true;
          }
          if (commandFunction.equals("updateWakeLock")) {
            viewMap.updateWakeLock();
            commandExecuted=true;
          }
          if (commandFunction.equals("updateMessages")) {
            if (viewMap.messageLayout!=null) {
              if (viewMap.messageLayout.getVisibility()==LinearLayout.VISIBLE) {
                viewMap.messageView.setText(GDApplication.messages);
                viewMap.messageView.invalidate();
              }
            }
            commandExecuted=true;
          }
          if (commandFunction.equals("setSplashVisibility")) {
            if (viewMap.messageLayout!=null) {
              if (commandArgs.get(0).equals("1")) {
                viewMap.setSplashVisibility(true);
              } else {
                viewMap.setSplashVisibility(false);
              }
              viewMap.viewMapRootLayout.requestLayout();
            }
            commandExecuted=true;
          }
          if (commandFunction.equals("showContextMenu")) {
            viewMap.showContextMenu();
            commandExecuted=true;
          }
          if (commandFunction.equals("askForAddress")) {
            viewMap.askForAddress(viewMap.getString(R.string.manually_entered_address),viewMap.lastAddressText);
            commandExecuted=true;
          }
          if (commandFunction.equals("exitActivity")) {
            viewMap.exitRequested = true;
            viewMap.stopService(new Intent(viewMap, GDService.class));
            viewMap.finish();
            commandExecuted=true;
          } 
          
          if (!commandExecuted) {
            GDApplication.addMessage(GDApplication.ERROR_MSG, "GDApp", "unknown command " + command + " received");
          }
          break;
      }
    }
  }
  CoreMessageHandler coreMessageHandler = new CoreMessageHandler(this);

  /** Sets the screen time out */
  @SuppressLint("Wakelock")
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
      sensorManager.registerListener(coreObject,sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER),SensorManager.SENSOR_DELAY_NORMAL);
      sensorManager.registerListener(coreObject,sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD),SensorManager.SENSOR_DELAY_NORMAL);
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
    viewMapRootLayout.requestLayout();
  }

  /** Restarts the core */
  void restartCore(boolean resetConfig) {
    busyTextView.setText(" " + getString(R.string.restarting_core_object) + " ");
    Message m=Message.obtain(coreObject.messageHandler);
    m.what = GDCore.RESTART_CORE;
    Bundle b = new Bundle();
    b.putBoolean("resetConfig", resetConfig);
    m.setData(b);    
    coreObject.messageHandler.sendMessage(m);
  }

  
  /** Asks the user for confirmation of the address */
  void askForAddress(final String subject, final String text) {
    AlertDialog.Builder builder = new AlertDialog.Builder(this);
    final EditText editText = new EditText(this);
    editText.setText(text);
    builder.setTitle(R.string.address_dialog_title);
    builder.setMessage(R.string.address_dialog_message);
    builder.setIcon(android.R.drawable.ic_dialog_info);
    builder.setView(editText);
    builder.setPositiveButton(R.string.finished, new DialogInterface.OnClickListener() {  
      public void onClick(DialogInterface dialog, int whichButton) {  
        LocationFromAddressTask task = new LocationFromAddressTask();
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

  /** Asks the user for confirmation of the route name */
  void askForRouteName(Uri srcURI, final String routeName) {
    AlertDialog.Builder builder = new AlertDialog.Builder(this);
    final ImportRouteTask task = new ImportRouteTask();
    task.srcURI = srcURI;
    final FrameLayout dialogRootLayout = new FrameLayout(this);
    builder.setTitle(R.string.route_name_dialog_title);
    builder.setMessage(R.string.route_name_dialog_message);
    builder.setIcon(android.R.drawable.ic_dialog_info);
    builder.setView(dialogRootLayout);
    builder.setPositiveButton(R.string.finished, new DialogInterface.OnClickListener() {  
      public void onClick(DialogInterface dialog, int whichButton) {  
        task.execute();
      }  
    });
    builder.setNegativeButton(R.string.cancel, null);
    builder.setCancelable(true);
    AlertDialog alert = builder.create();
    LayoutInflater inflater = alert.getLayoutInflater();
    final View contentView = inflater.inflate(R.layout.route_name, dialogRootLayout);
    final EditText editText = (EditText) contentView.findViewById(R.id.route_name_edit_text);
    editText.setText(gpxName);
    final TextView routeExistsTextView = (TextView) contentView.findViewById(R.id.route_name_route_exists_warning);
    TextWatcher textWatcher = new TextWatcher() {
      public void onTextChanged(CharSequence s, int start, int before, int count) {
        
        // Check if route exists
        String dstFilename = GDApplication.getHomeDirPath() + "/Route/" + s;
        final File dstFile = new File(dstFilename);
        if ((s=="")||(!dstFile.exists())) 
          routeExistsTextView.setVisibility(View.GONE);          
        else
          routeExistsTextView.setVisibility(View.VISIBLE);
        contentView.requestLayout();

        // Remember the select name
        task.name=s.toString();
        task.dstFilename=dstFilename;
        
      }
      public void beforeTextChanged(CharSequence s, int start, int count, int after) {
      }
      public void afterTextChanged(Editable s) {
      }
    };
    textWatcher.onTextChanged(editText.getText(),0,0,0);
    editText.addTextChangedListener(textWatcher);
    alert.show();                
  }

  /** Copies tracks from the Track into the Route directory */
  private class CopyTracksTask extends AsyncTask<Void, Integer, Void> {

    ProgressDialog progressDialog;
    public String[] trackNames;

    protected void onPreExecute() {

      // Prepare the progress dialog
      progressDialog = new ProgressDialog(ViewMap.this);
      progressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
      progressDialog.setMessage(getString(R.string.copying_tracks));
      progressDialog.setMax(trackNames.length);
      progressDialog.setCancelable(false);
      progressDialog.show();
    }

    protected Void doInBackground(Void... params) {

      // Copy all selected tracks to the route directory
      Integer progress = 0;
      for (String trackName : trackNames) {
        String srcFilename = coreObject.homePath + "/Track/" + trackName;
        String dstFilename = coreObject.homePath + "/Route/" + trackName;
        try {
          GDApplication.copyFile(srcFilename, dstFilename);
        } catch (IOException exception) {
          Toast.makeText(ViewMap.this,String.format(getString(R.string.cannot_copy_file), srcFilename, dstFilename), Toast.LENGTH_LONG).show();
        }
        progress++;
        publishProgress(progress);
      }

      return null;
    }

    protected void onProgressUpdate(Integer... progress) {
      progressDialog.setProgress(progress[0]);
    }

    protected void onPostExecute(Void result) {

      // Close the progress dialog
      progressDialog.dismiss();

      // Restart the core to load the new routes
      restartCore(false);
    }
  }

  /** Copies tracks from the Track into the Route directory */
  private class ImportRouteTask extends AsyncTask<Void, Integer, Void> {

    ProgressDialog progressDialog;
    public String name;
    public Uri srcURI;
    public String dstFilename;

    protected void onPreExecute() {

      // Prepare the progress dialog
      progressDialog = new ProgressDialog(ViewMap.this);
      progressDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
      progressDialog.setMessage(getString(R.string.importing_route,name));
      progressDialog.setCancelable(false);
      progressDialog.show();
    }

    protected Void doInBackground(Void... params) {

      // Open the content of the route file
      InputStream gpxContents = null;
      try {
        gpxContents = getContentResolver().openInputStream(srcURI);
      }
      catch (FileNotFoundException e) {
        errorDialog(getString(R.string.cannot_read_uri,srcURI.toString()));
      }
      if (gpxContents!=null) {
        
        // Create the destination file
        try {
          GDApplication.copyFile(gpxContents, dstFilename);
          gpxContents.close();
        } 
        catch (IOException e) {
          Toast.makeText(ViewMap.this,String.format(getString(R.string.cannot_import_route), name), Toast.LENGTH_LONG).show();          
        }
        
      }

      return null;
    }

    protected void onProgressUpdate(Integer... progress) {
    }

    protected void onPostExecute(Void result) {

      // Close the progress dialog
      progressDialog.dismiss();

      // Restart the core to load the new routes
      restartCore(false);
    }
  }
  
  /** Copies tracks to the route directory */
  void addTracksAsRoutes() {

    // Obtain the list of file in the folder
    File folderFile = new File(coreObject.homePath + "/Track");
    LinkedList<String> routes = new LinkedList<String>();
    for (File file : folderFile.listFiles()) {
      if ((!file.isDirectory())
          && (!file.getName().substring(file.getName().length() - 1)
              .equals("~"))) {
        routes.add(file.getName());
      }
    }
    final String[] items = new String[routes.size()];
    routes.toArray(items);
    Arrays.sort(items, Collections.reverseOrder());

    // Create the state array
    final boolean[] checkedItems = new boolean[routes.size()];

    // Create the dialog
    AlertDialog.Builder builder = new AlertDialog.Builder(this);
    builder.setTitle(R.string.track_as_route_selection_question);
    builder.setMultiChoiceItems(items, checkedItems,
        new DialogInterface.OnMultiChoiceClickListener() {
          public void onClick(DialogInterface dialog, int which,
              boolean isChecked) {
          }
        });
    builder.setCancelable(true);
    builder.setPositiveButton(R.string.finished,
        new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dialog, int which) {
            CopyTracksTask copyTracksTask = new CopyTracksTask();
            LinkedList<String> trackNames = new LinkedList<String>();
            for (int i = 0; i < items.length; i++) {
              if (checkedItems[i]) {
                trackNames.add(items[i]);
              }
            }
            if (trackNames.size() == 0)
              return;
            copyTracksTask.trackNames = new String[trackNames.size()];
            trackNames.toArray(copyTracksTask.trackNames);
            copyTracksTask.execute();
          }
        });
    builder.setNegativeButton(R.string.cancel, null);
    builder.setIcon(android.R.drawable.ic_dialog_info);
    AlertDialog alert = builder.create();
    alert.show();
  }

  /** Removes routes from the Route directory */
  private class RemoveRoutesTask extends AsyncTask<Void, Integer, Void> {

    ProgressDialog progressDialog;
    public String[] routeNames;

    protected void onPreExecute() {

      // Prepare the progress dialog
      progressDialog = new ProgressDialog(ViewMap.this);
      progressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
      progressDialog.setMessage(getString(R.string.removing_routes));
      progressDialog.setMax(routeNames.length);
      progressDialog.setCancelable(false);
      progressDialog.show();
    }

    protected Void doInBackground(Void... params) {

      // Copy all selected tracks to the route directory
      Integer progress = 0;
      for (String routeName : routeNames) {
        File route = new File(coreObject.homePath + "/Route/" + routeName);
        if (!route.delete()) {
          Toast.makeText(ViewMap.this,String.format(getString(R.string.cannot_remove_file), route.getPath()), Toast.LENGTH_LONG).show();
        }
        progress++;
        publishProgress(progress);
      }

      return null;
    }

    protected void onProgressUpdate(Integer... progress) {
      progressDialog.setProgress(progress[0]);
    }

    protected void onPostExecute(Void result) {

      // Close the progress dialog
      progressDialog.dismiss();

      // Restart the core to load the new routes
      restartCore(false);
    }
  }
  
  /** Removes routes from the route directory */
  void removeRoutes() {

    // Obtain the list of file in the folder
    File folderFile = new File(coreObject.homePath + "/Route");
    LinkedList<String> routes = new LinkedList<String>();
    for (File file : folderFile.listFiles()) {
      if ((!file.isDirectory())
          && (!file.getName().substring(file.getName().length() - 1)
              .equals("~"))) {
        routes.add(file.getName());
      }
    }
    final String[] items = new String[routes.size()];
    routes.toArray(items);
    Arrays.sort(items);

    // Create the state array
    final boolean[] checkedItems = new boolean[routes.size()];

    // Create the dialog
    AlertDialog.Builder builder = new AlertDialog.Builder(this);
    builder.setTitle(R.string.route_remove_selection_question);
    builder.setMultiChoiceItems(items, checkedItems,
        new DialogInterface.OnMultiChoiceClickListener() {
          public void onClick(DialogInterface dialog, int which,
              boolean isChecked) {
          }
        });
    builder.setCancelable(true);
    builder.setPositiveButton(R.string.finished,
        new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dialog, int which) {
            RemoveRoutesTask removeRoutesTask = new RemoveRoutesTask();
            LinkedList<String> routeNames = new LinkedList<String>();
            for (int i = 0; i < items.length; i++) {
              if (checkedItems[i]) {
                routeNames.add(items[i]);
              }
            }
            if (routeNames.size() == 0)
              return;
            removeRoutesTask.routeNames = new String[routeNames.size()];
            routeNames.toArray(removeRoutesTask.routeNames);
            removeRoutesTask.execute();
          }
        });
    builder.setNegativeButton(R.string.cancel, null);
    builder.setIcon(android.R.drawable.ic_dialog_info);
    AlertDialog alert = builder.create();
    alert.show();
  }
  
  /** Defines all entries in the navigation drawer */
  ArrayList<GDNavDrawerItem> createNavDrawerEntries() {
    ArrayList<GDNavDrawerItem> entries = new ArrayList<GDNavDrawerItem>();
    entries.add(new GDNavDrawerItem(GDNavDrawerItem.ID_APP_INFO,R.drawable.icon,null)); // special entry for app info
    entries.add(new GDNavDrawerItem(GDNavDrawerItem.ID_SHOW_LEGEND,android.R.drawable.ic_menu_view,getString(R.string.show_legend)));
    entries.add(new GDNavDrawerItem(GDNavDrawerItem.ID_ADD_TRACKS_AS_ROUTES,android.R.drawable.ic_menu_add,getString(R.string.add_tracks_as_routes)));
    entries.add(new GDNavDrawerItem(GDNavDrawerItem.ID_REMOVE_ROUTES,android.R.drawable.ic_menu_delete,getString(R.string.remove_routes)));
    entries.add(new GDNavDrawerItem(GDNavDrawerItem.ID_TOGGLE_MESSAGES,android.R.drawable.ic_menu_info_details,getString(R.string.toggle_messages)));
    entries.add(new GDNavDrawerItem(GDNavDrawerItem.ID_PREFERENCES,android.R.drawable.ic_menu_preferences,getString(R.string.preferences)));
    entries.add(new GDNavDrawerItem(GDNavDrawerItem.ID_RESET,android.R.drawable.ic_menu_revert,getString(R.string.reset)));
    entries.add(new GDNavDrawerItem(GDNavDrawerItem.ID_EXIT,android.R.drawable.ic_menu_close_clear_cancel,getString(R.string.exit)));
    return entries;
  }
  
  /** Prepares activity for functions only available on gingerbread */
  @TargetApi(Build.VERSION_CODES.GINGERBREAD)
  void onCreateGingerbread() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
      downloadManager = (DownloadManager) getSystemService(Context.DOWNLOAD_SERVICE);
      IntentFilter filter = new IntentFilter();
      filter.addAction(DownloadManager.ACTION_DOWNLOAD_COMPLETE);
      registerReceiver(downloadCompleteReceiver, filter);
    }
  }
  
  /** Downloads files extracted from intents */
  @TargetApi(Build.VERSION_CODES.GINGERBREAD)
  void downloadRoute(final Uri srcURI) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {

      // Create some variables
      final String name = srcURI.getLastPathSegment();
      final String dstFilename = GDApplication.getHomeDirPath() + "/Route/" + name;

      // Check if download is already ongoing
      DownloadManager.Query query = new DownloadManager.Query();
      query.setFilterByStatus(DownloadManager.STATUS_PAUSED|DownloadManager.STATUS_PENDING|DownloadManager.STATUS_RUNNING);
      Cursor cur = downloadManager.query(query);
      int col = cur.getColumnIndex(DownloadManager.COLUMN_ID);
      boolean downloadOngoing = false;
      for (Bundle download : downloads) {
        for (cur.moveToFirst(); !cur.isAfterLast(); cur.moveToNext()) {
          if (cur.getLong(col)==download.getLong("ID")) {
            downloadOngoing = true;
            break;
          }
        }
      }
      if (downloadOngoing) {
        infoDialog(getString(R.string.skipping_download));
      } else {

        // Ask the user if GPX file shall be downloaded and overwritten if file exists
        final File dstFile = new File(dstFilename);
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(getTitle());
        String message; 
        if (dstFile.exists())
          message=getString(R.string.overwrite_route_question);              
        else
          message=getString(R.string.copy_route_question);
        message = String.format(message, name);
        builder.setMessage(message);              
        builder.setCancelable(true);
        builder.setPositiveButton(R.string.yes,
          new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
              
              // Delete file if it exists
              if (dstFile.exists()) 
                dstFile.delete();
              
              // Request download of file
              DownloadManager.Request request = new DownloadManager.Request(srcURI);
              request.setDescription(getString(R.string.downloading_gpx_to_route_directory));
              request.setTitle(name);
              request.setDestinationUri(Uri.fromFile(dstFile));
              //request.setNotificationVisibility(DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED);
              long id = downloadManager.enqueue(request);
              Bundle download = new Bundle();
              download.putLong("ID", id);
              download.putString("Name",name);
              downloads.add(download);
              
            }
          });
        builder.setNegativeButton(R.string.no, null);
        builder.setIcon(android.R.drawable.ic_dialog_info);
        AlertDialog alert = builder.create();
        alert.show();
      }

    } else {
      errorDialog(getString(R.string.download_manager_not_available));
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
    devicePolicyManager = (DevicePolicyManager)this.getSystemService(Context.DEVICE_POLICY_SERVICE);
    
    // Prepare the window contents
    setContentView(R.layout.view_map);
    mapSurfaceView = (GDMapSurfaceView) findViewById(R.id.view_map_map_surface_view);
    mapSurfaceView.setCoreObject(coreObject);
    messageView = (TextView) findViewById(R.id.view_map_message_text_view);
    busyTextView = (TextView) findViewById(R.id.view_map_busy_text_view);
    viewMapRootLayout = (DrawerLayout) findViewById(R.id.view_map_root_layout);
    messageLayout = (LinearLayout) findViewById(R.id.view_map_message_layout);
    splashLayout = (LinearLayout) findViewById(R.id.view_map_splash_layout);
    navDrawerList = (ListView) findViewById(R.id.view_map_nav_drawer);
    navDrawerList.setAdapter(new GDNavDrawerAdapter((GDActivity)this,createNavDrawerEntries()));
    navDrawerList.setOnItemClickListener(new OnItemClickListener() {

      public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        GDNavDrawerAdapter adapter = (GDNavDrawerAdapter)parent.getAdapter();
        GDNavDrawerItem item = adapter.entries.get(position);
        switch (item.id) {
          case GDNavDrawerItem.ID_PREFERENCES:
            if (mapSurfaceView!=null) {
              Intent myIntent = new Intent(getApplicationContext(), Preferences.class);
              startActivityForResult(myIntent, SHOW_PREFERENCE_REQUEST);
            }
            break;
          case GDNavDrawerItem.ID_RESET:
            if (mapSurfaceView!=null) {
              AlertDialog.Builder builder = new AlertDialog.Builder(ViewMap.this);
              builder.setTitle(getTitle());
              builder.setMessage(R.string.reset_question);
              builder.setCancelable(true);
              builder.setPositiveButton(R.string.yes,
                  new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                      restartCore(true);
                    }
                  });
              builder.setNegativeButton(R.string.no, null);
              builder.setIcon(android.R.drawable.ic_dialog_alert);
              AlertDialog alert = builder.create();
              alert.show();
            }
            break;
          case GDNavDrawerItem.ID_EXIT:
            busyTextView.setText(" " + getString(R.string.stopping_core_object) + " ");
            Message m=Message.obtain(coreObject.messageHandler);
            m.what = GDCore.STOP_CORE;
            coreObject.messageHandler.sendMessage(m);
            break;
          case GDNavDrawerItem.ID_TOGGLE_MESSAGES:
            if (messageLayout.getVisibility()==LinearLayout.VISIBLE) {
              messageLayout.setVisibility(LinearLayout.INVISIBLE);
            } else {
              messageView.setText(GDApplication.messages);
              messageLayout.setVisibility(LinearLayout.VISIBLE);
            }
            viewMapRootLayout.requestLayout();
            break;
          case GDNavDrawerItem.ID_ADD_TRACKS_AS_ROUTES:
            addTracksAsRoutes();
            break;
          case GDNavDrawerItem.ID_REMOVE_ROUTES:
            removeRoutes();
            break;
          case GDNavDrawerItem.ID_SHOW_LEGEND:
            String legendPath = coreObject.executeCoreCommand("getMapLegendPath()");
            File legendPathFile = new File(legendPath);
            if (!legendPathFile.exists()) {
              errorDialog(getString(R.string.map_has_no_legend,coreObject.executeCoreCommand("getMapFolder()"),legendPath));
            } else {
              Intent intent = new Intent();
              intent.setAction(Intent.ACTION_VIEW);
              intent.setDataAndType(Uri.parse("file://" + legendPath), "image/*");
              startActivity(intent);
            }
            break;
        }
        navDrawerList.setItemChecked(position, false);
        viewMapRootLayout.closeDrawer(navDrawerList);
      }
    });
    setSplashVisibility(true); // to get the correct busy text
    setSplashVisibility(false);
    updateViewConfiguration(getResources().getConfiguration());

    // Get a wake lock
    wakeLock=powerManager.newWakeLock(PowerManager.FULL_WAKE_LOCK, "DoNotDimScreen");
    if (wakeLock==null) {
      fatalDialog("Can not obtain wake lock!");
    }
    
    // Prepare activity for gingerbread
    onCreateGingerbread();
    
    // Inform the user about the app drawer
    Toast.makeText(ViewMap.this,getString(R.string.nav_drawer_hint), Toast.LENGTH_LONG).show();

    /* Start test thread
    new Thread(new Runnable() {
      public void run() {
        while(true) {
          for (int angle=-180;angle<=180;angle++) {
            //angle=0;
            coreObject.executeAppCommand("updateNavigationInfos(0.0;0.0;Distance;1 km;Duration;5 m;" + Integer.toString(angle)+ ";50 m)");
            try {
              Thread.sleep(500);
            } 
            catch (Exception e) {
            }
          }
        }
      }
    }).start();*/
    
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
    Uri webURI = null;
    Uri fileURI = null;
    if (intent!=null) {
      if (Intent.ACTION_SEND.equals(intent.getAction())) {
        Bundle extras = intent.getExtras();
        if (extras.containsKey(Intent.EXTRA_STREAM)) {
          Uri uri = (Uri) extras.getParcelable(Intent.EXTRA_STREAM);
          if (uri.getScheme().equals("http") || uri.getScheme().equals("https")) {
            webURI=uri;
          } else {
            fileURI=uri;
          }
        } else if (extras.containsKey(Intent.EXTRA_TEXT)) {
          askForAddress(extras.getString(Intent.EXTRA_SUBJECT), extras.getString(Intent.EXTRA_TEXT));
        } else {
          warningDialog(getString(R.string.unsupported_intent));        
        }
      }
      if (Intent.ACTION_VIEW.equals(intent.getAction())) {
        Uri uri = intent.getData();
        if (uri.getScheme().equals("file")&&(uri.getLastPathSegment().endsWith(".gda"))) {
          srcFilename = uri.getPath();
        } else if (uri.getScheme().equals("http") || uri.getScheme().equals("https")) {
          webURI=uri;
        } else {
          fileURI=uri;
        }
      }
    }
    
    // Handle the intent
    if (!srcFilename.equals("")) {
            
      // Ask the user if the file should be copied to the route directory
      final File srcFile = new File(srcFilename);
      if (srcFile.exists()) {

        // Map archive?
        if (srcFilename.endsWith(".gda")) {
        
          // Ask the user if a new map shall be created based on the archive
          String mapFolderFilename = GDApplication.getHomeDirPath() + "/Map/" + srcFile.getName();
          mapFolderFilename = mapFolderFilename.substring(0, mapFolderFilename.lastIndexOf('.'));
          final String mapInfoFilename = mapFolderFilename + "/info.gds";
          final File mapFolder = new File(mapFolderFilename);
          AlertDialog.Builder builder = new AlertDialog.Builder(this);
          builder.setTitle(R.string.import_map);
          String message; 
          if (mapFolder.exists())
            message=getString(R.string.replace_map_folder_question,mapFolder.getName(),srcFile);              
          else
            message=getString(R.string.create_map_folder_question,mapFolder.getName(),srcFile);
          builder.setMessage(message);              
          builder.setCancelable(true);
          builder.setPositiveButton(R.string.yes,
            new DialogInterface.OnClickListener() {
              public void onClick(DialogInterface dialog, int which) {
                
                // Create the map folder
                try {
                  mapFolder.mkdir();
                  File cache = new File(mapFolder.getPath() + "/cache.bin");
                  cache.delete();
                  PrintWriter writer = new PrintWriter(mapInfoFilename,"UTF-8");
                  writer.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
                  writer.println("<GDS version=\"1.0\">");
                  writer.println("  <type>externalMapArchive</type>");
                  writer.println(String.format("  <mapArchivePath>%s</mapArchivePath>",srcFile.getAbsolutePath()));
                  writer.println("</GDS>");
                  writer.close();
                }
                catch(IOException e) {
                  errorDialog(String.format(getString(R.string.cannot_create_map_folder), mapFolder.getName()));
                }
                
                // Restart the core
                restartCore(false);
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
    if (webURI!=null) {
      downloadRoute(webURI);
    }
    if (fileURI!=null) {

      // Get the name of the GPX file
      gpxName = fileURI.getLastPathSegment();
      if (!gpxName.endsWith(".gpx")) {
        gpxName = gpxName + ".gpx";
      }
      askForRouteName(fileURI,gpxName);
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
  @SuppressLint("Wakelock")
  @Override
  public void onDestroy() {    
    super.onDestroy();
    GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "onDestroy called by " + Thread.currentThread().getName());
    coreObject.setActivity(null);
    if (wakeLock.isHeld())
      wakeLock.release();
    if (exitRequested) 
      System.exit(0);
  }

  /** Called when a configuration change (e.g., caused by a screen rotation) has occured */
  public void onConfigurationChanged(Configuration newConfig) {
    
    super.onConfigurationChanged(newConfig);
    updateViewConfiguration(newConfig);
          
  }
    
  /** Finds the geographic position for the given address */
  private class LocationFromAddressTask extends AsyncTask<Void, Void, Void> {

    String subject;
    String text;
    boolean locationFound=false;
    
    protected void onPreExecute() {
    }

    protected Void doInBackground(Void... params) {

      // Go through all lines and treat each line as an address
      // If the geocoder finds the address, add it as a POI
      String[] addressLines = text.split("\n");
      Geocoder geocoder = new Geocoder(ViewMap.this);
      for (String addressLine : addressLines) { 
        try {
          List<Address> addresses = geocoder.getFromLocationName(addressLine, 1);
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
      lastAddressSubject=subject;
      lastAddressText=text;
    }
  }
  
  /** Called when a called activity finishes */
  protected void onActivityResult(int requestCode, int resultCode, Intent data) {
      
    // Did the activity changes prefs?
    if (requestCode == SHOW_PREFERENCE_REQUEST) {
      if (resultCode==1) {
        
        // Restart the core object
        restartCore(false);
        
      }
    }
  }

  
  /** Called when a key is pressed */
  public boolean onKeyDown(int keyCode, KeyEvent event) {
    if (keyCode == KeyEvent.KEYCODE_BACK) {
      if (coreObject!=null) {
        if (Integer.parseInt(coreObject.configStoreGetStringValue("General", "backButtonTurnsScreenOff"))!=0) {
          try {
            devicePolicyManager.lockNow();
          } 
          catch (SecurityException e) {
            Toast.makeText(this, getString(R.string.device_admin_not_enabled), Toast.LENGTH_LONG).show();
          }
          return true;
        }
      }
    }
    if (keyCode == KeyEvent.KEYCODE_MENU) {
      if (!viewMapRootLayout.isDrawerOpen(navDrawerList)) {
        viewMapRootLayout.openDrawer(navDrawerList);
      }
      return true;
    }
    return super.onKeyDown(keyCode,event);
  }
  
  /** Called when a key is released */ 
  public boolean onKeyUp (int keyCode, KeyEvent event) {
    if (keyCode==KeyEvent.KEYCODE_DPAD_CENTER) {
      coreObject.executeCoreCommand("showContextMenu()");
      return true;
    }
    return super.onKeyUp(keyCode, event);
  }
 
}
