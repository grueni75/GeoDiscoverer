//============================================================================
// Name        : GDAddressHistoryAdapter.java
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2016 Matthias Gruenewald
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

package com.untouchableapps.android.geodiscoverer;

import android.content.res.ColorStateList;
import android.opengl.Visibility;
import android.support.v4.graphics.drawable.DrawableCompat;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.TextView;

import com.afollestad.materialdialogs.MaterialDialog;
import com.untouchableapps.android.geodiscoverer.core.GDCore;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedList;

public class GDAddressHistoryAdapter extends ArrayAdapter<String> {
  private final ViewMap viewMap;
  private final MaterialDialog dialog;
  private final MaterialDialog.Builder builder;
  private GDCore coreObject;
  private ColorStateList itemIconTint;

  static class ViewHolder {
    public TextView text;
    public EditText editText;
    public ImageButton removeButton;
    public ImageButton renameButton;
    public ImageButton clearButton;
    public ImageButton confirmButton;
  }

  public GDAddressHistoryAdapter(ViewMap viewMap, MaterialDialog.Builder builder, MaterialDialog dialog) {
    super(viewMap, R.layout.dialog_address_history_entry);

    coreObject=GDApplication.coreObject;
    String temp[] = coreObject.configStoreGetAttributeValues("Navigation/AddressPoint","name");
    LinkedList<String> unsortedNames = new LinkedList<String>(Arrays.asList(temp));
    GDApplication.addMessage(GDApplication.WARNING_MSG,"GDApp","sort array according to timestamp");
    while (unsortedNames.size()>0) {
      String newestName="";
      long newestTimestamp = 0;
      for (String name : unsortedNames) {
        String path = "Navigation/AddressPoint[@name='" + name + "']";
        long t = Long.parseLong(coreObject.configStoreGetStringValue(path, "timestamp"));
        if (t > newestTimestamp) {
          newestTimestamp = t;
          newestName = name;
        }
      }
      add(newestName);
      unsortedNames.remove(newestName);
    }
    this.viewMap = viewMap;
    this.dialog = dialog;
    this.builder = builder;
    this.itemIconTint = viewMap.navDrawerList.getItemIconTintList();
  }

  @Override
  public View getView(final int position, View convertView, ViewGroup parent) {
    View rowView = convertView;

    // Reuse views
    if (rowView == null) {
      LayoutInflater inflater = viewMap.getLayoutInflater();
      rowView = inflater.inflate(R.layout.dialog_address_history_entry, null);
      ViewHolder viewHolder = new ViewHolder();
      viewHolder.text = (TextView) rowView.findViewById(R.id.dialog_address_history_entry_text);
      viewHolder.editText = (EditText) rowView.findViewById(R.id.dialog_address_history_entry_edit_text);
      //builder.formatEditText(viewHolder.editText);
      viewHolder.editText.setImeOptions(EditorInfo.IME_ACTION_DONE);
      viewHolder.removeButton = (ImageButton) rowView.findViewById(R.id.dialog_address_history_entry_remove_button);
      DrawableCompat.setTintList(viewHolder.removeButton.getDrawable(),itemIconTint);
      viewHolder.renameButton = (ImageButton) rowView.findViewById(R.id.dialog_address_history_entry_rename_button);
      DrawableCompat.setTintList(viewHolder.renameButton.getDrawable(),itemIconTint);
      viewHolder.confirmButton = (ImageButton) rowView.findViewById(R.id.dialog_address_history_entry_confirm_button);
      DrawableCompat.setTintList(viewHolder.confirmButton.getDrawable(),itemIconTint);
      viewHolder.clearButton = (ImageButton) rowView.findViewById(R.id.dialog_address_history_entry_clear_button);
      DrawableCompat.setTintList(viewHolder.clearButton.getDrawable(),itemIconTint);
      rowView.setTag(viewHolder);
    }

    // Fill data
    final ViewHolder holder = (ViewHolder) rowView.getTag();
    final String name = getItem(position);
    holder.text.setText(name);
    holder.removeButton.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        coreObject.executeCoreCommand("removeAddressPoint(\"" + name + "\")");
        remove(name);
      }
    });
    holder.text.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        coreObject.scheduleCoreCommand("setTargetAtAddressPoint(\"" + name + "\")");
        dialog.dismiss();
      }
    });
    holder.renameButton.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        holder.editText.setVisibility(View.VISIBLE);
        holder.confirmButton.setVisibility(View.VISIBLE);
        holder.clearButton.setVisibility(View.VISIBLE);
        holder.text.setVisibility(View.GONE);
        holder.removeButton.setVisibility(View.GONE);
        holder.renameButton.setVisibility(View.GONE);
        holder.editText.setText(name);
      }
    });
    holder.clearButton.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        holder.editText.setText("");
      }
    });
    final View.OnClickListener confirmOnClickListener = new View.OnClickListener() {
      @Override
      public void onClick(View v) {

        // Skip if edit text is empty
        String newName = holder.editText.getText().toString();
        if ((!newName.equals(""))&&(!newName.equals(name))) {

          // Rename the entry
          coreObject.executeCoreCommand("renameAddressPoint(\"" + name + "\"," +
              "\"" + newName + "\")");
          remove(name);
          insert(newName, position);
          holder.text.setText(newName);

          // Remove any duplicates
          ArrayList<String> duplicates = new ArrayList<String>();
          for (int i = 0; i < getCount(); i++) {
            String otherName = getItem(i);
            if ((otherName != newName) && (otherName.equals(newName))) {
              duplicates.add(otherName);
            }
          }
          for (String otherName : duplicates)
            remove(otherName);

        }

        // Make the original views visible again
        holder.editText.setVisibility(View.GONE);
        holder.confirmButton.setVisibility(View.GONE);
        holder.clearButton.setVisibility(View.GONE);
        holder.text.setVisibility(View.VISIBLE);
        holder.removeButton.setVisibility(View.VISIBLE);
        holder.renameButton.setVisibility(View.VISIBLE);
      }
    };
    holder.confirmButton.setOnClickListener(confirmOnClickListener);
    holder.editText.setOnEditorActionListener(new EditText.OnEditorActionListener() {
      @Override
      public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
        if (actionId == EditorInfo.IME_ACTION_DONE) {
          confirmOnClickListener.onClick(null);
          return true;
        }
        return false;
      }
    });


    return rowView;
  }
}
