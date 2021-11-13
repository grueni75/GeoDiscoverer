//============================================================================
// Name        : ViewMap.java
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

package com.untouchableapps.android.geodiscoverer;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.ActivityManager;
import android.app.Dialog;
import android.app.DownloadManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.ConfigurationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Configuration;
import android.database.Cursor;
import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.location.Address;
import android.location.Geocoder;
import android.location.LocationManager;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.StrictMode;
import android.provider.OpenableColumns;
import android.provider.Settings;
import android.support.annotation.NonNull;
import android.support.design.widget.CoordinatorLayout;
import android.support.design.widget.NavigationView;
import android.support.design.widget.Snackbar;
import android.support.v4.app.NotificationCompat;
import android.support.v4.graphics.drawable.DrawableCompat;
import android.support.v4.view.GravityCompat;
import android.support.v4.widget.DrawerLayout;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.inputmethod.EditorInfo;
import android.widget.AdapterView;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import com.afollestad.materialdialogs.DialogAction;
import com.afollestad.materialdialogs.MaterialDialog;
import com.untouchableapps.android.geodiscoverer.core.GDCore;
import com.untouchableapps.android.geodiscoverer.core.GDMapSurfaceView;
import com.untouchableapps.android.geodiscoverer.core.GDTools;

import org.acra.*;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintWriter;
import java.lang.ref.WeakReference;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.LinkedList;
import java.util.List;
import java.util.Vector;

public class ViewMap extends GDActivity {

  // Request codes for calling other activities
  static final int SHOW_PREFERENCE_REQUEST = 0;

  // GUI components
  MaterialDialog progressDialog;
  int progressMax;
  int progressCurrent;
  String progressMessage;
  GDMapSurfaceView mapSurfaceView = null;
  TextView messageView = null;
  TextView busyTextView = null;
  DrawerLayout viewMapRootLayout = null;
  LinearLayout messageLayout = null;
  LinearLayout splashLayout = null;
  NavigationView navDrawerList= null;
  CoordinatorLayout snackbarPosition=null;
  MaterialDialog mapDownloadDialog=null;
  View renderBugFixView=null;

  /** Reference to the core object */
  GDCore coreObject = null;

  // Info about the current gpx file
  String gpxName = "";

  // Managers
  LocationManager locationManager;
  SensorManager sensorManager;
  PowerManager powerManager;
  DevicePolicyManager devicePolicyManager;
  DownloadManager downloadManager;
  NotificationManager notificationManager;

  // Wake lock
  WakeLock wakeLock = null;

  // Prefs
  SharedPreferences prefs = null;

  // Flags
  boolean compassWatchStarted = false;
  boolean exitRequested = false;
  boolean restartRequested = false;
  int nestedImportWaypointsDecisions = 0;
  boolean doubleBackToExitPressedOnce = false;

  // Handles finished queued downloads
  LinkedList<Bundle> downloads = new LinkedList<Bundle>();
  BroadcastReceiver downloadCompleteReceiver = null;

  /** Updates the progress dialog */
  protected void updateProgressDialog(String message, int progress) {
    progressCurrent=progress;
    progressMessage=message;
    if (progressDialog==null) {
      MaterialDialog.Builder builder = new MaterialDialog.Builder(this);
      if (progressMax==0)
        builder.progress(true, 0);
      else
        builder.progress(false, progressMax, true);
      builder.content(progressMessage);
      builder.cancelable(false);
      progressDialog=builder.build();
      progressDialog.show();
    }
    progressDialog.setContent(progressMessage);
    if (!progressDialog.isIndeterminateProgress())
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

  // Communication with the native core
  public static final int EXECUTE_COMMAND = 0;
  static protected class CoreMessageHandler extends Handler {

    protected final WeakReference<ViewMap> weakViewMap;

    CoreMessageHandler(ViewMap viewMap) {
      this.weakViewMap = new WeakReference<ViewMap>(viewMap);
    }

    /** Called when the core has a message */
    @SuppressLint("NewApi")
    @Override
    public void handleMessage(Message msg) {

      // Abort if the object is not available anymore
      final ViewMap viewMap = weakViewMap.get();
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
            viewMap.updateProgressDialog(commandArgs.get(0), 0);
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
          if (commandFunction.equals("askForAddress")) {
            viewMap.askForAddress(viewMap.getString(R.string.manually_entered_address),"");
            commandExecuted=true;
          }
          if (commandFunction.equals("exitActivity")) {
            viewMap.exitRequested = true;
            viewMap.stopService(new Intent(viewMap, GDService.class));
            viewMap.finish();
            commandExecuted=true;
          }
          if (commandFunction.equals("restartActivity")) {
            viewMap.stopService(new Intent(viewMap, GDService.class));
            SharedPreferences.Editor prefsEditor = viewMap.prefs.edit();
            prefsEditor.putBoolean("processIntent", false);
            prefsEditor.commit();
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
              viewMap.recreate();
            } else {
              viewMap.finish();
            }
            commandExecuted=true;
          }
          if (commandFunction.equals("lateInitComplete")) {

            // Process the latest intent (if any)
            viewMap.processIntent();

            // Inform the user about the app drawer
            if (!viewMap.prefs.getBoolean("bindDeviceAdminHintShown", false)) {

              MaterialDialog.Builder builder = new MaterialDialog.Builder(viewMap);
              builder.title(R.string.device_admin_info_dialog_title);
              builder.icon(viewMap.getResources().getDrawable(android.R.drawable.ic_dialog_info));
              builder.content(R.string.device_admin_info_dialog_description);
              builder.cancelable(false);
              builder.negativeText(viewMap.getString(R.string.button_label_do_not_show_again));
              builder.onNegative(new MaterialDialog.SingleButtonCallback() {
                 @Override
                 public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
                   SharedPreferences.Editor prefsEditor = viewMap.prefs.edit();
                   prefsEditor.putBoolean("bindDeviceAdminHintShown", true);
                   prefsEditor.commit();
                   showNavDrawerHint(viewMap);
                 }
               });
              builder.positiveText(viewMap.getString(R.string.button_label_ok));
              builder.onPositive(new MaterialDialog.SingleButtonCallback() {
                @Override
                public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
                  showNavDrawerHint(viewMap);
                }
              });
              builder.build().show();

            } else {
              showNavDrawerHint(viewMap);
            }
            commandExecuted=true;
          }
          if (commandFunction.equals("decideContinueOrNewTrack")) {
            viewMap.decideContinueOrNewTrack();
            commandExecuted=true;
          }
          if (commandFunction.equals("changeMapLayer")) {
            viewMap.changeMapLayer();
            commandExecuted=true;
          }
          if (commandFunction.equals("updateDownloadJobSize")) {
            MaterialDialog alert = viewMap.mapDownloadDialog;
            if (alert!=null) {
              alert.setContent(R.string.download_job_estimated_size_message,commandArgs.get(0),commandArgs.get(1));
              if (Integer.parseInt(commandArgs.get(2))==1) {
                alert.getActionButton(DialogAction.POSITIVE).setEnabled(false);
              } else {
                alert.getActionButton(DialogAction.POSITIVE).setEnabled(true);
              }
            }
            commandExecuted=true;
          }
          if (commandFunction.equals("askForMapDownloadDetails")) {
            viewMap.askForMapDownloadDetails(commandArgs.get(0));
            commandExecuted=true;
          }
          if (commandFunction.equals("showMenu")) {
            viewMap.viewMapRootLayout.openDrawer(GravityCompat.START);
            commandExecuted=true;
          }
          if (commandFunction.equals("decideWaypointImport")) {
            viewMap.nestedImportWaypointsDecisions++;
            MaterialDialog.Builder builder = new MaterialDialog.Builder(viewMap);
            builder.title(R.string.waypoint_import_title);
            builder.content(viewMap.getResources().getString(R.string.waypoint_import_message,commandArgs.get(1),commandArgs.get(0)));
            builder.cancelable(true);
            builder.positiveText(R.string.yes);
            builder.onPositive(new MaterialDialog.SingleButtonCallback() {
              @Override
              public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
                String path = "Navigation/Route[@name='"+commandArgs.get(0)+"']";
                viewMap.coreObject.configStoreSetStringValue(path,"importWaypoints","1");
                viewMap.nestedImportWaypointsDecisions--;
                if (viewMap.nestedImportWaypointsDecisions==0) {
                  viewMap.restartCore(false);
                }
              }
            });
            builder.negativeText(R.string.no);
            builder.onNegative(new MaterialDialog.SingleButtonCallback() {
              @Override
              public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
                String path = "Navigation/Route[@name='"+commandArgs.get(0)+"']";
                viewMap.coreObject.configStoreSetStringValue(path,"importWaypoints","2");
                viewMap.nestedImportWaypointsDecisions--;
              }
            });
            builder.icon(viewMap.getResources().getDrawable(android.R.drawable.ic_dialog_info));
            Dialog alert = builder.build();
            alert.show();
            commandExecuted=true;
          }
          if (commandFunction.equals("authenticateGoogleBookmarks")) {
            Intent intent = new Intent(viewMap, AuthenticateGoogleBookmarks.class);
            //intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            viewMap.startActivity(intent);
            commandExecuted=true;
          }
          if (commandFunction.equals("askForRouteRemovalKind")) {
            viewMap.askForRouteRemovalKind();
            commandExecuted=true;
          }
          if (!commandExecuted) {
            GDApplication.addMessage(GDApplication.ERROR_MSG, "GDApp", "unknown command " + command + " received");
          }
          break;
      }
    }

    // Inform the user about the app drawer
    protected void showNavDrawerHint(ViewMap viewMap) {
      if (!viewMap.prefs.getBoolean("navDrawerHintShown", false)) {
        Snackbar
            .make(viewMap.snackbarPosition,
                viewMap.getString(R.string.nav_drawer_hint), Snackbar.LENGTH_LONG)
            .setAction(R.string.got_it, new OnClickListener() {
              @Override
              public void onClick(View v) {
                ViewMap viewMap = weakViewMap.get();
                if (viewMap != null) {
                  SharedPreferences.Editor prefsEditor = viewMap.prefs.edit();
                  prefsEditor.putBoolean("navDrawerHintShown", true);
                  prefsEditor.commit();
                }
              }
            })
            .show();
      }
    }
  }
  CoreMessageHandler coreMessageHandler = new CoreMessageHandler(this);

  /** Sets the screen time out */
  @SuppressLint("Wakelock")
  void updateWakeLock() {
    if (mapSurfaceView!=null) {
      String state=coreObject.executeCoreCommand("getWakeLock");
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
      sensorManager.registerListener(coreObject, sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD), SensorManager.SENSOR_DELAY_NORMAL);
      compassWatchStarted=true;
    }
  }

  /** Stop listening for compass bearing */
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
    restartRequested=true;
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

    // Create the base dialog
    MaterialDialog.Builder builder = new MaterialDialog.Builder(this);
    builder.title(R.string.dialog_address_title);
    builder.icon(getResources().getDrawable(android.R.drawable.ic_dialog_info));
    builder.positiveText(R.string.finished);
    builder.cancelable(true);
    builder.customView(R.layout.dialog_address, false);
    final String address = new String();
    final MaterialDialog dialog=builder.build();
    final EditText editText = (EditText) dialog.getCustomView().findViewById(R.id.dialog_address_input_text);
    //builder.formatEditText(editText);
    editText.setText(text);
    editText.setImeOptions(EditorInfo.IME_ACTION_DONE);

    // Configure the list view
    final ListView listView = (ListView) dialog.getCustomView().findViewById(R.id.dialog_address_history_list);
    GDAddressHistoryAdapter addressAdapter = new GDAddressHistoryAdapter(this,dialog,listView);
    listView.setAdapter(addressAdapter);

    // Configure the views for editing a new address
    final ImageButton addAddressButton = (ImageButton) dialog.getCustomView().findViewById(R.id.dialog_address_history_entry_add_button);
    final OnClickListener addAddressOnClickListener = new OnClickListener() {
      @Override
      public void onClick(View v) {
        LocationFromAddressTask task = new LocationFromAddressTask();
        EditText editText = (EditText) dialog.getCustomView().findViewById(R.id.dialog_address_input_text);
        if (!editText.getText().equals("")) {
          task.subject = subject;
          task.viewMap = ViewMap.this;
          task.dialog = dialog;
          task.listView = listView;
          task.adapter = addressAdapter;
          task.text = editText.getText().toString();
          //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "text=" + task.text);
          task.execute();
        }
      }
    };
    addAddressButton.setOnClickListener(addAddressOnClickListener);
    DrawableCompat.setTintList(addAddressButton.getDrawable(),navDrawerList.getItemIconTintList());
    editText.setOnEditorActionListener(new EditText.OnEditorActionListener() {
      @Override
      public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
        if (actionId == EditorInfo.IME_ACTION_DONE) {
          addAddressOnClickListener.onClick(null);
          return true;
        }
        return false;
      }
    });
    final ImageButton clearAddressButton = (ImageButton) dialog.getCustomView().findViewById(R.id.dialog_address_history_entry_clear_button);
    DrawableCompat.setTintList(clearAddressButton.getDrawable(),navDrawerList.getItemIconTintList());
    clearAddressButton.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        editText.setText("");
      }
    });

    // Configure the views for handling group names
    final ImageView groupImage = (ImageView) dialog.getCustomView().findViewById(R.id.dialog_address_history_group_selector_image);
    DrawableCompat.setTintList(groupImage.getDrawable(),navDrawerList.getItemIconTintList());
    final LinearLayout groupSelector = (LinearLayout) dialog.getCustomView().findViewById(R.id.dialog_address_history_group_selector);
    final EditText groupNameEditText = (EditText) dialog.getCustomView().findViewById(R.id.dialog_address_history_group_name_edit_text);
    final ImageButton removeGroupButton = (ImageButton) dialog.getCustomView().findViewById(R.id.dialog_address_history_group_selector_remove_button);
    final ImageButton addGroupButton = (ImageButton) dialog.getCustomView().findViewById(R.id.dialog_address_history_group_selector_add_button);
    final ImageButton confirmGroupNameButton = (ImageButton) dialog.getCustomView().findViewById(R.id.dialog_address_history_group_name_confirm_button);
    final ImageButton clearGroupNameButton = (ImageButton) dialog.getCustomView().findViewById(R.id.dialog_address_history_group_name_clear_button);
    final Spinner groupSelectorSpinner = (Spinner) dialog.getCustomView().findViewById(R.id.dialog_address_history_group_selector_spinner);
    groupSelectorSpinner.setAdapter(addressAdapter.groupNamesAdapter);
    groupSelectorSpinner.setSelection(addressAdapter.groupNamesAdapter.getPosition(addressAdapter.selectedGroupName));
    groupSelectorSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
      @Override
      public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        String group = addressAdapter.groupNamesAdapter.getItem(position);
        coreObject.configStoreSetStringValue("Navigation","selectedAddressPointGroup",group);
        coreObject.executeCoreCommand("addressPointGroupChanged");
        addressAdapter.updateAddresses();
      }

      @Override
      public void onNothingSelected(AdapterView<?> parent) {
      }
    });
    DrawableCompat.setTintList(removeGroupButton.getDrawable(),navDrawerList.getItemIconTintList());
    removeGroupButton.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        String selectedGroupName = (String)groupSelectorSpinner.getSelectedItem();
        addressAdapter.removeGroupName(selectedGroupName);
      }
    });
    DrawableCompat.setTintList(addGroupButton.getDrawable(),navDrawerList.getItemIconTintList());
    addGroupButton.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        groupSelector.setVisibility(View.GONE);
        removeGroupButton.setVisibility(View.GONE);
        addGroupButton.setVisibility(View.GONE);
        groupNameEditText.setVisibility(View.VISIBLE);
        confirmGroupNameButton.setVisibility(View.VISIBLE);
        clearGroupNameButton.setVisibility(View.VISIBLE);
      }
    });
    DrawableCompat.setTintList(confirmGroupNameButton.getDrawable(),navDrawerList.getItemIconTintList());
    OnClickListener confirmGroupNameButtonOnClickListener = new OnClickListener() {
      @Override
      public void onClick(View v) {

        // Add the new group
        String newGroupName = groupNameEditText.getText().toString();
        addressAdapter.addGroupName(newGroupName);

        // Make the spinner visible again
        groupSelector.setVisibility(View.VISIBLE);
        removeGroupButton.setVisibility(View.VISIBLE);
        addGroupButton.setVisibility(View.VISIBLE);
        groupNameEditText.setVisibility(View.GONE);
        confirmGroupNameButton.setVisibility(View.GONE);
        clearGroupNameButton.setVisibility(View.GONE);
      }
    };
    confirmGroupNameButton.setOnClickListener(confirmGroupNameButtonOnClickListener);
    DrawableCompat.setTintList(clearGroupNameButton.getDrawable(),navDrawerList.getItemIconTintList());
    clearGroupNameButton.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        groupNameEditText.setText("");
      }
    });
    groupNameEditText.setOnEditorActionListener(new EditText.OnEditorActionListener() {
      @Override
      public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
        if (actionId == EditorInfo.IME_ACTION_DONE) {
          confirmGroupNameButtonOnClickListener.onClick(null);
          return true;
        }
        return false;
      }
    });
    dialog.show();
  }

  /** Asks the user for confirmation of the route name */
  void askForRouteName(Uri srcURI, final String routeName) {
    MaterialDialog.Builder builder = new MaterialDialog.Builder(this);
    final ImportRouteTask task = new ImportRouteTask();
    task.srcURI = srcURI;
    builder.title(R.string.dialog_route_name_title);
    builder.icon(getResources().getDrawable(android.R.drawable.ic_dialog_info));
    builder.customView(R.layout.dialog_route_name, false);
    builder.positiveText(R.string.finished);
    builder.negativeText(R.string.cancel);
    builder.cancelable(true);
    builder.callback(new MaterialDialog.ButtonCallback() {
      @Override
      public void onPositive(MaterialDialog dialog) {
        super.onPositive(dialog);
        task.execute();
      }
    });
    final MaterialDialog alert = builder.build();
    final EditText editText = (EditText) alert.getCustomView().findViewById(R.id.dialog_route_name_input_text);
    //builder.formatEditText(editText);
    editText.setText(gpxName);
    final TextView routeExistsTextView = (TextView) alert.getCustomView().findViewById(R.id.dialog_route_name_route_exists_warning);
    TextWatcher textWatcher = new TextWatcher() {
      public void onTextChanged(CharSequence s, int start, int before, int count) {

        // Check if route exists
        String dstFilename = GDCore.getHomeDirPath() + "/Route/" + s;
        final File dstFile = new File(dstFilename);
        if ((s.equals(""))||(!dstFile.exists()))
          routeExistsTextView.setVisibility(View.GONE);
        else
          routeExistsTextView.setVisibility(View.VISIBLE);
        alert.getCustomView().requestLayout();

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

  /** Asks the user if the core shall be resetted */
  void askForConfigReset() {
    MaterialDialog.Builder builder = new MaterialDialog.Builder(ViewMap.this);
    builder.title(getTitle());
    builder.content(R.string.reset_question);
    builder.cancelable(true);
    builder.positiveText(R.string.yes);
    builder.onPositive(new MaterialDialog.SingleButtonCallback() {
      @Override
      public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
        restartCore(true);
      }
    });
    builder.negativeText(R.string.no);
    builder.icon(getDrawable(android.R.drawable.ic_dialog_alert));
    Dialog alert = builder.build();
    alert.show();
  }

  /** Asks the user if the only the current zoom level or all zoom levels shall be removed */
  void askForMapCleanup() {
    MaterialDialog.Builder builder = new MaterialDialog.Builder(ViewMap.this);
    builder.title(getTitle());
    builder.content(R.string.map_cleanup_question);
    builder.cancelable(true);
    builder.positiveText(R.string.map_cleanup_all_zoom_levels);
    builder.onPositive(new MaterialDialog.SingleButtonCallback() {
      @Override
      public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
        coreObject.executeCoreCommand("forceMapRedownload","1");
      }
    });
    builder.negativeText(R.string.map_cleanup_current_zoom_level);
    builder.onNegative(new MaterialDialog.SingleButtonCallback() {
      @Override
      public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
        coreObject.executeCoreCommand("forceMapRedownload","0");
      }
    });
    builder.icon(getDrawable(android.R.drawable.ic_dialog_alert));
    Dialog alert = builder.build();
    alert.show();
  }

  /** Asks the user if the visible map or a map along a route shall be downloaded */
  void askForMapDownloadType() {
    MaterialDialog.Builder builder = new MaterialDialog.Builder(ViewMap.this);
    builder.title(getTitle());
    builder.content(R.string.map_download_type_question);
    builder.cancelable(true);
    builder.positiveText(R.string.map_download_type_option1);
    builder.onPositive(new MaterialDialog.SingleButtonCallback() {
      @Override
      public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
        askForMapDownloadDetails("");
      }
    });
    builder.negativeText(R.string.map_download_type_option2);
    builder.onNegative(new MaterialDialog.SingleButtonCallback() {
      @Override
      public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
        coreObject.executeCoreCommand("downloadActiveRoute");
      }
    });
    builder.icon(getResources().getDrawable(android.R.drawable.ic_dialog_alert));
    Dialog alert = builder.build();
    alert.show();
  }

  /** Asks the user which map layers of the visible map to download */
  void askForMapDownloadDetails(final String routeName) {
    String result=coreObject.executeCoreCommand("getMapLayers()");
    final String[] mapLayers=result.split(",");
    final String commandArgs=new String();
    final LinkedList<String> selectedMapLayers = new LinkedList<String>();
    MaterialDialog.Builder builder = new MaterialDialog.Builder(this);
    builder.title(R.string.download_job_level_selection_question);
    builder.items(mapLayers);
    builder.cancelable(true);
    builder.icon(getResources().getDrawable(android.R.drawable.ic_dialog_info));
    builder.positiveText(R.string.finished);
    builder.onPositive(new MaterialDialog.SingleButtonCallback() {
      @Override
      public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
        mapDownloadDialog = null;
        if (selectedMapLayers.size() > 0) {
          String [] args = new String[selectedMapLayers.size()+2];
          args[0]="0";
          args[1]=routeName;
          for (int i=0; i<selectedMapLayers.size(); i++){
            args[i+2]=selectedMapLayers.get(i);
          }
          coreObject.executeCoreCommand("addDownloadJob", args);
        }
      }
    });
    builder.negativeText(R.string.cancel);
    builder.onNegative(new MaterialDialog.SingleButtonCallback() {
      @Override
      public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
        mapDownloadDialog = null;
      }
    });
    builder.content(R.string.download_job_no_level_selected_message);
    builder.itemsCallbackMultiChoice(null, new MaterialDialog.ListCallbackMultiChoice() {
      @Override
      public boolean onSelection(MaterialDialog dialog, Integer[] which, CharSequence[] text) {
        Integer indexes[] = mapDownloadDialog.getSelectedIndices();
        selectedMapLayers.clear();
        for (Integer i : indexes) {
          selectedMapLayers.add(mapLayers[i]);
        }
        if (selectedMapLayers.size() > 0) {
          String [] args = new String[selectedMapLayers.size()+2];
          args[0]="1";
          args[1]=routeName;
          for (int i=0; i<selectedMapLayers.size(); i++){
            args[i+2]=selectedMapLayers.get(i);
          }
          coreObject.executeCoreCommand("addDownloadJob", args);
          mapDownloadDialog.setContent(R.string.download_job_estimating_size_message);
        } else {
          mapDownloadDialog.setContent(R.string.download_job_no_level_selected_message);
        }
        mapDownloadDialog.getActionButton(DialogAction.POSITIVE).setEnabled(false);
        return true;
      }
    });
    builder.alwaysCallMultiChoiceCallback();
    mapDownloadDialog = (MaterialDialog)builder.build();
    mapDownloadDialog.getActionButton(DialogAction.POSITIVE).setEnabled(false);
    mapDownloadDialog.show();
  }

  /** Asks the user which map legend to show */
  void askForMapLegend(final String[] names) {
    MaterialDialog.Builder builder = new MaterialDialog.Builder(this);
    builder.title(R.string.map_legend_selection_question);
    builder.items(names);
    builder.itemsCallbackSingleChoice(-1, new MaterialDialog.ListCallbackSingleChoice() {
      @Override
      public boolean onSelection(MaterialDialog dialog, View itemView, int which, CharSequence text) {
        showMapLegend(names[which]);
        dialog.dismiss();
        return true;
      }
    });
    builder.cancelable(true);
    builder.icon(getResources().getDrawable(android.R.drawable.ic_dialog_info));
    builder.negativeText(R.string.cancel);
    Dialog alert = builder.build();
    alert.show();
  }

  /** Asks the user if the route shall just be hidden or deleted */
  void askForRouteRemovalKind() {
    MaterialDialog.Builder builder = new MaterialDialog.Builder(ViewMap.this);
    builder.title(getTitle());
    builder.content(R.string.route_remove_question);
    builder.cancelable(true);
    builder.positiveText(R.string.remove);
    builder.onPositive(new MaterialDialog.SingleButtonCallback() {
      @Override
      public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
        coreObject.executeCoreCommand("trashPath");
      }
    });
    builder.negativeText(R.string.hide);
    builder.onNegative(new MaterialDialog.SingleButtonCallback() {
      @Override
      public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
        coreObject.executeCoreCommand("hidePath");
      }
    });
    builder.icon(getDrawable(android.R.drawable.ic_dialog_alert));
    Dialog alert = builder.build();
    alert.show();
  }

  /** Shows the legend with the given name */
  void showMapLegend(String name) {
    String legendPath = coreObject.executeCoreCommand("getMapLegendPath", name);
    File legendPathFile = new File(legendPath);
    if (!legendPathFile.exists()) {
      errorDialog(getString(R.string.map_has_no_legend,coreObject.executeCoreCommand("getMapFolder")));
    } else {
      Intent intent = new Intent();
      intent.setAction(Intent.ACTION_VIEW);
      if (legendPath.endsWith(".png"))
        intent.setDataAndType(Uri.parse("file://" + legendPath), "image/*");
      if (legendPath.endsWith(".pdf"))
        intent.setDataAndType(Uri.parse("file://" + legendPath), "application/pdf");
      startActivity(intent);
    }
  }

  /** Asks the user if the track shall be continued or a new track shall be started */
  void decideContinueOrNewTrack() {
    MaterialDialog.Builder builder = new MaterialDialog.Builder(ViewMap.this);
    builder.title(getTitle());
    builder.content(R.string.continue_or_new_track_question);
    builder.cancelable(true);
    builder.positiveText(R.string.new_track);
    builder.onPositive(new MaterialDialog.SingleButtonCallback() {
      @Override
      public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
        coreObject.executeCoreCommand("setRecordTrack","1");
        coreObject.executeCoreCommand("createNewTrack");
      }
    });
    builder.negativeText(R.string.contine_track);
    builder.onNegative(new MaterialDialog.SingleButtonCallback() {
      @Override
      public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
        coreObject.executeCoreCommand("setRecordTrack","1");
      }
    });
    builder.icon(getResources().getDrawable(android.R.drawable.ic_dialog_info));
    Dialog alert = builder.build();
    alert.show();
  }

  /** Asks the user if which map layer shall be selected */
  void changeMapLayer() {
    String layers=coreObject.executeCoreCommand("getMapLayers");
    String selectedLayer=coreObject.executeCoreCommand("getSelectedMapLayer");
    final String[] mapLayers=layers.split(",");
    int index=-1;
    for (int i=0;i<mapLayers.length;i++) {
      if (mapLayers[i].equals(selectedLayer)) {
        index = i;
        break;
      }
    }
    MaterialDialog.Builder builder = new MaterialDialog.Builder(this);
    builder.title(R.string.map_layer_selection_question);
    builder.items(mapLayers);
    builder.itemsCallbackSingleChoice(index, new MaterialDialog.ListCallbackSingleChoice() {
      @Override
      public boolean onSelection(MaterialDialog dialog, View itemView, int which, CharSequence text) {
        coreObject.executeCoreCommand("selectMapLayer",mapLayers[which]);
        dialog.dismiss();
        return true;
      }
    });
    builder.cancelable(true);
    builder.icon(getResources().getDrawable(android.R.drawable.ic_dialog_info));
    builder.negativeText(R.string.cancel);
    Dialog alert = builder.build();
    alert.show();
  }

  /** Checks if routes have been updated */
  private class CheckForOutdatedRoutesTask extends AsyncTask<Void, Integer, Void> {

    boolean routesOutdated=false;

    protected void onPreExecute() {
    }

    protected Void doInBackground(Void... params) {

      // Go through all route files
      File routeDir = new File(coreObject.homePath+"/Route");
      String cacheDir = coreObject.homePath+"/Route/.cache";
      for (File routeFile : routeDir.listFiles()) {
        //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",routeFile.getName());
        if (routeFile.isDirectory())
          continue;
        if (routeFile.getName().endsWith(".gpx")) {
          File cacheFile = new File(cacheDir+"/"+routeFile.getName());
          if (!cacheFile.exists()) {
            routesOutdated=true;
            break;
          }
          if (routeFile.lastModified()>cacheFile.lastModified()) {
            routesOutdated=true;
            break;
          }
        }
      }
      return null;
    }

    protected void onProgressUpdate(Integer... progress) {
    }

    protected void onPostExecute(Void result) {
      if (routesOutdated) {
        Toast.makeText(ViewMap.this,R.string.routes_outdated,Toast.LENGTH_LONG).show();
        restartCore(false);
      }
    }
  }


  /** Copies tracks from the Track into the Route directory */
  private class CopyTracksTask extends AsyncTask<Void, Integer, Void> {

    MaterialDialog progressDialog;
    public String[] trackNames;

    protected void onPreExecute() {

      // Prepare the progress dialog
      progressDialog = new MaterialDialog.Builder(ViewMap.this)
        .content(getString(R.string.copying_tracks))
        .progress(false,trackNames.length,true)
        .cancelable(false)
        .show();
    }

    protected Void doInBackground(Void... params) {

      // Copy all selected tracks to the route directory
      Integer progress = 0;
      for (String trackName : trackNames) {
        String srcFilename = coreObject.homePath + "/Track/" + trackName;
        String dstFilename = coreObject.homePath + "/Route/" + trackName;
        try {
          GDTools.copyFile(srcFilename, dstFilename);
        } catch (IOException exception) {
          GDApplication.showMessageBar(ViewMap.this, String.format(getString(R.string.cannot_copy_file), srcFilename, dstFilename), GDApplication.MESSAGE_BAR_DURATION_LONG);
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

  /** Sends logs via ACRA in the background */
  private class SendLogsTask extends AsyncTask<Void, Integer, Void> {

    public String[] logNames;

    protected void onPreExecute() {
    }

    protected Void doInBackground(Void... params) {

      // Read all logs into a string
      infoDialog("Please wait while log files are read in the background");
      String logContents = "";
      try {
        for (String logName : logNames) {
          String logPath = coreObject.homePath + "/Log/" + logName;
          logContents += logPath + ":\n";
          BufferedReader logReader = new BufferedReader(new FileReader(logPath));
          String inputLine;
          while ((inputLine=logReader.readLine())!=null) {
            logContents += inputLine + "\n";
            //GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDApp",inputLine);
          }
          logContents += "\n";
          logReader.close();
        }
      }
      catch (IOException e) {
        errorDialog(getString(R.string.send_logs_failed, e.getMessage()));
      }

      // Add the log to the ACRA report
      ACRA.getErrorReporter().putCustomData("userLogContents",logContents);

      // Send report via ACRA
      Exception e = new Exception("User has sent logs");
      ACRA.getErrorReporter().handleException(e,false);

      return null;
    }

    protected void onProgressUpdate(Integer... progress) {
    }

    protected void onPostExecute(Void result) {
    }
  }

  /** Copies tracks from the Track into the Route directory */
  private class ImportRouteTask extends AsyncTask<Void, Integer, Void> {

    MaterialDialog progressDialog;
    public String name;
    public Uri srcURI;
    public String dstFilename;

    protected void onPreExecute() {

      // Prepare the progress dialog
      progressDialog = new MaterialDialog.Builder(ViewMap.this)
      .content(getString(R.string.importing_route,name))
      .progress(true,0)
      .cancelable(false)
      .show();
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
          GDTools.copyFile(gpxContents, dstFilename);
          gpxContents.close();
        }
        catch (IOException e) {
          GDApplication.showMessageBar(ViewMap.this, String.format(getString(R.string.cannot_import_route), name), GDApplication.MESSAGE_BAR_DURATION_LONG);
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

  /** Imports *.gda files from external source */
  private class ImportMapArchivesTask extends AsyncTask<Void, Integer, Void> {

    MaterialDialog progressDialog;
    public File srcFile;
    public File mapFolder;
    public String mapInfoFilename;

    protected void onPreExecute() {

      // Prepare the progress dialog
      progressDialog = new MaterialDialog.Builder(ViewMap.this)
      .content(getString(R.string.importing_external_map_archives,srcFile.getName()))
      .progress(true,0)
      .cancelable(false)
      .show();
    }

    protected Void doInBackground(Void... params) {

      // Create the map folder
      try {

        // Find all gda files that have the pattern <name>.gda and <name>%d.gda
        String basename = srcFile.getName().replaceAll("(\\D*)\\d*\\.gda", "$1");
        LinkedList<String> archives = new LinkedList<String>();
        File dir = new File(srcFile.getParent());
        File[] dirListing = dir.listFiles();
        if (dirListing != null) {
          for (File child : dirListing) {
            if (child.getName().matches(basename + "\\d*\\.gda")) {
              archives.add(child.getAbsolutePath());
            }
          }
        }

        // Create the info file
        mapFolder.mkdir();
        File cache = new File(mapFolder.getPath() + "/cache.bin");
        cache.delete();
        PrintWriter writer = new PrintWriter(mapInfoFilename,"UTF-8");
        writer.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
        writer.println("<GDS version=\"1.0\">");
        writer.println("  <type>externalMapArchive</type>");
        for (String archive : archives) {
          writer.println(String.format("  <mapArchivePath>%s</mapArchivePath>",archive));
        }
        writer.println("</GDS>");
        writer.close();

        // Set the new folder
        coreObject.configStoreSetStringValue("Map", "folder", mapFolder.getName());

      }
      catch(IOException e) {
        errorDialog(String.format(getString(R.string.cannot_create_map_folder), mapFolder.getName()));
      }

      return null;
    }

    protected void onProgressUpdate(Integer... progress) {
    }

    protected void onPostExecute(Void result) {

      // Close the progress dialog
      progressDialog.dismiss();

      // Restart the core to load the new map folder
      restartCore(false);
    }
  }

  /** Send logs via ACRA */
  void sendLogs() {

    // Obtain the list of file in the folder
    File folderFile = new File(coreObject.homePath + "/Log");
    LinkedList<String> logs = new LinkedList<String>();
    for (File file : folderFile.listFiles()) {
      if ((!file.isDirectory())
          && (!file.getName().substring(file.getName().length() - 1).equals("~"))
          && (!file.getName().equals("send.log"))
          && (!file.getName().endsWith(".dmp"))) {
        logs.add(file.getName());
      }
    }
    final String[] items = new String[logs.size()];
    logs.toArray(items);
    Arrays.sort(items, new Comparator<String>() {

      Comparator<String> stringComparator = Collections.reverseOrder();

      @Override
      public int compare(String lhs, String rhs) {
        String pattern = "^(.*)-(\\d\\d\\d\\d\\d\\d\\d\\d-\\d\\d\\d\\d\\d\\d)\\.log$";
        if (lhs.matches(pattern)) {
          lhs = lhs.replaceAll(pattern, "$2-$1.log");
        }
        if (rhs.matches(pattern)) {
          rhs = rhs.replaceAll(pattern, "$2-$1.log");
        }
        return stringComparator.compare(lhs, rhs);
      }

    });

    // Create the dialog
    MaterialDialog.Builder builder = new MaterialDialog.Builder(this);
    builder.title(R.string.log_selection_question);
    builder.items(items);
    builder.cancelable(true);
    builder.positiveText(R.string.finished);
    builder.negativeText(R.string.cancel);
    builder.icon(getResources().getDrawable(android.R.drawable.ic_dialog_info));
    builder.itemsCallbackMultiChoice(null, new MaterialDialog.ListCallbackMultiChoice() {
      @Override
      public boolean onSelection(MaterialDialog dialog, Integer[] which,
          CharSequence[] text) {
        SendLogsTask sendLogsTask = new SendLogsTask();
        LinkedList<String> logNames = new LinkedList<String>();
        for (int i = 0; i < which.length; i++) {
          logNames.add(items[which[i]]);
        }
        if (logNames.size() == 0)
          return true;
        sendLogsTask.logNames = new String[logNames.size()];
        logNames.toArray(sendLogsTask.logNames);
        sendLogsTask.execute();
        return true;
      }
    });
    builder.show();
  }

  /** Opens the donate activity */
  void donate() {
    //Intent intent = new Intent(getApplicationContext(), Donate.class);
    //startActivity(intent);
    warningDialog("Coming soon!");
  }

  /** Show welcome dialog */
  void openWelcomeDialog() {

    // Create the dialog
    MaterialDialog.Builder builder = new MaterialDialog.Builder(this);
    builder.title(R.string.welcome_dialog_title);
    builder.content(R.string.welcome_dialog_message);
    builder.cancelable(true);
    builder.positiveText(R.string.button_label_ok);
    builder.neutralText(R.string.button_label_donate);
    builder.onNeutral(new MaterialDialog.SingleButtonCallback() {
      @Override
      public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
        donate();
      }
    });
    builder.negativeText(R.string.button_label_help);
    builder.onNegative(new MaterialDialog.SingleButtonCallback() {
      @Override
      public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
        Intent intent = new Intent(getApplicationContext(), ShowHelp.class);
        startActivity(intent);
      }
    });
    builder.icon(getResources().getDrawable(android.R.drawable.ic_dialog_info));
    Dialog alert = builder.build();
    alert.show();
  }

  /** Copies tracks to the route directory */
  void addTracksAsRoutes() {

    // Obtain the list of file in the folder
    File folderFile = new File(coreObject.homePath + "/Track");
    LinkedList<String> routes = new LinkedList<String>();
    for (File file : folderFile.listFiles()) {
      if ((!file.isDirectory())
          && (!file.getName().substring(file.getName().length() - 1).equals("~"))
          && (!file.getName().substring(file.getName().length() - 4).equals(".bin")))
      {
        routes.add(file.getName());
      }
    }
    final String[] items = new String[routes.size()];
    routes.toArray(items);
    Arrays.sort(items, Collections.reverseOrder());

    // Create the dialog
    MaterialDialog.Builder builder = new MaterialDialog.Builder(this);
    builder.title(R.string.track_as_route_selection_question);
    builder.cancelable(true);
    builder.positiveText(R.string.finished);
    builder.items(items);
    builder.itemsCallbackMultiChoice(null, new MaterialDialog.ListCallbackMultiChoice() {
      @Override
      public boolean onSelection(MaterialDialog dialog, Integer[] which,
                                 CharSequence[] text) {
        CopyTracksTask copyTracksTask = new CopyTracksTask();
        LinkedList<String> trackNames = new LinkedList<String>();
        for (int i = 0; i < which.length; i++) {
          trackNames.add(items[which[i]]);
        }
        if (trackNames.size() == 0)
          return true;
        copyTracksTask.trackNames = new String[trackNames.size()];
        trackNames.toArray(copyTracksTask.trackNames);
        copyTracksTask.execute();
        return true;
      }
    });
    builder.negativeText(R.string.cancel);
    builder.icon(getResources().getDrawable(android.R.drawable.ic_dialog_info));
    builder.show();
  }

  /** Removes routes from the Route directory */
  private class RemoveRoutesTask extends AsyncTask<Void, Integer, Void> {

    MaterialDialog progressDialog;
    public String[] routeNames;

    protected void onPreExecute() {

      // Prepare the progress dialog
      progressDialog = new MaterialDialog.Builder(ViewMap.this)
      .content(getString(R.string.removing_routes))
      .progress(false,routeNames.length,true)
      .cancelable(false)
      .show();
    }

    protected Void doInBackground(Void... params) {

      // Copy all selected tracks to the route directory
      Integer progress = 0;
      for (String routeName : routeNames) {
        File route = new File(coreObject.homePath + "/Route/" + routeName + ".bin");
        route.delete(); // Don't care if .bin does not exist
        route = new File(coreObject.homePath + "/Route/" + routeName);
        if (!route.delete()) {
          GDApplication.showMessageBar(ViewMap.this, String.format(getString(R.string.cannot_remove_file), route.getPath()), GDApplication.MESSAGE_BAR_DURATION_LONG);
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

  /** Finds the geographic position for the given address */
  private class LocationFromAddressTask extends AsyncTask<Void, Void, Void> {

    String subject;
    String text;
    ListView listView;
    ViewMap viewMap;
    MaterialDialog dialog;
    GDAddressHistoryAdapter adapter;
    boolean locationFound=false;
    ArrayList<String> validAddressLines = new ArrayList<String>();

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
            //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "addressLine=" + addressLine);
            coreObject.scheduleCoreCommand("addAddressPoint",
                addressLine, addressLine,
                String.valueOf(address.getLongitude()), String.valueOf(address.getLatitude()),
                adapter.selectedGroupName);
            coreObject.scheduleCoreCommand("setTargetAtGeographicCoordinate",
                String.valueOf(address.getLongitude()),
                String.valueOf(address.getLatitude()));
            validAddressLines.add(addressLine);
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
      } else {
        GDAddressHistoryAdapter adapter=(GDAddressHistoryAdapter)listView.getAdapter();
        adapter.updateAddresses();
      }
    }
  }

  /** Removes routes from the route directory */
  void removeRoutes() {

    // Obtain the list of file in the folder
    File folderFile = new File(coreObject.homePath + "/Route");
    LinkedList<String> routes = new LinkedList<String>();
    for (File file : folderFile.listFiles()) {
      if ((!file.isDirectory())
          && (!file.getName().substring(file.getName().length() - 1).equals("~"))
          && (!file.getName().substring(file.getName().length() - 4).equals(".bin")))
      {
        routes.add(file.getName());
      }
    }
    final String[] items = new String[routes.size()];
    routes.toArray(items);
    Arrays.sort(items);

    // Create the dialog
    MaterialDialog.Builder builder = new MaterialDialog.Builder(this);
    builder.title(R.string.route_remove_selection_question);
    builder.cancelable(true);
    builder.items(items);
    builder.positiveText(R.string.finished);
    builder.negativeText(R.string.cancel);  
    builder.itemsCallbackMultiChoice(null, new MaterialDialog.ListCallbackMultiChoice() {
      @Override
      public boolean onSelection(MaterialDialog dialog, Integer[] which,
          CharSequence[] text) {
        RemoveRoutesTask removeRoutesTask = new RemoveRoutesTask();
        LinkedList<String> routeNames = new LinkedList<String>();
        for (int i = 0; i < which.length; i++) {
          routeNames.add(items[which[i]]);
        }
        if (routeNames.size() == 0)
          return true;
        removeRoutesTask.routeNames = new String[routeNames.size()];
        routeNames.toArray(removeRoutesTask.routeNames);
        removeRoutesTask.execute();
        return true;
      }
    });
    builder.icon(getResources().getDrawable(android.R.drawable.ic_dialog_info));
    builder.show();
  }

  /** Downloads files extracted from intents */
  @TargetApi(Build.VERSION_CODES.GINGERBREAD)
  void downloadRoute(final Uri srcURI) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {

      // Create some variables
      final String name = srcURI.getLastPathSegment();
      final String dstFilename = GDCore.getHomeDirPath() + "/Route/" + name;

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
        MaterialDialog.Builder builder = new MaterialDialog.Builder(this);
        builder.title(getTitle());
        String message;
        if (dstFile.exists())
          message=getString(R.string.overwrite_route_question);
        else
          message=getString(R.string.copy_route_question);
        message = String.format(message, name);
        builder.content(message);
        builder.cancelable(true);
        builder.positiveText(R.string.yes);
        builder.onPositive(new MaterialDialog.SingleButtonCallback() {
          @Override
          public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
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
        builder.negativeText(R.string.no);
        builder.icon(getResources().getDrawable(android.R.drawable.ic_dialog_info));
        Dialog alert = builder.build();
        alert.show();
      }

    } else {
      errorDialog(getString(R.string.download_manager_not_available));
    }

  }

  /** Prepares activity for functions only available on gingerbread */
  @TargetApi(Build.VERSION_CODES.GINGERBREAD)
  void onCreateGingerbread() {
    /*StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder()
        .detectAll()
        .penaltyLog()
        .build());
    StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder()
        .detectAll()
        .penaltyLog()
        .build());*/
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
      downloadManager = (DownloadManager) getSystemService(Context.DOWNLOAD_SERVICE);
      IntentFilter filter = new IntentFilter();
      filter.addAction(DownloadManager.ACTION_DOWNLOAD_COMPLETE);
      downloadCompleteReceiver = new BroadcastReceiver() {

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
      registerReceiver(downloadCompleteReceiver, filter);
    }
  }

  /** Sets the exit busy text */
  public void setExitBusyText() {
    busyTextView.setText(" " + getString(R.string.stopping_core_object) + " ");
  }

  /** Exits the app */
  void exitApp() {
    setExitBusyText();
    Message m=Message.obtain(coreObject.messageHandler);
    m.what = GDCore.STOP_CORE;
    coreObject.messageHandler.sendMessage(m);
  }

  /** Called when the activity is first created. */
  @Override
  public void onCreate(Bundle savedInstanceState) {

    super.onCreate(savedInstanceState);
    restartRequested=false;
    exitRequested=false;

    // Check for OpenGL ES 2.00
    final ActivityManager activityManager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
    final ConfigurationInfo configurationInfo = activityManager.getDeviceConfigurationInfo();
    final boolean supportsEs2 = (configurationInfo.reqGlEsVersion >= 0x20000) || (Build.FINGERPRINT.startsWith("generic"));
    if (!supportsEs2)
    {
      fatalDialog(getString(R.string.opengles20_required));
      return;
    }

    // Restore the last processed intent from the prefs
    prefs = getApplication().getSharedPreferences("viewMap",Context.MODE_PRIVATE);
    if (prefs.contains("processIntent")) {
      boolean processIntent = prefs.getBoolean("processIntent", true);
      if (!processIntent) {
        getIntent().addFlags(Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY);
      }
      SharedPreferences.Editor prefsEditor = prefs.edit();
      prefsEditor.putBoolean("processIntent", true);
      prefsEditor.commit();
    }

    // Get the core object
    coreObject=GDApplication.coreObject;
    ((GDApplication)getApplication()).setActivity(this);

    // Get important handles
    locationManager = (LocationManager) this.getSystemService(Context.LOCATION_SERVICE);
    sensorManager = (SensorManager) this.getSystemService(Context.SENSOR_SERVICE);
    powerManager = (PowerManager) this.getSystemService(Context.POWER_SERVICE);
    devicePolicyManager = (DevicePolicyManager)this.getSystemService(Context.DEVICE_POLICY_SERVICE);
    notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);

    // Prepare the window contents
    setContentView(R.layout.view_map);
    mapSurfaceView = (GDMapSurfaceView) findViewById(R.id.view_map_map_surface_view);
    mapSurfaceView.setCoreObject(coreObject);
    messageView = (TextView) findViewById(R.id.view_map_message_text_view);
    busyTextView = (TextView) findViewById(R.id.view_map_busy_text_view);
    viewMapRootLayout = (DrawerLayout) findViewById(R.id.view_map_root_layout);
    messageLayout = (LinearLayout) findViewById(R.id.view_map_message_layout);
    splashLayout = (LinearLayout) findViewById(R.id.view_map_splash_layout);
    navDrawerList = (NavigationView) findViewById(R.id.view_map_nav_drawer);
    snackbarPosition = (CoordinatorLayout) findViewById(R.id.view_map_snackbar_position);
    renderBugFixView = (View) findViewById(R.id.view_map_render_bug_fix_view);

    PackageManager packageManager = getPackageManager();
    String appVersion;
    try {
      appVersion = "Version " + packageManager.getPackageInfo(getPackageName(), 0).versionName;
    }
    catch(NameNotFoundException e) {
      appVersion = "Version ?";
    }
    View headerLayout = navDrawerList.getHeaderView(0);
    TextView navDrawerAppVersion = (TextView) headerLayout.findViewById(R.id.nav_drawer_app_version);
    navDrawerAppVersion.setText(appVersion);
    navDrawerList.setNavigationItemSelectedListener(new NavigationView.OnNavigationItemSelectedListener() {
      @Override
      public boolean onNavigationItemSelected(MenuItem menuItem) {
        Intent intent;
        switch (menuItem.getItemId()) {
          case R.id.nav_preferences:
            if (mapSurfaceView!=null) {
              Intent myIntent = new Intent(getApplicationContext(), Preferences.class);
              startActivityForResult(myIntent, SHOW_PREFERENCE_REQUEST);
            }
            break;
          case R.id.nav_reset:
            if (mapSurfaceView!=null) {
              askForConfigReset();
            }
            break;
          case R.id.nav_exit:
            exitApp();
            break;
          case R.id.nav_restart:
            restartCore(false);
            break;
          case R.id.nav_toggle_messages:
            if (messageLayout.getVisibility()==LinearLayout.VISIBLE) {
              messageLayout.setVisibility(LinearLayout.INVISIBLE);
            } else {
              messageView.setText(GDApplication.messages);
              messageLayout.setVisibility(LinearLayout.VISIBLE);
            }
            viewMapRootLayout.requestLayout();
            break;
          case R.id.nav_add_tracks_as_routes:
            addTracksAsRoutes();
            break;
          case R.id.nav_remove_routes:
            removeRoutes();
            break;
          case R.id.nav_show_legend:
            String namesString = coreObject.executeCoreCommand("getMapLegendNames");
            String names[] = namesString.split(",");
            if (names.length!=1) {
              askForMapLegend(names);
            } else {
              showMapLegend(names[0]);
            }
            break;
          case R.id.nav_show_help:
            intent = new Intent(getApplicationContext(), ShowHelp.class);
            startActivity(intent);
            break;
          case R.id.nav_show_brouter:
            /*intent = new Intent(getApplicationContext(), ShowBRouter.class);
            startActivity(intent);*/
            String url = "http://localhost:8383/brouter-web/index.html";
            Intent i = new Intent(Intent.ACTION_VIEW);
            i.setData(Uri.parse(url));
            startActivity(i);
            break;
          case R.id.nav_send_logs:
            sendLogs();
            break;
          /*case R.id.nav_donate:
            donate();
            break;*/
          case R.id.nav_map_cleanup:
            if (mapSurfaceView!=null) {
              askForMapCleanup();
            }
            break;
          case R.id.nav_map_download:
            if (mapSurfaceView!=null) {
              askForMapDownloadType();
            }
            break;
          case R.id.nav_export_selected_route:
            coreObject.executeCoreCommand("exportActiveRoute");
            break;
          case R.id.nav_sync_google_bookmarks:
            coreObject.executeCoreCommand("updateGoogleBookmarks");
            break;
        }
        viewMapRootLayout.closeDrawers();
        return true;
      }
    });
    viewMapRootLayout.setDrawerShadow(R.drawable.navigationview_shadow, GravityCompat.START);
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

    // Show welcome dialog
    //openWelcomeDialog();
  }

  /** Called when the app suspends */
  @Override
  public void onPause() {
    super.onPause();
    GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "onPause called by " + Thread.currentThread().getName());
    if (mapSurfaceView!=null)
      mapSurfaceView.onPause();
    stopWatchingCompass();
    if ((!exitRequested)&&(!restartRequested)) {
      Intent intent = new Intent(this, GDService.class);
      intent.setAction("activityPaused");
      startService(intent);
    }
    GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "onPause finished");
  }

  /** Called when the app resumes */
  @Override
  public void onResume() {
    super.onResume();
    GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "onResume called by " + Thread.currentThread().getName());

    // Bug fix: Somehow the emulator calls onResume before onCreate
    // But the code relies on the fact that onCreate is called before
    // Do nothing if onCreate has not yet initialized the objects
    if (coreObject==null)
      return;

    // Resume the map surface view
    mapSurfaceView.onResume();

    // If we shall restart or exit, don't init anything here
    if ((exitRequested)||(restartRequested))
      return;

    // Resume all components only if a exit or restart is not requested
    startWatchingCompass();
    updateWakeLock();
    Intent intent = new Intent(this, GDService.class);
    intent.setAction("activityResumed");
    startService(intent);

    // Synchronize google bookmarks
    GDApplication.coreObject.executeCoreCommand("updateGoogleBookmarks");

    // Check for outdated routes
    if (coreObject.coreInitialized) {
      CheckForOutdatedRoutesTask checkForOutdatedRoutesTask = new CheckForOutdatedRoutesTask();
      checkForOutdatedRoutesTask.execute();
    }

    // Process intent only if geo discoverer is initialized
    if (coreObject.coreLateInitComplete)
      processIntent();

    // Schedule render bug fix
    renderBugFixView.setVisibility(View.VISIBLE);
    Handler handler = new Handler();
    handler.postDelayed(new Runnable() {
      @Override
      public void run() {
        renderBugFixView.setVisibility(View.INVISIBLE);
      }
    }, 100);
  }

  // Processes the intent of the activity
  private void processIntent() {

    // Extract the file path from the intent
    Intent intent;
    intent = getIntent();
    Uri uri = null;
    String subject = "";
    String text = "";
    boolean isAddress = false;
    boolean isGDA = false;
    boolean isGPXFromWeb = false;
    boolean isGPXFromFile = false;
    if (intent!=null) {
      if (Intent.ACTION_SEND.equals(intent.getAction())) {
        Bundle extras = intent.getExtras();
        if (extras.containsKey(Intent.EXTRA_STREAM)) {
          uri = (Uri) extras.getParcelable(Intent.EXTRA_STREAM);
          if (uri.getScheme().equals("http") || uri.getScheme().equals("https")) {
            isGPXFromWeb=true;
          } else {
            isGPXFromFile=true;
          }
        } else if (extras.containsKey(Intent.EXTRA_TEXT)) {
          subject=extras.getString(Intent.EXTRA_SUBJECT);
          text=extras.getString(Intent.EXTRA_TEXT);
          isAddress=true;
        } else {
          warningDialog(getString(R.string.unsupported_intent));
        }
      }
      if (Intent.ACTION_VIEW.equals(intent.getAction())) {
        uri = intent.getData();
        if (uri.getLastPathSegment().endsWith(".gda")) {
          isGDA=true;
        } else if (uri.getScheme().equals("http") || uri.getScheme().equals("https")) {
          isGPXFromWeb=true;
        } else {
          isGPXFromFile=true;
        }
      }
    }

    // Check if the intent was seen
    if (intent!=null) {
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "intent=" + intent.toString());
      intent.toUri(0);
      if ((intent.getFlags()&Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY)!=0) {
        isGDA=false;
        isGPXFromFile=false;
        isGPXFromWeb=false;
        isAddress=false;
      }
    }

    // Handle the intent
    if (isAddress) {
      askForAddress(subject, text);
    }
    if (isGDA) {

      // Receive the file path
      String filePath = null;
      if (uri.getScheme().equals("file")) {
        filePath = uri.getPath();
      } else {
        if (uri.getScheme().equals("content")) {
          filePath = uri.getPath();
        } else {
          GDApplication.addMessage(GDApplication.ERROR_MSG,"GDApp","scheme " + uri.getScheme() + " not supported!");
        }
      }
      //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","filePath=" + filePath);

      // Ask the user if the file should be linked to the map directory
      final File srcFile = new File(filePath);
      if (srcFile.exists()) {

        // Map archive?
        if (uri.getPath().endsWith(".gda")) {

          // Ask the user if a new map shall be created based on the archive
          String mapFolderFilename = GDCore.getHomeDirPath() + "/Map/" + srcFile.getName();
          mapFolderFilename = mapFolderFilename.substring(0, mapFolderFilename.lastIndexOf('.'));
          final String mapInfoFilename = mapFolderFilename + "/info.gds";
          final File mapFolder = new File(mapFolderFilename);
          MaterialDialog.Builder builder = new MaterialDialog.Builder(this);
          builder.title(R.string.import_map);
          String message;
          if (mapFolder.exists())
            message=getString(R.string.replace_map_folder_question,mapFolder.getName(),srcFile);
          else
            message=getString(R.string.create_map_folder_question,mapFolder.getName(),srcFile);
          builder.content(message);
          builder.cancelable(true);
          builder.positiveText(R.string.yes);
          builder.onPositive(new MaterialDialog.SingleButtonCallback() {
            @Override
            public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
              ImportMapArchivesTask task = new ImportMapArchivesTask();
              task.srcFile = srcFile;
              task.mapFolder = mapFolder;
              task.mapInfoFilename = mapInfoFilename;
              task.execute();
            }
          });
          builder.negativeText(R.string.no);
          builder.icon(getResources().getDrawable(android.R.drawable.ic_dialog_info));
          Dialog alert = builder.build();
          alert.show();
        }

      } else {
        errorDialog(getString(R.string.file_does_not_exist));
      }
    }
    if (isGPXFromWeb) {
      downloadRoute(uri);
    }
    if (isGPXFromFile) {

      // Get the name of the GPX file
      gpxName=null;
      if (uri.getScheme().equals("content")) {
        Cursor cursor = getContentResolver().query(uri, null, null, null, null);
        try {
          if (cursor != null && cursor.moveToFirst()) {
            gpxName = cursor.getString(cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME));
          }
        } finally {
          cursor.close();
        }
      }
      if (gpxName == null) {
        gpxName = uri.getLastPathSegment();
      }
      if (!gpxName.endsWith(".gpx")) {
        gpxName = gpxName + ".gpx";
      }
      askForRouteName(uri,gpxName);
    }
    setIntent(null);
  }

  /** Called before the activity is put into background (outState is non persistent!) */
  @Override
  protected void onSaveInstanceState(Bundle outState) {
  }

  /** Called when a new intent is available */
  @Override
  public void onNewIntent(Intent intent) {
    setIntent(intent);
    GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "onNewIntent: " + intent.toString());
  }

  /** Called when the activity is destroyed */
  @SuppressLint("Wakelock")
  @Override
  public void onDestroy() {
    super.onDestroy();
    GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "onDestroy called by " + Thread.currentThread().getName());
    ((GDApplication)getApplication()).setActivity(null);
    if ((wakeLock!=null)&&(wakeLock.isHeld()))
      wakeLock.release();
    if (downloadCompleteReceiver!=null)
      unregisterReceiver(downloadCompleteReceiver);
    if (exitRequested)
      System.exit(0);
    if (restartRequested) {
      if (Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB) {
        Intent intent = new Intent(getApplicationContext(), ViewMap.class);
        startActivity(intent);
      }
    }
  }

  /** Called when a configuration change (e.g., caused by a screen rotation) has occured */
  public void onConfigurationChanged(Configuration newConfig) {

    super.onConfigurationChanged(newConfig);
    updateViewConfiguration(newConfig);

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
            GDApplication.showMessageBar(this, getString(R.string.device_admin_not_enabled), GDApplication.MESSAGE_BAR_DURATION_LONG);
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

  @Override
  public void onBackPressed() {
    if (doubleBackToExitPressedOnce) {
      //super.onBackPressed();
      exitApp();
      return;
    }
    doubleBackToExitPressedOnce=true;
    Toast.makeText(this,R.string.back_button,Toast.LENGTH_LONG).show();
    new Handler().postDelayed(new Runnable() {
      @Override
      public void run() {
        doubleBackToExitPressedOnce=false;
      }
    }, 2000);
  }
}
