//============================================================================
// Name        : ViewContent.kt
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

import android.content.res.Configuration
import android.graphics.Typeface
import android.util.TypedValue
import android.view.Gravity
import android.view.ViewGroup
import android.widget.TextView
import androidx.compose.animation.*
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material.ExperimentalMaterialApi
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.ExperimentalComposeUiApi
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.*
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.*
import androidx.compose.ui.viewinterop.AndroidView
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.R
import com.untouchableapps.android.geodiscoverer.core.GDMapSurfaceView
import com.untouchableapps.android.geodiscoverer.ui.activity.ViewMap
import com.untouchableapps.android.geodiscoverer.ui.component.*
import kotlinx.coroutines.*
import kotlin.math.roundToInt

@ExperimentalAnimationApi
@ExperimentalGraphicsApi
@ExperimentalMaterial3Api
@ExperimentalMaterialApi
@ExperimentalComposeUiApi
class ViewContent(viewMap: ViewMap) {

  // Parameters
  val viewMap=viewMap

  // Layout parameters for the navigation drawer
  class LayoutParams() {
    val iconWidth = 60.dp
    val drawerItemHeight = 45.5.dp
    val titleIndent = 20.dp
    val drawerWidth = 250.dp
    val itemPadding = 5.dp
    val itemDistance = 15.dp
    val hintIndent = 15.dp
    val listIndent = 2.dp
    val drawerCornerRadius = 16.dp
    val snackbarHorizontalPadding = 20.dp
    val snackbarVerticalOffset = 40.dp
    val snackbarMaxWidth = 400.dp
    val askMaxContentHeight = 140.dp
    val askMaxDropdownMenuHeight = 200.dp
    val askMultipleChoiceMessageOffset = 15.dp
    val poiSearchRadiusWidth = 120.dp
    val dialogButtonRowHeight = 200.dp
    val integratedListHeight = 450.dp
    val integratedListWidth = 400.dp
    val integratedListCloseRowHeight = 45.dp
    val integratedListItemHeight = 60.dp
    val integratedListTabHeight = 50.dp
    val integratedListTabWidth = 100.dp
    val integratedListFABPadding = 20.dp
    val messageBackgroundColor = Color.Black.copy(alpha = 0.8f)
    val rippleWaitTime = 300L
    val progressIndicatorHeight = 7.dp
  }
  val layoutParams = LayoutParams()

  // Sub content
  val dialogContent = ViewContentDialog(this)
  val integratedListContent = ViewContentIntegratedList(this)
  val navigationDrawerContent = ViewContentNavigationDrawer(this)

  // Main content of the activity
  @ExperimentalAnimationApi
  @ExperimentalMaterial3Api
  @ExperimentalMaterialApi
  @Composable
  fun content(
    viewModel: ViewModel,
    appVersion: String,
    navigationItems: List<ViewContentNavigationDrawer.NavigationItem>
  ) {
    BoxWithConstraints(
      modifier = Modifier
        .fillMaxSize()
    ) {
      val contentBoxWithConstraintsScope=this
      Scaffold(
        content = { innerPadding ->
          val drawerState =
            rememberDrawerState(initialValue = viewModel.drawerStatus, confirmStateChange = {
              //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "drawerState=$it")
              viewModel.drawerStatus = it
              true
            })
          LaunchedEffect(viewModel.drawerStatus) {
            if ((drawerState.isOpen) && (viewModel.drawerStatus == DrawerValue.Closed))
              drawerState.close()
            if ((!drawerState.isOpen) && (viewModel.drawerStatus == DrawerValue.Open))
              drawerState.open()
          }
          val scope = rememberCoroutineScope()
          NavigationDrawer(
            drawerState = drawerState,
            modifier = Modifier
              .fillMaxWidth()
              .fillMaxHeight(),
            gesturesEnabled = drawerState.isOpen || viewModel.messagesVisible,
            drawerShape = navigationDrawerContent.drawerShape(),
            drawerContent = navigationDrawerContent.content(appVersion, innerPadding, navigationItems) {
              scope.launch() {
                viewModel.drawerStatus = DrawerValue.Closed
              }
            }
          ) {
            screenContent(viewModel,contentBoxWithConstraintsScope.maxWidth,contentBoxWithConstraintsScope.maxHeight)
          }
        }
      )
      dialogContent.content(viewModel,contentBoxWithConstraintsScope.maxHeight)
      //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "snackbarVisible=${viewModel.snackbarVisible}")
      AnimatedVisibility(
        modifier = Modifier
          .align(Alignment.BottomCenter),
        enter = fadeIn() + slideInVertically(initialOffsetY = { +it / 2 }),
        exit = fadeOut() + slideOutVertically(targetOffsetY = { +it / 2 }),
        visible = viewModel.snackbarVisible
      ) {
        GDSnackBar(
          modifier = Modifier
            .padding(horizontal = layoutParams.snackbarHorizontalPadding)
            .padding(bottom = layoutParams.snackbarVerticalOffset)
            .widthIn(max = layoutParams.snackbarMaxWidth)
        ) {
          Row(
            verticalAlignment = Alignment.CenterVertically
          ) {
            Text(
              modifier = Modifier
                .weight(1.0f),
              text = viewModel.snackbarText,
              color = MaterialTheme.colorScheme.onBackground
            )
            if (viewModel.snackbarActionText != "") {
              TextButton(
                onClick = {
                  viewModel.snackbarActionHandler()
                  viewModel.hideSnackbar()
                }
              ) {
                Text(
                  text = viewModel.snackbarActionText,
                  color = MaterialTheme.colorScheme.primary
                )
              }
            }
          }
        }
        LaunchedEffect(viewModel.snackbarVisible,viewModel.snackbarText,viewModel.snackbarActionText) {
          if (viewModel.snackbarVisible) {
            delay(3000)
            viewModel.hideSnackbar()
          }
        }
      }
    }
  }

  // Main content on the screen
  @ExperimentalAnimationApi
  @ExperimentalMaterialApi
  @Composable
  private fun screenContent(viewModel: ViewModel, maxScreenWidth: Dp, maxScreenHeight: Dp) {
    //val scope = rememberCoroutineScope()
    val configuration = LocalConfiguration.current
    Box(
      modifier = Modifier
        .fillMaxSize()
    ) {
      with(LocalDensity.current) {
        val listHeight=(layoutParams.integratedListHeight).toPx().roundToInt()
        val listWidth=(layoutParams.integratedListWidth).toPx().roundToInt()
        val screenHeight=(maxScreenHeight).toPx().roundToInt()
        val screenWidth=(maxScreenWidth).toPx().roundToInt()
        var mapHeight=screenHeight
        var mapWidth=screenWidth
        var mapX=0
        var mapY=0
        if (configuration.orientation==Configuration.ORIENTATION_LANDSCAPE) {
          if (viewModel.integratedListVisible) {
            mapWidth = screenWidth - listWidth
            mapX = -listWidth/2
          }
        } else {
          if (viewModel.integratedListVisible) {
            mapHeight = screenHeight - listHeight
            mapY = listHeight/2
          }
        }
        if (viewModel.integratedListVisible) {
          viewMap.coreObject!!.executeCoreCommand(
            "setMapWindow",
            mapX.toString(),
            mapY.toString(),
            mapWidth.toString(),
            mapHeight.toString()
          )
          viewMap.coreObject!!.executeCoreCommand("closeFingerMenu")
        } else {
          viewMap.coreObject!!.executeCoreCommand(
            "setMapWindow",
            mapX.toString(),
            mapY.toString(),
            mapWidth.toString(),
            mapHeight.toString()
          )
        }
      }
      Surface(
        modifier = Modifier
          .fillMaxSize()
      ) {
        mapSurface()
      }
      AnimatedVisibility(
        modifier = Modifier
          .align(
            if (configuration.orientation==Configuration.ORIENTATION_LANDSCAPE)
              Alignment.CenterEnd
            else
              Alignment.BottomCenter
          ),
        enter = if (configuration.orientation==Configuration.ORIENTATION_LANDSCAPE)
          slideInHorizontally(initialOffsetX = { +it })
        else
          slideInVertically(initialOffsetY = { +it }),
        exit = if (configuration.orientation==Configuration.ORIENTATION_LANDSCAPE)
          slideOutHorizontally(targetOffsetX = { +it })
        else
          slideOutVertically(targetOffsetY = { +it }),
        visible = viewModel.integratedListVisible
      ) {
        integratedListContent.content(configuration,maxScreenWidth,maxScreenHeight,viewMap.viewModel)
      }
      if (viewModel.fixSurfaceViewBug) {
        Box(
          modifier = Modifier
            .fillMaxSize()
        )
        LaunchedEffect(Unit) {
          delay(500)
          viewModel.fixSurfaceViewBug = false
        }
      }
      if (viewModel.messagesVisible) {
        if (configuration.orientation==Configuration.ORIENTATION_LANDSCAPE) {
          Row(
            modifier = Modifier
              .fillMaxSize()
              .background(layoutParams.messageBackgroundColor),
            verticalAlignment = Alignment.CenterVertically
          ) {
            messageContent(viewModel)
          }
        } else {
          Column(
            modifier = Modifier
              .fillMaxSize()
              .background(layoutParams.messageBackgroundColor),
            horizontalAlignment = Alignment.CenterHorizontally
          ) {
            messageContent(viewModel)
          }
        }
      }
    }
  }

  // Defines the message view along with the splash icon
  @Composable
  private fun messageContent(viewModel: ViewModel) {
    if (viewModel.splashVisible) {
      Column(
        horizontalAlignment = Alignment.CenterHorizontally
      ) {
        Image(
          painter = painterResource(R.drawable.splash),
          contentDescription = null
        )
        Text(
          text = viewModel.busyText,
          style = MaterialTheme.typography.headlineSmall,
          color = MaterialTheme.colorScheme.onBackground,
        )
        Spacer(Modifier.height(20.dp))
      }
    }
    AndroidView(
      modifier = Modifier
        .fillMaxSize(),
      factory = { context ->
        TextView(context).apply {
          layoutParams = ViewGroup.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT,
          )
          gravity = Gravity.BOTTOM
          typeface = Typeface.MONOSPACE
          setTextSize(TypedValue.COMPLEX_UNIT_SP, 10.0f)
        }
      },
      update = { view ->
        view.text = viewModel.messages
      }
    )
  }

  // Map surface view
  @Composable
  private fun mapSurface() {
    AndroidView(
      factory = { context ->
        GDMapSurfaceView(context, null).apply {
          layoutParams = ViewGroup.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT,
          )
          setCoreObject(viewMap.coreObject)
        }
      },
    )
  }
}

