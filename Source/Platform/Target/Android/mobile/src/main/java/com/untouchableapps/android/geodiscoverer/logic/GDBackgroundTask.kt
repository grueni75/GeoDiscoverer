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

import android.annotation.SuppressLint
import android.content.Context
import android.content.Intent
import android.location.Geocoder
import android.net.Uri
import android.os.*
import android.provider.OpenableColumns
import android.widget.Toast
import androidx.compose.material3.*
import com.untouchableapps.android.geodiscoverer.R
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.core.GDCore
import com.untouchableapps.android.geodiscoverer.ui.activity.ViewMap2

import kotlinx.coroutines.*
import java.io.IOException

class GDBackgroundTask(context: Context, coreObject: GDCore) : CoroutineScope by MainScope() {

  val context=context
  val coreObject=coreObject

  // Stops all running threads
  fun stop() {
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
            coreObject.scheduleCoreCommand(
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

  // Looksup an address
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

}

