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

import android.app.ActivityManager
import android.content.Intent
import android.content.pm.ConfigurationInfo
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.util.AttributeSet
import android.view.ViewGroup
import android.widget.Toast
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
import androidx.compose.foundation.shape.CornerSize
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.filled.*
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.geometry.RoundRect
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Outline
import androidx.compose.ui.graphics.Shape
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.modifier.modifierLocalConsumer
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.Density
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.LayoutDirection
import androidx.compose.ui.viewinterop.AndroidView
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.core.GDCore
import com.untouchableapps.android.geodiscoverer.core.GDMapSurfaceView
import com.untouchableapps.android.geodiscoverer.logic.GDService
import kotlin.jvm.internal.Intrinsics

class ViewMap2 : ComponentActivity() {

  // Layout parameters for the navigation drawer
  class NavigationDrawerLayout() {
    val iconWidth = 60.dp
    val rowHeight = 0.dp
    val titleIndent = 15.dp
    val rowVerticalPadding = 10.dp
    val drawerWidth = 250.dp
    val drawerCornerRadius = 16.dp
  }
  val drawerLayout = NavigationDrawerLayout()

  // Navigation drawer content
  class NavigationItem(imageVector: ImageVector?=null, title: String, onClick: ()->Unit) {
    val imageVector = imageVector
    val title = title
    val onClick = onClick
  }

  // Reference to the core object and it's view
  var coreObject: GDCore? = null

  // Flags
  var compassWatchStarted = false
  var exitRequested = false
  var restartRequested = false

  // Creates the activity
  @ExperimentalMaterial3Api
  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)

    // Check for OpenGL ES 2.00
    val activityManager = getSystemService(ACTIVITY_SERVICE) as ActivityManager
    val configurationInfo = activityManager.deviceConfigurationInfo
    val supportsEs2 =
      configurationInfo.reqGlEsVersion >= 0x20000 || Build.FINGERPRINT.startsWith("generic")
    if (!supportsEs2) {
      Toast.makeText(this, getString(R.string.opengles20_required), Toast.LENGTH_LONG)
      finish();
      return
    }

    // Get the core object
    coreObject = GDApplication.coreObject
    if (coreObject == null) {
      finish()
      return
    }
    (application as GDApplication).setActivity(this, null)

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

  override fun onPause() {
    super.onPause()
    GDApplication.addMessage(
      GDApplication.DEBUG_MSG,
      "GDApp",
      "onPause called by " + Thread.currentThread().name
    )

    //stopWatchingCompass()
    if (!exitRequested && !restartRequested) {
      val intent = Intent(this, GDService::class.java)
      intent.action = "activityPaused"
      startService(intent)
    }
  }

  override fun onResume() {
    super.onResume()
    GDApplication.addMessage(
      GDApplication.DEBUG_MSG,
      "GDApp",
      "onResume called by " + Thread.currentThread().name
    )

    // Bug fix: Somehow the emulator calls onResume before onCreate
    // But the code relies on the fact that onCreate is called before
    // Do nothing if onCreate has not yet initialized the objects
    if (coreObject == null) return

    // If we shall restart or exit, don't init anything here
    if (exitRequested || restartRequested) return

    // Resume all components only if a exit or restart is not requested

    // Resume all components only if a exit or restart is not requested
    //startWatchingCompass()
    val intent = Intent(this, GDService::class.java)
    intent.action = "activityResumed"
    startService(intent)
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
            .fillMaxWidth()
            .fillMaxHeight(),
          gesturesEnabled = drawerState.isOpen,
          drawerShape = navigationDrawerShape(),
          drawerContent = {
            Column(
              modifier=Modifier
                .fillMaxWidth()
            ) {
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
            AndroidView(
              factory = { context ->
                GDMapSurfaceView(context, null).apply {
                  layoutParams = ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT,
                  )
                  setCoreObject(coreObject)
                }
              },
              modifier = Modifier
                .fillMaxWidth()
                .fillMaxHeight(),
            )
          }
        )
      }
    )
  }

  // Sets the width of the navigation drawer correctly
  fun navigationDrawerShape() = object : Shape {
    override fun createOutline(
      size: Size,
      layoutDirection: LayoutDirection,
      density: Density
    ): Outline {
      val cornerRadius=with(density) { drawerLayout.drawerCornerRadius.toPx() }
      return Outline.Rounded(RoundRect(
        left=0f,
        top=0f,
        right=with(density) { drawerLayout.drawerWidth.toPx() },
        bottom=size.height,
        topRightCornerRadius = CornerRadius(cornerRadius),
        bottomRightCornerRadius = CornerRadius(cornerRadius)
      ))
    }
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

