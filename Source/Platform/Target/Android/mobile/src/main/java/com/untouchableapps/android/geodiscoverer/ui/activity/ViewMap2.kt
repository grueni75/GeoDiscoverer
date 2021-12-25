//============================================================================
// Name        : ViewMap2.kt
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

import android.content.pm.PackageManager
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.material.icons.Icons
import androidx.compose.material.ripple.rememberRipple
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.untouchableapps.android.geodiscoverer.R
import com.untouchableapps.android.geodiscoverer.ui.theme.AndroidTheme
import kotlinx.coroutines.launch
import java.util.*
import androidx.compose.foundation.Image
import androidx.compose.foundation.interaction.collectIsPressedAsState
import androidx.compose.foundation.selection.selectable
import androidx.compose.material.icons.filled.*
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.modifier.modifierLocalConsumer
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.Dp
import kotlin.jvm.internal.Intrinsics

class ViewMap2 : ComponentActivity() {

  // Layout parameters for the navigation drawer
  class NavigationDrawerLayout() {
    val iconWidth = 60.dp
    val rowHeight = 0.dp
    val titleIndent = 15.dp
    val rowVerticalPadding = 10.dp
    val drawerWidth = 250.dp
  }
  val drawerLayout = NavigationDrawerLayout()

  // Navigation drawer content
  class NavigationItem(imageVector: ImageVector?=null, title: String, onClick: ()->Unit) {
    val imageVector = imageVector
    val title = title
    val onClick = onClick
  }

  // Creates the activity
  @ExperimentalMaterial3Api
  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)

    // Create the content for the navigation drawer
    val navigationItems=arrayOf(
      NavigationItem(null, getString(R.string.map), { }),
      NavigationItem(Icons.Filled.Article, getString(R.string.show_legend), { }),
      NavigationItem(Icons.Filled.Download, getString(R.string.download_map), { }),
      NavigationItem(Icons.Filled.CleaningServices, getString(R.string.cleanup_map), { }),
      NavigationItem(null, getString(R.string.routes), { }),
      NavigationItem(Icons.Filled.AddCircle, getString(R.string.add_tracks_as_routes), { }),
      NavigationItem(Icons.Filled.RemoveCircle, getString(R.string.remove_routes), { }),
      NavigationItem(Icons.Filled.SendToMobile, getString(R.string.export_selected_route), { }),
      NavigationItem(Icons.Filled.Directions, getString(R.string.brouter), { }),
      NavigationItem(null, getString(R.string.general), { }),
      NavigationItem(Icons.Filled.Help, getString(R.string.help), { }),
      NavigationItem(Icons.Filled.Settings, getString(R.string.preferences), { }),
      NavigationItem(Icons.Filled.Replay, getString(R.string.restart), { }),
      NavigationItem(Icons.Filled.Logout, getString(R.string.exit), { }),
      NavigationItem(null, getString(R.string.debug), { }),
      NavigationItem(Icons.Filled.Message, getString(R.string.toggle_messages), { }),
      NavigationItem(Icons.Filled.UploadFile, getString(R.string.send_logs), { }),
      NavigationItem(Icons.Filled.Clear, getString(R.string.reset), { }),
    )

    // Get the app version
    val packageManager = packageManager
    val appVersion: String
    appVersion = try {
      "Version " + packageManager.getPackageInfo(packageName, 0).versionName
    } catch (e: PackageManager.NameNotFoundException) {
      "Version ?"
    }

    // Create the activity content
    setContent {
      AndroidTheme {
        content(appVersion,navigationItems.toList())
      }
    }
  }

  override fun onResume() {
    super.onResume()
  }

  @ExperimentalMaterial3Api
  @Composable
  fun content(appVersion: String, navigationItems: List<NavigationItem>) {
    Scaffold(
      content = { innerPadding ->
        val drawerState = rememberDrawerState(DrawerValue.Open)
        val scope = rememberCoroutineScope()
        NavigationDrawer(
          drawerState = drawerState,
          modifier = Modifier
            .width(drawerLayout.drawerWidth),
          drawerContent = {
            Column() {
              Box(
                contentAlignment = Alignment.CenterStart
              ) {
                Image(
                  modifier = Modifier.
                    fillMaxWidth(),
                  painter=painterResource(R.drawable.nav_header),
                  contentDescription = null,
                  contentScale = ContentScale.FillWidth)
                Row(
                  verticalAlignment = Alignment.CenterVertically
                ) {
                  Column(
                    modifier=Modifier
                      .width(drawerLayout.iconWidth),
                    horizontalAlignment = Alignment.CenterHorizontally
                  ) {
                    Image(
                      painter = painterResource(R.mipmap.ic_launcher),
                      contentDescription = null
                    )
                  }
                  Column() {
                    Text(
                      text = stringResource(id = R.string.app_name),
                      style = MaterialTheme.typography.headlineSmall
                    )
                    Text(
                      text = appVersion,
                      style = MaterialTheme.typography.bodyMedium
                    )
                    Spacer(Modifier.height(4.dp))
                  }
                }
              }
              LazyColumn(
                modifier = Modifier
                  .padding(innerPadding)
              ) {
                itemsIndexed(navigationItems) {
                    index, item -> navigationItem(index, item)
                }
              }
            }
          },
          content = {
            Column(
              modifier = Modifier
                .fillMaxSize(),
              horizontalAlignment = Alignment.CenterHorizontally
            ) {
              Text(text = if (drawerState.isClosed) ">>> Swipe >>>" else "<<< Swipe <<<")
              Spacer(Modifier.height(20.dp))
              Button(onClick = { scope.launch { drawerState.open() } }) {
                Text("Click to open")
              }
            }
          }
        )
      }
    )
  }

  // Creates a navigation item for the drawer
  @Composable
  fun navigationItem(index: Int, item: NavigationItem) {
    if (item.imageVector == null) {
      if (index != 0) {
        Box(
          modifier = Modifier
            .fillMaxWidth()
            .height(1.dp)
            .background(MaterialTheme.colorScheme.outline)
        )
      }
      Row(
        modifier = Modifier
          .fillMaxWidth()
          .padding(vertical = drawerLayout.rowVerticalPadding),
        verticalAlignment = Alignment.CenterVertically
      ) {
        Column(
          modifier = Modifier
            .absolutePadding(left = drawerLayout.titleIndent)
        ) {
          Text(
            text = item.title,
            style = MaterialTheme.typography.titleSmall
          )
        }
      }
    } else {
      val interactionSource = remember { MutableInteractionSource() }
      Box(
        modifier = Modifier
          .clickable(
            onClick = item.onClick,
            interactionSource = interactionSource,
            indication = rememberRipple(bounded = true)
          )
      ) {
        Row(
          modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = drawerLayout.rowVerticalPadding),
          verticalAlignment = Alignment.CenterVertically
        ) {
          Spacer(
            Modifier
              .width(0.dp)
              .height(drawerLayout.rowHeight)
          )
          Column(
            modifier = Modifier
              .width(drawerLayout.iconWidth),
            horizontalAlignment = Alignment.CenterHorizontally
          ) {
            Icon(
              imageVector = item.imageVector,
              contentDescription = null,
              tint = if (interactionSource.collectIsPressedAsState().value) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.onBackground
            )
          }
          Column(
            modifier = Modifier
              .weight(1f)
          ) {
            Text(
              text = item.title,
              style = MaterialTheme.typography.bodyMedium,
              color = if (interactionSource.collectIsPressedAsState().value) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.onBackground
            )
          }
        }
      }
    }
  }

  @ExperimentalMaterial3Api
  @Preview(showBackground = true)
  @Composable
  fun preview() {
    AndroidTheme {
    }
  }
}

