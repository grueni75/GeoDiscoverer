//============================================================================
// Name        : Preferences.java
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
import java.security.PublicKey;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.LinkedList;
import java.util.List;

import net.margaritov.preference.colorpicker.ColorPickerPreference;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.EditTextPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;
import android.preference.PreferenceGroup;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.text.method.DigitsKeyListener;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.widget.ListAdapter;
import android.widget.Toast;

public class Preferences extends PreferenceActivity implements
    OnPreferenceClickListener,OnPreferenceChangeListener {

  /** Interface to the native C++ core */
  GDCore coreObject = null;
  
  /** Current path */
  String currentPath;

  // Request codes for calling other activities
  static final int SHOW_PREFERENCE_SCREEN_REQUEST = 0;

  /** Indicates that the route list has changed */
  void routesHaveChanged() {
    coreObject.executeCoreCommand("updateRoutes()");
    setResult(1);
    boolean useIntent = true;
    if (coreObject.configStoreGetStringValue("", "expertMode").equals("0")) {
      if (currentPath.equals("")) {
        useIntent=false;
      }
    } else {
      if (currentPath.equals("Navigation")) {
        useIntent=false;
      }        
    }
    if (!useIntent) {
      updatePreferences();
    } else {
      Intent intent = new Intent(Preferences.this, Preferences.class);
      intent.putExtra("ShowPreferenceScreen", "Navigation");
      startActivityForResult(intent, SHOW_PREFERENCE_SCREEN_REQUEST);
    }    
  }
  
  /** Copies tracks from the Track into the Route directory */
  private class CopyTracksTask extends AsyncTask<Void, Integer, Void> {

    ProgressDialog progressDialog;
    public String[] trackNames;

    protected void onPreExecute() {

      // Prepare the progress dialog
      progressDialog = new ProgressDialog(Preferences.this);
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
          Toast.makeText(Preferences.this,String.format(getString(R.string.cannot_copy_file), srcFilename, dstFilename), Toast.LENGTH_LONG).show();
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

      // Indicate a route list change
      routesHaveChanged();
    }
  }

  /** Removes routes from the Route directory */
  private class RemoveRoutesTask extends AsyncTask<Void, Integer, Void> {

    ProgressDialog progressDialog;
    public String[] routeNames;

    protected void onPreExecute() {

      // Prepare the progress dialog
      progressDialog = new ProgressDialog(Preferences.this);
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
          Toast.makeText(Preferences.this,String.format(getString(R.string.cannot_remove_file), route.getPath()), Toast.LENGTH_LONG).show();
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

      // Indicate a route list change
      routesHaveChanged();
    }
  }

  /** Dynamically loads the next level of preferences */
  public boolean onPreferenceClick(Preference preference) {

    // Start the preference screen by using our activity
    String key = preference.getKey();
    Intent intent = new Intent(this, Preferences.class);
    intent.putExtra("ShowPreferenceScreen", key);
    startActivityForResult(intent, SHOW_PREFERENCE_SCREEN_REQUEST);
    return true;
  }

  /** Stores the changed preferences */
  public boolean onPreferenceChange(Preference preference, Object newValue) {
    String path = preference.getKey();
    String name;
    int pos = path.lastIndexOf('/');
    if (pos>=0) {
      name = path.substring(pos+1);
      path = path.substring(0,pos);
    } else {
      name = path;
      path = "";
    }
    if (preference instanceof CheckBoxPreference) {
      Boolean value = (Boolean) newValue;
      if (value) {
        coreObject.configStoreSetStringValue(path, name, "1");
      } else {
        coreObject.configStoreSetStringValue(path, name, "0");        
      }
    } else if (preference instanceof ColorPickerPreference) { 
      Integer value = (Integer) newValue;
      coreObject.configStoreSetStringValue(path + "/" + name, "alpha", String.valueOf((value>>24)&0xFF));
      coreObject.configStoreSetStringValue(path + "/" + name, "red", String.valueOf((value>>16)&0xFF));
      coreObject.configStoreSetStringValue(path + "/" + name, "green", String.valueOf((value>>8)&0xFF));
      coreObject.configStoreSetStringValue(path + "/" + name, "blue", String.valueOf((value>>0)&0xFF));
    } else {
      String value = (String) newValue;
      coreObject.configStoreSetStringValue(path, name, value);      
    }
    setResult(1);
    if ((path.equals(""))&&(name.equals("expertMode"))) {
      updatePreferences();
    }
    return true;
  }
  
  /** Creates the preferences entries for the given path */
  boolean addPreference(PreferenceGroup attributeParent, PreferenceGroup containerParent, String path, String name) {

    boolean attributesFound=false;
    
    // Get some information about the node
    String key;
    if (path == "") {
      key = name;
    } else {
      key = path + "/" + name;
    }
    Bundle info = coreObject.configStoreGetNodeInfo(key);

    // Do special things for special preferences
    if (key.equals("Map/folder")) {
      
      // Use an enumeration of available folders
      info.putString("type", "enumeration");
      File dir = new File(coreObject.homePath + "/Map");
      LinkedList<String> values = new LinkedList<String>();
      for (File file : dir.listFiles()) {
        if (file.isDirectory()) { 
          values.add(file.getName());
        }
      }
      String[] valuesArray = new String[values.size()];
      values.toArray(valuesArray);
      Arrays.sort(valuesArray);
      for (int i=0;i<valuesArray.length;i++) {
        info.putString(String.valueOf(i), valuesArray[i]);
      }
    }
    if (key.equals("Navigation/lastRecordedTrackFilename")) {
      
      // Use an enumeration of available files
      info.putString("type", "enumeration");
      boolean currentValueFound=false;
      String currentValue = coreObject.configStoreGetStringValue(path, name);
      File dir = new File(coreObject.homePath + "/Track");
      LinkedList<String> values = new LinkedList<String>();
      for (File file : dir.listFiles()) {
        if ((!file.isDirectory())
            && (!file.getName().substring(file.getName().length() - 1)
                .equals("~"))) {
          values.add(file.getName());
          if (file.getName().equals(currentValue))
            currentValueFound = true;
        }
      }
      if (!currentValueFound) 
        values.add(currentValue);
      String[] valuesArray = new String[values.size()];
      values.toArray(valuesArray);
      Arrays.sort(valuesArray,Collections.reverseOrder());
      for (int i=0;i<valuesArray.length;i++) {
        info.putString(String.valueOf(i), valuesArray[i]);
      }
    }
    if (key.equals("Navigation/activeRoute")) {
      
      // Use an enumeration of available routes
      info.putString("type", "enumeration");
      String[] valuesArray = coreObject.configStoreGetAttributeValues("Navigation/Route", "name");
      Arrays.sort(valuesArray);
      info.putString("0","none");
      for (int i=0;i<valuesArray.length;i++) {
        info.putString(String.valueOf(i+1), valuesArray[i]);
      }
    }
    
    // Prepare some variables     
    String type = info.getString("type");
    Preference entry = null;
    String prettyName = info.getString("name");
    prettyName = prettyName.substring(0, 1).toLowerCase()
        + prettyName.substring(1);
    String upperCaseCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (int i = 0; i < upperCaseCharacters.length(); i++) {
      String character = upperCaseCharacters.substring(i, i + 1);
      prettyName = prettyName.replace(character, " " + character);
    }
    prettyName = prettyName.substring(0, 1).toUpperCase()
        + prettyName.substring(1);

    // If the node is optional and there is no value, skip it
    if (info.containsKey("isOptional")) {
      if (!coreObject.configStorePathExists(key))
        return false;
    }      

    // Container node?
    if (type.equals("container")) {

      // Check if the path is an unbounded one
      if (info.containsKey("isUnbounded")) {

        // Get all values of the iterating attribute
        String[] values = coreObject.configStoreGetAttributeValues(key,
            info.getString("isUnbounded"));
        PreferenceCategory category = new PreferenceCategory(this);
        if (key.equals("Navigation/Route")) 
          category.setTitle(prettyName + " list (use menu to modify)");
        else
          category.setTitle(prettyName + " list");
        category.setPersistent(false);
        category.setSummary(info.getString("documentation"));
        containerParent.addPreference(category);
        if (values.length == 0) {

          PreferenceScreen childScreen = getPreferenceManager()
              .createPreferenceScreen(this);
          childScreen.setTitle("No entries!");
          childScreen.setPersistent(false);
          category.addPreference(childScreen);

        } else {
          for (String value : values) {

            // Construct the new path
            String key2 = key + "[@" + info.getString("isUnbounded") + "='"
                + value + "']";
            PreferenceScreen childScreen = getPreferenceManager()
                .createPreferenceScreen(this);
            childScreen.setKey(key2);
            childScreen.setTitle(value);
            childScreen.setPersistent(false);
            childScreen.setOnPreferenceClickListener(this);
            category.addPreference(childScreen);
          }
        }

      } else {

        PreferenceScreen childScreen = getPreferenceManager()
            .createPreferenceScreen(this);
        childScreen.setOnPreferenceClickListener(this);
        entry = childScreen;
      }

      // Color node?
    } else if (type.equals("color")) {
      ColorPickerPreference colorPicker = new ColorPickerPreference(this);
      entry = colorPicker;
      colorPicker.setAlphaSliderEnabled(true);
      int red = Integer.valueOf(coreObject.configStoreGetStringValue(key, "red"));
      int green = Integer.valueOf(coreObject.configStoreGetStringValue(key, "green"));
      int blue = Integer.valueOf(coreObject.configStoreGetStringValue(key, "blue"));
      int alpha = Integer.valueOf(coreObject.configStoreGetStringValue(key, "alpha"));
      int color = alpha << 24 | red << 16 | green << 8 | blue;
      entry.setDefaultValue(color);
      entry.setOnPreferenceChangeListener(this);

      // Text-based entry nodes?
    } else if ((type.equals("string"))
        || (type.equals("integer") || (type.equals("double")))) {
      EditTextPreference editText = new EditTextPreference(this);
      entry = editText;
      entry.setDefaultValue(coreObject.configStoreGetStringValue(path, name));
      editText.setDialogTitle("Value of \"" + prettyName + "\"");
      if (type.equals("integer")) {
        DigitsKeyListener listener = new DigitsKeyListener(true, false);
        editText.getEditText().setKeyListener(listener);
      }
      if (type.equals("double")) {
        DigitsKeyListener listener = new DigitsKeyListener(true, true);
        editText.getEditText().setKeyListener(listener);
      }
      entry.setOnPreferenceChangeListener(this);

      // Boolean node?
    } else if (type.equals("boolean")) {
      String value = coreObject.configStoreGetStringValue(path, name);
      CheckBoxPreference checkBox = new CheckBoxPreference(this);
      entry = checkBox;
      if (value.equals("1")) {
        entry.setDefaultValue(true);
      } else {
        entry.setDefaultValue(false);
      }
      entry.setOnPreferenceChangeListener(this);

      // Enumeration node?
    } else if (type.equals("enumeration")) {
      ListPreference list = new ListPreference(this);
      entry = list;
      list.setDefaultValue(coreObject.configStoreGetStringValue(path, name));
      list.setDialogTitle("Value of \"" + prettyName + "\"");
      int i = 0;
      LinkedList<String> values = new LinkedList<String>();
      while (info.containsKey(String.valueOf(i))) {
        values.add(info.getString(String.valueOf(i)));
        i++;
      }
      String[] valuesArray = new String[values.size()];
      values.toArray(valuesArray);
      list.setEntries(valuesArray);
      list.setEntryValues(valuesArray);
      entry.setOnPreferenceChangeListener(this);
    }

    // Add the entry
    if (entry != null) {        
      entry.setKey(key);
      entry.setTitle(prettyName);
      entry.setSummary(info.getString("documentation"));
      entry.setPersistent(false);
      attributesFound = true;
      attributeParent.addPreference(entry);
    }
    
    // Return result
    return attributesFound;
  }
  
  /** Creates the preferences entries for the given path */
  void inflatePreferences(String path, PreferenceScreen screen) {

    // Set the key of this screen
    if (path.equals(""))
      screen.setKey("GDC");
    else
      screen.setKey(path);

    // Add the category for separating unbounded lists from attributes
    PreferenceCategory attributeCategory = new PreferenceCategory(this);
    attributeCategory.setTitle("Preference list");
    attributeCategory.setPersistent(false);
    screen.addPreference(attributeCategory);
        
    // Inflate this path
    String[] names = coreObject.configStoreGetNodeNames(path);
    boolean attributesFound = false;
    Arrays.sort(names);
    for (String name : names) {
      if (addPreference(attributeCategory, screen, path, name))        
        attributesFound = true;
    }
    
    // Remove the attribute category if no attributes were found
    if (!attributesFound)
      screen.removePreference(attributeCategory);
  }

  /** Updates the preference screen depending on expertMode */
  void updatePreferences() {
    PreferenceScreen rootScreen = getPreferenceScreen();    
    rootScreen.removeAll();
    if ((coreObject.configStoreGetStringValue("", "expertMode").equals("0"))&&(currentPath.equals(""))) {
      PreferenceCategory generalCategory = new PreferenceCategory(this);
      generalCategory.setTitle("General");
      rootScreen.addPreference(generalCategory);
      addPreference(generalCategory, generalCategory, "General", "unitSystem");
      addPreference(generalCategory, generalCategory, "General", "wakeLock");
      addPreference(generalCategory, generalCategory, "General", "backButtonTurnsScreenOff");
      addPreference(generalCategory, generalCategory, "MetaWatch", "activateMetaWatchApp");
      addPreference(generalCategory, generalCategory, "", "expertMode");
      PreferenceCategory mapCategory = new PreferenceCategory(this);
      mapCategory.setTitle("Map");
      rootScreen.addPreference(mapCategory);
      addPreference(mapCategory, mapCategory, "Map", "folder");
      addPreference(mapCategory, mapCategory, "Map", "returnToLocation");
      addPreference(mapCategory, mapCategory, "Map", "returnToLocationTimeout");
      addPreference(mapCategory, mapCategory, "Map", "zoomLevelLock");
      PreferenceCategory trackRecordingCategory = new PreferenceCategory(this);
      trackRecordingCategory.setTitle("Track recording");
      rootScreen.addPreference(trackRecordingCategory);
      addPreference(trackRecordingCategory, trackRecordingCategory, "Navigation", "TrackColor");
      addPreference(trackRecordingCategory, trackRecordingCategory, "Navigation", "recordTrack");
      addPreference(trackRecordingCategory, trackRecordingCategory, "Navigation", "lastRecordedTrackFilename");
      PreferenceCategory targetCategory = new PreferenceCategory(this);
      targetCategory.setTitle("Target");
      rootScreen.addPreference(targetCategory);
      addPreference(targetCategory, targetCategory, "Navigation/Target", "visible");
      addPreference(targetCategory, targetCategory, "Navigation/Target", "lng");
      addPreference(targetCategory, targetCategory, "Navigation/Target", "lat");
      PreferenceCategory routeCategory = new PreferenceCategory(this);
      routeCategory.setTitle("Route list (use menu to modify)");
      addPreference(rootScreen, rootScreen, "Navigation", "Route");
      PreferenceCategory routeNavigationCategory = new PreferenceCategory(this);
      routeNavigationCategory.setTitle("Route navigation");
      rootScreen.addPreference(routeNavigationCategory);
      addPreference(routeNavigationCategory, routeNavigationCategory, "Navigation", "activeRoute");
    } else {
      inflatePreferences(currentPath, rootScreen);
    }
  }

  /** Called when the activity is first created. */
  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    // Get the core object
    coreObject = GDApplication.coreObject;

    // Get the path to use
    String path = "";
    if (getIntent().hasExtra("ShowPreferenceScreen")) {
      path = getIntent().getStringExtra("ShowPreferenceScreen");
    }
    currentPath = path;

    // Set the content
    addPreferencesFromResource(R.xml.preferences);
    updatePreferences();

    // Rename the preference screen
    if (!path.equals("")) {
      String prettyPath = path;
      int pos = prettyPath.lastIndexOf('/');
      if (pos >= 0) {
        prettyPath = prettyPath.substring(pos + 1);
        pos = prettyPath.indexOf('[');
        if (pos >= 0) {
          prettyPath = prettyPath.substring(pos + 1);
          prettyPath = prettyPath.substring(prettyPath.indexOf('=') + 2);
          pos = prettyPath.indexOf('\'');
          prettyPath = prettyPath.substring(0, pos);
        }
      }
      setTitle("Preferences: " + prettyPath);
    }
    
    // No preferences was changed so far
    setResult(0);
       
  }

  /** Called when a option menu shall be created */
  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    MenuInflater inflater = getMenuInflater();
    inflater.inflate(R.menu.preferences, menu);
    return true;
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

  /** Called when an option menu item has been selected */
  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    switch (item.getItemId()) {
    case R.id.add_tracks_as_routes:
      addTracksAsRoutes();
      return true;
    case R.id.remove_routes:
      removeRoutes();
      return true;
    default:
      return super.onOptionsItemSelected(item);
    }
  }

  /** Called when a called activity finishes */
  protected void onActivityResult(int requestCode, int resultCode, Intent data) {
      
    // Did the activity changes prefs?
    if (requestCode == SHOW_PREFERENCE_SCREEN_REQUEST) {
      if (resultCode==1) {
        setResult(1);  // then also this activity changed prefs
      }
    }
  }
}
