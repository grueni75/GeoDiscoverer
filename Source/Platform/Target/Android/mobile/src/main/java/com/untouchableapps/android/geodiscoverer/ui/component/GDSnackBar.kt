//============================================================================
// Name        : GDSnackBar.kt
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

import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.shape.ZeroCornerSize
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.*
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Shape
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.input.VisualTransformation
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import com.untouchableapps.android.geodiscoverer.ui.theme.Material2Theme

// Missing component in material3
// Replace later when eventually available
@Composable
fun GDSnackBar(
  modifier: Modifier = Modifier,
  action: @Composable (() -> Unit)? = null,
  actionOnNewLine: Boolean = false,
  shape: Shape = androidx.compose.material.MaterialTheme.shapes.small,
  backgroundColor: Color = SnackbarDefaults.backgroundColor,
  contentColor: Color = androidx.compose.material.MaterialTheme.colors.surface,
  elevation: Dp = 6.dp,
  content: @Composable () -> Unit
) {
  Material2Theme {
    Snackbar(
      modifier = modifier,
      action = action,
      actionOnNewLine = actionOnNewLine,
      shape = shape,
      backgroundColor = backgroundColor,
      contentColor = contentColor,
      elevation = elevation,
      content = content
    )
  }
}
