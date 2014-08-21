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
import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedList;

import net.margaritov.preference.colorpicker.ColorPickerPreference;
import android.annotation.SuppressLint;
import android.app.ProgressDialog;
import android.content.Intent;
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
import android.preference.PreferenceScreen;
import android.text.method.DigitsKeyListener;
import android.util.TimingLogger;
import android.widget.Toast;

public class Preferences extends PreferenceActivity implements
    OnPreferenceClickListener,OnPreferenceChangeListener {

  /** Interface to the native C++ core */
  GDCore coreObject = null;
  
  /** Current path */
  String currentPath;

  // Request codes for calling other activities
  static final int SHOW_PREFERENCE_SCREEN_REQUEST = 0;

  /** Copies tracks from the Track into the Route directory */
  private class FillListPreferenceItemTask extends AsyncTask<Void, Integer, Void> {

    public ListPreference entry;
    public String key;
    public String path;
    public String name;
    String[] valuesArray;
    
    protected void onPreExecute() {
    }

    protected Void doInBackground(Void... params) {

      // Handle the different types
      if (key.equals("Map/folder")) {
        File dir = new File(coreObject.homePath + "/Map");
        LinkedList<String> values = new LinkedList<String>();
        for (File file : dir.listFiles()) {
          if (file.isDirectory()) { 
            values.add(file.getName());
          }
        }
        valuesArray = new String[values.size()];
        values.toArray(valuesArray);
        Arrays.sort(valuesArray);
      }
      if (key.equals("Navigation/lastRecordedTrackFilename")) {
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
        valuesArray = new String[values.size()];
        values.toArray(valuesArray);
        Arrays.sort(valuesArray,Collections.reverseOrder());
      }
      if (key.equals("Navigation/activeRoute")) {
        valuesArray = coreObject.configStoreGetAttributeValues("Navigation/Route", "name");
        Arrays.sort(valuesArray);
      }
      
      return null;
    }

    protected void onProgressUpdate(Integer... progress) {
    }

    protected void onPostExecute(Void result) {
      entry.setEntries(valuesArray);
      entry.setEntryValues(valuesArray);
      entry.setEnabled(true);
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
  @SuppressLint("DefaultLocale")
  boolean addPreference(PreferenceGroup attributeParent, PreferenceGroup containerParent, String path, String name) {

    //TimingLogger timings = new TimingLogger("GDApp", "addPreference");
    boolean attributesFound=false;
    
    // Get some information about the node
    String key;
    if (path == "") {
      key = name;
    } else {
      key = path + "/" + name;
    }
    Bundle info = coreObject.configStoreGetNodeInfo(key);
    //timings.addSplit("configStoreGetNodeInfo");

    // Do special things for special preferences
    boolean backgroundFill=false;
    if (key.equals("Map/folder")) {
      
      // Use an enumeration of available folders
      info.putString("type", "enumeration");
      backgroundFill=true;
      //timings.addSplit("folder");
    }
    if (key.equals("Navigation/lastRecordedTrackFilename")) {
      
      // Use an enumeration of available files
      info.putString("type", "enumeration");
      backgroundFill=true;
      //timings.addSplit("lastRecordedTrackFilename");
    }
    if (key.equals("Navigation/activeRoute")) {
      
      // Use an enumeration of available routes
      info.putString("type", "enumeration");
      backgroundFill=true;
      //timings.addSplit("activeRoute");
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
    //timings.addSplit("prettyName");

    // If the node is optional and there is no value, skip it
    if (info.containsKey("isOptional")) {
      if (!coreObject.configStorePathExists(key)) {
        //timings.addSplit("is optional");
        //timings.dumpToLog();
        return false;
      }
    }      

    // Container node?
    if (type.equals("container")) {

      // Check if the path is an unbounded one
      if (info.containsKey("isUnbounded")) {

        // Get all values of the iterating attribute
        String[] values = coreObject.configStoreGetAttributeValues(key,
            info.getString("isUnbounded"));
        PreferenceCategory category = new PreferenceCategory(this);
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
        //timings.addSplit("is unbounded");

      } else {

        PreferenceScreen childScreen = getPreferenceManager()
            .createPreferenceScreen(this);
        childScreen.setOnPreferenceClickListener(this);
        entry = childScreen;
        //timings.addSplit("is bounded");
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
      //timings.addSplit("color");

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
      //timings.addSplit("string");

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
      //timings.addSplit("boolean");

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
      //timings.addSplit("enumeration");
    }

    // Add the entry
    if (entry != null) {        
      entry.setKey(key);
      entry.setTitle(prettyName);
      entry.setSummary(info.getString("documentation"));
      entry.setPersistent(false);
      attributesFound = true;
      attributeParent.addPreference(entry);
      if (backgroundFill) {
        FillListPreferenceItemTask task = new FillListPreferenceItemTask();
        task.entry = (ListPreference)entry;
        task.key = key;
        task.path = path;
        task.name = name;
        entry.setEnabled(false);
        task.execute();
      }
    }
    //timings.addSplit("add entry");
    //timings.dumpToLog();
    
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
      addPreference(generalCategory, generalCategory, "Map", "folder");
      addPreference(generalCategory, generalCategory, "General", "wakeLock");
      addPreference(generalCategory, generalCategory, "General", "backButtonTurnsScreenOff");
      addPreference(generalCategory, generalCategory, "MetaWatch", "activateMetaWatchApp");
      addPreference(generalCategory, generalCategory, "", "expertMode");
      PreferenceCategory routeCategory = new PreferenceCategory(this);
      routeCategory.setTitle("Route list (use menu to modify)");
      addPreference(rootScreen, rootScreen, "Navigation", "Route");
      PreferenceCategory navigationCategory = new PreferenceCategory(this);
      navigationCategory.setTitle("Navigation");
      rootScreen.addPreference(navigationCategory);
      addPreference(navigationCategory, navigationCategory, "Navigation", "lastRecordedTrackFilename");
      addPreference(navigationCategory, navigationCategory, "Navigation", "activeRoute");
      addPreference(navigationCategory, navigationCategory, "Navigation", "averageTravelSpeed");
      PreferenceCategory targetCategory = new PreferenceCategory(this);
      targetCategory.setTitle("Target");
      rootScreen.addPreference(targetCategory);
      addPreference(targetCategory, targetCategory, "Navigation/Target", "lng");
      addPreference(targetCategory, targetCategory, "Navigation/Target", "lat");
      PreferenceCategory debugCategory = new PreferenceCategory(this);
      debugCategory.setTitle("Debug");
      rootScreen.addPreference(debugCategory);
      addPreference(debugCategory, generalCategory, "General", "createTraceLog");
      addPreference(debugCategory, generalCategory, "General", "createMessageLog");
      addPreference(debugCategory, generalCategory, "General", "createMutexLog");

    } else {
      inflatePreferences(currentPath, rootScreen);
    }
  }

  /** Called when the activity is first created. */
  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    //TimingLogger timings = new TimingLogger("GDApp", "onCreate");

    // Get the core object
    coreObject = GDApplication.coreObject;

    // Get the path to use
    String path = "";
    if (getIntent().hasExtra("ShowPreferenceScreen")) {
      path = getIntent().getStringExtra("ShowPreferenceScreen");
    }
    currentPath = path;

    // Set the content
    //timings.addSplit("init");
    addPreferencesFromResource(R.xml.preferences);
    updatePreferences();
    //timings.addSplit("updatePreferences");

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

    //timings.addSplit("rest");
    //timings.dumpToLog();
       
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
