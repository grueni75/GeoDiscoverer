package org.acra;

import static org.acra.ACRA.LOG_TAG;
import static org.acra.ReportField.USER_COMMENT;
import static org.acra.ReportField.USER_EMAIL;

import java.io.IOException;

import org.acra.collector.CrashReportData;
import org.acra.util.ToastSender;

import android.app.Dialog;
import android.app.NotificationManager;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.AppCompatEditText;
import android.text.InputType;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import com.afollestad.materialdialogs.DialogAction;
import com.afollestad.materialdialogs.MaterialDialog;

/**
 * This is the dialog Activity used by ACRA to get authorization from the user
 * to send reports. Requires android:launchMode="singleInstance" in your
 * AndroidManifest to work properly.
 **/
public class CrashReportDialog extends AppCompatActivity implements MaterialDialog.SingleButtonCallback, OnDismissListener {
    private static final String STATE_EMAIL = "email";
    private static final String STATE_COMMENT = "comment";
    private SharedPreferences prefs;
    private AppCompatEditText userComment;
    private AppCompatEditText userEmail;
    String mReportFileName;
    Dialog mDialog;
    static boolean dialogDone = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        boolean forceCancel = getIntent().getBooleanExtra(ACRAConstants.EXTRA_FORCE_CANCEL, false);
        if(forceCancel) {
            ACRA.log.d(ACRA.LOG_TAG, "Forced reports deletion.");
            cancelReports();
            finish();
            return;
        }

        mReportFileName = getIntent().getStringExtra(ACRAConstants.EXTRA_REPORT_FILE_NAME);
        Log.d(LOG_TAG, "Opening CrashReportDialog for " + mReportFileName);
        if (mReportFileName == null) {
            finish();
        }
        MaterialDialog.Builder dialogBuilder = new MaterialDialog.Builder(this);
        int resourceId = ACRA.getConfig().resDialogTitle();
        if(resourceId != 0) {
            dialogBuilder.title(resourceId);
        }
        resourceId = ACRA.getConfig().resDialogIcon();
        if(resourceId != 0) {
            dialogBuilder.icon(getResources().getDrawable(resourceId));
        }
        dialogBuilder.customView(buildCustomView(dialogBuilder,savedInstanceState),true);
        dialogBuilder.positiveText(android.R.string.ok);
        dialogBuilder.onPositive(CrashReportDialog.this);
        dialogBuilder.negativeText(android.R.string.cancel);
        dialogBuilder.onNegative(CrashReportDialog.this);
        cancelNotification();
        mDialog = dialogBuilder.build();
        mDialog.setCanceledOnTouchOutside(false);
        mDialog.setOnDismissListener(this);
        mDialog.show();
    }

    private View buildCustomView(MaterialDialog.Builder builder, Bundle savedInstanceState) {
        final float textSize = 16;
        final LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(
            getResources().getDimensionPixelSize(com.afollestad.materialdialogs.R.dimen.md_dialog_frame_margin),
            getResources().getDimensionPixelSize(com.afollestad.materialdialogs.R.dimen.md_content_padding_top),
            getResources().getDimensionPixelSize(com.afollestad.materialdialogs.R.dimen.md_dialog_frame_margin),
            getResources().getDimensionPixelSize(com.afollestad.materialdialogs.R.dimen.md_content_padding_bottom));
        root.setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
        root.setFocusable(true);
        root.setFocusableInTouchMode(true);

        final ScrollView scroll = new ScrollView(this);
        root.addView(scroll, new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT, 1.0f));
        final LinearLayout scrollable = new LinearLayout(this);
        scrollable.setOrientation(LinearLayout.VERTICAL);
        scroll.addView(scrollable);

        final TextView text = new TextView(this);
        final int dialogTextId = ACRA.getConfig().resDialogText();
        if (dialogTextId != 0) {
            text.setTextSize(textSize);
            text.setText(getText(dialogTextId));
        }
        scrollable.addView(text);

        // Add an optional prompt for user comments
        final int commentPromptId = ACRA.getConfig().resDialogCommentPrompt();
        if (commentPromptId != 0) {
            final TextView label = new TextView(this);
            label.setTextSize(textSize);
            label.setText(getText(commentPromptId));

            label.setPadding(label.getPaddingLeft(), 10, label.getPaddingRight(), label.getPaddingBottom());
            scrollable.addView(label, new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT,
                    LayoutParams.WRAP_CONTENT));

            userComment = new AppCompatEditText(this);

            //builder.formatEditText(userComment);
            userComment.setTextSize(textSize);
            userComment.setLines(3);
            userComment.setGravity(Gravity.TOP);
            if (savedInstanceState != null) {
                String savedValue = savedInstanceState.getString(STATE_COMMENT);
                if (savedValue != null) {
                    userComment.setText(savedValue);
                }
            }
            scrollable.addView(userComment);
        }

        // Add an optional user email field
        final int emailPromptId = ACRA.getConfig().resDialogEmailPrompt();
        if (emailPromptId != 0) {
            final TextView label = new TextView(this);
            label.setText(getText(emailPromptId));
            label.setTextSize(textSize);

            label.setPadding(label.getPaddingLeft(), 10, label.getPaddingRight(), label.getPaddingBottom());
            scrollable.addView(label);

            userEmail = new AppCompatEditText(this);

            //builder.formatEditText(userEmail);
            userEmail.setSingleLine();
            userEmail.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS);
            userEmail.setTextSize(textSize);
            prefs = getSharedPreferences(ACRA.getConfig().sharedPreferencesName(), ACRA.getConfig()
                    .sharedPreferencesMode());
            String savedValue = null;
            if (savedInstanceState != null) {
                savedValue = savedInstanceState.getString(STATE_EMAIL);
            }
            if (savedValue != null) {
                userEmail.setText(savedValue);
            } else {
                userEmail.setText(prefs.getString(ACRA.PREF_USER_EMAIL_ADDRESS, ""));
            }
            scrollable.addView(userEmail);
        }

        return root;
    }

    /**
     * Disable the notification in the Status Bar.
     */
    protected void cancelNotification() {
        final NotificationManager notificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        notificationManager.cancel(ACRAConstants.NOTIF_CRASH_ID);
    }

    @Override
    public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
        if (which == DialogAction.POSITIVE)
            sendCrash();
        else {
            cancelReports();
        }
        finish();
        ErrorReporter.dialogWaitEnded = true;

    }

    private void cancelReports() {
        ACRA.getErrorReporter().deletePendingNonApprovedReports(false);
    }

    private void sendCrash() {
        // Retrieve user comment
        final String comment = userComment != null ? userComment.getText().toString() : "";

        // Store the user email
        final String usrEmail;
        if (prefs != null && userEmail != null) {
            usrEmail = userEmail.getText().toString();
            final Editor prefEditor = prefs.edit();
            prefEditor.putString(ACRA.PREF_USER_EMAIL_ADDRESS, usrEmail);
            prefEditor.commit();
        } else {
            usrEmail = "";
        }

        final CrashReportPersister persister = new CrashReportPersister(getApplicationContext());
        try {
            Log.d(LOG_TAG, "Add user comment to " + mReportFileName);
            final CrashReportData crashData = persister.load(mReportFileName);
            crashData.put(USER_COMMENT, comment);
            crashData.put(USER_EMAIL, usrEmail);
            persister.store(crashData, mReportFileName);
        } catch (IOException e) {
            Log.w(LOG_TAG, "User comment not added: ", e);
        }

        // Start the report sending task
        Log.v(ACRA.LOG_TAG, "About to start SenderWorker from CrashReportDialog");
        ACRA.getErrorReporter().startSendingReports(false, true);

        // Optional Toast to thank the user
        final int toastId = ACRA.getConfig().resDialogOkToast();
        if (toastId != 0) {
            ToastSender.sendToast(getApplicationContext(), toastId, Toast.LENGTH_LONG);
        }
    }

    /*
     * (non-Javadoc)
     * 
     * @see android.app.Activity#onSaveInstanceState(android.os.Bundle)
     */
    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        if (userComment != null && userComment.getText() != null) {
            outState.putString(STATE_COMMENT, userComment.getText().toString());
        }
        if (userEmail != null && userEmail.getText() != null) {
            outState.putString(STATE_EMAIL, userEmail.getText().toString());
        }
    }

    public void onDismiss(DialogInterface dialog) {
        finish();
    }
}