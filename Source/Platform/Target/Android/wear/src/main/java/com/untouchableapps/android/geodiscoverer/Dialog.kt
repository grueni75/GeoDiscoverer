//============================================================================
// Name        : Dialog
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2022 Matthias Gruenewald
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
package com.untouchableapps.android.geodiscoverer

import android.Manifest
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.provider.Settings
import android.view.WindowManager
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.TextUnit
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.wear.ambient.AmbientModeSupport
import androidx.wear.compose.material.*
import com.untouchableapps.android.geodiscoverer.theme.WearAppTheme

import com.untouchableapps.android.geodiscoverer.core.GDAppInterface
import android.transition.Explode
import android.transition.Fade
import android.view.Window
import androidx.compose.foundation.Image
import androidx.compose.ui.res.painterResource


class Dialog : ComponentActivity() {

  // Identifiers for extras
  object Extras {
    const val TEXT = "com.untouchableapps.android.geodiscoverer.dialog.TEXT"
    const val MAX = "com.untouchableapps.android.geodiscoverer.dialog.MAX"
    const val PROGRESS = "com.untouchableapps.android.geodiscoverer.dialog.PROGRESS"
    const val KIND = "com.untouchableapps.android.geodiscoverer.dialog.KIND"
    const val CLOSE = "com.untouchableapps.android.geodiscoverer.dialog.CLOSE"
    const val GET_PERMISSIONS = "com.untouchableapps.android.geodiscoverer.dialog.GET_PERMISSIONS"
  }

  // Identifiers for dialogs
  object Types {
    const val FATAL = 0
    const val ERROR = 2
    const val WARNING = 1
    const val INFO = 3
  }

  // UI state
  inner class ViewModel() : androidx.lifecycle.ViewModel() {
    var kind: Int by mutableStateOf(-1)
    var message: String by mutableStateOf("")
    var progressMax: Int by mutableStateOf(-1)
    var progressCurrent: Int by mutableStateOf(0)
  }
  val viewModel = ViewModel()

  // Called when the activity is created
  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)

    // Prevent animation when closed
    window.setWindowAnimations(0)

    // Set the content
    viewModel.message=getString(R.string.permission_instructions)
    setContent{
      content()
    }

    // Check which type is requested
    onNewIntent(intent);
  }

  // Checks for a new intent
  override fun onNewIntent(intent: Intent) {
    super.onNewIntent(intent)

    // Message update?
    if (intent.hasExtra(Extras.TEXT)) {
      viewModel.message=intent.getStringExtra(Extras.TEXT)!!
      viewModel.progressMax=-1
      viewModel.kind=-1
    }

    // Shall permissions be granted?
    if (intent.hasExtra(Extras.GET_PERMISSIONS)) {

      // Request the permissions
      GDApplication.addMessage(GDAppInterface.DEBUG_MSG, "GDApp", "Requesting general permissions")
      requestPermissions(
        arrayOf(
          Manifest.permission.WRITE_EXTERNAL_STORAGE,
          Manifest.permission.VIBRATE,
        ), 0
      )
      if (!Settings.canDrawOverlays(applicationContext)) {
        GDApplication.addMessage(GDAppInterface.DEBUG_MSG, "GDApp", "Requesting overlay permission")
        GDApplication.showMessageBar(
          applicationContext,
          resources.getString(R.string.overlay_instructions),
          GDApplication.MESSAGE_BAR_DURATION_LONG
        )
        val packageName = applicationContext.packageName
        val t = Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:$packageName"))
        startActivity(t)
      }
    }

    // Max value for progress dialog?
    if (intent.hasExtra(Extras.MAX)) {
      viewModel.progressMax = intent.getIntExtra(Extras.MAX, 0)
      viewModel.progressCurrent = 0
      window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    // Progress update?
    if (intent.hasExtra(Extras.PROGRESS)) {
      viewModel.progressCurrent = intent.getIntExtra(Extras.PROGRESS, 0)
    }

    // Dialog with button?
    if (intent.hasExtra(Extras.KIND)) {
      viewModel.kind = intent.getIntExtra(Extras.KIND, -1)
    }

    // Shall we close?
    if (intent.hasExtra(Extras.CLOSE)) {
      window.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
      finish()
    }
  }

  // Displays the permission request content
  @Composable
  fun content() {
    WearAppTheme {
      Box(
        modifier = Modifier
          .fillMaxSize()
          .background(MaterialTheme.colors.background),
      ) {
        if (viewModel.progressMax != -1) {
          if (viewModel.progressMax == 0) {
            CircularProgressIndicator(
              modifier = Modifier
                .fillMaxSize(),
              strokeWidth = 10.dp
            )
          } else {
            CircularProgressIndicator(
              modifier = Modifier
                .fillMaxSize(),
              progress = viewModel.progressCurrent.toFloat() / viewModel.progressMax.toFloat(),
              strokeWidth = 10.dp
            )
          }
        }
        Column(
          modifier = Modifier
            .fillMaxSize(),
          verticalArrangement = Arrangement.Center,
          horizontalAlignment = Alignment.CenterHorizontally
        ) {
          if ((viewModel.kind == Types.FATAL) || (viewModel.kind == Types.ERROR)) {
            Icon(
              imageVector = Icons.Default.Warning,
              contentDescription = null,
            )
          } else {
            Image(
              modifier = Modifier
                .size(40.dp),
              painter = painterResource(R.mipmap.ic_launcher),
              contentDescription = null
            )
          }
          if (viewModel.message != "") {
            Text(
              modifier = Modifier
                .fillMaxWidth()
                .padding(20.dp),
              textAlign = TextAlign.Center,
              fontSize = 20.sp,
              text = viewModel.message
            )
          }
          if ((viewModel.kind == Types.ERROR) || (viewModel.kind == Types.FATAL)) {
            Button(
              onClick = {
                finish()
                if (viewModel.kind == Types.FATAL) {
                  System.exit(1)
                }
              }
            ) {
              Icon(
                imageVector = Icons.Default.Done,
                contentDescription = null,
              )
            }
          }
        }
      }
    }
  }
}

