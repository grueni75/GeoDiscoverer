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
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.material.ExperimentalMaterialApi
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.*
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.*
import androidx.compose.ui.viewinterop.AndroidView
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.R
import com.untouchableapps.android.geodiscoverer.core.GDMapSurfaceView
import com.untouchableapps.android.geodiscoverer.ui.activity.ViewMap2
import com.untouchableapps.android.geodiscoverer.ui.component.*
import kotlinx.coroutines.*
import kotlin.math.roundToInt

@ExperimentalMaterial3Api
class ViewContent(viewMap: ViewMap2) {

  // Parameters
  val viewMap=viewMap

  // Layout parameters for the navigation drawer
  class LayoutParams() {
    val iconWidth = 60.dp
    val drawerItemHeight = 41.dp
    val titleIndent = 20.dp
    val drawerWidth = 250.dp
    val itemPadding = 5.dp
    val itemDistance = 15.dp
    val hintIndent = 15.dp
    val drawerCornerRadius = 16.dp
    val snackbarHorizontalPadding = 20.dp
    val snackbarVerticalOffset = 40.dp
    val snackbarMaxWidth = 400.dp
    val askMaxContentHeight = 140.dp
    val askMaxDropdownMenuHeight = 200.dp
    val askMultipleChoiceMessageOffset = 15.dp
    val dialogButonRowHeight = 200.dp
    val integratedListHeight = 400.dp
    val integratedListCloseRowHeight = 45.dp
    val integratedListItemHeight = 60.dp
    val integratedListTabHeight = 50.dp
    val integratedListTabWidth = 100.dp
    val integratedListFABPadding = 20.dp
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
          val density = LocalDensity.current
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
      AnimatedVisibility(
        modifier = Modifier
          .align(Alignment.BottomCenter),
        enter = fadeIn() + slideInVertically(initialOffsetY = { +it / 2 }),
        exit = fadeOut() + slideOutVertically(targetOffsetY = { +it / 2 }),
        visible = viewModel.snackbarText != ""
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
                  viewModel.showSnackbar("")
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
        LaunchedEffect(viewModel.snackbarText) {
          if (viewModel.snackbarText != "") {
            delay(3000)
            viewModel.showSnackbar("")
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
    val scope = rememberCoroutineScope()
    Box(
      modifier = Modifier
        .fillMaxSize()
    ) {
      val configuration = LocalConfiguration.current
      when (configuration.orientation) {
        Configuration.ORIENTATION_PORTRAIT -> {
          Box(
            modifier = Modifier
              .fillMaxSize()
          ) {
            with(LocalDensity.current) {
              val listHeight=(layoutParams.integratedListHeight).toPx().roundToInt()
              val screenHeight=(maxScreenHeight).toPx().roundToInt()
              val mapHeight=
                if (viewModel.integratedListVisible)
                  screenHeight-listHeight
                else
                  screenHeight
              val width=maxScreenWidth.toPx().roundToInt()
              if (viewModel.integratedListVisible)
                viewMap.coreObject!!.executeCoreCommand("setMapWindow","0", (listHeight/2).toString(), width.toString(), mapHeight.toString())
              else
                viewMap.coreObject!!.executeCoreCommand("setMapWindow","0", "0", width.toString(), mapHeight.toString())
              if (viewModel.mapChanged) {
                viewModel.mapChanged(false)
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
                .align(Alignment.BottomCenter),
              enter = slideInVertically(initialOffsetY = { +it }),
              exit = slideOutVertically(targetOffsetY = { +it }),
              visible = viewModel.integratedListVisible
            ) {
              Surface(
                modifier = Modifier
                  .fillMaxWidth()
                  .height(layoutParams.integratedListHeight),
              ) {
                Box() {
                  Column {
                    Surface(
                    ) {
                      Column(
                        modifier = Modifier
                          .background(MaterialTheme.colorScheme.surfaceVariant)
                      ) {
                        Surface(
                          shadowElevation = 6.dp
                        ) {
                          Box(
                            modifier = Modifier
                              .background(MaterialTheme.colorScheme.surfaceVariant)
                              .fillMaxWidth()
                              .height(layoutParams.integratedListCloseRowHeight)
                          ) {
                            IconButton(
                              modifier = Modifier
                                .align(Alignment.Center),
                              onClick = {
                                viewModel.closeIntegratedList()
                              }
                            ) {
                              Icon(
                                imageVector = Icons.Default.ExpandMore,
                                contentDescription = null
                              )
                            }
                          }
                        }
                        Text(
                          modifier = Modifier
                            .padding(layoutParams.itemPadding)
                            .fillMaxWidth(),
                          text = viewModel.integratedListTitle,
                          style = MaterialTheme.typography.titleLarge,
                          textAlign = TextAlign.Center
                        )
                        if (viewModel.integratedListTabs.size > 0) {
                          val tabListState = rememberLazyListState()
                          LaunchedEffect(viewModel.integratedListVisible) {
                            if (viewModel.integratedListSelectedTab != -1)
                              tabListState.scrollToItem(viewModel.integratedListSelectedTab)
                          }
                          LazyRow(
                            state = tabListState,
                            modifier = Modifier
                              .fillMaxWidth()
                          ) {
                            itemsIndexed(viewModel.integratedListTabs) { index, tab ->
                              integratedListContent.tabContent(index, tab, viewModel)
                            }
                          }
                        }
                      }
                    }
                    val itemListState = rememberLazyListState()
                    LaunchedEffect(viewModel.integratedListVisible) {
                      if (viewModel.integratedListSelectedItem != -1)
                        itemListState.scrollToItem(viewModel.integratedListSelectedItem)
                    }
                    LazyColumn(
                      state = itemListState,
                      modifier = Modifier
                        .fillMaxWidth()
                    ) {
                      itemsIndexed(viewModel.integratedListItems) { index, item ->
                        integratedListContent.itemContainer(index, item, viewModel)
                      }
                    }
                  }
                  if (viewModel.integratedListTabs.isNotEmpty()) {
                    integratedListContent.floatingActionButton(
                      modifier = Modifier
                        .align(Alignment.BottomEnd)
                        .padding(layoutParams.integratedListFABPadding),
                      viewModel = viewModel
                    )
                  }
                }
              }
            }
          }
        }
        else -> {
          mapSurface()
        }
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
        Column(
          modifier = Modifier
            .fillMaxSize()
            .background(Color.Black.copy(alpha = 0.8f)),
          horizontalAlignment = Alignment.CenterHorizontally
        ) {
          if (viewModel.splashVisible) {
            Image(
              painter = painterResource(R.drawable.splash),
              contentDescription = null
            )
            Text(
              text = viewModel.busyText,
              style = MaterialTheme.typography.headlineSmall,
              color = MaterialTheme.colorScheme.onBackground
            )
            Spacer(Modifier.height(20.dp))
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
      }
    }
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

