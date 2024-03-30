//============================================================================
// Name        : CoreMessageHandler.kt
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

package com.untouchableapps.android.geodiscoverer.ui.activity.viewmap

import android.content.*
import android.os.*
import androidx.compose.animation.ExperimentalAnimationApi
import androidx.compose.material.ExperimentalMaterialApi
import androidx.compose.material3.*
import androidx.compose.ui.ExperimentalComposeUiApi
import androidx.compose.ui.graphics.ExperimentalGraphicsApi
import com.untouchableapps.android.geodiscoverer.R
import java.util.*
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.logic.GDBackgroundTask
import com.untouchableapps.android.geodiscoverer.logic.GDService
import com.untouchableapps.android.geodiscoverer.ui.activity.AuthenticateGoogleBookmarks
import com.untouchableapps.android.geodiscoverer.ui.activity.ViewMap
import java.lang.ref.WeakReference
import kotlin.random.Random.Default.nextBoolean

@ExperimentalGraphicsApi
@ExperimentalMaterial3Api
@ExperimentalAnimationApi
@ExperimentalMaterialApi
@ExperimentalComposeUiApi
class CoreMessageHandler(viewMap: ViewMap) : Handler(Looper.getMainLooper()) {

  var weakViewMap: WeakReference<ViewMap> = WeakReference(viewMap)

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
          viewMap.viewModel.openProgress(commandArgs[0], commandArgs[1].toInt())
          commandExecuted = true
        }
        if (commandFunction == "updateProgressDialog") {
          viewMap.viewModel.setProgress(commandArgs[1].toInt(),commandArgs[0])
          commandExecuted = true
        }
        if (commandFunction == "closeProgressDialog") {
          viewMap.viewModel.closeProgress()
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
          viewMap.viewModel.messages = GDApplication.messages
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
          viewMap.viewModel.manageAddressPoints()
          commandExecuted = true
        }
        if (commandFunction == "nearbyAddressPoint") {
          GDApplication.backgroundTask.selectAddressPoint(viewMap,commandArgs[0])
          commandExecuted = true
        }
        if (commandFunction == "addressPointsUpdated") {
          viewMap.viewModel.refreshAddressPoints()
          commandExecuted = true
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
        if (commandFunction == "earlyInitComplete") {
          viewMap.viewModel.onCoreEarlyInitComplete()
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
          viewMap.viewModel.askForMapLayer() { index ->
            viewMap.coreObject!!.executeCoreCommand("selectMapLayer", viewMap.viewModel.integratedListItems[index].left)
          }
          commandExecuted = true
        }
        if (commandFunction == "askForMapDownloadDetails") {
          viewMap.viewModel.askForMapDownloadDetails(commandArgs[0]) { selectedMapLayers ->
            if (selectedMapLayers.isNotEmpty()) {
              viewMap.addMapDownloadJob(false, commandArgs[0], selectedMapLayers)
            }
          }
          commandExecuted = true
        }
        if (commandFunction == "updateDownloadJobSize") {
          viewMap.viewModel.setMessage(
            viewMap.getString(
              R.string.dialog_download_job_estimated_size_message, commandArgs[0], commandArgs[1]
            )
          )
          commandExecuted = true
        }
        if (commandFunction == "showMenu") {
          viewMap.viewModel.drawerStatus = DrawerValue.Open
          commandExecuted = true
        }
        if (commandFunction == "decideWaypointImport") {
          viewMap.viewModel.askForImportWaypointsDecision(
            gpxFilename=commandArgs[0],
            waypointCount=commandArgs[1],
            confirmHandler = {
              val path = "Navigation/Route[@name='" + commandArgs[0] + "']"
              viewMap.coreObject!!.configStoreSetStringValue(path, "importWaypoints", "1")
              if (!viewMap.viewModel.isImportWaypointsDecisionPending()) {
                viewMap.restartCore(false)
              } else {
                viewMap.viewModel.restartCoreAfterNoPendingWaypointsDecision=true
              }
            },
            dismissHandler = {
              val path = "Navigation/Route[@name='" + commandArgs[0] + "']"
              viewMap.coreObject!!.configStoreSetStringValue(path, "importWaypoints", "2")
              if (!viewMap.viewModel.isImportWaypointsDecisionPending()) {

                // We only need to restart in case yes was selected in other decisions
                if (viewMap.viewModel.restartCoreAfterNoPendingWaypointsDecision)
                  viewMap.restartCore(false)
              }
            }
          )
          commandExecuted = true
        }
        if (commandFunction == "authenticateGoogleBookmarks") {
          val intent = Intent(viewMap, AuthenticateGoogleBookmarks::class.java)
          //intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
          viewMap.startActivity(intent)
          commandExecuted = true
        }
        if (commandFunction == "askForRouteRemovalKind") {
          viewMap.viewModel.askForRouteRemovalKind(
            confirmHandler = {
              viewMap.coreObject!!.executeCoreCommand("hidePath")
            },
            dismissHandler = {
              viewMap.coreObject!!.executeCoreCommand("trashPath")
            }
          )
          commandExecuted = true
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

