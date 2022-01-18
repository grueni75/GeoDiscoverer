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

import androidx.compose.animation.*
import androidx.compose.animation.core.MutableTransitionState
import androidx.compose.animation.core.animateDpAsState
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.ContentAlpha
import androidx.compose.material.DismissDirection
import androidx.compose.material.DismissValue
import androidx.compose.material.ExperimentalMaterialApi
import androidx.compose.material.FractionalThreshold
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material.rememberDismissState
import androidx.compose.material.ripple.rememberRipple
import androidx.compose.material3.*
import androidx.compose.material3.Icon
import androidx.compose.material3.LocalContentColor
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.scale
import androidx.compose.ui.graphics.*
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.*
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.ui.component.*
import kotlinx.coroutines.*

@ExperimentalGraphicsApi
@ExperimentalMaterial3Api
class ViewContentIntegratedList(viewContent: ViewContent) {

  // Parameters
  val viewContent=viewContent
  val viewMap=viewContent.viewMap
  val layoutParams=viewContent.layoutParams

  // Creates an item for the integrated list
  @ExperimentalAnimationApi
  @ExperimentalMaterialApi
  @Composable
  fun itemContainer(index: Int, item: String, viewModel: ViewModel) {
    //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","$index: ${item}")
    if (viewModel.integratedListTabs.isEmpty()) {
      itemContent(index, item, viewModel)
    } else {
      val selectedTab = remember { mutableStateOf(-1) }
      val updateCount = remember { mutableStateOf(-1) }
      val animVisibilityState = remember { MutableTransitionState<Boolean>(false) }

      // Handle the case if the edit is confirmed
      val dismissState = rememberDismissState(
        confirmStateChange = {
          if (it == DismissValue.DismissedToStart) {
            animVisibilityState.targetState = false
            return@rememberDismissState true
          }
          if (it == DismissValue.DismissedToEnd) {
            GDApplication.addMessage(
              GDApplication.DEBUG_MSG,
              "GDApp",
              "edit confirmed for ${index}"
            )
            viewModel.integratedListEditItemHandler(index)
            return@rememberDismissState false
          }
          false
        }
      )

      // Reset the views if tab is switched
      // Reset the views if list has changed
      //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","${viewModel.integratedListUpdateCount}")
      if ((selectedTab.value != viewModel.integratedListSelectedTab)||(updateCount.value!=viewModel.integratedListUpdateCount)) {
        //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","resetting views")
        animVisibilityState.targetState = true
        LaunchedEffect(Unit) {
          dismissState.reset()
        }
        selectedTab.value = viewModel.integratedListSelectedTab
        updateCount.value = viewModel.integratedListUpdateCount
      }

      // Handle the event when the delete is confirmed and the item is not visible anymore
      LaunchedEffect(animVisibilityState.targetState, animVisibilityState.currentState) {
        if (!animVisibilityState.targetState && !animVisibilityState.currentState) {
          if (dismissState.targetValue == DismissValue.DismissedToStart) {
            GDApplication.addMessage(
              GDApplication.DEBUG_MSG,
              "GDApp",
              "delete confirmed for ${index}"
            )
            dismissState.reset()
            viewModel.integratedListDeleteItemHandler(item)
          }
        }
      }

      // Generate the content
      AnimatedVisibility(
        enter = EnterTransition.None,
        exit = shrinkVertically() + fadeOut(),
        visibleState = animVisibilityState
      ) {
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
              itemContent(index, item, viewModel)
            }
          }
        )
      }
    }
  }

  // Creates the content for an item for the integrated list
  @ExperimentalMaterialApi
  @Composable
  fun itemContent(index: Int, item: String, viewModel: ViewModel) {
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
            MaterialTheme.colorScheme.primary.copy(alpha=ContentAlpha.medium)
          else
            MaterialTheme.colorScheme.surface
        )
    ) {
      Text(
        modifier = Modifier
          .align(Alignment.Center)
          .padding(horizontal = layoutParams.itemPadding),
        style = MaterialTheme.typography.titleMedium,
        softWrap = false,
        overflow = TextOverflow.Ellipsis,
        text = item
      )
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

  // Creates a floating action button
  @Composable
  fun floatingActionButton(modifier: Modifier, viewModel: ViewModel) {
    FloatingActionButton(
      modifier = modifier,
      onClick = {
        viewModel.integratedListAddItemHandler()
      }
    ) {
      Icon(Icons.Default.Add, null)
    }
  }

}

