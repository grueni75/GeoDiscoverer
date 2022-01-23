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
import android.util.Log
import android.widget.Toast
import androidx.compose.animation.ExperimentalAnimationApi
import androidx.compose.material.ExperimentalMaterialApi
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.ui.graphics.ExperimentalGraphicsApi
import com.untouchableapps.android.geodiscoverer.R
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.core.GDCore
import com.untouchableapps.android.geodiscoverer.core.GDTools
import com.untouchableapps.android.geodiscoverer.ui.activity.ViewMap
import com.untouchableapps.android.geodiscoverer.ui.activity.viewmap.ViewModel

import kotlinx.coroutines.*
import org.acra.ACRA
import org.mapsforge.core.model.LatLong
import org.mapsforge.poi.android.storage.AndroidPoiPersistenceManagerFactory
import org.mapsforge.poi.storage.*
import java.io.*
import java.lang.Exception

@ExperimentalGraphicsApi
@ExperimentalMaterialApi
@ExperimentalMaterial3Api
@ExperimentalAnimationApi
class GDBackgroundTask() : CoroutineScope by MainScope() {

  // Arguments
  var coreObject: GDCore? = null

  // POI search engine
  val poiManagers = mutableListOf<PoiPersistenceManager>()
  var poiInitComplete = false
  var poiMaxCount = 0
  var poiFindJob: Job? = null

  // Address point item
  class AddressPointItem(nameOriginal: String, nameUniquified: String, distanceRaw: Double, distanceFormatted: String, latitude: Double, longitude: Double) {
    var nameOriginal=nameOriginal
    var nameUniquified=nameUniquified
    var distanceRaw=distanceRaw
    var distanceFormatted=distanceFormatted
    val latitude=latitude
    val longitude=longitude
  }

  // Inits everything
  fun onCreate(coreObject: GDCore?) {
    GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp", "onCreate in GDBackgroundTask called")
    // Store important references
    this.coreObject = coreObject

    // Read settings
    poiMaxCount=coreObject!!.configStoreGetStringValue("Navigation", "poiMaxCount").toInt()

    // Add all available POIs
    launch() {
      withContext(Dispatchers.IO) {
        val mapsPath = coreObject!!.homePath + "/Server/Mapsforge/Map"
        val mapsDir = File(mapsPath)
        val children = mapsDir.listFiles()
        val poiFiles = mutableListOf<File>()
        if (children != null) {
          for (child in children) {
            if (child.isFile && child.path.endsWith(".poi")) {
              poiFiles.add(child)
            }
          }
        }
        if (poiFiles.size == 0) {
          coreObject.executeAppCommand("errorDialog(\"No POI databases installed in <$mapsPath>!\")")
          return@withContext
        } else {
          for (poiFile in poiFiles) {
            //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", poiFile.name)
            val poiManager = AndroidPoiPersistenceManagerFactory.getPoiPersistenceManager(
              poiFile.absolutePath
            )
            poiManagers.add(poiManager)
          }
        }
        poiInitComplete = true
      }
    }
  }

  // Stops all running threads
  fun onDestroy() {

    // Cancel all runnings threads
    cancel()

    // Free up all poi databases
    for (poiManager in poiManagers) {
      poiManager.close()
    }
    poiManagers.clear()
  }

  // Looksup an address
  fun getLocationFromAddress(
    context: Context, name: String, address: String, group: String, result: (Boolean)->Unit = {}
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
    context: Context, name: String, address: String, group: String
  ) {
    getLocationFromAddress(context, name, address, group) { locationFound ->
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
  fun importRoute(context: Context, name: String, uri: Uri, result: (Boolean, String)->Unit) {
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
  fun copyTracksToRoutes(context: Context, selectedTracks: List<String>, viewMap: ViewMap) {
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
  fun removeRoutes(context: Context, selectedRoutes: List<String>, viewMap: ViewMap) {
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

  // Returns the poi category for the given path
  private fun findPOICategory(poiManager: PoiPersistenceManager, path: List<String>): PoiCategory? {
    var allFound = true
    var fullTitle = "* "
    var poiCategory = poiManager.categoryManager.rootCategory
    for (level in path) {
      //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","level=${level}")
      var found = false
      for (childPoiCategory in poiCategory.children) {
        if (childPoiCategory.title == level) {
          poiCategory = childPoiCategory
          if (fullTitle != "* ")
            fullTitle = "${fullTitle} -> "
          fullTitle = "${fullTitle}${poiCategory.title}"
          found = true
          break
        }
      }
      if (!found) {
        allFound = false
        break
      }
    }
    if (allFound) {
      //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","category found: ${fullTitle} with ${poiCategory.children.size} entries")
      return poiCategory
    } else {
      //coreObject!!.executeAppCommand("errorDialog(\"Category <$fullTitle> can not be found!\")")
      return null
    }
  }

  // Adds all current address points
  fun fillAddressPoints(): List<AddressPointItem> {

    // Get all address points for the currently selected address group
    var result = mutableListOf<AddressPointItem>()
    val selectedGroupName = coreObject!!.configStoreGetStringValue("Navigation", "selectedAddressPointGroup")
    val unsortedNames = coreObject!!.configStoreGetAttributeValues("Navigation/AddressPoint", "name").toMutableList()
    //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","selectedGroupName=${selectedGroupName} size=${unsortedNames.size}")
    while (unsortedNames.size > 0) {
      var newestName = unsortedNames.removeFirst()
      val foreignRemovalRequest: String = coreObject!!.configStoreGetStringValue(
        "Navigation/AddressPoint[@name='$newestName']",
        "foreignRemovalRequest"
      )
      val groupName: String = coreObject!!.configStoreGetStringValue(
        "Navigation/AddressPoint[@name='$newestName']",
        "group"
      )
      if (groupName == selectedGroupName && foreignRemovalRequest != "1") {
        //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","newestName=${newestName} foreignRemovalRequest=${foreignRemovalRequest} groupName=${groupName}")

        // Create the address point
        val latitude: String = coreObject!!.configStoreGetStringValue(
          "Navigation/AddressPoint[@name='$newestName']",
          "lat"
        )
        val longitude: String = coreObject!!.configStoreGetStringValue(
          "Navigation/AddressPoint[@name='$newestName']",
          "lng"
        )
        val distanceInfo = coreObject!!.executeCoreCommand("computeDistanceToAddressPoint",newestName)
        val distanceFields = distanceInfo.split(";")
        val ap=AddressPointItem(
          nameOriginal = newestName,
          nameUniquified = newestName,
          distanceRaw = distanceFields[0].toDouble(),
          distanceFormatted = distanceFields[1],
          latitude = latitude.toDouble(),
          longitude = longitude.toDouble()
        )

        // Sort it into the list according how far away it is
        var inserted=false
        for (i in result.indices) {
          if (ap.distanceRaw<result[i].distanceRaw) {
            result.add(i, ap)
            inserted=true
            break
          }
        }
        if (!inserted) {
          result.add(ap)
        }
      }
    }
    return result
  }

  // Adds all categories
  fun fillPOICategories(categoryPath: List<String>): List<String> {
    if (!poiInitComplete)
      return emptyList()
    val result = mutableListOf<String>()
    for (poiManager in poiManagers) {
      val poiCategory=findPOICategory(poiManager,categoryPath)
      if (poiCategory!=null) {
        for (childPoiCategory in poiCategory.children) {
          if (!result.contains(childPoiCategory.title)) {
            result.add(childPoiCategory.title)
          }
        }
      }
    }
    result.sort()
    return result.toList()
  }

  // Check for outdated routes
  fun findPOIs(categoryPath: List<String>, searchRadius: Int, resultCallback: (List<AddressPointItem>,Boolean)->Unit): Boolean {

    // Only start working if initialized and no job running already
    val result = mutableListOf<AddressPointItem>()
    if (!poiInitComplete) {
      resultCallback(result,false)
      return true
    }
    if (poiFindJob!=null) {
      //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","poi search job already running")
      return false
    }
    poiFindJob = launch() {
      var limitReached=false
      withContext(Dispatchers.IO) {

        // Decode the parameters
        var fullPath="* "
        for (level in categoryPath) {
          fullPath="${fullPath} -> ${level}"
        }
        //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","Searching in category path ${fullPath}")
        val t=coreObject!!.executeCoreCommand("getMapPos")
        val fields=t.split(",")
        val currentPos=LatLong(fields[0].toDouble(),fields[1].toDouble())

        // Go through all available POI databases
        for (poiManager in poiManagers) {
          val poiCategory=findPOICategory(poiManager,categoryPath)
          if (poiCategory!=null) {
            var nearbyPOIs: Collection<PointOfInterest>? = null
            try {

              // Search within the radius and selected category
              val categoryFilter: PoiCategoryFilter = WhitelistPoiCategoryFilter()
              categoryFilter.addCategory(poiCategory)
              nearbyPOIs=poiManager.findNearPosition(
                currentPos,
                searchRadius,
                categoryFilter,
                null,
                poiMaxCount
              )
            } catch (t: Throwable) {
              GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",t.message)
            }
            if (nearbyPOIs!=null) {

              // Did we hit the search limit?
              if (nearbyPOIs.size>=poiMaxCount)
                limitReached=true

              // Add the found POIs incl. duplicate handling
              for (poi in nearbyPOIs) {

                // Decide on the POI details
                var name = poi.toString()
                if (poi.name != null) {
                  name = poi.name
                } else {
                  if (poi.category != null) {
                    name = "${poi.category.title} (${poi.latitude},${poi.longitude})"
                  }
                }
                var distance = currentPos.sphericalDistance(poi.latLong)

                // Check for duplicates
                var duplicate=false
                for (point in result) {
                  if (point.nameOriginal == name) {
                    if ((point.latitude==poi.latitude)&&(point.longitude==poi.longitude)) {
                      //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","POI ${name} already exists in search result")
                      duplicate=true
                      break
                    }
                  }
                }
                if (duplicate)
                  continue

                // Uniquify the name
                var uniqueName = name
                var nameIsUnique = false
                var count = 1
                while (!nameIsUnique) {
                  nameIsUnique = true
                  for (point in result) {
                    if (point.nameUniquified == uniqueName) {
                      uniqueName = "${name} ${count}"
                      count++
                      break
                    }
                  }
                }
                /*if (count>1) {
                  GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","POI ${name} uniquified to ${uniqueName}")
                }*/

                // Add the entry
                var item = AddressPointItem(
                  name,
                  uniqueName,
                  distance,
                  coreObject!!.executeCoreCommand("formatMeters", distance.toString()),
                  poi.latitude,
                  poi.longitude
                )
                var added = false
                for (i in result.indices) {
                  if (distance < result[i].distanceRaw) {
                    result.add(i, item)
                    added=true
                    break
                  }
                }
                if (!added)
                  result.add(item)
              }
            }
          }
        }
      }
      poiFindJob=null
      resultCallback(result,limitReached)
    }
    return true
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

