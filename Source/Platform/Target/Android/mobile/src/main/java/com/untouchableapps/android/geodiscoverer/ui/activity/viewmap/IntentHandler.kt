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

package com.untouchableapps.android.geodiscoverer.ui.activity.viewmap

import android.annotation.SuppressLint
import android.app.DownloadManager
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.database.Cursor
import android.net.Uri
import android.os.*
import android.provider.OpenableColumns
import androidx.activity.ComponentActivity
import androidx.compose.material3.*
import androidx.lifecycle.viewmodel.compose.viewModel
import com.untouchableapps.android.geodiscoverer.R
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.core.GDCore
import com.untouchableapps.android.geodiscoverer.ui.activity.ViewMap2

import kotlinx.coroutines.*
import java.io.File
import java.util.*

@ExperimentalMaterial3Api
class IntentHandler(viewMap: ViewMap2) : CoroutineScope by MainScope() {

  // References
  val viewMap = viewMap
  var downloadManager: DownloadManager? = null

  // Handles finished queued downloads
  var downloads = LinkedList<Bundle>()
  var downloadCompleteReceiver: BroadcastReceiver? = null

  // Init important stuff
  fun onCreate() {

    // Create the download manager
    downloadManager = viewMap.getSystemService(ComponentActivity.DOWNLOAD_SERVICE) as DownloadManager
    if (downloadManager==null) {
      viewMap.dialogHandler.fatalDialog(viewMap.getString(R.string.missing_system_service))
    } else {
      val filter = IntentFilter()
      filter.addAction(DownloadManager.ACTION_DOWNLOAD_COMPLETE)
      downloadCompleteReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {

          // If one of the requested download finished, restart the core
          for (download in downloads) {
            val query = DownloadManager.Query()
            query.setFilterById(download.getLong("ID"))
            val cur: Cursor = downloadManager!!.query(query)
            val col = cur.getColumnIndex(DownloadManager.COLUMN_STATUS)
            if (cur.moveToFirst()) {
              if (cur.getInt(col) == DownloadManager.STATUS_SUCCESSFUL) {
                viewMap.restartCore(false)
              } else {
                viewMap.dialogHandler.errorDialog(
                  viewMap.getString(
                    R.string.download_failed,
                    download.getString("Name")
                  )
                )
              }
            }
            downloads.remove(download)
            break
          }
        }
      }
      viewMap.registerReceiver(downloadCompleteReceiver, filter)
    }
  }

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

      viewMap.viewModel.askForAddressPointAdd(text) { address, group ->
        viewMap.backgroundTask!!.getLocationFromAddress(
          name=subject,
          address=address,
          group=group
        ) { locationFound ->
          viewMap.viewModel.informLocationLookupResult(address, locationFound)
        }
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
      if (uri!=null) downloadRoute(uri!!)
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
      viewMap.viewModel.askForRouteName(gpxName) { name, _ ->
        viewMap.backgroundTask!!.importRoute(
          name=name,
          uri=uri
        ) { success, message ->
          if (success)
            viewMap.restartCore(false)
          else
            viewMap.dialogHandler.errorDialog(message)
        }
      }
    }
    viewMap.intent = null
  }

  // Downloads a route
  fun downloadRoute(srcURI: Uri) {

    // Create some variables
    val name: String = if (srcURI.lastPathSegment==null) "unknown" else srcURI.lastPathSegment!!
    val dstFilename = GDCore.getHomeDirPath() + "/Route/" + name

    // Check if download is already ongoing
    val query = DownloadManager.Query()
    query.setFilterByStatus(DownloadManager.STATUS_PAUSED or DownloadManager.STATUS_PENDING or DownloadManager.STATUS_RUNNING)
    val cur: Cursor = downloadManager!!.query(query)
    val col = cur.getColumnIndex(DownloadManager.COLUMN_ID)
    var downloadOngoing = false
    for (download in downloads) {
      cur.moveToFirst()
      while (!cur.isAfterLast) {
        if (cur.getLong(col) == download.getLong("ID")) {
          downloadOngoing = true
          break
        }
        cur.moveToNext()
      }
    }
    if (downloadOngoing) {
      viewMap.dialogHandler.infoDialog(viewMap.getString(R.string.skipping_download))
    } else {

      // Ask the user if GPX file shall be downloaded and overwritten if file exists
      val dstFile = File(dstFilename)
      viewMap.viewModel.askForRouteDownload(name,dstFile) {

        // Delete old file if it exists
        if (dstFile.exists()) dstFile.delete()

        // Request download of file
        val request = DownloadManager.Request(srcURI)
        request.setDescription(viewMap.getString(R.string.downloading_gpx_to_route_directory))
        request.setTitle(name)
        request.setDestinationUri(Uri.fromFile(dstFile))
        //request.setNotificationVisibility(DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED);
        val id: Long = downloadManager!!.enqueue(request)
        val download = Bundle()
        download.putLong("ID", id)
        download.putString("Name", name)
        downloads.add(download)
      }
    }
  }

}

