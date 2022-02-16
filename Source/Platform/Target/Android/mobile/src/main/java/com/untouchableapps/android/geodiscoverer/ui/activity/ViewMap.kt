//============================================================================
// Name        : ViewMap2.kt
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

import android.annotation.SuppressLint
import android.app.ActivityManager
import android.content.*
import android.content.pm.PackageManager
import android.content.res.Configuration
import android.hardware.Sensor
import android.hardware.SensorManager
import android.net.Uri
import android.os.*
import android.view.WindowManager
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.ActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.animation.*
import androidx.compose.material.ExperimentalMaterialApi
import androidx.compose.material.icons.Icons
import androidx.compose.material3.*
import androidx.compose.ui.tooling.preview.Preview
import com.untouchableapps.android.geodiscoverer.R
import com.untouchableapps.android.geodiscoverer.ui.theme.AndroidTheme
import androidx.compose.material.icons.outlined.*
import androidx.compose.runtime.*
import androidx.compose.ui.ExperimentalComposeUiApi
import androidx.compose.ui.graphics.ExperimentalGraphicsApi
import androidx.core.content.FileProvider
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.core.GDCore
import com.untouchableapps.android.geodiscoverer.logic.GDBackgroundTask
import com.untouchableapps.android.geodiscoverer.logic.GDBackgroundTask.AddressPointItem
import com.untouchableapps.android.geodiscoverer.logic.GDService
import com.untouchableapps.android.geodiscoverer.ui.activity.viewmap.*
import com.untouchableapps.android.geodiscoverer.ui.component.GDDialog
import kotlinx.coroutines.*
import org.mapsforge.core.model.LatLong
import java.io.File
import java.lang.Exception
import java.net.URI

@ExperimentalGraphicsApi
@ExperimentalMaterialApi
@ExperimentalMaterial3Api
@ExperimentalAnimationApi
@ExperimentalComposeUiApi
class ViewMap : ComponentActivity(), CoroutineScope by MainScope() {

  // Callback when a called activity finishes
  val startPreferencesForResult =
    registerForActivityResult(ActivityResultContracts.StartActivityForResult())
    { result: ActivityResult ->
      // Did the activity change prefs?
      if (result.resultCode == 1) {
        restartCore(false)
      }
    }

  // View model for jetpack compose communication
  val viewModel = ViewModel(this)

  // Reference to the core object and it's view
  var coreObject: GDCore? = null

  // The dialog handler
  val dialogHandler = GDDialog(
    showSnackbar = { message ->
      viewModel.showSnackbar(message)
    },
    showDialog = viewModel::setDialog
  )

  // The intent handler
  val intentHandler = IntentHandler(this)

  // Activity content
  val viewContent = ViewContent(this)
  
  // Prefs
  var prefs: SharedPreferences? = null

  // Flags
  var compassWatchStarted = false
  var exitRequested = false
  var restartRequested = false
  var doubleBackToExitPressedOnce = false

  // Managers
  var sensorManager: SensorManager? = null

  // Sets the screen time out
  @SuppressLint("Wakelock")
  fun updateWakeLock() {
    if (coreObject != null) {
      val state = coreObject!!.executeCoreCommand("getWakeLock")
      if (state == "true") {
        GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "wake lock enabled")
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
      } else {
        GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "wake lock disabled")
        window.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
      }
    }
  }

  // Start listening for compass bearing
  @Synchronized
  fun startWatchingCompass() {
    if (coreObject != null && !compassWatchStarted) {
      sensorManager?.registerListener(
        coreObject,
        sensorManager?.getDefaultSensor(Sensor.TYPE_ACCELEROMETER),
        SensorManager.SENSOR_DELAY_NORMAL
      )
      sensorManager?.registerListener(
        coreObject,
        sensorManager?.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD),
        SensorManager.SENSOR_DELAY_NORMAL
      )
      compassWatchStarted = true
    }
  }

  // Stop listening for compass bearing
  @Synchronized
  fun stopWatchingCompass() {
    if (compassWatchStarted) {
      sensorManager?.unregisterListener(coreObject)
      compassWatchStarted = false
    }
  }

  // Sets the exit busy text
  fun setExitBusyText() {
    viewModel.busyText = getString(R.string.stopping_core_object)
  }

  // Shows the splash screen
  fun setSplashVisibility(isVisible: Boolean) {
    if (isVisible) {
      viewModel.splashVisible = true
      viewModel.messagesVisible = true
      if (coreObject != null) coreObject?.setSplashIsVisible(true)
    } else {
      viewModel.splashVisible = false
      viewModel.messagesVisible = false
      viewModel.busyText = getString(R.string.starting_core_object)
    }
  }

  // Exits the app
  fun exitApp() {
    setExitBusyText()
    val m = Message.obtain(coreObject!!.messageHandler)
    m.what = GDCore.STOP_CORE
    coreObject!!.messageHandler.sendMessage(m)
  }

  // Restarts the core
  fun restartCore(resetConfig: Boolean = false) {
    restartRequested = true
    viewModel.busyText = getString(R.string.restarting_core_object)
    val m = Message.obtain(coreObject!!.messageHandler)
    m.what = GDCore.RESTART_CORE
    val b = Bundle()
    b.putBoolean("resetConfig", resetConfig)
    m.data = b
    coreObject!!.messageHandler.sendMessage(m)
  }

  // Called when a configuration change (e.g., caused by a screen rotation) has occured
  override fun onConfigurationChanged(newConfig: Configuration) {
    super.onConfigurationChanged(newConfig)
    viewModel.closeQuestion()
  }

  // Ensure that double back press quits the app
  override fun onBackPressed() {
    if (doubleBackToExitPressedOnce) {
      //super.onBackPressed();
      exitApp()
      return
    }
    doubleBackToExitPressedOnce = true
    viewModel.showSnackbar(getString(R.string.back_button))
    launch() {
      delay(2000)
      doubleBackToExitPressedOnce = false
    }
  }

  // Adds a map download job
  fun addMapDownloadJob(
    estimate: Boolean,
    routeName: String,
    selectedMapLayers: List<String>,
  ) {
    val args = arrayOfNulls<String>(selectedMapLayers.size + 2)
    args[0] = if (estimate) "1" else "0"
    args[1] = routeName
    for (i in selectedMapLayers.indices) {
      args[i + 2] = selectedMapLayers[i]
    }
    coreObject?.executeCoreCommand("addDownloadJob", *args)
  }

  // Shows the legend with the given name
  fun showMapLegend(name: String) {
    val legendPath = coreObject!!.executeCoreCommand("getMapLegendPath", name)
    val legendFile = File(legendPath)
    if (!legendFile.exists()) {
      dialogHandler.errorDialog(
        getString(
          R.string.map_has_no_legend,
          coreObject?.executeCoreCommand("getMapFolder")
        )
      )
    } else {
      val legendUri = FileProvider.getUriForFile(
        applicationContext, "com.untouchableapps.android.geodiscoverer.fileprovider", legendFile
      )
      val intent = Intent()
      intent.action = Intent.ACTION_VIEW
      intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
      if (legendPath.endsWith(".png")) intent.setDataAndType(legendUri, "image/*")
      if (legendPath.endsWith(".pdf")) intent.setDataAndType(legendUri, "application/pdf")
      GDApplication.addMessage(
        GDApplication.DEBUG_MSG, "GDApp",
        "Viewing $legendPath"
      )
      startActivity(intent)
    }
  }

  // Communication with the native core
  var coreMessageHandler = CoreMessageHandler(this)

  // Creates the activity
  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","ViewMap activity started")

    // Check for OpenGL ES 2.00
    val activityManager = getSystemService(ACTIVITY_SERVICE) as ActivityManager
    val configurationInfo = activityManager.deviceConfigurationInfo
    val supportsEs2 =
      configurationInfo.reqGlEsVersion >= 0x20000 || Build.FINGERPRINT.startsWith("generic")
    if (!supportsEs2) {
      Toast.makeText(this, getString(R.string.opengles20_required), Toast.LENGTH_LONG)
      finish();
      return
    }

    // Get important handles
    sensorManager = this.getSystemService(SENSOR_SERVICE) as SensorManager
    if (sensorManager == null) {
      Toast.makeText(this, getString(R.string.missing_system_service), Toast.LENGTH_LONG)
      finish();
      return
    }

    // Get the core object
    coreObject = GDApplication.coreObject
    if (coreObject == null) {
      finish()
      return
    }
    (application as GDApplication).setActivity(this, coreMessageHandler)

    // Init the child objects
    intentHandler.onCreate()
    if (coreObject!!.coreEarlyInitComplete) {
      viewModel.onCoreEarlyInitComplete()
    }

    // Create the content for the navigation drawer
    val navigationItems = arrayOf(
      ViewContentNavigationDrawer.NavigationItem(null, getString(R.string.map), { }),
      ViewContentNavigationDrawer.NavigationItem(Icons.Outlined.Article, getString(R.string.show_legend)) {
        if (coreObject!=null) {
          val namesString = coreObject!!.executeCoreCommand("getMapLegendNames")
          val names = namesString.split(",".toRegex()).toList()
          if (names.size != 1) {
            viewModel.askForMapLegend(names) { name ->
              showMapLegend(name)
            }
          } else {
            showMapLegend(names[0])
          }
        }
      },
      ViewContentNavigationDrawer.NavigationItem(Icons.Outlined.Download, getString(R.string.download_map)) {
        viewModel.askForMapDownloadType(
          dismissHandler = {
            coreObject?.executeAppCommand("askForMapDownloadDetails(\"\")")
          },
          confirmHandler = {
            coreObject?.executeCoreCommand("downloadActiveRoute")
          }
        )
      },
      ViewContentNavigationDrawer.NavigationItem(Icons.Outlined.CleaningServices, getString(R.string.cleanup_map)) {
        if (coreObject!=null) {
          viewModel.askForMapCleanup(
            confirmHandler = {
              coreObject?.executeCoreCommand(
                "forceMapRedownload",
                "1"
              )
            },
            dismissHandler = {
              coreObject?.executeCoreCommand(
                "forceMapRedownload",
                "0"
              )
            }
          )
        }
      },
      ViewContentNavigationDrawer.NavigationItem(null, getString(R.string.routes), { }),
      ViewContentNavigationDrawer.NavigationItem(Icons.Outlined.AddCircle, getString(R.string.add_tracks_as_routes)) {
        viewModel.askForTracksAsRoutes() { selectedTracks ->
          if (selectedTracks.isNotEmpty()) {
            GDApplication.backgroundTask.copyTracksToRoutes(this,selectedTracks,this)
          }
        }
      },
      ViewContentNavigationDrawer.NavigationItem(Icons.Outlined.RemoveCircle, getString(R.string.remove_routes)) {
        viewModel.askForRemoveRoutes() { selectedRoutes ->
          if (selectedRoutes.isNotEmpty()) {
            GDApplication.backgroundTask.removeRoutes(this,selectedRoutes,this)
          }
        }
      },
      ViewContentNavigationDrawer.NavigationItem(Icons.Outlined.SendToMobile, getString(R.string.export_selected_route)) {
        if (coreObject!=null) {
          coreObject?.executeCoreCommand("exportActiveRoute")
        }
      },
      ViewContentNavigationDrawer.NavigationItem(Icons.Outlined.Directions, getString(R.string.brouter)) {

        // Hand over all currently available address points
        val t=coreObject!!.executeCoreCommand("getMapPos")
        GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "t=$t")
        val currentPos=t.split(",")
        val zoomLevel=coreObject!!.executeCoreCommand("getMapServerZoomLevel")
        var path = "http://localhost:8383/brouter-web/index.html#map=${zoomLevel}/${currentPos[0]}/${currentPos[1]}/GeoDiscoverer"
        var pois = ""
        val addressPoints = GDApplication.backgroundTask.fillAddressPoints()
        for (ap in addressPoints) {
          if (pois != "") pois += ";"
          pois += "${ap.longitude},${ap.latitude},${Uri.encode(ap.nameUniquified)}"
        }
        if (pois != "") {
          path += "&pois=$pois"
        }
        GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "path=$path")

        // Load the brouter
        val i = Intent(Intent.ACTION_VIEW)
        i.data = Uri.parse(path)
        startActivity(i)
      },
      ViewContentNavigationDrawer.NavigationItem(null, getString(R.string.general), { }),
      ViewContentNavigationDrawer.NavigationItem(Icons.Outlined.Help, getString(R.string.help)) {
        intent = Intent(applicationContext, ShowHelp::class.java)
        startActivity(intent)
      },
      ViewContentNavigationDrawer.NavigationItem(Icons.Outlined.Settings, getString(R.string.preferences)) {
        val myIntent = Intent(applicationContext, Preferences::class.java)
        startPreferencesForResult.launch(myIntent)
      },
      ViewContentNavigationDrawer.NavigationItem(Icons.Outlined.Replay, getString(R.string.restart)) {
        restartCore(false)
      },
      ViewContentNavigationDrawer.NavigationItem(Icons.Outlined.Logout, getString(R.string.exit)) {
        exitApp()
      },
      ViewContentNavigationDrawer.NavigationItem(null, getString(R.string.debug), { }),
      ViewContentNavigationDrawer.NavigationItem(Icons.Outlined.Message, getString(R.string.toggle_messages)) {
        viewModel.toggleMessagesVisibility()
      },
      ViewContentNavigationDrawer.NavigationItem(Icons.Outlined.UploadFile, getString(R.string.send_logs)) {
        viewModel.askForSendLogs() { selectedLogs ->
          if (selectedLogs.isNotEmpty()) {
            GDApplication.backgroundTask.sendLogs(selectedLogs,this)
          }
        }
      },
      ViewContentNavigationDrawer.NavigationItem(Icons.Outlined.Clear, getString(R.string.reset)) {
        viewModel.askForConfigReset() {
          restartCore(true)
        }
      },
    )

    // Get the app version
    val packageManager = packageManager
    val appVersion: String
    appVersion = try {
      "Version " + packageManager.getPackageInfo(packageName, 0).versionName
    } catch (e: PackageManager.NameNotFoundException) {
      "Version ?"
    }

    // Create the activity content
    setContent {
      AndroidTheme {
        viewContent.content(viewModel, appVersion, navigationItems.toList())
      }
    }

    // Restore the last processed intent from the prefs
    prefs = application.getSharedPreferences("viewMap", MODE_PRIVATE)
    if (prefs!!.contains("processIntent")) {
      val processIntent: Boolean = prefs!!.getBoolean("processIntent", true)
      if (!processIntent) {
        getIntent().addFlags(Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY)
      }
      val prefsEditor: SharedPreferences.Editor = prefs!!.edit()
      prefsEditor.putBoolean("processIntent", true)
      prefsEditor.commit()
    }
  }

  // Destroys the activity
  @SuppressLint("Wakelock")
  override fun onDestroy() {
    super.onDestroy()
    GDApplication.addMessage(
      GDApplication.DEBUG_MSG,
      "GDApp",
      "onDestroy called by " + Thread.currentThread().name
    )
    cancel() // Stop all coroutines
    (application as GDApplication).setActivity(null, null)
    intentHandler.onDestroy()
    //if (downloadCompleteReceiver != null) unregisterReceiver(downloadCompleteReceiver)
    if (exitRequested) System.exit(0)
    if (restartRequested) {
      if (Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB) {
        val intent = Intent(applicationContext, ViewMap::class.java)
        startActivity(intent)
      }
    }
  }

  // Called when a new intent is available
  override fun onNewIntent(intent: Intent) {
    setIntent(intent)
    GDApplication.addMessage(
      GDApplication.DEBUG_MSG, "GDApp",
      "onNewIntent: $intent"
    )
  }

  override fun onPause() {
    super.onPause()
    GDApplication.addMessage(
      GDApplication.DEBUG_MSG,
      "GDApp",
      "onPause called by " + Thread.currentThread().name
    )
    viewModel.fixSurfaceViewBug = true
    stopWatchingCompass()
    if (!exitRequested && !restartRequested) {
      val intent = Intent(this, GDService::class.java)
      intent.action = "activityPaused"
      startService(intent)
    }
  }

  override fun onResume() {
    super.onResume()
    GDApplication.addMessage(
      GDApplication.DEBUG_MSG,
      "GDApp",
      "onResume called by " + Thread.currentThread().name
    )

    // Bug fix: Somehow the emulator calls onResume before onCreate
    // But the code relies on the fact that onCreate is called before
    // Do nothing if onCreate has not yet initialized the objects
    if (coreObject == null) return

    // If we shall restart or exit, don't init anything here
    if (exitRequested || restartRequested) return

    // Resume all components only if a exit or restart is not requested
    startWatchingCompass()
    val intent = Intent(this, GDService::class.java)
    intent.action = "activityResumed"
    startService(intent)

    // Synchronize google bookmarks
    GDApplication.coreObject.executeCoreCommand("updateGoogleBookmarks");

    // Check for outdated routes
    if (coreObject!!.coreInitialized) {
      GDApplication.backgroundTask.checkForOutdatedRoutes(this)
    }

    // Process intent only if geo discoverer is initialized
    if (coreObject!!.coreLateInitComplete) intentHandler.processIntent()
  }

  @ExperimentalMaterial3Api
  @Preview(showBackground = true)
  @Composable
  fun preview() {
    AndroidTheme {
    }
  }

}

