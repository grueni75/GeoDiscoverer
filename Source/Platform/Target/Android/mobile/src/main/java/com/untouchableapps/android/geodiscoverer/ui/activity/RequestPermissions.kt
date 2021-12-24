//============================================================================
// Name        : RequestPermissions.kt
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

import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.R
import com.untouchableapps.android.geodiscoverer.ui.theme.AndroidTheme

import java.util.*
import kotlin.system.exitProcess

class RequestPermissions : ComponentActivity() {

  val requestPermissionLauncher = registerForActivityResult(
    ActivityResultContracts.RequestMultiplePermissions()
  ) { permissions ->
    permissions.entries.forEach {
      Log.d("GDApp", "${it.key} = ${it.value}")
    }
  }

  @ExperimentalMaterial3Api
  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    setContent {
      AndroidTheme {
        content()
      }
    }
  }

  override fun onResume() {
    super.onResume()
    if (!GDApplication.checkPermissions(this)) {
      requestPermissionLauncher.launch(GDApplication.requiredPermissions)
    } else {
      Toast.makeText(
        this,
        getString(R.string.request_permissions_wait_for_restart),
        Toast.LENGTH_LONG
      ).show()
      val app: GDApplication = application as GDApplication
      app.scheduleRestart();
      Timer().schedule(object : TimerTask() {
        override fun run() {
          exitProcess(0);
        }
      }, 3000)
      finish();
    }
  }

  @ExperimentalMaterial3Api
  @Composable
  fun content() {
    Scaffold(
      topBar = {
        SmallTopAppBar(
          title = { Text(stringResource(id = R.string.app_name)) }
        )
      },
      content = { innerPadding ->
        Column(
          modifier = Modifier
            .padding(10.dp)
        ) {
          Text(
            text = stringResource(id = R.string.request_permissions_description)
          )
        }
      }
    )
  }

  @ExperimentalMaterial3Api
  @Preview(showBackground = true)
  @Composable
  fun preview() {
    AndroidTheme {
      content()
    }
  }
}

