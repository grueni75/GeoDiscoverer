//============================================================================
// Name        : GDSwipeToDismiss.kt
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2022 Matthias Gruenewald
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

import androidx.compose.foundation.layout.RowScope
import androidx.compose.material.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import com.untouchableapps.android.geodiscoverer.ui.theme.Material2Theme

// Missing component in material3
// Replace later when eventually available
@ExperimentalMaterialApi
@Composable
fun GDSwipeToDismiss(
  state: DismissState,
  modifier: Modifier = Modifier,
  directions: Set<DismissDirection> = setOf(
    DismissDirection.EndToStart,
    DismissDirection.StartToEnd
  ),
  dismissThresholds: (DismissDirection) -> ThresholdConfig = { FractionalThreshold(0.5f) },
  background: @Composable RowScope.() -> Unit,
  dismissContent: @Composable RowScope.() -> Unit
) {
  SwipeToDismiss(
    state=state,
    modifier=modifier,
    directions=directions,
    dismissThresholds=dismissThresholds,
    background=background,
    dismissContent=dismissContent
  )
}
