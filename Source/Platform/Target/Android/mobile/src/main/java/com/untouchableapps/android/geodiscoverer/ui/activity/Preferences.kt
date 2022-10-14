//============================================================================
// Name        : Preferences.kt
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2021 Matthias Gruenewald
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

package com.untouchableapps.android.geodiscoverer.ui.activity

import android.content.Intent
import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.ActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.ArrowBack
import androidx.compose.material.icons.outlined.ArrowForward
import androidx.compose.material.ripple.rememberRipple
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.ExperimentalGraphicsApi
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import com.godaddy.android.colorpicker.ClassicColorPicker
import com.godaddy.android.colorpicker.HsvColor
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.R
import com.untouchableapps.android.geodiscoverer.core.GDCore
import com.untouchableapps.android.geodiscoverer.logic.ble.GDEBikeService
import com.untouchableapps.android.geodiscoverer.logic.ble.GDHeartRateService
import com.untouchableapps.android.geodiscoverer.ui.component.GDTextField
import com.untouchableapps.android.geodiscoverer.ui.theme.AndroidTheme
import kotlinx.coroutines.*
import java.io.File
import java.util.*

@ExperimentalGraphicsApi
@ExperimentalMaterial3Api
class Preferences : ComponentActivity(), CoroutineScope by MainScope() {

  // References
  var coreObject: GDCore = GDApplication.coreObject;

  // Activity title
  var activityTitle=""

  // Preference path this activity handles
  var path=""

  // Callback when a called activity finishes
  val startForResult = registerForActivityResult(ActivityResultContracts.StartActivityForResult())
  { result: ActivityResult ->
    // Did the activity change prefs?
    if (result.resultCode == 1) {
      // Then this one also need to indicate this
      setResult(1)
      setContent {
        AndroidTheme {
          content(activityTitle, path)
        }
      }
    }
  }

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)

    // Get the path to use
    path=""
    if (intent.hasExtra("path")) {
      if (intent.getStringExtra("path") != null)
        path = intent.getStringExtra("path")!!
    }

    // Rename the preference screen
    activityTitle=getString(R.string.preferences)
    if (path != "") {
      var prettyPath = path
      var pos = prettyPath.lastIndexOf('/')
      if (pos >= 0) {
        prettyPath = prettyPath.substring(pos + 1)
        pos = prettyPath.indexOf('[')
        if (pos >= 0) {
          prettyPath = prettyPath.substring(pos + 1)
          prettyPath = prettyPath.substring(prettyPath.indexOf('=') + 2)
          pos = prettyPath.indexOf('\'')
          prettyPath = prettyPath.substring(0, pos)
        }
      }
      activityTitle += ": $prettyPath"
    }

    // No preferences was changed so far
    setResult(0)

    // Set the content
    setContent {
      AndroidTheme {
        content(activityTitle, path)
      }
    }
  }

  override fun onDestroy() {
    super.onDestroy()
    cancel() // Stop all coroutines
  }

  // Creates a preference section
  fun definePreferenceSection(title: String, skipDivider: Boolean=false): Bundle {
    val info = Bundle()
    info.putString("type","section")
    info.putString("name", title)
    info.putString("parentPath", "")
    info.putString("fullPath", "")
    if (skipDivider)
      info.putBoolean("skipDivider", true)
    return info
  }

    // Gets a single preference for the given path and name
  fun definePreferenceEntry(path: String, name: String): Bundle? {

    // Get some information about the node
    val key: String = if (path === "") {
      name
    } else {
      "$path/$name"
    }
    val info = coreObject.configStoreGetNodeInfo(key)
    info.putString("parentPath", path)
    info.putString("fullPath", key)

    // Do special things for special preferences
    when (key) {
      "Map/folder",
      "Navigation/lastRecordedTrackFilename",
      "Navigation/activeRoute",
      "HeartRateMonitor/bluetoothAddress",
      "EBikeMonitor/bluetoothAddress"
      -> {
        info.putString("type", "enumeration")
        //start co routine in IO thread pool  to fill the variables
        //    store the result in a map with the key
      }
    }

    // Prepare some variables
    var prettyName = info.getString("name")
    prettyName = (prettyName!!.substring(0, 1).lowercase() + prettyName.substring(1))
    val upperCaseCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    for (i in upperCaseCharacters.indices) {
      val character = upperCaseCharacters.substring(i, i + 1)
      prettyName = prettyName!!.replace(character, " $character")
    }
    prettyName = (prettyName!!.substring(0, 1).uppercase() + prettyName.substring(1))
    info.putString("prettyName", prettyName)

    // If the node is optional and there is no value, skip it
    if (info.containsKey("isOptional")) {
      if (!coreObject.configStorePathExists(key)) {
        return null
      }
    }
    return info
  }

  // Construct a list of entries for this preference screen
  fun collectPreferences(path: String, expertMode: Boolean): List<Bundle> {

    // Get all entries for this path
    var entries: MutableList<Bundle> = mutableListOf()
    if ((!expertMode)&&(path=="")) {
      entries.add(definePreferenceSection("General",true))
      entries.add(definePreferenceEntry("General","unitSystem")!!)
      entries.add(definePreferenceEntry("General","wakeLock")!!)
      entries.add(definePreferenceEntry("","expertMode")!!)
      entries.add(definePreferenceSection("Map"))
      entries.add(definePreferenceEntry("Map","folder")!!)
      entries.add(definePreferenceEntry("Map","downloadAreaLength")!!)
      entries.add(definePreferenceEntry("Navigation","Route")!!)
      entries.add(definePreferenceSection("Map Tile Server",true))
      entries.add(definePreferenceEntry("MapTileServer","userScale")!!)
      entries.add(definePreferenceEntry("MapTileServer","textScale")!!)
      entries.add(definePreferenceEntry("MapTileServer","language")!!)
      entries.add(definePreferenceEntry("MapTileServer","port")!!)
      entries.add(definePreferenceEntry("MapTileServer","workerCount")!!)
      entries.add(definePreferenceSection("Navigation"))
      entries.add(definePreferenceEntry("Navigation","lastRecordedTrackFilename")!!)
      entries.add(definePreferenceEntry("Navigation","activeRoute")!!)
      entries.add(definePreferenceEntry("Navigation","averageTravelSpeed")!!)
      entries.add(definePreferenceSection("Target"))
      entries.add(definePreferenceEntry("Navigation/Target","lng")!!)
      entries.add(definePreferenceEntry("Navigation/Target","lat")!!)
      entries.add(definePreferenceSection("Cockpit"))
      val apps = coreObject.configStoreGetNodeNames("Cockpit/App")
      Arrays.sort(apps)
      for (app in apps) {
        entries.add(definePreferenceEntry("Cockpit/App",app)!!)
      }
      entries.add(definePreferenceSection("Heart Rate Monitor"))
      entries.add(definePreferenceEntry("HeartRateMonitor","active")!!)
      entries.add(definePreferenceEntry("HeartRateMonitor","bluetoothAddress")!!)
      entries.add(definePreferenceEntry("HeartRateMonitor","maxHeartRate")!!)
      entries.add(definePreferenceEntry("HeartRateMonitor","startHeartRateZoneTwo")!!)
      entries.add(definePreferenceEntry("HeartRateMonitor","startHeartRateZoneThree")!!)
      entries.add(definePreferenceEntry("HeartRateMonitor","startHeartRateZoneFour")!!)
      entries.add(definePreferenceSection("E-Bike Monitor"))
      entries.add(definePreferenceEntry("EBikeMonitor","active")!!)
      entries.add(definePreferenceEntry("EBikeMonitor","bluetoothAddress")!!)
      /*entries.add(definePreferenceSection("Google Bookmarks Synchronization"))
      entries.add(definePreferenceEntry("GoogleBookmarksSync","active")!!)
      entries.add(definePreferenceEntry("GoogleBookmarksSync","placesApiKey")!!)*/
      entries.add(definePreferenceSection("Debug"))
      entries.add(definePreferenceEntry("General","createTraceLog")!!)
      entries.add(definePreferenceEntry("General","createMessageLog")!!)
      entries.add(definePreferenceEntry("General","createMutexLog")!!)
    } else {
      val names = coreObject.configStoreGetNodeNames(path)
      Arrays.sort(names)
      var lastEntryWasDivider=true
      for (name in names) {
        var entry: Bundle? = definePreferenceEntry(path, name)
        if (entry != null) {
          if (lastEntryWasDivider) entry.putBoolean("skipDivider",true)
          lastEntryWasDivider = (entry.getString("type")=="container")&&(entry.containsKey("isUnbounded"))
          entries.add(entry)
        }
      }
    }
    return entries
  }

  @Composable
  fun content(title: String, path: String) {

    // Decide which content to show
    var m=false
    if (path=="")
      m=coreObject.configStoreGetStringValue("", "expertMode") != "0"
    val expertMode = remember { mutableStateOf(m) }

    // Get all entries to show
    var entries: List<Bundle> = collectPreferences(path,expertMode.value)

    // Create the content
    BoxWithConstraints(
      modifier = Modifier
        .fillMaxSize()
    ) {
      val contentBoxWithConstraintsScope = this
      Scaffold(
        topBar = {
          SmallTopAppBar(
            title = { Text(title) },
            navigationIcon = {
              IconButton(onClick = { finish() }) {
                Icon(
                  imageVector = Icons.Outlined.ArrowBack,
                  contentDescription = null
                )
              }
            },
          )
        },
        content = { innerPadding ->
          LazyColumn(
            modifier = Modifier
              .padding(innerPadding)
          ) {
            itemsIndexed(entries) { _, entry ->
              inflatePreference(entry, contentBoxWithConstraintsScope.maxHeight) {
                expertMode.value = it
              }
            }
          }
        }
      )
    }
  }

  @Composable
  fun inflatePreference(info: Bundle, maxHeight: Dp, setExpertMode: (Boolean)->Unit) {

    // Process the type of info
    val parentPath: String = info.getString("parentPath")!!
    val fullPath: String = info.getString("fullPath")!!
    val name: String = info.getString("name")!!
    when (info.getString("type")) {

      // Section title
      "section" -> {
        sectionTitle(
          skipDivider = info.getBoolean("skipDivider"),
          title = info.getString("name")!!
        )
      }

      // Container entry that launches another preferences activity
      "container" -> {

        // Check if the path is an unbounded one
        if (info.containsKey("isUnbounded")) {

          // Add a section title
          sectionTitle(
            skipDivider = info.getBoolean("skipDivider"),
            title = info.getString("prettyName") + " " + stringResource(id = R.string.prefs_list)
          )

          // Get all values of the iterating attribute
          val values = coreObject.configStoreGetAttributeValues(
            fullPath,
            info.getString("isUnbounded")
          )
          if (values.isEmpty()) {
            item(
              title = stringResource(id = R.string.prefs_no_entries),
            )
          } else {
            for (value in values) {

              // Construct the new path
              val key2: String =
                (fullPath + "[@" + info.getString("isUnbounded") + "='" + value + "']")

              // Define a summary
              var summary = ""
              if (fullPath == "Navigation/Route") {
                val value2 = coreObject.configStoreGetStringValue(key2, "visible")
                summary = if (value2.toInt() == 1) {
                  stringResource(R.string.visible_on_map)
                } else {
                  stringResource(R.string.hidden_on_map)
                }
              }

              // Add the component to show the details
              item(
                title = value,
                summary = summary,
                onClick = {
                  val intent = Intent(this, Preferences::class.java)
                  intent.putExtra("path", key2)
                  startForResult.launch(intent)
                }
              )
            }
          }

          // Add another separator
          sectionDivider()

        } else {

          // Just create the entry for the sub preference
          item(
            title = info.getString("prettyName")!!,
            summary = info.getString("documentation")!!,
            onClick = {
              val intent = Intent(this, Preferences::class.java)
              intent.putExtra("path", fullPath)
              startForResult.launch(intent)
            }
          )
        }
      }

      // Color preference
      "color" -> {

        // Get the color
        val readColor: () -> Color = {
          val red = Integer.valueOf(coreObject.configStoreGetStringValue(fullPath, "red"))
          val green = Integer.valueOf(coreObject.configStoreGetStringValue(fullPath, "green"))
          val blue = Integer.valueOf(coreObject.configStoreGetStringValue(fullPath, "blue"))
          val alpha = Integer.valueOf(coreObject.configStoreGetStringValue(fullPath, "alpha"))
          Color(red, green, blue, alpha)
        }
        val c = readColor()
        val currentColor = remember { mutableStateOf(c) }

        // Create the entry
        val openDialog = remember { mutableStateOf(false) }
        val newColor = remember { mutableStateOf(currentColor.value) }
        item(
          title = info.getString("prettyName")!!,
          summary = info.getString("documentation")!!,
          colorValue = currentColor.value,
          onClick = { openDialog.value = true }
        )
        setValueDialog(
          info = info,
          visible = openDialog.value,
          content = {
            ClassicColorPicker(
              color = newColor.value,
              modifier = Modifier
                .height(250.dp),
              onColorChanged = { c: HsvColor ->
                newColor.value = c.toColor()
              }
            )
          },
          resetValue = {
            coreObject.configStoreRemovePath(fullPath)
            currentColor.value = readColor()
            newColor.value = currentColor.value
          },
          setValue = {
            coreObject.configStoreSetStringValue(
              fullPath,
              "alpha",
              (newColor.value.alpha * 255.0).toInt().toString()
            )
            coreObject.configStoreSetStringValue(
              fullPath,
              "red",
              (newColor.value.red * 255.0).toInt().toString()
            )
            coreObject.configStoreSetStringValue(
              fullPath,
              "green",
              (newColor.value.green * 255.0).toInt().toString()
            )
            coreObject.configStoreSetStringValue(
              fullPath,
              "blue",
              (newColor.value.blue * 255.0).toInt().toString()
            )
            currentColor.value = newColor.value
          },
          setVisible = { visible -> openDialog.value = visible }
        )
      }

      // Text preference
      "string", "integer", "double", "long" -> {

        // Get the value
        val value = coreObject.configStoreGetStringValue(parentPath, name)
        val currentValue = remember { mutableStateOf(value) }

        // Create the entry
        val openDialog = remember { mutableStateOf(false) }
        val newValue = remember { mutableStateOf(currentValue.value) }
        item(
          title = info.getString("prettyName")!!,
          summary = info.getString("documentation")!!,
          stringValue = currentValue.value,
          onClick = { openDialog.value = true }
        )
        setValueDialog(
          info = info,
          visible = openDialog.value,
          content = {
            GDTextField(
              value = newValue.value,
              onValueChange = { v: String ->
                newValue.value = v
              }
            )
          },
          resetValue = {
            coreObject.configStoreRemovePath(fullPath)
            currentValue.value = coreObject.configStoreGetStringValue(parentPath, name)
            newValue.value = currentValue.value
          },
          setValue = {
            coreObject.configStoreSetStringValue(parentPath, name, newValue.value)
            currentValue.value = newValue.value
          },
          setVisible = { visible -> openDialog.value = visible }
        )
      }

      // Checkbox preference
      "boolean" -> {

        // Get the value
        val value = coreObject.configStoreGetStringValue(parentPath, name) != "0"
        val currentValue = remember { mutableStateOf(value) }

        // Create the entry
        item(
          title = info.getString("prettyName")!!,
          summary = info.getString("documentation")!!,
          booleanValue = currentValue.value,
          onClick = {
            currentValue.value = !currentValue.value
            coreObject.configStoreSetStringValue(
              parentPath,
              name,
              if (currentValue.value) "1" else "0"
            )
            setResult(1)
            if (fullPath=="expertMode") {
              setExpertMode(currentValue.value)
            }
          },
        )
      }

      // Enumeration preference
      "enumeration" -> {

        // Get the value
        val value = coreObject.configStoreGetStringValue(parentPath, name)
        var values = mutableListOf<String>()

        // Do special things for special preferences
        var backgroundFill = false
        when (fullPath) {
          "Map/folder",
          "Navigation/lastRecordedTrackFilename",
          "Navigation/activeRoute",
          "HeartRateMonitor/bluetoothAddress",
          "EBikeMonitor/bluetoothAddress"
          -> {
            backgroundFill = true
          }
          else -> {
            var j = 0
            while (info.containsKey(j.toString())) {
              values.add(info.getString(j.toString())!!)
              j++
            }
          }
        }
        val currentValue = remember { mutableStateOf(value) }
        val allValues = remember { mutableStateOf(values) }

        // Do the background fill if necessary
        if (backgroundFill) {
          LaunchedEffect(Unit) {
            withContext(Dispatchers.IO) {

              // Handle the different types
              var values2 = mutableListOf<String>()
              var reverseSort=false
              when(fullPath) {
                "Map/folder" -> {
                  val dir = File(coreObject.homePath + "/Map")
                  dir.listFiles()?.forEach { file ->
                    if (file.isDirectory) {
                      values2.add(file.name)
                    }
                  }
                }
                "Navigation/lastRecordedTrackFilename" -> {
                  var currentValueFound = false
                  val dir = File(coreObject.homePath + "/Track")
                  val currentValue2 = coreObject.configStoreGetStringValue(parentPath, name)
                  dir.listFiles()?.forEach { file ->
                    if (!file.isDirectory
                      && file.name.substring(file.name.length - 1) != "~"
                      && file.name.substring(file.name.length - 4) != ".bin"
                    ) {
                      values2.add(file.name)
                      if (file.name == currentValue2) currentValueFound = true
                    }
                  }
                  if (!currentValueFound) values2.add(currentValue2)
                  reverseSort=true
                }
                "Navigation/activeRoute" -> {
                  values2 =
                    coreObject.configStoreGetAttributeValues("Navigation/Route", "name")
                      .toMutableList()
                }
                "HeartRateMonitor/bluetoothAddress" -> {
                  values2 = GDHeartRateService.knownDeviceAddresses.toMutableList()
                }
                "EBikeMonitor/bluetoothAddress" -> {
                  values2 = GDEBikeService.knownDeviceAddresses.toMutableList()
                }
              }
              values2.sort();
              if (reverseSort)
                values2.reverse()
              allValues.value=values2;
              //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","background fill finished for $fullPath")
            }
          }
        }

        // Create the entry
        val openDialog = remember { mutableStateOf(false) }
        val newValue = remember { mutableStateOf(currentValue.value) }
        item(
          title = info.getString("prettyName")!!,
          summary = info.getString("documentation")!!,
          stringValue = currentValue.value,
          onClick = {
            if ((allValues.value.size == 0) && (backgroundFill))
              Toast.makeText(this, getString(R.string.prefs_not_ready), Toast.LENGTH_LONG).show()
            else
              openDialog.value = true
          }
        )
        setValueDialog(
          info = info,
          visible = openDialog.value,
          content = {
            LazyColumn(
              modifier = Modifier
                .heightIn(max = maxHeight - 250.dp)
                .fillMaxWidth()
            ) {
              items(allValues.value) {
                Row(
                  verticalAlignment = Alignment.CenterVertically
                ) {
                  RadioButton(
                    selected = it == newValue.value,
                    onClick = { newValue.value = it }
                  )
                  Spacer(
                    Modifier
                      .width(0.dp)
                  )
                  Text(
                    text = it
                  )
                }
              }
            }
          },
          resetValue = {
            coreObject.configStoreRemovePath(fullPath)
            currentValue.value = coreObject.configStoreGetStringValue(parentPath, name)
            newValue.value = currentValue.value
          },
          setValue = {
            coreObject.configStoreSetStringValue(parentPath, name, newValue.value)
            currentValue.value = newValue.value
          },
          setVisible = { visible -> openDialog.value = visible }
        )
      }

      // Unknown preference
      else -> {
        GDApplication.addMessage(
          GDApplication.DEBUG_MSG,
          "GDApp",
          "preference type ${info.getString("type")} not yet implemented"
        )
      }
    }
  }

  // Creates a divider
  @Composable
  fun sectionDivider() {
    Box(
      modifier = Modifier
        .fillMaxWidth()
        .height(1.dp)
        .background(MaterialTheme.colorScheme.outline)
    )
  }

  // Creates a section title
  @Composable
  fun sectionTitle(skipDivider: Boolean, title: String) {
    if (!skipDivider) {
      sectionDivider()
    }
    Row(
      modifier = Modifier
        .fillMaxWidth()
        .padding(10.dp),
      verticalAlignment = Alignment.CenterVertically
    ) {
      Column(
        modifier = Modifier
          .absolutePadding(left = 10.dp)
      ) {

        Text(
          text = title,
          style = MaterialTheme.typography.titleSmall,
          color = MaterialTheme.colorScheme.primary
        )
      }
    }
  }

  // Creates a preference item
  @Composable
  fun item(
    title: String, summary: String = "",
    colorValue: Color? = null,
    stringValue: String? = null,
    booleanValue: Boolean? = null,
    onClick: () -> Unit = {}
  ) {
    val interactionSource = remember { MutableInteractionSource() }
    Box(
      modifier = Modifier
        .clickable(
          onClick = onClick,
          interactionSource = interactionSource,
          indication = rememberRipple(bounded = true)
        )
    ) {
      Row(
        modifier = Modifier
          .fillMaxWidth()
          .padding(10.dp),
        verticalAlignment = Alignment.CenterVertically
      ) {
        Spacer(
          Modifier
            .width(0.dp)
            .height(40.dp)
        )
        Column(
          modifier = Modifier
            .absolutePadding(left = 10.dp)
            .weight(1f)
        ) {
          Text(
            text = title,
            style = MaterialTheme.typography.titleMedium
          )
          if (summary != "") {
            Spacer(Modifier.size(1.dp))
            Text(
              text = summary,
              style = MaterialTheme.typography.bodyMedium
            )
          }
        }
        if (stringValue == null) {
          Column(
            modifier = Modifier
              .width(40.dp),
            horizontalAlignment = Alignment.CenterHorizontally
          ) {
            if (booleanValue != null) {
              Checkbox(
                checked = booleanValue,
                onCheckedChange = { onClick() }
              )
            } else if (colorValue != null) {
              Box(
                modifier = Modifier
                  .width(30.dp)
                  .height(30.dp)
                  .background(colorValue)
              )
            } else {
              Icon(
                imageVector = Icons.Outlined.ArrowForward,
                tint = MaterialTheme.colorScheme.primary,
                contentDescription = null
              )
            }
          }
        }
      }
    }
  }

  @Composable
  fun setValueDialog(
    info: Bundle,
    visible: Boolean,
    content: @Composable () -> Unit,
    resetValue: () -> Unit,
    setValue: () -> Unit,
    setVisible: (Boolean) -> Unit
  ) {
    if (visible) {
      AlertDialog(
        modifier = Modifier
          .wrapContentHeight(),
        onDismissRequest = {
          setVisible(false)
        },
        title = {
          Text(text = stringResource(id = R.string.prefs_set_value, info.getString("prettyName")!!))
        },
        text = {
          content()
        },
        confirmButton = {
          Row() {
            TextButton(
              onClick = {
                resetValue()
                setResult(1)
                setVisible(false)
              }
            ) {
              Text(stringResource(id = R.string.reset))
            }
            Spacer(modifier = Modifier.width(10.dp))
            TextButton(
              onClick = {
                setValue()
                setResult(1)
                setVisible(false)
              }
            ) {
              Text(stringResource(id = R.string.finished))
            }
          }
        },
        dismissButton = {
          TextButton(
            onClick = {
              setVisible(false)
            }
          ) {
            Text(stringResource(id = R.string.cancel))
          }
        }
      )
    }
  }
}

