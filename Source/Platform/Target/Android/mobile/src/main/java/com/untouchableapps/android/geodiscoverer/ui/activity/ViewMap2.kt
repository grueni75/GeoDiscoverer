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

import android.Manifest
import android.annotation.SuppressLint
import android.app.ActivityManager
import android.content.*
import android.content.SharedPreferences.Editor
import android.content.pm.PackageManager
import android.graphics.Typeface
import android.hardware.Sensor
import android.hardware.SensorManager
import android.location.LocationManager
import android.os.*
import android.util.TypedValue
import android.view.Gravity
import android.view.ViewGroup
import android.view.WindowManager
import android.widget.TextView
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.animation.*
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.material.icons.Icons
import androidx.compose.material.ripple.rememberRipple
import androidx.compose.material3.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.tooling.preview.Preview
import com.untouchableapps.android.geodiscoverer.R
import com.untouchableapps.android.geodiscoverer.ui.theme.AndroidTheme
import java.util.*
import androidx.compose.foundation.Image
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.filled.Error
import androidx.compose.material.icons.filled.Warning
import androidx.compose.material.icons.outlined.*
import androidx.compose.runtime.*
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.RoundRect
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Outline
import androidx.compose.ui.graphics.Shape
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.*
import androidx.compose.ui.viewinterop.AndroidView
import androidx.lifecycle.ViewModel
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.core.GDCore
import com.untouchableapps.android.geodiscoverer.core.GDMapSurfaceView
import com.untouchableapps.android.geodiscoverer.logic.GDBackgroundTask
import com.untouchableapps.android.geodiscoverer.logic.GDService
import com.untouchableapps.android.geodiscoverer.logic.viewmap.GDIntent
import com.untouchableapps.android.geodiscoverer.ui.component.GDDialog
import com.untouchableapps.android.geodiscoverer.ui.component.GDLinearProgressIndicator
import java.lang.ref.WeakReference

import com.untouchableapps.android.geodiscoverer.ui.component.GDSnackBar
import com.untouchableapps.android.geodiscoverer.ui.component.GDTextField
import kotlinx.coroutines.*
import java.io.File


@ExperimentalMaterial3Api
class ViewMap2 : ComponentActivity(), CoroutineScope by MainScope() {

  // Layout parameters for the navigation drawer
  class LayoutParams() {
    val iconWidth = 60.dp
    val itemHeight = 41.dp
    val titleIndent = 20.dp
    val drawerWidth = 250.dp
    val itemPadding = 5.dp
    val hintIndent = 15.dp
    val drawerCornerRadius = 16.dp
    val snackbarHorizontalPadding = 20.dp
    val snackbarVerticalOffset = 40.dp
    val snackbarMaxWidth = 400.dp
    val askMaxContentHeight = 190.dp
  }
  val layoutParams = LayoutParams()

  // Navigation drawer content
  class NavigationItem(imageVector: ImageVector?=null, title: String, onClick: ()->Unit) {
    val imageVector = imageVector
    val title = title
    val onClick = onClick
  }

  // Communication with the composable world
  inner class ActivityViewModel() : ViewModel() {

    // Check box item
    inner class CheckboxItem(text: String) {
      var text: String = text
      var checked: Boolean = false
    }

    // State
    var drawerStatus : DrawerValue by mutableStateOf(DrawerValue.Closed)
    var messages : String by mutableStateOf("")
    var splashVisible : Boolean by mutableStateOf(false)
    var messagesVisible : Boolean by mutableStateOf(false)
    var busyText : String by mutableStateOf("")
    var snackbarText : String by mutableStateOf("")
      private set
    var snackbarActionText : String by mutableStateOf("")
      private set
    var snackbarActionHandler : ()->Unit={}
      private set
    var progressMax : Int by mutableStateOf(0)
    var progressCurrent : Int by mutableStateOf(0)
      private set
    var progressMessage : String by mutableStateOf("")
      private set
    var dialogMessage : String by mutableStateOf("")
      private set
    var dialogIsFatal : Boolean by mutableStateOf(false)
      private set
    var askTitle : String by mutableStateOf("")
      private set
    var askMessage : String by mutableStateOf("")
      private set
    var askEditTextValue : String by mutableStateOf("")
      private set
    var askEditTextValueChangeHandler : (String)->Unit={}
      private set
    var askEditTextHint : String by mutableStateOf("")
      private set
    var askEditTextError : String by mutableStateOf("")
      private set
    var askMultipleChoiceList : MutableList<CheckboxItem> by mutableStateOf(mutableListOf<CheckboxItem>())
      private set
    var askConfirmText : String by mutableStateOf("")
      private set
    var askDismissText : String by mutableStateOf("")
      private set
    var askEditTextConfirmHandler : (String)->Unit={}
      private set
    var askQuestionConfirmHandler : ()->Unit={}
      private set
    var askQuestionDismissHandler : ()->Unit={}
      private set
    var askMultipleChoiceConfirmHandler : (List<String>)->Unit={}
      private set
    var fixSurfaceViewBug : Boolean by mutableStateOf(false)

    // Methods to modify the state
    fun toggleMessagesVisibility() {
      messagesVisible=!messagesVisible
    }
    fun setProgress(message: String, value: Int) {
      progressMessage=message
      progressCurrent=value
    }
    fun setDialog(message: String, isFatal: Boolean=false) {
      dialogIsFatal=isFatal
      dialogMessage=message
    }
    fun showSnackbar(text: String, actionText: String="", actionHandler: ()->Unit = {}) {
      snackbarActionHandler=actionHandler
      snackbarActionText=actionText
      snackbarText=text
    }
    fun askForAddress(subject: String, address: String, confirmHandler: (String)->Unit = {}) {
      askEditTextValue=address
      askEditTextHint=getString(R.string.dialog_address_input_hint)
      askConfirmText=getString(R.string.dialog_lookup)
      askDismissText=getString(R.string.dialog_dismiss)
      askEditTextConfirmHandler=confirmHandler
      askMessage=getString(R.string.dialog_address)
      askTitle=getString(R.string.dialog_address_title)
    }
    fun askForRouteDownload(name: String, dstFile: File, confirmHandler: ()->Unit = {}) {
      var message: String = if (dstFile.exists())
          getString(R.string.dialog_overwrite_route_question)
        else getString(R.string.dialog_copy_route_question)
      message = String.format(message, name)
      askMessage = message
      askConfirmText = getString(R.string.dialog_yes)
      askDismissText = getString(R.string.dialog_no)
      askQuestionConfirmHandler = confirmHandler
      askTitle=getString(R.string.dialog_route_name_title)
    }
    fun askForRouteName(gpxName: String, confirmHandler: (String)->Unit = {}) {
      askEditTextValue=gpxName
      askEditTextValueChangeHandler={ value ->
        val dstFilename = GDCore.getHomeDirPath() + "/Route/" + value
        val dstFile = File(dstFilename)
        askEditTextError = if (value=="" || !dstFile.exists())
          ""
        else
          getString(R.string.route_exists)
      }
      askConfirmText=getString(R.string.dialog_import)
      askDismissText=getString(R.string.dialog_dismiss)
      askEditTextConfirmHandler=confirmHandler
      askMessage=getString(R.string.dialog_route_name_message)
      askTitle=getString(R.string.dialog_route_name_title)
    }
    fun askForTrackTreatment(confirmHandler: ()->Unit = {}, dismissHandler: ()->Unit = {}) {
      askMessage = getString(R.string.dialog_continue_or_new_track_question)
      askConfirmText = getString(R.string.dialog_new_track)
      askDismissText = getString(R.string.dialog_contine_track)
      askQuestionConfirmHandler = confirmHandler
      askQuestionDismissHandler = dismissHandler
      askTitle=getString(R.string.dialog_track_treatment_title)
    }
    fun askForMapDownloadType(confirmHandler: ()->Unit = {}, dismissHandler: ()->Unit = {}) {
      askMessage = getString(R.string.map_download_type_question)
      askConfirmText = getString(R.string.map_download_type_option2)
      askDismissText = getString(R.string.map_download_type_option1)
      askQuestionConfirmHandler = confirmHandler
      askQuestionDismissHandler = dismissHandler
      askTitle=getString(R.string.dialog_map_download_type_title)
    }
    fun askForMapDownloadDetails(confirmHandler: (List<String>)->Unit = {}) {
      val result = coreObject!!.executeCoreCommand("getMapLayers()")
      val mapLayers = result.split(",".toRegex()).toList()
      askConfirmText = getString(R.string.dialog_download)
      askDismissText = getString(R.string.dialog_dismiss)
      askMultipleChoiceList = mutableListOf<CheckboxItem>()
      mapLayers.forEach {
        askMultipleChoiceList.add(viewModel.CheckboxItem(it))
      }
      askMultipleChoiceConfirmHandler = confirmHandler
      askTitle=getString(R.string.download_job_level_selection_question)
    }
    fun closeQuestion() {
      askTitle=""
      askEditTextValue=""
      askEditTextValueChangeHandler={}
      askEditTextHint=""
      askEditTextError=""
      askQuestionConfirmHandler={}
      askQuestionDismissHandler={}
      askMultipleChoiceList=mutableListOf<CheckboxItem>()
      askMultipleChoiceConfirmHandler={}
      askMessage=""
    }
  }
  val viewModel = ActivityViewModel()

  // Reference to the core object and it's view
  var coreObject: GDCore? = null

  // The dialog handler
  val dialogHandler=GDDialog(
    showSnackbar = { message ->
      viewModel.showSnackbar(message)
    },
    showDialog = viewModel::setDialog
  )

  // The intent handler
  val intentHandler=GDIntent(this)

  // Background tasks
  var backgroundTask=GDBackgroundTask(this)

  // Prefs
  var prefs: SharedPreferences? = null

  // Flags
  var compassWatchStarted = false
  var exitRequested = false
  var restartRequested = false
  var doubleBackToExitPressedOnce = false

  // Managers
  var sensorManager: SensorManager? = null
  var locationManager: LocationManager? = null

  /** Sets the screen time out  */
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
      sensorManager!!.registerListener(
        coreObject,
        sensorManager!!.getDefaultSensor(Sensor.TYPE_ACCELEROMETER),
        SensorManager.SENSOR_DELAY_NORMAL
      )
      sensorManager!!.registerListener(
        coreObject,
        sensorManager!!.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD),
        SensorManager.SENSOR_DELAY_NORMAL
      )
      compassWatchStarted = true
    }
  }

  // Stop listening for compass bearing
  @Synchronized
  fun stopWatchingCompass() {
    if (compassWatchStarted) {
      sensorManager!!.unregisterListener(coreObject)
      compassWatchStarted = false
    }
  }

  fun setExitBusyText() {
    viewModel.busyText = getString(R.string.stopping_core_object)
  }

  // Shows the splash screen
  fun setSplashVisibility(isVisible: Boolean) {
    if (isVisible) {
      viewModel.splashVisible=true
      viewModel.messagesVisible=true
      if (coreObject!=null) coreObject!!.setSplashIsVisible(true)
    } else {
      viewModel.splashVisible=false
      viewModel.messagesVisible=false
      viewModel.busyText=getString(R.string.starting_core_object)
    }
  }

  fun exitApp() {
    setExitBusyText()
    val m = Message.obtain(coreObject!!.messageHandler)
    m.what = GDCore.STOP_CORE
    coreObject!!.messageHandler.sendMessage(m)
  }

  // Restarts the core
  fun restartCore(resetConfig: Boolean=false) {
    restartRequested = true
    viewModel.busyText = getString(R.string.restarting_core_object)
    val m = Message.obtain(coreObject!!.messageHandler)
    m.what = GDCore.RESTART_CORE
    val b = Bundle()
    b.putBoolean("resetConfig", resetConfig)
    m.data = b
    coreObject!!.messageHandler.sendMessage(m)
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

  // Communication with the native core
  class CoreMessageHandler(viewMap: ViewMap2) : Handler(Looper.getMainLooper()) {
    var weakViewMap: WeakReference<ViewMap2> = WeakReference(viewMap)

    /** Called when the core has a message  */
    override fun handleMessage(msg: Message) {

      // Abort if the object is not available anymore
      val viewMap = weakViewMap.get() ?: return

      // Handle the message
      val b = msg.data
      when (msg.what) {
        0 -> {

          // Extract the command
          val command = b.getString("command")
          val args_start = command!!.indexOf("(")
          val args_end = command.lastIndexOf(")")
          val commandFunction = command.substring(0, args_start)
          val t = command.substring(args_start + 1, args_end)
          val commandArgs = Vector<String>()
          var stringStarted = false
          var startPos = 0
          var i = 0
          while (i < t.length) {
            if (t.substring(i, i + 1) == "\"") {
              stringStarted = if (stringStarted) false else true
            }
            if (!stringStarted) {
              if (t.substring(i, i + 1) == "," || i == t.length - 1) {
                var arg: String
                arg =
                  if (i == t.length - 1) t.substring(startPos, i + 1) else t.substring(startPos, i)
                if (arg.startsWith("\"")) {
                  arg = arg.substring(1)
                }
                if (arg.endsWith("\"")) {
                  arg = arg.substring(0, arg.length - 1)
                }
                commandArgs.add(arg)
                startPos = i + 1
              }
            }
            i++
          }

          // Execute command
          var commandExecuted = false
          if (commandFunction == "fatalDialog") {
            viewMap.dialogHandler.fatalDialog(commandArgs[0])
            commandExecuted = true
          }
          if (commandFunction == "errorDialog") {
            viewMap.dialogHandler.errorDialog(commandArgs[0])
            commandExecuted = true
          }
          if (commandFunction == "warningDialog") {
            viewMap.dialogHandler.warningDialog(commandArgs[0])
            commandExecuted = true
          }
          if (commandFunction == "infoDialog") {
            viewMap.dialogHandler.infoDialog(commandArgs[0])
            commandExecuted = true
          }
          if (commandFunction == "createProgressDialog") {
            viewMap.viewModel.setProgress(commandArgs[0],commandArgs[1].toInt())
            viewMap.viewModel.progressMax=commandArgs[1].toInt()
            commandExecuted = true
          }
          if (commandFunction == "updateProgressDialog") {
            viewMap.viewModel.setProgress(commandArgs[0], commandArgs[1].toInt())
            commandExecuted = true
          }
          if (commandFunction == "closeProgressDialog") {
            viewMap.viewModel.progressMax=0
            commandExecuted = true
          }
          if (commandFunction == "getLastKnownLocation") {
            if (viewMap.checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION)==PackageManager.PERMISSION_GRANTED) {
              if (viewMap.coreObject!=null) {
                viewMap.coreObject!!.onLocationChanged(
                  viewMap.locationManager!!.getLastKnownLocation(
                    LocationManager.NETWORK_PROVIDER
                  )!!
                )
                viewMap.coreObject!!.onLocationChanged(
                  viewMap.locationManager!!.getLastKnownLocation(
                    LocationManager.GPS_PROVIDER
                  )!!
                )
              }
            }
            commandExecuted = true
          }
          if (commandFunction == "coreInitialized") {
            commandExecuted = true
          }
          if (commandFunction == "updateWakeLock") {
            viewMap.updateWakeLock()
            commandExecuted = true
          }
          if (commandFunction == "updateMessages") {
            viewMap.viewModel.messages=GDApplication.messages
            commandExecuted = true
          }
          if (commandFunction == "setSplashVisibility") {
            if (commandArgs[0] == "1") {
              viewMap.setSplashVisibility(true)
            } else {
              viewMap.setSplashVisibility(false)
            }
            commandExecuted = true
          }
          if (commandFunction == "askForAddress") {
            //viewMap.askForAddress(viewMap.getString(R.string.manually_entered_address), "")
            //commandExecuted = true
          }
          if (commandFunction == "exitActivity") {
            viewMap.exitRequested = true
            viewMap.stopService(Intent(viewMap, GDService::class.java))
            viewMap.finish()
            commandExecuted = true
          }
          if (commandFunction == "restartActivity") {
            viewMap.stopService(Intent(viewMap, GDService::class.java))
            val prefsEditor = viewMap.prefs!!.edit()
            prefsEditor.putBoolean("processIntent", false)
            prefsEditor.commit()
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
              viewMap.recreate()
            } else {
              viewMap.finish()
            }
            commandExecuted = true
          }
          if (commandFunction == "lateInitComplete") {

            // Process the latest intent (if any)
            viewMap.intentHandler.processIntent()

            // Inform the user about the app drawer
            if (!viewMap.prefs!!.getBoolean("navDrawerHintShown", false)) {
              viewMap.viewModel.showSnackbar(
                viewMap.getString(R.string.nav_drawer_hint),
                viewMap.getString(R.string.got_it)
              ) {
                val prefsEditor = viewMap.prefs!!.edit()
                prefsEditor.putBoolean("navDrawerHintShown", true)
                prefsEditor.commit()
              }
            }
            commandExecuted = true
          }
          if (commandFunction == "decideContinueOrNewTrack") {
            viewMap.viewModel.askForTrackTreatment(
              confirmHandler = {
                viewMap.coreObject!!.executeCoreCommand("setRecordTrack", "1")
                viewMap.coreObject!!.executeCoreCommand("createNewTrack")
              },
              dismissHandler = {
                viewMap.coreObject!!.executeCoreCommand("setRecordTrack", "1")
              }
            )
            commandExecuted = true
          }
          if (commandFunction == "changeMapLayer") {
            //viewMap.changeMapLayer()
            //commandExecuted = true
          }
          if (commandFunction == "askForMapDownloadDetails") {
            viewMap.viewModel.askForMapDownloadDetails() { selectedMapLayers ->
              if (selectedMapLayers.isNotEmpty()) {
                val args = arrayOfNulls<String>(selectedMapLayers.size + 2)
                args[0] = "0"
                args[1] = commandArgs[0]
                for (i in selectedMapLayers.indices) {
                  args[i + 2] = selectedMapLayers[i]
                }
                viewMap.coreObject!!.executeCoreCommand("addDownloadJob", *args)
              }
            }
            commandExecuted = true
          }
          if (commandFunction == "updateDownloadJobSize") {
            /*val alert = viewMap.mapDownloadDialog
            if (alert != null) {
              alert.setContent(
                R.string.download_job_estimated_size_message,
                commandArgs[0], commandArgs[1]
              )
              if (commandArgs[2].toInt() == 1) {
                alert.getActionButton(DialogAction.POSITIVE).isEnabled = false
              } else {
                alert.getActionButton(DialogAction.POSITIVE).isEnabled = true
              }
            }
            commandExecuted = true*/
          }
          if (commandFunction == "showMenu") {
            viewMap.viewModel.drawerStatus=DrawerValue.Open
            commandExecuted = true
          }
          if (commandFunction == "decideWaypointImport") {
            /*viewMap.nestedImportWaypointsDecisions++
            val builder = MaterialDialog.Builder(viewMap)
            builder.title(R.string.waypoint_import_title)
            builder.content(
              viewMap.resources.getString(
                R.string.waypoint_import_message,
                commandArgs[1], commandArgs[0]
              )
            )
            builder.cancelable(true)
            builder.positiveText(R.string.yes)
            builder.onPositive { dialog, which ->
              val path = "Navigation/Route[@name='" + commandArgs[0] + "']"
              viewMap.coreObject.configStoreSetStringValue(path, "importWaypoints", "1")
              viewMap.nestedImportWaypointsDecisions--
              if (viewMap.nestedImportWaypointsDecisions == 0) {
                viewMap.restartCore(false)
              }
            }
            builder.negativeText(R.string.no)
            builder.onNegative { dialog, which ->
              val path = "Navigation/Route[@name='" + commandArgs[0] + "']"
              viewMap.coreObject.configStoreSetStringValue(path, "importWaypoints", "2")
              viewMap.nestedImportWaypointsDecisions--
            }
            builder.icon(viewMap.resources.getDrawable(android.R.drawable.ic_dialog_info))
            val alert: Dialog = builder.build()
            alert.show()
            commandExecuted = true*/
          }
          if (commandFunction == "authenticateGoogleBookmarks") {
            val intent = Intent(viewMap, AuthenticateGoogleBookmarks::class.java)
            //intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            viewMap.startActivity(intent)
            commandExecuted = true
          }
          if (commandFunction == "askForRouteRemovalKind") {
            /*viewMap.askForRouteRemovalKind()
            commandExecuted = true*/
          }
          if (commandFunction == "setExitBusyText") {
            viewMap.setExitBusyText()
            commandExecuted = true
          }
          if (!commandExecuted) {
            GDApplication.addMessage(
              GDApplication.ERROR_MSG, "GDApp",
              "unknown command $command received"
            )
          }
        }
      }
    }
  }
  var coreMessageHandler = CoreMessageHandler(this)

  // Creates the activity
  @ExperimentalAnimationApi
  @ExperimentalMaterial3Api
  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)

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
    if (sensorManager==null) {
      Toast.makeText(this, getString(R.string.missing_system_service), Toast.LENGTH_LONG)
      finish();
      return
    }
    locationManager = this.getSystemService(LOCATION_SERVICE) as LocationManager
    if (locationManager==null) {
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

    // Create the background task
    backgroundTask.onCreate(coreObject)

    // Init the intent handler
    intentHandler.onCreate()

    // Create the content for the navigation drawer
    val navigationItems=arrayOf(
      NavigationItem(null, getString(R.string.map), { }),
      NavigationItem(Icons.Outlined.Article, getString(R.string.show_legend), { }),
      NavigationItem(Icons.Outlined.Download, getString(R.string.download_map)) {
        viewModel.askForMapDownloadType(
          dismissHandler = {
            coreObject!!.executeAppCommand("askForMapDownloadDetails(\"\")")
          },
          confirmHandler={
            coreObject!!.executeCoreCommand("downloadActiveRoute")
          }
        )
      },
      NavigationItem(Icons.Outlined.CleaningServices, getString(R.string.cleanup_map), { }),
      NavigationItem(null, getString(R.string.routes), { }),
      NavigationItem(Icons.Outlined.AddCircle, getString(R.string.add_tracks_as_routes), { }),
      NavigationItem(Icons.Outlined.RemoveCircle, getString(R.string.remove_routes), { }),
      NavigationItem(Icons.Outlined.SendToMobile, getString(R.string.export_selected_route), { }),
      NavigationItem(Icons.Outlined.Directions, getString(R.string.brouter), { }),
      NavigationItem(null, getString(R.string.general), { }),
      NavigationItem(Icons.Outlined.Help, getString(R.string.help), { }),
      NavigationItem(Icons.Outlined.Settings, getString(R.string.preferences), { }),
      NavigationItem(Icons.Outlined.Replay, getString(R.string.restart), { }),
      NavigationItem(Icons.Outlined.Logout, getString(R.string.exit), { }),
      NavigationItem(null, getString(R.string.debug), { }),
      NavigationItem(Icons.Outlined.Message, getString(R.string.toggle_messages)) {
        viewModel.toggleMessagesVisibility()
      },
      NavigationItem(Icons.Outlined.UploadFile, getString(R.string.send_logs), { }),
      NavigationItem(Icons.Outlined.Clear, getString(R.string.reset), { }),
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
        content(viewModel,appVersion,navigationItems.toList())
      }
    }

    // Restore the last processed intent from the prefs
    prefs = application.getSharedPreferences("viewMap", MODE_PRIVATE)
    if (prefs!!.contains("processIntent")) {
      val processIntent: Boolean = prefs!!.getBoolean("processIntent", true)
      if (!processIntent) {
        getIntent().addFlags(Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY)
      }
      val prefsEditor: Editor = prefs!!.edit()
      prefsEditor.putBoolean("processIntent", true)
      prefsEditor.commit()
    }
  }

  // Destroys the activity
  @SuppressLint("Wakelock")
  override fun onDestroy() {
    super.onDestroy()
    cancel() // Stop all coroutines
    GDApplication.addMessage(
      GDApplication.DEBUG_MSG,
      "GDApp",
      "onDestroy called by " + Thread.currentThread().name
    )
    (application as GDApplication).setActivity(null, null)
    if (backgroundTask!=null) backgroundTask!!.onDestroy()
    //if (wakeLock != null && wakeLock.isHeld()) wakeLock.release()
    //if (downloadCompleteReceiver != null) unregisterReceiver(downloadCompleteReceiver)
    if (exitRequested) System.exit(0)
    if (restartRequested) {
      if (Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB) {
        val intent = Intent(applicationContext, ViewMap2::class.java)
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
    viewModel.fixSurfaceViewBug=true
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

    // Process intent only if geo discoverer is initialized

    // Process intent only if geo discoverer is initialized
    if (coreObject!!.coreLateInitComplete) intentHandler.processIntent()
  }

  // Main content of the activity
  @ExperimentalAnimationApi
  @ExperimentalMaterial3Api
  @Composable
  fun content(viewModel: ActivityViewModel, appVersion: String, navigationItems: List<NavigationItem>) {
    Box(
      modifier=Modifier
        .fillMaxSize()
    ) {
      Scaffold(
        content = { innerPadding ->
          val drawerState =
            rememberDrawerState(initialValue = viewModel.drawerStatus, confirmStateChange = {
              GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","drawerState=$it")
              viewModel.drawerStatus = it
              true
            })
          LaunchedEffect(viewModel.drawerStatus) {
            if ((drawerState.isOpen) && (viewModel.drawerStatus == DrawerValue.Closed))
              drawerState.close()
            if ((!drawerState.isOpen) && (viewModel.drawerStatus == DrawerValue.Open))
              drawerState.open()
          }
          val density = LocalDensity.current
          val scope = rememberCoroutineScope()
          NavigationDrawer(
            drawerState = drawerState,
            modifier = Modifier
              .fillMaxWidth()
              .fillMaxHeight(),
            gesturesEnabled = drawerState.isOpen || viewModel.messagesVisible,
            drawerShape = drawerShape(),
            drawerContent = drawerContent(appVersion, innerPadding, navigationItems) {
              scope.launch() {
                viewModel.drawerStatus = DrawerValue.Closed
              }
            }
          ) {
            screenContent(viewModel)
          }
        }
      )
      if (viewModel.progressMax!=0) {
        AlertDialog(
          modifier = Modifier
            .wrapContentHeight(),
          onDismissRequest = {
          },
          confirmButton = {
          },
          title = {
            Text(text = viewModel.progressMessage)
          },
          text = {
            GDLinearProgressIndicator(
              modifier = Modifier.fillMaxWidth(),
              progress = viewModel.progressCurrent.toFloat() / viewModel.progressMax.toFloat()
            )
          }
        )
      }
      if (viewModel.dialogMessage!="") {
        AlertDialog(
          modifier = Modifier
            .wrapContentHeight(),
          onDismissRequest = {
            viewModel.setDialog("")
          },
          confirmButton = {
            TextButton(
              onClick = {
                if (viewModel.dialogIsFatal)
                  finish()
                viewModel.setDialog("")
              }
            ) {
              if (viewModel.dialogIsFatal)
                Text(stringResource(id = R.string.button_label_exit))
              else
                Text(stringResource(id = R.string.button_label_ok))
            }
          },
          icon = {
            if (viewModel.dialogIsFatal)
              Icon(Icons.Filled.Error, contentDescription = null)
            else
              Icon(Icons.Filled.Warning, contentDescription = null)
          },
          text = {
            Text(
              text = viewModel.dialogMessage,
              style = MaterialTheme.typography.bodyLarge
            )
          }
        )
      }
      if (viewModel.askTitle!="") {
        if (viewModel.askEditTextValue!="") {
          val editTextValue = remember { mutableStateOf(viewModel.askEditTextValue) }
          askAlertDialog(
            viewModel=viewModel,
            confirmHandler = {
              viewModel.askEditTextConfirmHandler(editTextValue.value)
            },
            content = {
              Column(
                modifier = Modifier
                  .wrapContentHeight()
              ) {
                viewModel.askEditTextValueChangeHandler(editTextValue.value)
                GDTextField(
                  value = editTextValue.value,
                  textStyle = MaterialTheme.typography.bodyLarge,
                  label = {
                    Text(
                      text = viewModel.askMessage
                    )
                  },
                  onValueChange = {
                    editTextValue.value = it
                    viewModel.askEditTextValueChangeHandler(it)
                  })
                if (viewModel.askEditTextHint != "") {
                  Text(
                    modifier = Modifier
                      .padding(top = layoutParams.itemPadding, start = layoutParams.hintIndent),
                    text = viewModel.askEditTextHint
                  )
                }
                if (viewModel.askEditTextError != "") {
                  Text(
                    modifier = Modifier
                      .padding(top = layoutParams.itemPadding, start = layoutParams.hintIndent),
                    text = viewModel.askEditTextError,
                    color = MaterialTheme.colorScheme.error
                  )
                }
              }
            }
          )
        } else if (viewModel.askMultipleChoiceList.isNotEmpty()) {
          askAlertDialog(
            viewModel=viewModel,
            confirmHandler = {
              val result = mutableListOf<String>()
              viewModel.askMultipleChoiceList.forEach() {
                if (it.checked) {
                  result.add(it.text)
                  //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","${it.text} selected")
                }
              }
              viewModel.askMultipleChoiceConfirmHandler(result)
            },
            content = {
              LazyColumn(
                modifier = Modifier
                  .fillMaxWidth()
                  .wrapContentHeight()
                  .heightIn(max=layoutParams.askMaxContentHeight)
              ) {
                itemsIndexed(viewModel.askMultipleChoiceList) { index, item ->
                  val checked = remember { mutableStateOf(item.checked) }
                  Row(
                    modifier = Modifier
                      .fillMaxWidth()
                      .wrapContentHeight(),
                    verticalAlignment = Alignment.CenterVertically
                  ) {
                    Checkbox(
                      checked = checked.value,
                      onCheckedChange = {
                        checked.value=it
                        item.checked=it
                      }
                    )
                    Text(
                      text = item.text,
                      style = MaterialTheme.typography.bodyLarge
                    )
                  }
                }
              }
            }
          )
        } else {
          askAlertDialog(
            viewModel=viewModel,
            confirmHandler = {
              viewModel.askQuestionConfirmHandler()
            },
            content = {
              Text(
                text = viewModel.askMessage,
                style = MaterialTheme.typography.bodyLarge
              )
            }
          )
        }
      }
      AnimatedVisibility(
        modifier = Modifier
          .align(Alignment.BottomCenter),
        enter = fadeIn() + slideInVertically(initialOffsetY = { +it / 2 }),
        exit = fadeOut() + slideOutVertically(targetOffsetY = { +it / 2 }),
        visible = viewModel.snackbarText != ""
      ) {
        GDSnackBar(
          modifier = Modifier
            .padding(horizontal = layoutParams.snackbarHorizontalPadding)
            .padding(bottom = layoutParams.snackbarVerticalOffset)
            .widthIn(max = layoutParams.snackbarMaxWidth)
        ) {
          Row(
            verticalAlignment = Alignment.CenterVertically
          ) {
            Text(
              modifier = Modifier
                .weight(1.0f),
              text = viewModel.snackbarText,
              color = MaterialTheme.colorScheme.onBackground
            )
            if (viewModel.snackbarActionText!="") {
              TextButton(
                onClick = {
                  viewModel.snackbarActionHandler()
                  viewModel.showSnackbar("")
                }
              ) {
                Text(
                  text = viewModel.snackbarActionText,
                  color = MaterialTheme.colorScheme.primary
                )
              }
            }
          }
        }
      }
    }
  }

  // Alert dialog for multiple use cases
  @Composable
  private fun askAlertDialog(
    viewModel: ActivityViewModel,
    confirmHandler: ()->Unit,
    content: @Composable ()->Unit
  ) {
    AlertDialog(
      modifier = Modifier
        .wrapContentHeight(),
      onDismissRequest = {
        viewModel.closeQuestion()
      },
      confirmButton = {
        TextButton(
          onClick = {
            confirmHandler()
            viewModel.closeQuestion()
          }
        ) {
          Text(viewModel.askConfirmText)
        }
      },
      dismissButton = {
        TextButton(
          onClick = {
            viewModel.askQuestionDismissHandler()
            viewModel.closeQuestion()
          }
        ) {
          Text(viewModel.askDismissText)
        }
      },
      title = {
        Text(text = viewModel.askTitle)
      },
      text = {
        content()
      }
    )
  }

  // Main content on the screen
  @ExperimentalAnimationApi
  @Composable
  private fun screenContent(viewModel: ActivityViewModel) {
    val scope = rememberCoroutineScope()
    Box(
      modifier = Modifier
        .fillMaxSize()
    ) {
      AndroidView(
        factory = { context ->
          GDMapSurfaceView(context, null).apply {
            layoutParams = ViewGroup.LayoutParams(
              ViewGroup.LayoutParams.MATCH_PARENT,
              ViewGroup.LayoutParams.MATCH_PARENT,
            )
            setCoreObject(coreObject)
          }
        },
        modifier = Modifier
          .fillMaxWidth()
          .fillMaxHeight(),
      )
      if (viewModel.fixSurfaceViewBug) {
        Box(
          modifier=Modifier
            .fillMaxSize()
            //.background(Color.Red)
        )
        LaunchedEffect(Unit) {
          delay(500)
          viewModel.fixSurfaceViewBug=false
        }
      }
      if (viewModel.messagesVisible) {
        Column(
          modifier = Modifier
            .fillMaxSize()
            .background(Color.Black.copy(alpha = 0.8f)),
          horizontalAlignment = Alignment.CenterHorizontally
        ) {
          if (viewModel.splashVisible) {
            Image(
              painter = painterResource(R.drawable.splash),
              contentDescription = null
            )
            Text(
              text = viewModel.busyText,
              style = MaterialTheme.typography.headlineSmall,
              color = MaterialTheme.colorScheme.onBackground
            )
            Spacer(Modifier.height(20.dp))
          }
          AndroidView(
            modifier = Modifier
              .fillMaxSize(),
            factory = { context ->
              TextView(context).apply {
                layoutParams = ViewGroup.LayoutParams(
                  ViewGroup.LayoutParams.MATCH_PARENT,
                  ViewGroup.LayoutParams.MATCH_PARENT,
                )
                gravity = Gravity.BOTTOM
                typeface = Typeface.MONOSPACE
                setTextSize(TypedValue.COMPLEX_UNIT_SP, 10.0f)
              }
            },
            update = { view ->
              view.text = viewModel.messages
            }
          )
        }
      }
      LaunchedEffect(viewModel.snackbarText) {
        if (viewModel.snackbarText != "") {
          delay(3000)
          viewModel.showSnackbar("")
        }
      }
    }
  }

  // Content of the navigation drawer
  @Composable
  private fun drawerContent(
    appVersion: String,
    innerPadding: PaddingValues,
    navigationItems: List<NavigationItem>,
    closeDrawer: ()->Unit
  ): @Composable() (ColumnScope.() -> Unit) =
    {
      Column(
        modifier = Modifier
          .width(layoutParams.drawerWidth)
      ) {
        Box(
          modifier= Modifier
            .background(MaterialTheme.colorScheme.surfaceVariant)
            .fillMaxWidth(),
          contentAlignment = Alignment.CenterStart
        ) {
          /*Image(
            modifier = Modifier.fillMaxWidth(),
            painter = painterResource(R.drawable.nav_header),
            contentDescription = null,
            contentScale = ContentScale.FillWidth
          )*/
          Row(
            verticalAlignment = Alignment.CenterVertically
          ) {
            Column(
              modifier = Modifier
                .width(layoutParams.iconWidth),
              horizontalAlignment = Alignment.CenterHorizontally
            ) {
              Image(
                painter = painterResource(R.mipmap.ic_launcher),
                contentDescription = null
              )
            }
            Column() {
              Text(
                text = stringResource(id = R.string.app_name),
                style = MaterialTheme.typography.headlineSmall
              )
              Text(
                text = appVersion,
                style = MaterialTheme.typography.bodyMedium
              )
              Spacer(Modifier.height(4.dp))
            }
          }
        }
        LazyColumn(
          modifier = Modifier
            .padding(innerPadding)
            .fillMaxWidth()
        ) {
          itemsIndexed(navigationItems) { index, item ->
            navigationItem(index, item, closeDrawer)
          }
        }
      }
    }

  // Sets the width of the navigation drawer correctly
  fun drawerShape() = object : Shape {
    override fun createOutline(
      size: Size,
      layoutDirection: LayoutDirection,
      density: Density
    ): Outline {
      val cornerRadius=with(density) { layoutParams.drawerCornerRadius.toPx() }
      return Outline.Rounded(RoundRect(
        left=0f,
        top=0f,
        right=with(density) { layoutParams.drawerWidth.toPx() },
        bottom=size.height,
        topRightCornerRadius = CornerRadius(cornerRadius),
        bottomRightCornerRadius = CornerRadius(cornerRadius)
      ))
    }
  }

  // Creates a navigation item for the drawer
  @Composable
  fun navigationItem(index: Int, item: NavigationItem, closeDrawer: ()->Unit) {
    if (item.imageVector == null) {
      if (index != 0) {
        Box(
          modifier = Modifier
            .fillMaxWidth()
            .padding(layoutParams.itemPadding)
            .height(1.dp)
            .background(MaterialTheme.colorScheme.outline)
        )
      }
      Row(
        modifier = Modifier
          .fillMaxWidth()
          .padding(vertical = layoutParams.itemPadding),
        verticalAlignment = Alignment.CenterVertically
      ) {
        Column(
          modifier = Modifier
            .absolutePadding(left = layoutParams.titleIndent)
        ) {
          Text(
            text = item.title,
            style = MaterialTheme.typography.titleSmall
          )
        }
      }
    } else {
      val interactionSource = remember { MutableInteractionSource() }
      Box(
        modifier = Modifier
          .fillMaxWidth()
          .padding(horizontal = layoutParams.itemPadding)
          .clip(shape = RoundedCornerShape(layoutParams.drawerCornerRadius))
          .clickable(
            onClick = {
              item.onClick()
              closeDrawer()
            },
            interactionSource = interactionSource,
            indication = rememberRipple(bounded = true)
          )
      ) {
        Row(
          modifier = Modifier
            .fillMaxWidth(),
          verticalAlignment = Alignment.CenterVertically
        ) {
          Spacer(
            Modifier
              .width(0.dp)
              .height(layoutParams.itemHeight)
          )
          Column(
            modifier = Modifier
              .width(layoutParams.iconWidth),
            horizontalAlignment = Alignment.CenterHorizontally
          ) {
            Icon(
              imageVector = item.imageVector,
              contentDescription = null
            )
          }
          Column(
            modifier = Modifier
              .weight(1f)
          ) {
            Text(
              text = item.title,
              style = MaterialTheme.typography.bodyMedium
            )
          }
        }
      }
    }
  }

  @ExperimentalMaterial3Api
  @Preview(showBackground = true)
  @Composable
  fun preview() {
    AndroidTheme {
    }
  }
}

