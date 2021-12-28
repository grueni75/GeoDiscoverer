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
import com.untouchableapps.android.geodiscoverer.R
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.core.GDCore
import com.untouchableapps.android.geodiscoverer.core.GDTools

import kotlinx.coroutines.*
import java.io.FileNotFoundException
import java.io.IOException
import java.io.InputStream

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
            coreObject!!.scheduleCoreCommand(
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
}

