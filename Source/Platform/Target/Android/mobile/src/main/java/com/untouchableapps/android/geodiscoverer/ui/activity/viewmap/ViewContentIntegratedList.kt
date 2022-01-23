//============================================================================
// Name        : ViewContentIntegratedList.kt
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
import androidx.compose.animation.*
import androidx.compose.animation.core.animateDpAsState
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.ContentAlpha
import androidx.compose.material.DismissDirection
import androidx.compose.material.DismissValue
import androidx.compose.material.ExperimentalMaterialApi
import androidx.compose.material.FractionalThreshold
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material.ripple.rememberRipple
import androidx.compose.material3.*
import androidx.compose.material3.FloatingActionButton
import androidx.compose.material3.Icon
import androidx.compose.material3.LocalContentColor
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.scale
import androidx.compose.ui.graphics.*
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.*
import com.google.accompanist.swiperefresh.SwipeRefresh
import com.google.accompanist.swiperefresh.rememberSwipeRefreshState
import com.untouchableapps.android.geodiscoverer.R
import com.untouchableapps.android.geodiscoverer.ui.component.*
import kotlinx.coroutines.*

@ExperimentalAnimationApi
@ExperimentalGraphicsApi
@ExperimentalMaterial3Api
@ExperimentalMaterialApi
class ViewContentIntegratedList(viewContent: ViewContent) {

  // Parameters
  val viewContent = viewContent
  val viewMap = viewContent.viewMap
  val layoutParams = viewContent.layoutParams

  // Creates the main content
  @ExperimentalMaterialApi
  @Composable
  fun content(
    configuration: Configuration,
    maxScreenWidth: Dp,
    maxScreenHeight: Dp,
    viewModel: ViewModel
  ) {
    val scope = rememberCoroutineScope()
    Surface(
      modifier = Modifier
        .wrapContentSize()
        //.animateContentSize()
    ) {
      val width: Dp by animateDpAsState(
        if (viewModel.integratedListPOIFilerEnabled)
          maxScreenWidth
        else
          layoutParams.integratedListWidth
      )
      Column(
        modifier = Modifier
          .width(width),
        verticalArrangement = Arrangement.Bottom
      ) {

        // Handlebar with close icon and progress indicator
        var handlebarHeight = layoutParams.integratedListCloseRowHeight+layoutParams.progressIndicatorHeight
        BoxWithConstraints(
        ) {
          Surface() {
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
                      imageVector =
                      if (configuration.orientation == Configuration.ORIENTATION_LANDSCAPE)
                        Icons.Default.NavigateNext
                      else
                        Icons.Default.ExpandMore,
                      contentDescription = null
                    )
                  }
                }
              }
              Box(
                modifier = Modifier
                  .height(layoutParams.progressIndicatorHeight),
              ) {
                if (viewModel.integratedListBusy) {
                    GDLinearProgressIndicator(
                    modifier = Modifier
                      .fillMaxWidth()
                  )
                }
              }
            }
          }
        }

        // Helper to position the two sub boxes
        val height: Dp by animateDpAsState(
          if (viewModel.integratedListPOIFilerEnabled)
            maxScreenHeight - handlebarHeight
          else
            layoutParams.integratedListHeight - handlebarHeight
        )
        Box(
          modifier = Modifier
            .fillMaxWidth()
            .height(height)
            .width(width)
        ) {

          // POI category list
          if (viewModel.integratedListPOIFilerEnabled) {
            Column(
              modifier = Modifier
                .height(
                  if (configuration.orientation == Configuration.ORIENTATION_LANDSCAPE)
                    maxScreenHeight - handlebarHeight
                  else
                    maxScreenHeight - layoutParams.integratedListHeight
                )
                .width(
                  if (configuration.orientation == Configuration.ORIENTATION_LANDSCAPE)
                    maxScreenWidth - layoutParams.integratedListWidth
                  else
                    maxScreenWidth
                )
                .align(
                  if (configuration.orientation == Configuration.ORIENTATION_LANDSCAPE)
                    Alignment.TopStart
                  else
                    Alignment.TopStart
                )
            ) {
              Surface() {
                Column(
                  modifier = Modifier
                    .background(MaterialTheme.colorScheme.surfaceVariant)
                ) {
                  Text(
                    modifier = Modifier
                      .padding(layoutParams.itemPadding)
                      .fillMaxWidth(),
                    text = stringResource(id = R.string.dialog_poi_category_title),
                    style = MaterialTheme.typography.titleLarge,
                    textAlign = TextAlign.Center
                  )
                }
              }
              Box(
                modifier = Modifier
                  .weight(1f)
              ) {
                val categoryListState = rememberLazyListState()
                val categoryLastListSize = remember { mutableStateOf(0) }
                LaunchedEffect(
                  viewModel.integratedListPOICategoryList.size,
                  categoryLastListSize.value
                ) {
                  ///GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","size=${viewModel.integratedListPOICategoryList.size}")
                  if (categoryLastListSize.value != viewModel.integratedListPOICategoryList.size) {
                    //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","resetting scroll pos")
                    var index = 0
                    if (viewModel.integratedListPOISelectedCategory < viewModel.integratedListPOICategoryList.size)
                      index = viewModel.integratedListPOISelectedCategory
                    if (index>=0)
                      categoryListState.animateScrollToItem(index)
                  }
                }
                LazyColumn(
                  modifier = Modifier
                    .fillMaxWidth(),
                  state = categoryListState
                ) {
                  itemsIndexed(viewModel.integratedListPOICategoryList) { index, item ->
                    itemContent(
                      index = index,
                      selected = index == viewModel.integratedListPOISelectedCategory,
                      selectHandler = {
                        scope.launch {
                          delay(layoutParams.rippleWaitTime) // ugly hack to avoid ripple continue after selecting an item
                          viewModel.integratedListPOICategoryHandler(index, item)
                        }
                      },
                      viewModel = viewModel
                    ) { modifier ->
                      Text(
                        modifier = modifier,
                        text = item,
                        softWrap = false,
                        overflow = TextOverflow.Ellipsis,
                        style = MaterialTheme.typography.titleMedium
                      )
                    }
                  }
                }
              }
            }
          }

          // Address list
          Column(
            modifier = Modifier
              .align(Alignment.BottomEnd)
              .height(
                if (configuration.orientation == Configuration.ORIENTATION_LANDSCAPE)
                  maxScreenHeight - handlebarHeight
                else
                  layoutParams.integratedListHeight - handlebarHeight
              )
              .width(
                if (configuration.orientation == Configuration.ORIENTATION_LANDSCAPE)
                  layoutParams.integratedListWidth
                else
                  maxScreenWidth
              )
          ) {
            Surface() {
              Column(
                modifier = Modifier
                  .background(MaterialTheme.colorScheme.surfaceVariant)
              ) {
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
                      tabListState.animateScrollToItem(viewModel.integratedListSelectedTab)
                  }
                  LazyRow(
                    state = tabListState,
                    modifier = Modifier
                      .fillMaxWidth()
                  ) {
                    itemsIndexed(viewModel.integratedListTabs) { index, tab ->
                      tabContent(index, tab, viewModel)
                    }
                  }
                }
              }
            }
            Box(
              modifier = Modifier
                .weight(1f)
                .background(MaterialTheme.colorScheme.surface)
            ) {
              val itemListState = rememberLazyListState()
              LaunchedEffect(viewModel.integratedListVisible) {
                if (viewModel.integratedListSelectedItem != -1)
                  itemListState.animateScrollToItem(viewModel.integratedListSelectedItem)
              }
              SwipeRefresh(
                state = rememberSwipeRefreshState(false),
                onRefresh = { viewModel.integratedListRefreshItemsHandler() },
              ) {
                LazyColumn(
                  state = itemListState,
                  modifier = Modifier
                    .fillMaxSize(),
                ) {
                  itemsIndexed(viewModel.integratedListItems) { index, item ->
                    itemContainer(index, item, viewModel)
                  }
                  itemsIndexed(viewModel.integratedListPOIItems) { index, item ->
                    itemContainer(index, item, viewModel)
                  }
                }
              }
              if (viewModel.integratedListTabs.isNotEmpty()) {
                floatingActionButton(
                  modifier = Modifier
                    .align(Alignment.BottomEnd)
                    .padding(layoutParams.integratedListFABPadding),
                  viewModel = viewModel
                )
              }
            }
            AnimatedVisibility(
              visible = (
                  (viewModel.integratedListSelectedTab == viewModel.integratedListTabs.size - 1) &&
                      (viewModel.integratedListTabs.isNotEmpty())
                  )
            ) {
              Row(
                modifier = Modifier
                  .background(MaterialTheme.colorScheme.surfaceVariant),
                verticalAlignment = Alignment.CenterVertically
              ) {
                Text(
                  modifier = Modifier
                    .padding(start = 2 * layoutParams.itemPadding)
                    .width(layoutParams.poiSearchRadiusWidth),
                  text = "${stringResource(R.string.search_radius)} ${
                    viewModel.integratedListPOISearchRadius.toInt().toString()
                  } km",
                )
                GDSlider(
                  modifier = Modifier
                    .weight(1f)
                    .padding(horizontal = layoutParams.itemPadding),
                  value = viewModel.integratedListPOISearchRadius,
                  valueRange = 1f..viewModel.integratedListPOISearchRadiusMax,
                  steps = viewModel.integratedListPOISearchRadiusMax.toInt(),
                  onValueChange = {
                    viewModel.integratedListPOISearchRadiusHandler(it)
                  }
                )
              }
            }
          }
        }
      }
    }
  }

  // Creates an item for the integrated list
  @Composable
  fun itemContainer(index: Int, item: ViewModel.IntegratedListItem, viewModel: ViewModel) {
    //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","$index: ${item}")
    if (viewModel.integratedListTabs.isEmpty()) {
      itemContent(
        index = index,
        selected = index == viewModel.integratedListSelectedItem,
        selectHandler = {
          viewModel.selectIntegratedListItem(index)
          viewModel.integratedListSelectItemHandler(index)
        },
        viewModel = viewModel
      ) { modifier ->
        itemText(modifier, item, viewModel)
      }
    } else {

      // Handle the event when the delete is confirmed and the item is not visible anymore
      LaunchedEffect(item.visibilityState.targetState, item.visibilityState.currentState) {
        item.checkDismissState()
      }

      // Generate the content
      AnimatedVisibility(
        enter = EnterTransition.None,
        exit = shrinkVertically() + fadeOut(),
        visibleState = item.visibilityState
      ) {
        GDSwipeToDismiss(
          state = item.dismissState,
          directions = setOf(DismissDirection.StartToEnd, DismissDirection.EndToStart),
          dismissThresholds = { direction ->
            if (direction == DismissDirection.StartToEnd)
              return@GDSwipeToDismiss FractionalThreshold(0.15f)
            else
              return@GDSwipeToDismiss FractionalThreshold(0.15f)
          },
          background = {
            //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","updating background for ${index}")
            val direction = item.dismissState.dismissDirection ?: return@GDSwipeToDismiss
            val color by animateColorAsState(
              when (item.dismissState.targetValue) {
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
              DismissDirection.StartToEnd -> if (item.isPOI) Icons.Default.Add else Icons.Default.Edit
              DismissDirection.EndToStart -> Icons.Default.Delete
            }
            val scale by animateFloatAsState(
              if (item.dismissState.targetValue == DismissValue.Default) 0.75f else 1f
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
              /* shadowElevation = animateDpAsState(
                if (dismissState.dismissDirection != null) 6.dp else 0.dp
              ).value */
            ) {
              itemContent(
                index = index,
                selected = index == viewModel.integratedListSelectedItem,
                selectHandler = {
                  viewModel.selectIntegratedListItem(index)
                  viewModel.integratedListSelectItemHandler(index)
                },
                viewModel = viewModel
              ) { modifier ->
                itemText(modifier,item,viewModel)
              }
            }
          }
        )
      }
    }
  }

  // Creates the content for an item for the integrated list
  @ExperimentalMaterialApi
  @Composable
  fun itemContent(index: Int, selected: Boolean, viewModel: ViewModel, selectHandler: (Int)->Unit, text: @Composable (Modifier)->Unit) {
    val interactionSource = remember { MutableInteractionSource() }
    Box(
      modifier = Modifier
        .fillMaxWidth()
        .height(layoutParams.integratedListItemHeight)
        .padding(layoutParams.itemPadding)
        .clip(
          shape =
          if (selected)
            RoundedCornerShape(layoutParams.drawerCornerRadius)
          else
            RectangleShape
        )
        .clickable(
          onClick = {
            selectHandler(index)
          },
          interactionSource = interactionSource,
          indication = rememberRipple(bounded = true)
        )
        .background(
          if (selected)
            MaterialTheme.colorScheme.primary.copy(alpha = ContentAlpha.medium)
          else
            MaterialTheme.colorScheme.surface
        )
    ) {
      text(
        Modifier
          .align(Alignment.CenterStart)
          .padding(horizontal = layoutParams.itemPadding)
      )
    }
  }

  // Creates the text content for an item
  @Composable
  fun itemText(modifier: Modifier, item: ViewModel.IntegratedListItem, viewModel: ViewModel) {
    if (item.right=="") {
      Text(
        modifier = modifier
          .fillMaxWidth(),
        style = MaterialTheme.typography.titleMedium,
        softWrap = false,
        overflow = TextOverflow.Ellipsis,
        textAlign = if (viewModel.integratedListCenterItem)
          TextAlign.Center
        else
          TextAlign.Start,
        text = item.left
      )
    } else {
      Row(modifier = modifier) {
        Text(
          modifier = Modifier
            .weight(1.0f),
          text = item.left,
          softWrap = false,
          overflow = TextOverflow.Ellipsis,
          style = MaterialTheme.typography.bodyLarge
        )
        Spacer(
          modifier = Modifier
            .width(layoutParams.itemPadding)
        )
        Text(
          text = item.right,
          style = MaterialTheme.typography.bodyLarge
        )
      }
    }
  }

  // Creates a tab for the integrated list
  @Composable
  fun tabContent(index: Int, tab: String, viewModel: ViewModel) {
    val interactionSource = remember { MutableInteractionSource() }
    Box(
      modifier = Modifier
        .height(layoutParams.integratedListTabHeight)
        .width(layoutParams.integratedListTabWidth)
        .clickable(
          onClick = {
            if (index != viewModel.integratedListSelectedTab) {
              viewModel.selectIntegratedListTab(index)
              viewModel.integratedListSelectTabHandler(index)
            }
          },
          interactionSource = interactionSource,
          indication = rememberRipple(bounded = true)
        )
    ) {
      Row(
        modifier = Modifier
          .align(Alignment.Center)
      ) {
        Text(
          modifier = Modifier
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
        AnimatedVisibility(
          visible = (
              (index==viewModel.integratedListTabs.size-1)&&
              (viewModel.integratedListSelectedTab==viewModel.integratedListTabs.size-1)&&
              (viewModel.integratedListLimitReached)
          )
        ) {
          Icon(
            imageVector = Icons.Default.Warning,
            contentDescription = null,
            tint = MaterialTheme.colorScheme.primary
          )
        }
      }
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

  // Creates a floating action button
  @Composable
  fun floatingActionButton(modifier: Modifier, viewModel: ViewModel) {
    FloatingActionButton(
      modifier = modifier,
      onClick = {
        if (viewModel.integratedListSelectedTab==viewModel.integratedListTabs.size-1)
          viewModel.setPOIFilterEnabled(!viewModel.integratedListPOIFilerEnabled)
        else
          viewModel.integratedListAddItemHandler()
      }
    ) {
      if (viewModel.integratedListSelectedTab==viewModel.integratedListTabs.size-1)
        Icon(Icons.Default.FilterList, null)
      else
        Icon(Icons.Default.Add, null)
    }
  }

  /* Creates an item for the integrated list
  @ExperimentalAnimationApi
  @ExperimentalMaterialApi
  @Composable
  fun poiItemContainer(index: Int, item: GDBackgroundTask.AddressPointItem, viewModel: ViewModel) {

    val selectedTab = remember { mutableStateOf(-1) }
    val updateCount = remember { mutableStateOf(-1) }

    // Handle the case if the edit is confirmed
    val dismissState = rememberDismissState(
      confirmStateChange = {
        if (it == DismissValue.DismissedToStart) {
          GDApplication.addMessage(
            GDApplication.DEBUG_MSG,
            "GDApp",
            "delete confirmed for ${index}"
          )
          viewModel.integratedListDeleteItemHandler(item.nameUniquified)
          return@rememberDismissState false
        }
        if (it == DismissValue.DismissedToEnd) {
          GDApplication.addMessage(
            GDApplication.DEBUG_MSG,
            "GDApp",
            "export confirmed for ${index}"
          )
          viewModel.integratedListPOIImportHandler(index)
          return@rememberDismissState false
        }
        false
      }
    )

    // Reset the views if tab is switched
    // Reset the views if list has changed
    //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","${viewModel.integratedListUpdateCount}")
    if ((selectedTab.value != viewModel.integratedListSelectedTab) || (updateCount.value != viewModel.integratedListUpdateCount)) {
      LaunchedEffect(Unit) {
        dismissState.reset()
      }
      selectedTab.value = viewModel.integratedListSelectedTab
      updateCount.value = viewModel.integratedListUpdateCount
    }

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
          DismissDirection.StartToEnd -> Icons.Default.Add
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
          /*shadowElevation = animateDpAsState(
            if (dismissState.dismissDirection != null) 6.dp else 0.dp
          ).value */
        ) {
          itemContent(
            index = index,
            selected = index == viewModel.integratedListSelectedItem,
            selectHandler = {
              viewModel.selectIntegratedListItem(index)
              viewModel.integratedListSelectItemHandler(index)
            },
            viewModel = viewModel
          ) { modifier ->
            Row(modifier = modifier) {
              Text(
                modifier = Modifier
                  .weight(1.0f),
                text = item.nameUniquified,
                softWrap = false,
                overflow = TextOverflow.Ellipsis,
                style = MaterialTheme.typography.bodyLarge
              )
              Spacer(
                modifier = Modifier
                  .width(layoutParams.itemPadding)
              )
              Text(
                text = item.distanceFormatted,
                style = MaterialTheme.typography.bodyLarge
              )
            }
          }
        }
      }
    )
  }*/

}