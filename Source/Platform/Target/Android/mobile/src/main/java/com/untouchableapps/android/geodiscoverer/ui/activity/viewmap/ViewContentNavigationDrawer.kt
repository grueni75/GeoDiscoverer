//============================================================================
// Name        : ViewContentNavigationDrawer.kt
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
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.ExperimentalMaterialApi
import androidx.compose.material.icons.filled.*
import androidx.compose.material.ripple.rememberRipple
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.ExperimentalComposeUiApi
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.RoundRect
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.*
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.*
import com.untouchableapps.android.geodiscoverer.R
import com.untouchableapps.android.geodiscoverer.ui.component.*
import kotlinx.coroutines.*

@ExperimentalGraphicsApi
@ExperimentalMaterial3Api
@ExperimentalMaterialApi
@ExperimentalAnimationApi
@ExperimentalComposeUiApi
class ViewContentNavigationDrawer(viewContent: ViewContent) {

  // Parameters
  val viewContent=viewContent
  val viewMap=viewContent.viewMap
  val layoutParams=viewContent.layoutParams

  // Navigation drawer item
  class NavigationItem(imageVector: ImageVector? = null, title: String, onClick: () -> Unit) {
    val imageVector = imageVector
    val title = title
    val onClick = onClick
  }

  // Content of the navigation drawer
  @Composable
  fun content(
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
                painter = painterResource(R.mipmap.ic_launcher_foreground),
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
}

