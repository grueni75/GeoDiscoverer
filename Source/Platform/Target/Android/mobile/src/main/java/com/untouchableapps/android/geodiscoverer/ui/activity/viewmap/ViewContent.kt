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

import android.graphics.Typeface
import android.util.TypedValue
import android.view.Gravity
import android.view.ViewGroup
import android.widget.TextView
import androidx.compose.animation.*
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.material.icons.Icons
import androidx.compose.material.ripple.rememberRipple
import androidx.compose.material3.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.res.painterResource
import com.untouchableapps.android.geodiscoverer.R
import androidx.compose.foundation.Image
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.filled.Error
import androidx.compose.material.icons.filled.Warning
import androidx.compose.runtime.*
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.RoundRect
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Outline
import androidx.compose.ui.graphics.Shape
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.*
import androidx.compose.ui.viewinterop.AndroidView
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.core.GDMapSurfaceView
import com.untouchableapps.android.geodiscoverer.ui.activity.ViewMap2
import com.untouchableapps.android.geodiscoverer.ui.component.GDLinearProgressIndicator
import com.untouchableapps.android.geodiscoverer.ui.component.GDSnackBar
import com.untouchableapps.android.geodiscoverer.ui.component.GDTextField
import kotlinx.coroutines.*

@ExperimentalMaterial3Api
class ViewContent(viewMap: ViewMap2) {

  // Parameters
  val viewMap=viewMap

  // Layout parameters for the navigation drawer
  class LayoutParams() {
    val iconWidth = 60.dp
    val itemHeight = 41.dp
    val titleIndent = 20.dp
    val drawerWidth = 250.dp
    val itemPadding = 5.dp
    val hintIndent = 15.dp
    val drawerCornerRadius = 16.dp
    val snackbarHorizontalPadding = 20.dp
    val snackbarVerticalOffset = 40.dp
    val snackbarMaxWidth = 400.dp
    val askMaxContentHeight = 140.dp
    val askMultipleChoiceMessageOffset = 15.dp
    val dialogButonRowHeight = 200.dp
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
            screenContent(viewModel)
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
              Icon(Icons.Filled.Error, contentDescription = null)
            else
              Icon(Icons.Filled.Warning, contentDescription = null)
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
          askAlertDialog(
            viewModel = viewModel,
            confirmHandler = {
              viewModel.askEditTextConfirmHandler(editTextValue.value)
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
                  })
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
  @Composable
  private fun screenContent(viewModel: ViewModel) {
    val scope = rememberCoroutineScope()
    Box(
      modifier = Modifier
        .fillMaxSize()
    ) {
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
        modifier = Modifier
          .fillMaxWidth()
          .fillMaxHeight(),
      )
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
      LaunchedEffect(viewModel.snackbarText) {
        if (viewModel.snackbarText != "") {
          delay(3000)
          viewModel.showSnackbar("")
        }
      }
    }
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
        Box(
          modifier = Modifier
            .fillMaxWidth()
            .padding(layoutParams.itemPadding)
            .height(1.dp)
            .background(MaterialTheme.colorScheme.outline)
        )
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
              .height(layoutParams.itemHeight)
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
}

