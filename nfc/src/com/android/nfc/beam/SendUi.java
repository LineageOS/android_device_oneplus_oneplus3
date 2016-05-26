/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.nfc.beam;

import com.android.nfc.R;
import com.android.nfc.beam.FireflyRenderer;

import android.animation.Animator;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.animation.PropertyValuesHolder;
import android.animation.TimeAnimator;
import android.app.ActivityManager;
import android.app.StatusBarManager;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.PixelFormat;
import android.graphics.SurfaceTexture;
import android.os.AsyncTask;
import android.os.Binder;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.ActionMode;
import android.view.Display;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import com.android.internal.policy.PhoneWindow;
import android.view.SearchEvent;
import android.view.Surface;
import android.view.SurfaceControl;
import android.view.TextureView;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.view.accessibility.AccessibilityEvent;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.DecelerateInterpolator;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

/**
 * This class is responsible for handling the UI animation
 * around Android Beam. The animation consists of the following
 * animators:
 *
 * mPreAnimator: scales the screenshot down to INTERMEDIATE_SCALE
 * mSlowSendAnimator: scales the screenshot down to 0.2f (used as a "send in progress" animation)
 * mFastSendAnimator: quickly scales the screenshot down to 0.0f (used for send success)
 * mFadeInAnimator: fades the current activity back in (used after mFastSendAnimator completes)
 * mScaleUpAnimator: scales the screenshot back up to full screen (used for failure or receiving)
 * mHintAnimator: Slowly turns up the alpha of the "Touch to Beam" hint
 *
 * Possible sequences are:
 *
 * mPreAnimator => mSlowSendAnimator => mFastSendAnimator => mFadeInAnimator (send success)
 * mPreAnimator => mSlowSendAnimator => mScaleUpAnimator (send failure)
 * mPreAnimator => mScaleUpAnimator (p2p link broken, or data received)
 *
 * Note that mFastSendAnimator and mFadeInAnimator are combined in a set, as they
 * are an atomic animation that cannot be interrupted.
 *
 * All methods of this class must be called on the UI thread
 */
public class SendUi implements Animator.AnimatorListener, View.OnTouchListener,
        TimeAnimator.TimeListener, TextureView.SurfaceTextureListener, android.view.Window.Callback {
    static final String TAG = "SendUi";

    static final float INTERMEDIATE_SCALE = 0.6f;

    static final float[] PRE_SCREENSHOT_SCALE = {1.0f, INTERMEDIATE_SCALE};
    static final int PRE_DURATION_MS = 350;

    static final float[] SEND_SCREENSHOT_SCALE = {INTERMEDIATE_SCALE, 0.2f};
    static final int SLOW_SEND_DURATION_MS = 8000; // Stretch out sending over 8s
    static final int FAST_SEND_DURATION_MS = 350;

    static final float[] SCALE_UP_SCREENSHOT_SCALE = {INTERMEDIATE_SCALE, 1.0f};
    static final int SCALE_UP_DURATION_MS = 300;

    static final int FADE_IN_DURATION_MS = 250;
    static final int FADE_IN_START_DELAY_MS = 350;

    static final int SLIDE_OUT_DURATION_MS = 300;

    static final float[] BLACK_LAYER_ALPHA_DOWN_RANGE = {0.9f, 0.0f};
    static final float[] BLACK_LAYER_ALPHA_UP_RANGE = {0.0f, 0.9f};

    static final float[] TEXT_HINT_ALPHA_RANGE = {0.0f, 1.0f};
    static final int TEXT_HINT_ALPHA_DURATION_MS = 500;
    static final int TEXT_HINT_ALPHA_START_DELAY_MS = 300;

    public static final int FINISH_SCALE_UP = 0;
    public static final int FINISH_SEND_SUCCESS = 1;

    static final int STATE_IDLE = 0;
    static final int STATE_W4_SCREENSHOT = 1;
    static final int STATE_W4_SCREENSHOT_PRESEND_REQUESTED = 2;
    static final int STATE_W4_SCREENSHOT_PRESEND_NFC_TAP_REQUESTED = 3;
    static final int STATE_W4_SCREENSHOT_THEN_STOP = 4;
    static final int STATE_W4_PRESEND = 5;
    static final int STATE_W4_TOUCH = 6;
    static final int STATE_W4_NFC_TAP = 7;
    static final int STATE_SENDING = 8;
    static final int STATE_COMPLETE = 9;

    // all members are only used on UI thread
    final WindowManager mWindowManager;
    final Context mContext;
    final Display mDisplay;
    final DisplayMetrics mDisplayMetrics;
    final Matrix mDisplayMatrix;
    final WindowManager.LayoutParams mWindowLayoutParams;
    final LayoutInflater mLayoutInflater;
    final StatusBarManager mStatusBarManager;
    final View mScreenshotLayout;
    final ImageView mScreenshotView;
    final ImageView mBlackLayer;
    final TextureView mTextureView;
    final TextView mTextHint;
    final TextView mTextRetry;
    final Callback mCallback;

    // The mFrameCounter animation is purely used to count down a certain
    // number of (vsync'd) frames. This is needed because the first 3
    // times the animation internally calls eglSwapBuffers(), large buffers
    // are allocated by the graphics drivers. This causes the animation
    // to look janky. So on platforms where we can use hardware acceleration,
    // the animation order is:
    // Wait for hw surface => start frame counter => start pre-animation after 3 frames
    // For platforms where no hw acceleration can be used, the pre-animation
    // is started immediately.
    final TimeAnimator mFrameCounterAnimator;

    final ObjectAnimator mPreAnimator;
    final ObjectAnimator mSlowSendAnimator;
    final ObjectAnimator mFastSendAnimator;
    final ObjectAnimator mFadeInAnimator;
    final ObjectAnimator mHintAnimator;
    final ObjectAnimator mScaleUpAnimator;
    final ObjectAnimator mAlphaDownAnimator;
    final ObjectAnimator mAlphaUpAnimator;
    final AnimatorSet mSuccessAnimatorSet;

   // Besides animating the screenshot, the Beam UI also renders
    // fireflies on platforms where we can do hardware-acceleration.
    // Firefly rendering is only started once the initial
    // "pre-animation" has scaled down the screenshot, to avoid
    // that animation becoming janky. Likewise, the fireflies are
    // stopped in their tracks as soon as we finish the animation,
    // to make the finishing animation smooth.
    final boolean mHardwareAccelerated;
    final FireflyRenderer mFireflyRenderer;

    String mToastString;
    Bitmap mScreenshotBitmap;

    int mState;
    int mRenderedFrames;

    View mDecor;

    // Used for holding the surface
    SurfaceTexture mSurface;
    int mSurfaceWidth;
    int mSurfaceHeight;

    public interface Callback {
        public void onSendConfirmed();
        public void onCanceled();
    }

    public SendUi(Context context, Callback callback) {
        mContext = context;
        mCallback = callback;

        mDisplayMetrics = new DisplayMetrics();
        mDisplayMatrix = new Matrix();
        mWindowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        mStatusBarManager = (StatusBarManager) context.getSystemService(Context.STATUS_BAR_SERVICE);

        mDisplay = mWindowManager.getDefaultDisplay();

        mLayoutInflater = (LayoutInflater)
                context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mScreenshotLayout = mLayoutInflater.inflate(R.layout.screenshot, null);

        mScreenshotView = (ImageView) mScreenshotLayout.findViewById(R.id.screenshot);
        mScreenshotLayout.setFocusable(true);

        mTextHint = (TextView) mScreenshotLayout.findViewById(R.id.calltoaction);
        mTextRetry = (TextView) mScreenshotLayout.findViewById(R.id.retrytext);
        mBlackLayer = (ImageView) mScreenshotLayout.findViewById(R.id.blacklayer);
        mTextureView = (TextureView) mScreenshotLayout.findViewById(R.id.fireflies);
        mTextureView.setSurfaceTextureListener(this);

        // We're only allowed to use hardware acceleration if
        // isHighEndGfx() returns true - otherwise, we're too limited
        // on resources to do it.
        mHardwareAccelerated = ActivityManager.isHighEndGfx();
        int hwAccelerationFlags = mHardwareAccelerated ?
                WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED : 0;

        mWindowLayoutParams = new WindowManager.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT, 0, 0,
                WindowManager.LayoutParams.TYPE_SYSTEM_ALERT,
                WindowManager.LayoutParams.FLAG_FULLSCREEN
                | hwAccelerationFlags
                | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
                PixelFormat.OPAQUE);
        mWindowLayoutParams.privateFlags |=
                WindowManager.LayoutParams.PRIVATE_FLAG_SHOW_FOR_ALL_USERS;
        mWindowLayoutParams.token = new Binder();

        mFrameCounterAnimator = new TimeAnimator();
        mFrameCounterAnimator.setTimeListener(this);

        PropertyValuesHolder preX = PropertyValuesHolder.ofFloat("scaleX", PRE_SCREENSHOT_SCALE);
        PropertyValuesHolder preY = PropertyValuesHolder.ofFloat("scaleY", PRE_SCREENSHOT_SCALE);
        mPreAnimator = ObjectAnimator.ofPropertyValuesHolder(mScreenshotView, preX, preY);
        mPreAnimator.setInterpolator(new DecelerateInterpolator());
        mPreAnimator.setDuration(PRE_DURATION_MS);
        mPreAnimator.addListener(this);

        PropertyValuesHolder postX = PropertyValuesHolder.ofFloat("scaleX", SEND_SCREENSHOT_SCALE);
        PropertyValuesHolder postY = PropertyValuesHolder.ofFloat("scaleY", SEND_SCREENSHOT_SCALE);
        PropertyValuesHolder alphaDown = PropertyValuesHolder.ofFloat("alpha",
                new float[]{1.0f, 0.0f});

        mSlowSendAnimator = ObjectAnimator.ofPropertyValuesHolder(mScreenshotView, postX, postY);
        mSlowSendAnimator.setInterpolator(new DecelerateInterpolator());
        mSlowSendAnimator.setDuration(SLOW_SEND_DURATION_MS);

        mFastSendAnimator = ObjectAnimator.ofPropertyValuesHolder(mScreenshotView, postX,
                postY, alphaDown);
        mFastSendAnimator.setInterpolator(new DecelerateInterpolator());
        mFastSendAnimator.setDuration(FAST_SEND_DURATION_MS);
        mFastSendAnimator.addListener(this);

        PropertyValuesHolder scaleUpX = PropertyValuesHolder.ofFloat("scaleX", SCALE_UP_SCREENSHOT_SCALE);
        PropertyValuesHolder scaleUpY = PropertyValuesHolder.ofFloat("scaleY", SCALE_UP_SCREENSHOT_SCALE);

        mScaleUpAnimator = ObjectAnimator.ofPropertyValuesHolder(mScreenshotView, scaleUpX, scaleUpY);
        mScaleUpAnimator.setInterpolator(new DecelerateInterpolator());
        mScaleUpAnimator.setDuration(SCALE_UP_DURATION_MS);
        mScaleUpAnimator.addListener(this);

        PropertyValuesHolder fadeIn = PropertyValuesHolder.ofFloat("alpha", 1.0f);
        mFadeInAnimator = ObjectAnimator.ofPropertyValuesHolder(mScreenshotView, fadeIn);
        mFadeInAnimator.setInterpolator(new AccelerateDecelerateInterpolator());
        mFadeInAnimator.setDuration(FADE_IN_DURATION_MS);
        mFadeInAnimator.setStartDelay(FADE_IN_START_DELAY_MS);
        mFadeInAnimator.addListener(this);

        PropertyValuesHolder alphaUp = PropertyValuesHolder.ofFloat("alpha", TEXT_HINT_ALPHA_RANGE);
        mHintAnimator = ObjectAnimator.ofPropertyValuesHolder(mTextHint, alphaUp);
        mHintAnimator.setInterpolator(null);
        mHintAnimator.setDuration(TEXT_HINT_ALPHA_DURATION_MS);
        mHintAnimator.setStartDelay(TEXT_HINT_ALPHA_START_DELAY_MS);

        alphaDown = PropertyValuesHolder.ofFloat("alpha", BLACK_LAYER_ALPHA_DOWN_RANGE);
        mAlphaDownAnimator = ObjectAnimator.ofPropertyValuesHolder(mBlackLayer, alphaDown);
        mAlphaDownAnimator.setInterpolator(new DecelerateInterpolator());
        mAlphaDownAnimator.setDuration(400);

        alphaUp = PropertyValuesHolder.ofFloat("alpha", BLACK_LAYER_ALPHA_UP_RANGE);
        mAlphaUpAnimator = ObjectAnimator.ofPropertyValuesHolder(mBlackLayer, alphaUp);
        mAlphaUpAnimator.setInterpolator(new DecelerateInterpolator());
        mAlphaUpAnimator.setDuration(200);

        mSuccessAnimatorSet = new AnimatorSet();
        mSuccessAnimatorSet.playSequentially(mFastSendAnimator, mFadeInAnimator);

        // Create a Window with a Decor view; creating a window allows us to get callbacks
        // on key events (which require a decor view to be dispatched).
        mContext.setTheme(android.R.style.Theme_Black_NoTitleBar_Fullscreen);
        Window window = new PhoneWindow(mContext);
        window.setCallback(this);
        window.requestFeature(Window.FEATURE_NO_TITLE);
        mDecor = window.getDecorView();
        window.setContentView(mScreenshotLayout, mWindowLayoutParams);

        if (mHardwareAccelerated) {
            mFireflyRenderer = new FireflyRenderer(context);
        } else {
            mFireflyRenderer = null;
        }
        mState = STATE_IDLE;
    }

    public void takeScreenshot() {
        // There's no point in taking the screenshot if
        // we're still finishing the previous animation.
        if (mState >= STATE_W4_TOUCH) {
            return;
        }
        mState = STATE_W4_SCREENSHOT;
        new ScreenshotTask().execute();
    }

    /** Show pre-send animation */
    public void showPreSend(boolean promptToNfcTap) {
        switch (mState) {
            case STATE_IDLE:
                Log.e(TAG, "Unexpected showPreSend() in STATE_IDLE");
                return;
            case STATE_W4_SCREENSHOT:
                // Still waiting for screenshot, store request in state
                // and wait for screenshot completion.
                if (promptToNfcTap) {
                    mState = STATE_W4_SCREENSHOT_PRESEND_NFC_TAP_REQUESTED;
                } else {
                    mState = STATE_W4_SCREENSHOT_PRESEND_REQUESTED;
                }
                return;
            case STATE_W4_SCREENSHOT_PRESEND_REQUESTED:
            case STATE_W4_SCREENSHOT_PRESEND_NFC_TAP_REQUESTED:
                Log.e(TAG, "Unexpected showPreSend() in STATE_W4_SCREENSHOT_PRESEND_REQUESTED");
                return;
            case STATE_W4_PRESEND:
                // Expected path, continue below
                break;
            default:
                Log.e(TAG, "Unexpected showPreSend() in state " + Integer.toString(mState));
                return;
        }
        // Update display metrics
        mDisplay.getRealMetrics(mDisplayMetrics);

        final int statusBarHeight = mContext.getResources().getDimensionPixelSize(
                                        com.android.internal.R.dimen.status_bar_height);

        mBlackLayer.setVisibility(View.GONE);
        mBlackLayer.setAlpha(0f);
        mScreenshotLayout.setOnTouchListener(this);
        mScreenshotView.setImageBitmap(mScreenshotBitmap);
        mScreenshotView.setTranslationX(0f);
        mScreenshotView.setAlpha(1.0f);
        mScreenshotView.setPadding(0, statusBarHeight, 0, 0);

        mScreenshotLayout.requestFocus();

        if (promptToNfcTap) {
            mTextHint.setText(mContext.getResources().getString(R.string.ask_nfc_tap));
        } else {
            mTextHint.setText(mContext.getResources().getString(R.string.touch));
        }
        mTextHint.setAlpha(0.0f);
        mTextHint.setVisibility(View.VISIBLE);
        mHintAnimator.start();

        // Lock the orientation.
        // The orientation from the configuration does not specify whether
        // the orientation is reverse or not (ie landscape or reverse landscape).
        // So we have to use SENSOR_LANDSCAPE or SENSOR_PORTRAIT to make sure
        // we lock in portrait / landscape and have the sensor determine
        // which way is up.
        int orientation = mContext.getResources().getConfiguration().orientation;

        switch (orientation) {
            case Configuration.ORIENTATION_LANDSCAPE:
                mWindowLayoutParams.screenOrientation =
                        ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE;
                break;
            case Configuration.ORIENTATION_PORTRAIT:
                mWindowLayoutParams.screenOrientation =
                        ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT;
                break;
            default:
                mWindowLayoutParams.screenOrientation =
                        ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT;
                break;
        }

        mWindowManager.addView(mDecor, mWindowLayoutParams);
        // Disable statusbar pull-down
        mStatusBarManager.disable(StatusBarManager.DISABLE_EXPAND);

        mToastString = null;

        if (!mHardwareAccelerated) {
            mPreAnimator.start();
        } // else, we will start the animation once we get the hardware surface
        mState = promptToNfcTap ? STATE_W4_NFC_TAP : STATE_W4_TOUCH;
    }

    /** Show starting send animation */
    public void showStartSend() {
        if (mState < STATE_SENDING) return;

        mTextRetry.setVisibility(View.GONE);
        // Update the starting scale - touchscreen-mashers may trigger
        // this before the pre-animation completes.
        float currentScale = mScreenshotView.getScaleX();
        PropertyValuesHolder postX = PropertyValuesHolder.ofFloat("scaleX",
                new float[] {currentScale, 0.0f});
        PropertyValuesHolder postY = PropertyValuesHolder.ofFloat("scaleY",
                new float[] {currentScale, 0.0f});

        mSlowSendAnimator.setValues(postX, postY);

        float currentAlpha = mBlackLayer.getAlpha();
        if (mBlackLayer.isShown() && currentAlpha > 0.0f) {
           PropertyValuesHolder alphaDown = PropertyValuesHolder.ofFloat("alpha",
                    new float[] {currentAlpha, 0.0f});
            mAlphaDownAnimator.setValues(alphaDown);
            mAlphaDownAnimator.start();
        }
        mSlowSendAnimator.start();
    }

    public void finishAndToast(int finishMode, String toast) {
        mToastString = toast;

        finish(finishMode);
    }

    /** Return to initial state */
    public void finish(int finishMode) {
        switch (mState) {
            case STATE_IDLE:
                return;
            case STATE_W4_SCREENSHOT:
            case STATE_W4_SCREENSHOT_PRESEND_REQUESTED:
            case STATE_W4_SCREENSHOT_PRESEND_NFC_TAP_REQUESTED:
                // Screenshot is still being captured on a separate thread.
                // Update state, and stop everything when the capture is done.
                mState = STATE_W4_SCREENSHOT_THEN_STOP;
                return;
            case STATE_W4_SCREENSHOT_THEN_STOP:
                Log.e(TAG, "Unexpected call to finish() in STATE_W4_SCREENSHOT_THEN_STOP");
                return;
            case STATE_W4_PRESEND:
                // We didn't build up any animation state yet, but
                // did store the bitmap. Clear out the bitmap, reset
                // state and bail.
                mScreenshotBitmap = null;
                mState = STATE_IDLE;
                return;
            default:
                // We've started animations and attached a view; tear stuff down below.
                break;
        }

        // Stop rendering the fireflies
        if (mFireflyRenderer != null) {
            mFireflyRenderer.stop();
        }

        mTextHint.setVisibility(View.GONE);
        mTextRetry.setVisibility(View.GONE);

        float currentScale = mScreenshotView.getScaleX();
        float currentAlpha = mScreenshotView.getAlpha();

        if (finishMode == FINISH_SCALE_UP) {
            mBlackLayer.setVisibility(View.GONE);
            PropertyValuesHolder scaleUpX = PropertyValuesHolder.ofFloat("scaleX",
                    new float[] {currentScale, 1.0f});
            PropertyValuesHolder scaleUpY = PropertyValuesHolder.ofFloat("scaleY",
                    new float[] {currentScale, 1.0f});
            PropertyValuesHolder scaleUpAlpha = PropertyValuesHolder.ofFloat("alpha",
                    new float[] {currentAlpha, 1.0f});
            mScaleUpAnimator.setValues(scaleUpX, scaleUpY, scaleUpAlpha);

            mScaleUpAnimator.start();
        } else if (finishMode == FINISH_SEND_SUCCESS){
            // Modify the fast send parameters to match the current scale
            PropertyValuesHolder postX = PropertyValuesHolder.ofFloat("scaleX",
                    new float[] {currentScale, 0.0f});
            PropertyValuesHolder postY = PropertyValuesHolder.ofFloat("scaleY",
                    new float[] {currentScale, 0.0f});
            PropertyValuesHolder alpha = PropertyValuesHolder.ofFloat("alpha",
                    new float[] {currentAlpha, 0.0f});
            mFastSendAnimator.setValues(postX, postY, alpha);

            // Reset the fadeIn parameters to start from alpha 1
            PropertyValuesHolder fadeIn = PropertyValuesHolder.ofFloat("alpha",
                    new float[] {0.0f, 1.0f});
            mFadeInAnimator.setValues(fadeIn);

            mSlowSendAnimator.cancel();
            mSuccessAnimatorSet.start();
        }
        mState = STATE_COMPLETE;
    }

    void dismiss() {
        if (mState < STATE_W4_TOUCH) return;
        // Immediately set to IDLE, to prevent .cancel() calls
        // below from immediately calling into dismiss() again.
        // (They can do so on the same thread).
        mState = STATE_IDLE;
        mSurface = null;
        mFrameCounterAnimator.cancel();
        mPreAnimator.cancel();
        mSlowSendAnimator.cancel();
        mFastSendAnimator.cancel();
        mSuccessAnimatorSet.cancel();
        mScaleUpAnimator.cancel();
        mAlphaUpAnimator.cancel();
        mAlphaDownAnimator.cancel();
        mWindowManager.removeView(mDecor);
        mStatusBarManager.disable(StatusBarManager.DISABLE_NONE);
        mScreenshotBitmap = null;
        if (mToastString != null) {
            Toast.makeText(mContext, mToastString, Toast.LENGTH_LONG).show();
        }
        mToastString = null;
    }

    /**
     * @return the current display rotation in degrees
     */
    static float getDegreesForRotation(int value) {
        switch (value) {
        case Surface.ROTATION_90:
            return 90f;
        case Surface.ROTATION_180:
            return 180f;
        case Surface.ROTATION_270:
            return 270f;
        }
        return 0f;
    }

    final class ScreenshotTask extends AsyncTask<Void, Void, Bitmap> {
        @Override
        protected Bitmap doInBackground(Void... params) {
            return createScreenshot();
        }

        @Override
        protected void onPostExecute(Bitmap result) {
            if (mState == STATE_W4_SCREENSHOT) {
                // Screenshot done, wait for request to start preSend anim
                mScreenshotBitmap = result;
                mState = STATE_W4_PRESEND;
            } else if (mState == STATE_W4_SCREENSHOT_THEN_STOP) {
                // We were asked to finish, move to idle state and exit
                mState = STATE_IDLE;
            } else if (mState == STATE_W4_SCREENSHOT_PRESEND_REQUESTED ||
                    mState == STATE_W4_SCREENSHOT_PRESEND_NFC_TAP_REQUESTED) {
                if (result != null) {
                    mScreenshotBitmap = result;
                    boolean requestTap = (mState == STATE_W4_SCREENSHOT_PRESEND_NFC_TAP_REQUESTED);
                    mState = STATE_W4_PRESEND;
                    showPreSend(requestTap);
                } else {
                    // Failed to take screenshot; reset state to idle
                    // and don't do anything
                    Log.e(TAG, "Failed to create screenshot");
                    mState = STATE_IDLE;
                }
            } else {
                Log.e(TAG, "Invalid state on screenshot completion: " + Integer.toString(mState));
            }
        }
    };

   /**
     * Returns a screenshot of the current display contents.
     */
    Bitmap createScreenshot() {
        // We need to orient the screenshot correctly (and the Surface api seems to
        // take screenshots only in the natural orientation of the device :!)
        mDisplay.getRealMetrics(mDisplayMetrics);
        boolean hasNavBar =  mContext.getResources().getBoolean(
                com.android.internal.R.bool.config_showNavigationBar);

        float[] dims = {mDisplayMetrics.widthPixels, mDisplayMetrics.heightPixels};
        float degrees = getDegreesForRotation(mDisplay.getRotation());
        final int statusBarHeight = mContext.getResources().getDimensionPixelSize(
                                        com.android.internal.R.dimen.status_bar_height);

        // Navbar has different sizes, depending on orientation
        final int navBarHeight = hasNavBar ? mContext.getResources().getDimensionPixelSize(
                                        com.android.internal.R.dimen.navigation_bar_height) : 0;
        final int navBarHeightLandscape = hasNavBar ? mContext.getResources().getDimensionPixelSize(
                                        com.android.internal.R.dimen.navigation_bar_height_landscape) : 0;

        final int navBarWidth = hasNavBar ? mContext.getResources().getDimensionPixelSize(
                                        com.android.internal.R.dimen.navigation_bar_width) : 0;

        boolean requiresRotation = (degrees > 0);
        if (requiresRotation) {
            // Get the dimensions of the device in its native orientation
            mDisplayMatrix.reset();
            mDisplayMatrix.preRotate(-degrees);
           mDisplayMatrix.mapPoints(dims);
            dims[0] = Math.abs(dims[0]);
            dims[1] = Math.abs(dims[1]);
        }

        Bitmap bitmap = SurfaceControl.screenshot((int) dims[0], (int) dims[1]);
        // Bail if we couldn't take the screenshot
        if (bitmap == null) {
            return null;
        }
        if (requiresRotation) {
            // Rotate the screenshot to the current orientation
            Bitmap ss = Bitmap.createBitmap(mDisplayMetrics.widthPixels,
                    mDisplayMetrics.heightPixels, Bitmap.Config.ARGB_8888);
            Canvas c = new Canvas(ss);
            c.translate(ss.getWidth() / 2, ss.getHeight() / 2);
            c.rotate(360f - degrees);
            c.translate(-dims[0] / 2, -dims[1] / 2);
            c.drawBitmap(bitmap, 0, 0, null);

            bitmap = ss;
        }

        // TODO this is somewhat device-specific; need generic solution.
        // Crop off the status bar and the nav bar
        // Portrait: 0, statusBarHeight, width, height - status - nav
        // Landscape: 0, statusBarHeight, width - navBar, height - status
        int newLeft = 0;
        int newTop = statusBarHeight;
        int newWidth = bitmap.getWidth();
        int newHeight = bitmap.getHeight();
        float smallestWidth = (float)Math.min(newWidth, newHeight);
        float smallestWidthDp = smallestWidth / (mDisplayMetrics.densityDpi / 160f);
        if (bitmap.getWidth() < bitmap.getHeight()) {
            // Portrait mode: status bar is at the top, navbar bottom, width unchanged
            newHeight = bitmap.getHeight() - statusBarHeight - navBarHeight;
        } else {
            // Landscape mode: status bar is at the top
            // Navbar: bottom on >599dp width devices, otherwise to the side
            if (smallestWidthDp > 599) {
                newHeight = bitmap.getHeight() - statusBarHeight - navBarHeightLandscape;
            } else {
                newHeight = bitmap.getHeight() - statusBarHeight;
                newWidth = bitmap.getWidth() - navBarWidth;
            }
        }
        bitmap = Bitmap.createBitmap(bitmap, newLeft, newTop, newWidth, newHeight);

        return bitmap;
    }

    @Override
    public void onAnimationStart(Animator animation) {  }

    @Override
    public void onAnimationEnd(Animator animation) {
        if (animation == mScaleUpAnimator || animation == mSuccessAnimatorSet ||
            animation == mFadeInAnimator) {
            // These all indicate the end of the animation
            dismiss();
        } else if (animation == mFastSendAnimator) {
            // After sending is done and we've faded out, reset the scale to 1
            // so we can fade it back in.
            mScreenshotView.setScaleX(1.0f);
            mScreenshotView.setScaleY(1.0f);
        } else if (animation == mPreAnimator) {
            if (mHardwareAccelerated && (mState == STATE_W4_TOUCH || mState == STATE_W4_NFC_TAP)) {
                mFireflyRenderer.start(mSurface, mSurfaceWidth, mSurfaceHeight);
            }
        }
    }

    @Override
    public void onAnimationCancel(Animator animation) {  }

    @Override
    public void onAnimationRepeat(Animator animation) {  }

    @Override
    public void onTimeUpdate(TimeAnimator animation, long totalTime, long deltaTime) {
        // This gets called on animation vsync
        if (++mRenderedFrames < 4) {
            // For the first 3 frames, call invalidate(); this calls eglSwapBuffers
            // on the surface, which will allocate large buffers the first three calls
            // as Android uses triple buffering.
            mScreenshotLayout.invalidate();
        } else {
            // Buffers should be allocated, start the real animation
            mFrameCounterAnimator.cancel();
            mPreAnimator.start();
        }
    }

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        if (mState != STATE_W4_TOUCH) {
            return false;
        }
        mState = STATE_SENDING;
        // Ignore future touches
        mScreenshotView.setOnTouchListener(null);

        // Cancel any ongoing animations
        mFrameCounterAnimator.cancel();
        mPreAnimator.cancel();

        mCallback.onSendConfirmed();
        return true;
    }

    @Override
    public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
        if (mHardwareAccelerated && mState < STATE_COMPLETE) {
            mRenderedFrames = 0;

            mFrameCounterAnimator.start();
            mSurface = surface;
            mSurfaceWidth = width;
            mSurfaceHeight = height;
        }
    }

    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
        // Since we've disabled orientation changes, we can safely ignore this
    }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
        mSurface = null;

        return true;
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surface) { }

    public void showSendHint() {
        if (mAlphaDownAnimator.isRunning()) {
           mAlphaDownAnimator.cancel();
        }
        if (mSlowSendAnimator.isRunning()) {
            mSlowSendAnimator.cancel();
        }
        mBlackLayer.setScaleX(mScreenshotView.getScaleX());
        mBlackLayer.setScaleY(mScreenshotView.getScaleY());
        mBlackLayer.setVisibility(View.VISIBLE);
        mTextHint.setVisibility(View.GONE);

        mTextRetry.setText(mContext.getResources().getString(R.string.beam_try_again));
        mTextRetry.setVisibility(View.VISIBLE);

        PropertyValuesHolder alphaUp = PropertyValuesHolder.ofFloat("alpha",
                new float[] {mBlackLayer.getAlpha(), 0.9f});
        mAlphaUpAnimator.setValues(alphaUp);
        mAlphaUpAnimator.start();
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        int keyCode = event.getKeyCode();
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            mCallback.onCanceled();
            return true;
        } else if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN ||
                keyCode == KeyEvent.KEYCODE_VOLUME_UP) {
            // Treat as if it's a touch event
            return onTouch(mScreenshotView, null);
        } else {
            return false;
        }
    }

    @Override
    public boolean dispatchKeyShortcutEvent(KeyEvent event) {
        return false;
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        return mScreenshotLayout.dispatchTouchEvent(event);
    }

    @Override
    public boolean dispatchTrackballEvent(MotionEvent event) {
        return false;
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent event) {
        return false;
    }

    @Override
    public boolean dispatchPopulateAccessibilityEvent(AccessibilityEvent event) {
        return false;
    }

    @Override
    public View onCreatePanelView(int featureId) {
        return null;
    }

    @Override
    public boolean onCreatePanelMenu(int featureId, Menu menu) {
        return false;
    }

    @Override
    public boolean onPreparePanel(int featureId, View view, Menu menu) {
        return false;
    }

    @Override
    public boolean onMenuOpened(int featureId, Menu menu) {
        return false;
    }
    @Override
    public boolean onMenuItemSelected(int featureId, MenuItem item) {
        return false;
    }

    @Override
    public void onWindowAttributesChanged(LayoutParams attrs) {
    }

    @Override
    public void onContentChanged() {
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
    }

    @Override
    public void onAttachedToWindow() {

    }

    @Override
    public void onDetachedFromWindow() {
    }

    @Override
    public void onPanelClosed(int featureId, Menu menu) {

    }
    @Override
    public boolean onSearchRequested(SearchEvent searchEvent) {
         return onSearchRequested();
    }

    @Override
    public boolean onSearchRequested() {
        return false;
    }

    @Override
    public ActionMode onWindowStartingActionMode(
            android.view.ActionMode.Callback callback) {
        return null;
    }

    public ActionMode onWindowStartingActionMode(
            android.view.ActionMode.Callback callback, int type) {
        return null;
    }

    @Override
    public void onActionModeStarted(ActionMode mode) {
    }

    @Override
    public void onActionModeFinished(ActionMode mode) {
    }
}
