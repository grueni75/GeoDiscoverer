package net.margaritov.preference.colorpicker;

import android.app.Activity;
import android.graphics.PixelFormat;
import android.view.View;
import android.widget.LinearLayout;

import com.afollestad.materialdialogs.MaterialDialog;

public class ColorPickerDialog implements 
  View.OnClickListener,ColorPickerView.OnColorChangedListener {

  private MaterialDialog mDialog;
  private ColorPickerView mColorPicker;
  private ColorPickerPanelView mOldColor;
  private ColorPickerPanelView mNewColor;
  private OnColorChangedListener mListener;

  public interface OnColorChangedListener {
    public void onColorChanged(int color);
  }
  
  public ColorPickerDialog(Activity activity, int initialColor) {
    super();
    activity.getWindow().setFormat(PixelFormat.RGBA_8888);
    mDialog = new MaterialDialog.Builder(activity)
      .title(R.string.dialog_color_picker)
      .autoDismiss(false)
      .customView(R.layout.dialog_color_picker, false)
      .build();
    View layout = mDialog.getCustomView();
    mColorPicker = (ColorPickerView) layout.findViewById(R.id.color_picker_view);
    mOldColor = (ColorPickerPanelView) layout.findViewById(R.id.old_color_panel);
    mNewColor = (ColorPickerPanelView) layout.findViewById(R.id.new_color_panel);
    
    ((LinearLayout) mOldColor.getParent()).setPadding(
      Math.round(mColorPicker.getDrawingOffset()), 
      0, 
      Math.round(mColorPicker.getDrawingOffset()), 
      0
    );  
    
    mOldColor.setOnClickListener(this);
    mNewColor.setOnClickListener(this);
    mColorPicker.setOnColorChangedListener(this);
    mOldColor.setColor(initialColor);
    mColorPicker.setColor(initialColor, true);
  }
  
  /**
   * Set the title of the dialog for editing the color
   * @param title
   */
  public void setDialogTitle(String title) {
    mDialog.setTitle(title);
  }
  
  /**
   * Set a OnColorChangedListener to get notified when the color
   * selected by the user has changed.
   * @param listener
   */
  public void setOnColorChangedListener(OnColorChangedListener listener) {
    mListener = listener;
  }
  
  public void build(Activity activity) {
  }
  
  @Override
  public void onColorChanged(int color) {
    mNewColor.setColor(color);
    /*
    if (mListener != null) {
      mListener.onColorChanged(color);
    }
    */
  }

  public void setAlphaSliderVisible(boolean visible) {
    mColorPicker.setAlphaSliderVisible(visible);
  }
  
  public int getColor() {
    return mColorPicker.getColor();
  }

  @Override
  public void onClick(View v) {
    if (v.getId() == R.id.new_color_panel) {
      if (mListener != null) {
        mListener.onColorChanged(mNewColor.getColor());
      }
    }
    mDialog.dismiss();
  }
  
  public void show() {
    mDialog.show();
  }

}
