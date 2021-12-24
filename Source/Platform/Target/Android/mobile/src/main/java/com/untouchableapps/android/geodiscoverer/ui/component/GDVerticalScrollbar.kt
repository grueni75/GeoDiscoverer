//============================================================================
// Name        : GDVerticalScrollbar.kt
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

package com.untouchableapps.android.geodiscoverer.ui.component

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.ScrollState
import androidx.compose.foundation.lazy.LazyListState
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.MaterialTheme.colorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.drawWithContent
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import com.untouchableapps.android.geodiscoverer.GDApplication

@Composable
fun Modifier.GDVerticalScrollbar(
  state: LazyListState,
  color: Color = MaterialTheme.colorScheme.primary,
  width: Dp = 8.dp
): Modifier {
  val targetAlpha = if (state.isScrollInProgress) 1f else 0f
  val duration = if (state.isScrollInProgress) 150 else 500

  val alpha by animateFloatAsState(
    targetValue = targetAlpha,
    animationSpec = tween(durationMillis = duration)
  )

  return drawWithContent {
    drawContent()

    val firstVisibleElementIndex = state.layoutInfo.visibleItemsInfo.firstOrNull()?.index
    val needDrawScrollbar = state.isScrollInProgress || alpha > 0.0f

    // Draw scrollbar if scrolling or if the animation is still running and lazy column has content
    if (needDrawScrollbar && firstVisibleElementIndex != null) {
      val elementHeight = this.size.height / state.layoutInfo.totalItemsCount
      val scrollbarOffsetY = firstVisibleElementIndex * elementHeight
      val scrollbarHeight = state.layoutInfo.visibleItemsInfo.size * elementHeight
      //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp", "${state.firstVisibleItemScrollOffset} ${state.layoutInfo.viewportStartOffset} ${state.layoutInfo.viewportEndOffset}")
      drawRect(
        color = color,
        topLeft = Offset(this.size.width - width.toPx(), scrollbarOffsetY),
        size = Size(width.toPx(), scrollbarHeight),
        alpha = alpha
      )
    }
  }
}