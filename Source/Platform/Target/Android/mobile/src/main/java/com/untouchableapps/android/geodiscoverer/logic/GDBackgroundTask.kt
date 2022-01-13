//============================================================================
// Name        : GDBackgroundTasks.kt
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

package com.untouchableapps.android.geodiscoverer.logic

import android.content.Context
import android.location.Geocoder
import android.net.Uri
import android.widget.Toast
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.ui.graphics.ExperimentalGraphicsApi
import com.untouchableapps.android.geodiscoverer.R
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.core.GDCore
import com.untouchableapps.android.geodiscoverer.core.GDTools
import com.untouchableapps.android.geodiscoverer.ui.activity.ViewMap

import kotlinx.coroutines.*
import org.acra.ACRA
import java.io.*
import java.lang.Exception

@ExperimentalGraphicsApi
@ExperimentalMaterial3Api
class GDBackgroundTask(context: Context) : CoroutineScope by MainScope() {

  // Arguments
  val context=context
  var coreObject: GDCore? = null

  // Inits everything
  fun onCreate(coreObject: GDCore?) {
    this.coreObject=coreObject
  }

  // Stops all running threads
  fun onDestroy() {
    cancel()
  }

  // Looksup an address
  fun getLocationFromAddress(
    name: String, address: String, group: String, result: (Boolean)->Unit = {}
  ) {
    launch() {
      var locationFound = false
      withContext(Dispatchers.IO) {
        val geocoder = Geocoder(context)
        try {
          val addresses = geocoder.getFromLocationName(address, 1)
          locationFound = if (addresses.size > 0) {
            val a = addresses[0]
            coreObject!!.executeCoreCommand(
              "addAddressPoint",
              name, address, a.longitude.toString(), a.latitude.toString(),
              group
            )
            true
          } else {
            false
          }
        } catch (e: IOException) {
          GDApplication.addMessage(
            GDApplication.WARNING_MSG,
            "GDApp",
            "Geocoding not successful: " + e.message
          )
        }
      }
      result(locationFound)
    }
  }

  // Looks up an address
  fun getLocationFromAddress(
    name: String, address: String, group: String
  ) {
    getLocationFromAddress(name, address, group) { locationFound ->
      if (!locationFound) {
        Toast.makeText(
          context,
          context.getString(R.string.address_point_not_found, name),
          Toast.LENGTH_LONG
        ).show()
      } else {
        Toast.makeText(
          context,
          context.getString(R.string.address_point_added, name),
          Toast.LENGTH_LONG
        ).show()
        coreObject!!.executeAppCommand("addressPointsUpdated()")
      }
    }
  }

  // Imports a route
  fun importRoute(name: String, uri: Uri, result: (Boolean, String)->Unit) {
    launch() {
      val dstFilename = GDCore.getHomeDirPath() + "/Route/" + name
      var success = true
      var message = ""
      withContext(Dispatchers.IO) {

        // Open the content of the route file
        var gpxContents: InputStream? = null
        try {
          gpxContents = context.contentResolver.openInputStream(uri)
        } catch (e: FileNotFoundException) {
          success=false
          message=context.getString(R.string.cannot_read_uri,uri.toString())
        }
        if ((success)&&(gpxContents!=null)) {

          // Create the destination file
          try {
            GDTools.copyFile(gpxContents, dstFilename)
            gpxContents.close()
          } catch (e: IOException) {
            success=false
            message=context.getString(R.string.cannot_import_route, name)
          }
        }
      }
      result(success, message)
    }
  }

  // Copies tracks as routes
  @ExperimentalMaterial3Api
  fun copyTracksToRoutes(selectedTracks: List<String>, viewMap: ViewMap) {
    launch() {

      // Open the progress dialog
      viewMap.viewModel.openProgress(context.getString(R.string.copying_tracks),selectedTracks.size)
      withContext(Dispatchers.IO) {

        // Copy all selected tracks to the route directory
        val progress = 0
        for (trackName in selectedTracks) {
          val srcFilename: String = coreObject!!.homePath + "/Track/" + trackName
          val dstFilename: String = coreObject!!.homePath + "/Route/" + trackName
          try {
            GDTools.copyFile(srcFilename, dstFilename)
          } catch (exception: IOException) {
            viewMap.viewModel.showSnackbar(context.getString(R.string.cannot_copy_file, srcFilename, dstFilename))
          }
          viewMap.viewModel.setProgress(progress)
        }

      }
      viewMap.viewModel.closeProgress()
      viewMap.restartCore(false)
    }
  }

  // Removes routes
  @ExperimentalMaterial3Api
  fun removeRoutes(selectedRoutes: List<String>, viewMap: ViewMap) {
    launch() {

      // Open the progress dialog
      viewMap.viewModel.openProgress(context.getString(R.string.removing_routes),selectedRoutes.size)
      withContext(Dispatchers.IO) {

        // Remove the routes
        var progress = 0
        for (routeName in selectedRoutes) {
          var route = File(coreObject!!.homePath + "/Route/" + routeName + ".bin")
          route.delete() // Don't care if .bin does not exist
          route = File(coreObject!!.homePath + "/Route/" + routeName)
          if (!route.delete()) {
            viewMap.viewModel.showSnackbar(context.getString(R.string.cannot_remove_file, route.path))
          }
          progress++
          viewMap.viewModel.setProgress(progress)
        }
      }
      viewMap.viewModel.closeProgress()
      viewMap.restartCore(false)
    }
  }

  // Send logs to the developer
  @ExperimentalMaterial3Api
  fun sendLogs(selectedLogs: List<String>, viewMap: ViewMap) {
    launch() {

      // Indicate operation to user
      viewMap.dialogHandler.infoDialog(viewMap.getString(R.string.log_processing_message))
      var success=true
      var message=""
      var logContents = ""
      withContext(Dispatchers.IO) {
        try {
          for (logName in selectedLogs) {
            val logPath = coreObject!!.homePath + "/Log/" + logName
            logContents += "$logPath:\n"
            val logReader = BufferedReader(FileReader(logPath))
            var inputLine: String?
            inputLine = logReader.readLine()
            while (inputLine != null) {
              logContents += inputLine + "\n"
              //GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"GDApp",inputLine);
              inputLine = logReader.readLine()
            }
            logContents += "\n"
            logReader.close()
          }
        } catch (e: IOException) {
          success=false
          message=e.message!!
        }
      }
      if (!success) {
        viewMap.dialogHandler.errorDialog(viewMap.getString(R.string.send_logs_failed, message))
      } else {

        // Add the log to the ACRA report
        ACRA.getErrorReporter().putCustomData("userLogContents", logContents)

        // Send report via ACRA

        // Send report via ACRA
        val e = Exception("User has sent logs")
        ACRA.getErrorReporter().handleException(e, false)
      }
    }
  }

  // Check for outdated routes
  @ExperimentalMaterial3Api
  fun checkForOutdatedRoutes(viewMap: ViewMap) {
    launch() {

      // Indicate operation to user
      var routesOutdated=false
      withContext(Dispatchers.IO) {

        // Go through all route files
        val routeDir = File(coreObject!!.homePath + "/Route")
        val cacheDir = coreObject!!.homePath + "/Route/.cache"
        val routeFiles = routeDir.listFiles()
        routeFiles?.forEach { routeFile ->
          //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",routeFile.getName());
          if (routeFile.isDirectory) return@forEach
          if (routeFile.name.endsWith(".gpx")) {
            val cacheFile = File(cacheDir + "/" + routeFile.name)
            if (!cacheFile.exists()) {
              routesOutdated = true
              return@forEach
            }
            if (routeFile.lastModified() > cacheFile.lastModified()) {
              routesOutdated = true
              return@forEach
            }
          }
        }
      }
      if (routesOutdated) {
        viewMap.viewModel.showSnackbar(viewMap.getString(R.string.routes_outdated))
        viewMap.restartCore(false)
      }
    }
  }

  /* Imports *.gda files from external source
  private class ImportMapArchivesTask : AsyncTask<Void?, Int?, Void?>() {
    var progressDialog: MaterialDialog? = null
    var srcFile: File? = null
    var mapFolder: File? = null
    var mapInfoFilename: String? = null
    override fun onPreExecute() {

      // Prepare the progress dialog
      progressDialog = MaterialDialog.Builder(this@ViewMap)
        .content(getString(R.string.importing_external_map_archives, srcFile!!.name))
        .progress(true, 0)
        .cancelable(false)
        .show()
    }

    protected override fun doInBackground(vararg params: Void): Void? {

      // Create the map folder
      try {

        // Find all gda files that have the pattern <name>.gda and <name>%d.gda
        val basename: String = srcFile!!.name.replaceAll("(\\D*)\\d*\\.gda", "$1")
        val archives: LinkedList<String> = LinkedList<String>()
        val dir = File(srcFile!!.parent)
        val dirListing = dir.listFiles()
        if (dirListing != null) {
          for (child in dirListing) {
            if (child.name.matches("$basename\\d*\\.gda")) {
              archives.add(child.absolutePath)
            }
          }
        }

        // Create the info file
        mapFolder!!.mkdir()
        val cache = File(mapFolder!!.path.toString() + "/cache.bin")
        cache.delete()
        val writer = PrintWriter(mapInfoFilename, "UTF-8")
        writer.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>")
        writer.println("<GDS version=\"1.0\">")
        writer.println("  <type>externalMapArchive</type>")
        for (archive in archives) {
          writer.println(String.format("  <mapArchivePath>%s</mapArchivePath>", archive))
        }
        writer.println("</GDS>")
        writer.close()

        // Set the new folder
        coreObject.configStoreSetStringValue("Map", "folder", mapFolder!!.name)
      } catch (e: IOException) {
        errorDialog(
          java.lang.String.format(
            getString(R.string.cannot_create_map_folder),
            mapFolder!!.name
          )
        )
      }
      return null
    }

    protected override fun onProgressUpdate(vararg progress: Int) {}
    override fun onPostExecute(result: Void?) {

      // Close the progress dialog
      progressDialog!!.dismiss()

      // Restart the core to load the new map folder
      restartCore(false)
    }
  } */

}

