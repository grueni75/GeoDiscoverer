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

package com.untouchableapps.android.geodiscoverer.ui.activity.viewmap

import android.content.res.Configuration
import android.graphics.Typeface
import android.util.TypedValue
import android.view.Gravity
import android.view.ViewGroup
import android.widget.TextView
import androidx.compose.animation.*
import androidx.compose.animation.core.MutableTransitionState
import androidx.compose.animation.core.animateDpAsState
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.DismissValue
import androidx.compose.material.DismissDirection
import androidx.compose.material.ContentAlpha
import androidx.compose.material.ExperimentalMaterialApi
import androidx.compose.material.TextFieldDefaults
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material.rememberDismissState
import androidx.compose.material.ripple.rememberRipple
import androidx.compose.material.FractionalThreshold
import androidx.compose.material3.*
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Checkbox
import androidx.compose.material3.DrawerValue
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LocalContentColor
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.rotate
import androidx.compose.ui.draw.scale
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.RoundRect
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.*
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.*
import androidx.compose.ui.viewinterop.AndroidView
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.R
import com.untouchableapps.android.geodiscoverer.core.GDMapSurfaceView
import com.untouchableapps.android.geodiscoverer.ui.activity.ViewMap2
import com.untouchableapps.android.geodiscoverer.ui.component.*
import com.untouchableapps.android.geodiscoverer.ui.theme.Material2Theme
import com.untouchableapps.android.geodiscoverer.ui.theme.SurfaceColorAtElevation
import kotlinx.coroutines.*
import kotlin.math.ln
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
    val itemDistance = 10.dp
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
  }
  val layoutParams = LayoutParams()

  // Navigation drawer content
  class NavigationItem(imageVector: ImageVector? = null, title: String, onClick: () -> Unit) {
    val imageVector = imageVector
    val title = title
    val onClick = onClick
  }

  // Main content of the activity
  @ExperimentalAnimationApi
  @ExperimentalMaterial3Api
  @ExperimentalMaterialApi
  @Composable
  fun content(
    viewModel: ViewModel,
    appVersion: String,
    navigationItems: List<NavigationItem>
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
              GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "drawerState=$it")
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
            drawerShape = drawerShape(),
            drawerContent = drawerContent(appVersion, innerPadding, navigationItems) {
              scope.launch() {
                viewModel.drawerStatus = DrawerValue.Closed
              }
            }
          ) {
            screenContent(viewModel,contentBoxWithConstraintsScope.maxWidth,contentBoxWithConstraintsScope.maxHeight)
          }
        }
      )
      if (viewModel.progressMax != -1) {
        AlertDialog(
          modifier = Modifier
            .wrapContentHeight(),
          onDismissRequest = {
          },
          confirmButton = {
          },
          title = {
            Text(text = viewModel.progressMessage)
          },
          text = {
            if (viewModel.progressMax==0) {
              GDLinearProgressIndicator(
                modifier = Modifier.fillMaxWidth(),
              )
            } else {
              GDLinearProgressIndicator(
                modifier = Modifier.fillMaxWidth(),
                progress = viewModel.progressCurrent.toFloat() / viewModel.progressMax.toFloat()
              )
            }
          }
        )
      }
      if (viewModel.dialogMessage != "") {
        AlertDialog(
          modifier = Modifier
            .wrapContentHeight(),
          onDismissRequest = {
            viewModel.setDialog("")
          },
          confirmButton = {
            TextButton(
              onClick = {
                if (viewModel.dialogIsFatal)
                  viewMap.finish()
                viewModel.setDialog("")
              }
            ) {
              if (viewModel.dialogIsFatal)
                Text(stringResource(id = R.string.button_label_exit))
              else
                Text(stringResource(id = R.string.button_label_ok))
            }
          },
          icon = {
            if (viewModel.dialogIsFatal)
              Icon(Icons.Default.Error, contentDescription = null)
            else
              Icon(Icons.Default.Warning, contentDescription = null)
          },
          text = {
            Text(
              text = viewModel.dialogMessage,
              style = MaterialTheme.typography.bodyLarge
            )
          }
        )
      }
      if (viewModel.askTitle != "") {
        if (viewModel.askEditTextValue != "") {
          val editTextValue = remember { mutableStateOf(viewModel.askEditTextValue) }
          val editTextTag = remember { mutableStateOf(viewModel.askEditTextTag) }
          askAlertDialog(
            viewModel = viewModel,
            confirmHandler = {
              viewModel.askEditTextConfirmHandler(editTextValue.value, editTextTag.value)
            },
            content = {
              Column(
                modifier = Modifier
                  .wrapContentHeight()
              ) {
                viewModel.askEditTextValueChangeHandler(editTextValue.value)
                GDTextField(
                  value = editTextValue.value,
                  textStyle = MaterialTheme.typography.bodyLarge,
                  label = {
                    Text(
                      text = viewModel.askMessage
                    )
                  },
                  onValueChange = {
                    editTextValue.value = it
                    viewModel.askEditTextValueChangeHandler(it)
                  },
                  trailingIcon = {
                    IconButton(
                      onClick = { editTextValue.value="" }
                    ) {
                      Icon(
                        imageVector = Icons.Default.Clear,
                        contentDescription = null,
                      )
                    }
                  }
                )
                if (viewModel.askEditTextTag != "") {
                  Spacer(Modifier.height(layoutParams.itemDistance))
                  val editTextTagListExpanded = remember { mutableStateOf(false) }
                  val editTextTagListExpandIconAngle: Float by animateFloatAsState(
                    targetValue= if (editTextTagListExpanded.value) 180f else 0f
                  )
                  Column() {
                    val textFieldWidth = remember { mutableStateOf(0.dp) }
                    BoxWithConstraints() {
                      textFieldWidth.value=this.maxWidth
                      GDTextField(
                        value = editTextTag.value,
                        textStyle = MaterialTheme.typography.bodyLarge,
                        label = {
                          Text(
                            text = viewModel.askEditTextTagLabel
                          )
                        },
                        onValueChange = {
                          editTextTag.value = it
                        },
                        trailingIcon = {
                          Row(
                            modifier = Modifier
                              .align(Alignment.CenterEnd),
                          ) {
                            IconButton(
                              onClick = { editTextTag.value="" }
                            ) {
                              Icon(
                                imageVector = Icons.Default.Clear,
                                contentDescription = null,
                              )
                            }
                            IconButton(
                              onClick = { editTextTagListExpanded.value = true }
                            ) {
                              Icon(
                                modifier = Modifier
                                  .rotate(editTextTagListExpandIconAngle),
                                imageVector = Icons.Default.ArrowDropDown,
                                contentDescription = null,
                              )
                            }
                          }
                        }
                      )
                    }
                    GDDropdownMenu(
                      modifier = Modifier
                        .width(textFieldWidth.value)
                        .heightIn(max=layoutParams.askMaxDropdownMenuHeight)
                        .background(SurfaceColorAtElevation(6.dp)), // a hack since dialog background color not accessible
                      expanded = editTextTagListExpanded.value,
                      onDismissRequest = { editTextTagListExpanded.value = false }
                    ) {
                      for (tag in viewModel.askEditTextTagList) {
                        GDDropdownMenuItem(
                          onClick = {
                            editTextTag.value=tag
                            editTextTagListExpanded.value = false
                          }
                        ) {
                          Text(tag)
                        }
                      }
                    }
                  }
                }
                if (viewModel.askEditTextHint != "") {
                  Text(
                    modifier = Modifier
                      .padding(top = layoutParams.itemPadding, start = layoutParams.hintIndent),
                    text = viewModel.askEditTextHint
                  )
                }
                if (viewModel.askEditTextError != "") {
                  Text(
                    modifier = Modifier
                      .padding(top = layoutParams.itemPadding, start = layoutParams.hintIndent),
                    text = viewModel.askEditTextError,
                    color = MaterialTheme.colorScheme.error
                  )
                }
              }
            }
          )
        } else if (viewModel.askMultipleChoiceList.isNotEmpty()) {
          askAlertDialog(
            viewModel = viewModel,
            confirmHandler = {
              viewModel.askMultipleChoiceConfirmHandler(viewModel.getSelectedChoices())
            },
            content = {
              Column(
                modifier = Modifier
                  .fillMaxWidth()
                  .heightIn(max = contentBoxWithConstraintsScope.maxHeight - layoutParams.dialogButonRowHeight)
              ) {
                if (viewModel.askMessage != "") {
                  Text(
                    modifier = Modifier,
                    text = viewModel.askMessage,
                  )
                  Spacer(Modifier.height(layoutParams.askMultipleChoiceMessageOffset))
                }
                LazyColumn(
                  modifier = Modifier
                    .fillMaxWidth()
                ) {
                  itemsIndexed(viewModel.askMultipleChoiceList) { index, item ->
                    val checked = remember { mutableStateOf(item.checked) }
                    Row(
                      modifier = Modifier
                        .fillMaxWidth()
                        .wrapContentHeight(),
                      verticalAlignment = Alignment.CenterVertically
                    ) {
                      Checkbox(
                        checked = checked.value,
                        onCheckedChange = {
                          checked.value = it
                          item.checked = it
                          viewModel.askMultipleChoiceCheckedHandler()
                        }
                      )
                      Text(
                        text = item.text,
                        softWrap = false,
                        overflow = TextOverflow.Ellipsis,
                        style = MaterialTheme.typography.bodyLarge
                      )
                    }
                  }
                }
              }
            }
          )
        } else if (viewModel.askSingleChoiceList.isNotEmpty()) {
          askAlertDialog(
            viewModel = viewModel,
            content = {
              Column(
                modifier = Modifier
                  .fillMaxWidth()
                  .heightIn(max = contentBoxWithConstraintsScope.maxHeight - layoutParams.dialogButonRowHeight)
              ) {
                if (viewModel.askMessage != "") {
                  Text(
                    modifier = Modifier,
                    text = viewModel.askMessage
                  )
                  Spacer(Modifier.height(layoutParams.askMultipleChoiceMessageOffset))
                }
                LazyColumn(
                  modifier = Modifier
                    .fillMaxWidth(),
                ) {
                  itemsIndexed(viewModel.askSingleChoiceList) { index, item ->
                    TextButton(
                      onClick = {
                        viewModel.askSingleChoiceConfirmHandler(item)
                        viewModel.closeQuestion()
                      }
                    ) {
                      Text(
                        text = item,
                        softWrap = false,
                        overflow = TextOverflow.Ellipsis,
                        style = MaterialTheme.typography.bodyLarge
                      )
                    }
                  }
                }
              }
            }
          )
        } else {
          askAlertDialog(
            viewModel = viewModel,
            confirmHandler = {
              viewModel.askQuestionConfirmHandler()
            },
            content = {
              Text(
                text = viewModel.askMessage,
                style = MaterialTheme.typography.bodyLarge
              )
            }
          )
        }
      } else {
        viewModel.allAskAlertDialogsClosed()
      }
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

  // Alert dialog for multiple use cases
  @Composable
  private fun askAlertDialog(
    viewModel: ViewModel,
    confirmHandler: () -> Unit = {},
    content: @Composable () -> Unit
  ) {
    AlertDialog(
      modifier = Modifier
        .wrapContentHeight(),
      onDismissRequest = {
        viewModel.closeQuestion()
      },
      confirmButton = {
        if(viewModel.askConfirmText!="") {
          TextButton(
            modifier = Modifier,
            onClick = {
              confirmHandler()
              viewModel.closeQuestion()
            }
          ) {
            Text(viewModel.askConfirmText)
          }
        }
      },
      dismissButton = {
        if(viewModel.askDismissText!="") {
          TextButton(
            onClick = {
              viewModel.askQuestionDismissHandler()
              viewModel.closeQuestion()
            }
          ) {
            Text(viewModel.askDismissText)
          }
        }
      },
      title = {
        Text(text = viewModel.askTitle)
      },
      text = {
        content()
      }
    )
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
                            integratedListTab(index, tab, viewModel)
                          }
                        }
                      }
                    }
                  }
                  val itemListState = rememberLazyListState()
                  LaunchedEffect(viewModel.integratedListVisible) {
                    if (viewModel.integratedListSelectedItem!=-1)
                      itemListState.scrollToItem(viewModel.integratedListSelectedItem)
                  }
                  LazyColumn(
                    state = itemListState,
                    modifier = Modifier
                      .fillMaxWidth()
                  ) {
                    itemsIndexed(viewModel.integratedListItems) { index, item ->
                      integratedListItemContainer(index, item, viewModel)
                    }
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

  // Content of the navigation drawer
  @Composable
  private fun drawerContent(
    appVersion: String,
    innerPadding: PaddingValues,
    navigationItems: List<NavigationItem>,
    closeDrawer: () -> Unit
  ): @Composable() (ColumnScope.() -> Unit) =
    {
      Column(
        modifier = Modifier
          .width(layoutParams.drawerWidth)
      ) {
        Box(
          modifier = Modifier
            .background(MaterialTheme.colorScheme.surfaceVariant)
            .fillMaxWidth(),
          contentAlignment = Alignment.CenterStart
        ) {
          /*Image(
            modifier = Modifier.fillMaxWidth(),
            painter = painterResource(R.drawable.nav_header),
            contentDescription = null,
            contentScale = ContentScale.FillWidth
          )*/
          Row(
            verticalAlignment = Alignment.CenterVertically
          ) {
            Column(
              modifier = Modifier
                .width(layoutParams.iconWidth),
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
            .fillMaxWidth()
        ) {
          itemsIndexed(navigationItems) { index, item ->
            navigationItem(index, item, closeDrawer)
          }
        }
      }
    }

  // Sets the width of the navigation drawer correctly
  fun drawerShape() = object : Shape {
    override fun createOutline(
      size: Size,
      layoutDirection: LayoutDirection,
      density: Density
    ): Outline {
      val cornerRadius = with(density) { layoutParams.drawerCornerRadius.toPx() }
      return Outline.Rounded(
        RoundRect(
          left = 0f,
          top = 0f,
          right = with(density) { layoutParams.drawerWidth.toPx() },
          bottom = size.height,
          topRightCornerRadius = CornerRadius(cornerRadius),
          bottomRightCornerRadius = CornerRadius(cornerRadius)
        )
      )
    }
  }

  // Creates a navigation item for the drawer
  @Composable
  fun navigationItem(index: Int, item: NavigationItem, closeDrawer: () -> Unit) {
    if (item.imageVector == null) {
      if (index != 0) {
        separator()
      }
      Row(
        modifier = Modifier
          .fillMaxWidth()
          .padding(vertical = layoutParams.itemPadding),
        verticalAlignment = Alignment.CenterVertically
      ) {
        Column(
          modifier = Modifier
            .absolutePadding(left = layoutParams.titleIndent)
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
          .fillMaxWidth()
          .padding(horizontal = layoutParams.itemPadding)
          .clip(shape = RoundedCornerShape(layoutParams.drawerCornerRadius))
          .clickable(
            onClick = {
              item.onClick()
              closeDrawer()
            },
            interactionSource = interactionSource,
            indication = rememberRipple(bounded = true)
          )
      ) {
        Row(
          modifier = Modifier
            .fillMaxWidth(),
          verticalAlignment = Alignment.CenterVertically
        ) {
          Spacer(
            Modifier
              .width(0.dp)
              .height(layoutParams.drawerItemHeight)
          )
          Column(
            modifier = Modifier
              .width(layoutParams.iconWidth),
            horizontalAlignment = Alignment.CenterHorizontally
          ) {
            Icon(
              imageVector = item.imageVector,
              contentDescription = null
            )
          }
          Column(
            modifier = Modifier
              .weight(1f)
          ) {
            Text(
              text = item.title,
              style = MaterialTheme.typography.bodyMedium
            )
          }
        }
      }
    }
  }

  // Creates a seperator
  @Composable
  private fun separator() {
    Box(
      modifier = Modifier
        .fillMaxWidth()
        .padding(layoutParams.itemPadding)
        .height(1.dp)
        .background(MaterialTheme.colorScheme.outline)
    )
  }

  // Creates an item for the integrated list
  @ExperimentalAnimationApi
  @ExperimentalMaterialApi
  @Composable
  fun integratedListItemContainer(index: Int, item: String, viewModel: ViewModel) {
    if (viewModel.integratedListTabs.isEmpty()) {
      integratedListItemContent(index, item, viewModel)
    } else {
      val selectedTab = remember { mutableStateOf(-1) }

      // Handle the dismiss state
      val dismissState = rememberDismissState(
        confirmStateChange = {
          if (it == DismissValue.DismissedToStart) {
            GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "delete confirmed for ${index}")
            viewModel.integratedListDeleteItemHandler(item)
          }
          if (it == DismissValue.DismissedToEnd) {
            GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "edit confirmed for ${index}")
            viewModel.integratedListEditItemHandler(index)
          }
          false
        }
      )

      // Reset the views if tab is switched
      LaunchedEffect(selectedTab.value != viewModel.integratedListSelectedTab) {
        dismissState.reset()
        selectedTab.value=viewModel.integratedListSelectedTab
      }

      // Generate the content
      GDSwipeToDismiss(
        state = dismissState,
        directions = setOf(DismissDirection.StartToEnd, DismissDirection.EndToStart),
        dismissThresholds = { direction ->
          if (direction == DismissDirection.StartToEnd)
            return@GDSwipeToDismiss FractionalThreshold(0.15f)
          else
            return@GDSwipeToDismiss FractionalThreshold(0.15f)
        },
        background = {
          //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","updating background for ${index}")
          val direction = dismissState.dismissDirection ?: return@GDSwipeToDismiss
          val color by animateColorAsState(
            when (dismissState.targetValue) {
              DismissValue.Default -> MaterialTheme.colorScheme.surfaceVariant
              DismissValue.DismissedToEnd -> Color.Green.copy(alpha = ContentAlpha.medium)
              DismissValue.DismissedToStart -> Color.Red.copy(alpha = ContentAlpha.medium)
            }
          )
          val alignment = when (direction) {
            DismissDirection.StartToEnd -> Alignment.CenterStart
            DismissDirection.EndToStart -> Alignment.CenterEnd
          }
          val icon = when (direction) {
            DismissDirection.StartToEnd -> Icons.Default.Edit
            DismissDirection.EndToStart -> Icons.Default.Delete
          }
          val scale by animateFloatAsState(
            if (dismissState.targetValue == DismissValue.Default) 0.75f else 1f
          )
          Box(
            Modifier
              .fillMaxSize()
              .background(color)
              .padding(horizontal = 20.dp),
            contentAlignment = alignment
          ) {
            Icon(
              icon,
              contentDescription = null,
              modifier = Modifier.scale(scale)
            )
          }
        },
        dismissContent = {
          //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","updating content for ${index}")
          Surface(
            shadowElevation = animateDpAsState(
              if (dismissState.dismissDirection != null) 6.dp else 0.dp
            ).value
          ) {
            integratedListItemContent(index, item, viewModel)
          }
        }
      )
    }
  }

  // Creates the content for an item for the integrated list
  @ExperimentalMaterialApi
  @Composable
  fun integratedListItemContent(index: Int, item: String, viewModel: ViewModel) {
    val interactionSource = remember { MutableInteractionSource() }
    Box(
      modifier= Modifier
        .fillMaxWidth()
        .height(layoutParams.integratedListItemHeight)
        .padding(layoutParams.itemPadding)
        .clip(
          shape =
          if (index == viewModel.integratedListSelectedItem)
            RoundedCornerShape(layoutParams.drawerCornerRadius)
          else
            RectangleShape
        )
        .clickable(
          onClick = {
            viewModel.selectIntegratedListItem(index)
            viewModel.integratedListSelectItemHandler(item)
          },
          interactionSource = interactionSource,
          indication = rememberRipple(bounded = true)
        )
        .background(
          if (index == viewModel.integratedListSelectedItem)
            MaterialTheme.colorScheme.primary.copy(alpha = ContentAlpha.medium)
          else
            MaterialTheme.colorScheme.surface
        )
    ) {
      Text(
        modifier = Modifier
          .align(Alignment.Center),
        style = MaterialTheme.typography.titleMedium,
        text = item
      )
    }
  }

  // Creates a tab for the integrated list
  @Composable
  fun integratedListTab(index: Int, tab: String, viewModel: ViewModel) {
    val interactionSource = remember { MutableInteractionSource() }
    Box(
      modifier = Modifier
        .height(layoutParams.integratedListTabHeight)
        .width(layoutParams.integratedListTabWidth)
        .clickable(
          onClick = {
            if (index != viewModel.integratedListSelectedTab) {
              viewModel.selectIntegratedListTab(index)
              viewModel.integratedListSelectTabHandler(tab)
            }
          },
          interactionSource = interactionSource,
          indication = rememberRipple(bounded = true)
        )
    ) {
      Text(
        modifier = Modifier
          .align(Alignment.Center)
          .padding(horizontal = layoutParams.itemPadding),
        style = MaterialTheme.typography.titleMedium,
        color = if (index == viewModel.integratedListSelectedTab)
          MaterialTheme.colorScheme.primary
        else
          LocalContentColor.current.copy(alpha = ContentAlpha.medium),
        softWrap = false,
        overflow = TextOverflow.Ellipsis,
        text = tab
      )
      Box(
        modifier = Modifier
          .width(layoutParams.integratedListTabWidth)
          .height(2.dp)
          .background(
            if (index == viewModel.integratedListSelectedTab)
              MaterialTheme.colorScheme.primary
            else
              Color.Transparent
          )
          .align(Alignment.BottomCenter),
      )
    }
  }
}

