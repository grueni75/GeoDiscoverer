//============================================================================
// Name        : GDDialog.kt
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

package com.untouchableapps.android.geodiscoverer.ui.component

import android.R
import android.app.Dialog
import android.os.SystemClock
import androidx.appcompat.app.AppCompatActivity
import com.afollestad.materialdialogs.DialogAction
import com.afollestad.materialdialogs.MaterialDialog
import com.afollestad.materialdialogs.MaterialDialog.SingleButtonCallback
import com.untouchableapps.android.geodiscoverer.GDApplication

open class GDDialog(showSnackbar: (String) -> Unit, showDialog: (String, Boolean) -> Unit) {

  // Opens a snackbar with the given parameters in the outer activity
  private val showSnackbar = showSnackbar

  // Opens a dialog with the given parameters in the outer activity
  private val showDialog = showDialog

  // Types of dialogs
  companion object {
    // Types of dialogs
    const val FATAL_DIALOG = 0
    const val ERROR_DIALOG = 2
    const val WARNING_DIALOG = 1
    const val INFO_DIALOG = 3
  }

  // Time the last toast was shown
  private var lastToastTimestamp: Long = 0

  // Minimum distance between two toasts in milliseconds
  private val toastDistance = 5000

  // Shows a dialog
  @Synchronized
  private fun dialog(kind: Int, message: String) {
    when(kind) {
      WARNING_DIALOG -> {
        val diff = SystemClock.uptimeMillis() - lastToastTimestamp
        if (diff <= toastDistance) {
          GDApplication.addMessage(
            GDApplication.DEBUG_MSG, "GDApp",
            "skipping dialog request <$message> because toast is still visible"
          )
          return
        }
        showSnackbar(message)
        lastToastTimestamp = SystemClock.uptimeMillis()
      }

      INFO_DIALOG -> {
        showSnackbar(message)
      }

      ERROR_DIALOG -> {
        showDialog(message, false)
      }

      FATAL_DIALOG -> {
        showDialog(message, true)
      }
    }
  }

  // Shows a fatal dialog and quits the applications
  fun fatalDialog(message: String) {
    dialog(FATAL_DIALOG, message)
  }

  // Shows an error dialog without quitting the application
  fun errorDialog(message: String) {
    dialog(ERROR_DIALOG, message)
  }

  // Shows a warning dialog without quitting the application
  fun warningDialog(message: String) {
    dialog(WARNING_DIALOG, message)
  }

  // Shows an info dialog without quitting the application
  fun infoDialog(message: String) {
    dialog(INFO_DIALOG, message)
  }
}
