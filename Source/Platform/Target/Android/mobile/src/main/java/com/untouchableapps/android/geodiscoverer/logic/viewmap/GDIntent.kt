//============================================================================
// Name        : GDIntent.kt
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

package com.untouchableapps.android.geodiscoverer.logic.viewmap

import android.annotation.SuppressLint
import android.content.Intent
import android.location.Geocoder
import android.net.Uri
import android.os.*
import android.provider.OpenableColumns
import android.widget.Toast
import androidx.compose.material3.*
import com.untouchableapps.android.geodiscoverer.R
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.ui.activity.ViewMap2

import kotlinx.coroutines.*
import java.io.IOException

@ExperimentalMaterial3Api
class GDIntent(viewMap: ViewMap2) : CoroutineScope by MainScope() {

  // Opens a dialog to ask for the address in the outer activity
  private val viewMap = viewMap

  // Processes the intent of the activity
  @SuppressLint("Range")
  fun processIntent() {

    // Extract the file path from the intent
    val intent = viewMap.intent
    var uri: Uri? = null
    var subject = ""
    var text = ""
    var isAddress = false
    var isGDA = false
    var isGPXFromWeb = false
    var isGPXFromFile = false
    if (intent != null) {
      if (Intent.ACTION_SEND == intent.action) {
        val extras = intent.extras
        if (extras!!.containsKey(Intent.EXTRA_STREAM)) {
          uri = extras.getParcelable<Parcelable>(Intent.EXTRA_STREAM) as Uri?
          if (uri!!.scheme == "http" || uri.scheme == "https") {
            isGPXFromWeb = true
          } else {
            isGPXFromFile = true
          }
        } else if (extras.containsKey(Intent.EXTRA_TEXT)) {
          subject = extras.getString(Intent.EXTRA_SUBJECT)!!
          text = extras.getString(Intent.EXTRA_TEXT)!!
          isAddress = true
        } else {
          viewMap.dialogHandler.warningDialog(viewMap.getString(R.string.unsupported_intent))
        }
      }
      if (Intent.ACTION_VIEW == intent.action) {
        uri = intent.data
        if (uri!!.lastPathSegment!!.endsWith(".gda")) {
          isGDA = true
        } else if (uri.scheme == "http" || uri.scheme == "https") {
          isGPXFromWeb = true
        } else {
          isGPXFromFile = true
        }
      }
    }

    // Check if the intent was seen
    if (intent != null) {
      GDApplication.addMessage(
        GDApplication.DEBUG_MSG, "GDApp",
        "intent=$intent"
      )
      intent.toUri(0)
      if (intent.flags and Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY != 0) {
        isGDA = false
        isGPXFromFile = false
        isGPXFromWeb = false
        isAddress = false
      }
    }

    // Handle the intent
    if (isAddress) {
      viewMap.viewModel.askForAddress(subject,text) {
        viewMap.viewModel.showSnackbar(it)
      }
    }
    if (isGDA) {
      viewMap.dialogHandler.errorDialog("GDA intents not supported anymore!")
/*
      // Receive the file path
      var filePath: String? = null
      if (uri!!.scheme == "file") {
        filePath = uri.path
      } else {
        if (uri.scheme == "content") {
          filePath = uri.path
        } else {
          GDApplication.addMessage(
            GDApplication.ERROR_MSG,
            "GDApp",
            "scheme " + uri.scheme + " not supported!"
          )
        }
      }
      //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","filePath=" + filePath);

      // Ask the user if the file should be linked to the map directory
      val srcFile = File(filePath)
      if (srcFile.exists()) {

        // Map archive?
        if (uri.path!!.endsWith(".gda")) {

          // Ask the user if a new map shall be created based on the archive
          var mapFolderFilename = GDCore.getHomeDirPath() + "/Map/" + srcFile.name
          mapFolderFilename = mapFolderFilename.substring(0, mapFolderFilename.lastIndexOf('.'))
          val mapInfoFilename = "$mapFolderFilename/info.gds"
          val mapFolder = File(mapFolderFilename)
          val builder = MaterialDialog.Builder(this)
          builder.title(R.string.import_map)
          val message: String
          message = if (mapFolder.exists()) getString(
            R.string.replace_map_folder_question,
            mapFolder.name,
            srcFile
          ) else getString(R.string.create_map_folder_question, mapFolder.name, srcFile)
          builder.content(message)
          builder.cancelable(true)
          builder.positiveText(R.string.yes)
          builder.onPositive { dialog, which ->
            val task = ImportMapArchivesTask()
            task.srcFile = srcFile
            task.mapFolder = mapFolder
            task.mapInfoFilename = mapInfoFilename
            task.execute()
          }
          builder.negativeText(R.string.no)
          builder.icon(resources.getDrawable(android.R.drawable.ic_dialog_info))
          val alert: Dialog = builder.build()
          alert.show()
        }
      } else {
        errorDialog(getString(R.string.file_does_not_exist))
      }
*/
    }
    if (isGPXFromWeb) {
      //downloadRoute(uri)
      viewMap.dialogHandler.errorDialog("downloadRoute not yet implemented!")
    }
    if (isGPXFromFile) {

      // Get the name of the GPX file
      var gpxName: String? = null
      if (uri!!.scheme == "content") {
        val cursor = viewMap.contentResolver.query(uri, null, null, null, null)
        try {
          if (cursor != null && cursor.moveToFirst()) {
            gpxName = cursor.getString(cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME))
          }
        } finally {
          cursor!!.close()
        }
      }
      if (gpxName == null) {
        gpxName = uri.lastPathSegment
      }
      if (!gpxName!!.endsWith(".gpx")) {
        gpxName = gpxName + ".gpx"
      }
      //askForRouteName(uri, gpxName)
      viewMap.dialogHandler.errorDialog("askForRouteName not yet implemented!")
    }
    viewMap.setIntent(null)
  }
}

