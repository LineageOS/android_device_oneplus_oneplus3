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

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.SurfaceTexture;
import android.opengl.GLUtils;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL10;

public class FireflyRenderer {
    private static final String LOG_TAG = "NfcFireflyThread";

    static final int NUM_FIREFLIES = 200;

    static final float NEAR_CLIPPING_PLANE = 50f;
    static final float FAR_CLIPPING_PLANE = 100f;

    // All final variables below only need to be allocated once
    // and can be reused between subsequent Beams
    static final int[] sEglConfig = {
        EGL10.EGL_RED_SIZE, 8,
        EGL10.EGL_GREEN_SIZE, 8,
        EGL10.EGL_BLUE_SIZE, 8,
        EGL10.EGL_ALPHA_SIZE, 0,
        EGL10.EGL_DEPTH_SIZE, 0,
        EGL10.EGL_STENCIL_SIZE, 0,
        EGL10.EGL_NONE
    };

    // Vertices for drawing a 32x32 rect
    static final float mVertices[] = {
        0.0f,  0.0f, 0.0f,  // 0, Top Left
        0.0f,  32.0f, 0.0f, // 1, Bottom Left
        32.0f, 32.0f, 0.0f, // 2, Bottom Right
        32.0f, 0.0f, 0.0f,  // 3, Top Right
    };

    // Mapping coordinates for the texture
    static final float mTextCoords[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    // Connecting order (draws a square)
    static final short[] mIndices = { 0, 1, 2, 0, 2, 3 };

    final Context mContext;

    // Buffer holding the vertices
    final FloatBuffer mVertexBuffer;

    // Buffer holding the indices
    final ShortBuffer mIndexBuffer;

    // Buffer holding the texture mapping coordinates
    final FloatBuffer mTextureBuffer;

    final Firefly[] mFireflies;

    FireflyRenderThread mFireflyRenderThread;

    // The surface to render the flies on, including width and height
    SurfaceTexture mSurface;
    int mDisplayWidth;
    int mDisplayHeight;

    public FireflyRenderer(Context context) {
        mContext = context;

        // First, build the vertex, texture and index buffers
        ByteBuffer vbb = ByteBuffer.allocateDirect(mVertices.length * 4); // Float => 4 bytes
        vbb.order(ByteOrder.nativeOrder());
        mVertexBuffer = vbb.asFloatBuffer();
        mVertexBuffer.put(mVertices);
        mVertexBuffer.position(0);

        ByteBuffer ibb = ByteBuffer.allocateDirect(mIndices.length * 2); // Short => 2 bytes
        ibb.order(ByteOrder.nativeOrder());
        mIndexBuffer = ibb.asShortBuffer();
        mIndexBuffer.put(mIndices);
        mIndexBuffer.position(0);

        ByteBuffer tbb = ByteBuffer.allocateDirect(mTextCoords.length * 4);
        tbb.order(ByteOrder.nativeOrder());
        mTextureBuffer = tbb.asFloatBuffer();
        mTextureBuffer.put(mTextCoords);
        mTextureBuffer.position(0);

        mFireflies = new Firefly[NUM_FIREFLIES];
        for (int i = 0; i < NUM_FIREFLIES; i++) {
            mFireflies[i] = new Firefly();
        }
    }

    /**
     * Starts rendering fireflies on the given surface.
     * Must be called from the UI-thread.
     */
    public void start(SurfaceTexture surface, int width, int height) {
        mSurface = surface;
        mDisplayWidth = width;
        mDisplayHeight = height;

        mFireflyRenderThread = new FireflyRenderThread();
        mFireflyRenderThread.start();
    }

    /**
     * Stops rendering fireflies.
     * Must be called from the UI-thread.
     */
    public void stop() {
        if (mFireflyRenderThread != null) {
            mFireflyRenderThread.finish();
            try {
                mFireflyRenderThread.join();
            } catch (InterruptedException e) {
                Log.e(LOG_TAG, "Couldn't wait for FireflyRenderThread.");
            }
            mFireflyRenderThread = null;
        }
    }

    private class FireflyRenderThread extends Thread {
        EGL10 mEgl;
        EGLDisplay mEglDisplay;
        EGLConfig mEglConfig;
        EGLContext mEglContext;
        EGLSurface mEglSurface;
        GL10 mGL;

        // Holding the handle to the texture
        int mTextureId;

        // Read/written by multiple threads
        volatile boolean mFinished;

        @Override
        public void run() {
            if (!initGL()) {
                Log.e(LOG_TAG, "Failed to initialize OpenGL.");
                return;
            }
            loadStarTexture();

            mGL.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

            mGL.glViewport(0, 0, mDisplayWidth, mDisplayHeight);

            // make adjustments for screen ratio
            mGL.glMatrixMode(GL10.GL_PROJECTION);
           mGL.glLoadIdentity();
            mGL.glFrustumf(-mDisplayWidth, mDisplayWidth, mDisplayHeight, -mDisplayHeight, NEAR_CLIPPING_PLANE, FAR_CLIPPING_PLANE);

            // Switch back to modelview
            mGL.glMatrixMode(GL10.GL_MODELVIEW);
            mGL.glLoadIdentity();

            mGL.glHint(GL10.GL_PERSPECTIVE_CORRECTION_HINT, GL10.GL_NICEST);
            mGL.glDepthMask(true);


            for (Firefly firefly : mFireflies) {
                firefly.reset();
            }

            for (int i = 0; i < 3; i++) {
                // Call eglSwapBuffers 3 times - this will allocate the necessary
                // buffers, and make sure the animation looks smooth from the start.
                mGL.glClear(GL10.GL_COLOR_BUFFER_BIT);
                if (!mEgl.eglSwapBuffers(mEglDisplay, mEglSurface)) {
                    Log.e(LOG_TAG, "Could not swap buffers");
                    mFinished = true;
                }
            }

            long startTime = System.currentTimeMillis();

            while (!mFinished) {
                long timeElapsedMs = System.currentTimeMillis() - startTime;
                startTime = System.currentTimeMillis();

                checkCurrent();

                mGL.glClear(GL10.GL_COLOR_BUFFER_BIT);
                mGL.glLoadIdentity();

                mGL.glEnable(GL10.GL_TEXTURE_2D);
                mGL.glEnable(GL10.GL_BLEND);
                mGL.glBlendFunc(GL10.GL_SRC_ALPHA, GL10.GL_ONE);

                for (Firefly firefly : mFireflies) {
                    firefly.updatePositionAndScale(timeElapsedMs);
                    firefly.draw(mGL);
                }

                if (!mEgl.eglSwapBuffers(mEglDisplay, mEglSurface)) {
                    Log.e(LOG_TAG, "Could not swap buffers");
                    mFinished = true;
                }

                long elapsed = System.currentTimeMillis() - startTime;
                try {
                    Thread.sleep(Math.max(30 - elapsed, 0));
                } catch (InterruptedException e) {

                }
            }
            finishGL();
        }

        public void finish() {
            mFinished = true;
        }

        void loadStarTexture() {
            int[] textureIds = new int[1];
            mGL.glGenTextures(1, textureIds, 0);
            mTextureId = textureIds[0];

            InputStream in = null;
            try {
                // Remember that both texture dimensions must be a power of 2!
                in = mContext.getAssets().open("star.png");

                Bitmap bitmap = BitmapFactory.decodeStream(in);
                mGL.glBindTexture(GL10.GL_TEXTURE_2D, mTextureId);

                mGL.glTexParameterx(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_LINEAR);
                mGL.glTexParameterx(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);

                GLUtils.texImage2D(GL10.GL_TEXTURE_2D, 0, bitmap, 0);

                bitmap.recycle();

            } catch (IOException e) {
                Log.e(LOG_TAG, "IOException opening assets.");
            } finally {
                if (in != null) {
                    try {
                        in.close();
                    } catch (IOException e) { }
                }
            }
        }

        private void checkCurrent() {
            if (!mEglContext.equals(mEgl.eglGetCurrentContext()) ||
                    !mEglSurface.equals(mEgl.eglGetCurrentSurface(EGL10.EGL_DRAW))) {
                if (!mEgl.eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext)) {
                    throw new RuntimeException("eglMakeCurrent failed "
                            + GLUtils.getEGLErrorString(mEgl.eglGetError()));
                }
            }
        }

        boolean initGL() {
            // Initialize openGL engine
            mEgl = (EGL10) EGLContext.getEGL();

            mEglDisplay = mEgl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
            if (mEglDisplay == EGL10.EGL_NO_DISPLAY) {
                Log.e(LOG_TAG, "eglGetDisplay failed " +
                        GLUtils.getEGLErrorString(mEgl.eglGetError()));
                return false;
            }

            int[] version = new int[2];
            if (!mEgl.eglInitialize(mEglDisplay, version)) {
                Log.e(LOG_TAG, "eglInitialize failed " +
                        GLUtils.getEGLErrorString(mEgl.eglGetError()));
                return false;
            }

            mEglConfig = chooseEglConfig();
            if (mEglConfig == null) {
                Log.e(LOG_TAG, "eglConfig not initialized.");
                return false;
            }

            mEglContext = mEgl.eglCreateContext(mEglDisplay, mEglConfig, EGL10.EGL_NO_CONTEXT, null);

            mEglSurface = mEgl.eglCreateWindowSurface(mEglDisplay, mEglConfig, mSurface, null);

            if (mEglSurface == null || mEglSurface == EGL10.EGL_NO_SURFACE) {
                int error = mEgl.eglGetError();
                Log.e(LOG_TAG,"createWindowSurface returned error " + Integer.toString(error));
                return false;
            }

            if (!mEgl.eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext)) {
                Log.e(LOG_TAG, "eglMakeCurrent failed " +
                        GLUtils.getEGLErrorString(mEgl.eglGetError()));
                return false;
            }

            mGL = (GL10) mEglContext.getGL();

            return true;
        }

        private void finishGL() {
            if (mEgl == null || mEglDisplay == null) {
                // Nothing to free
                return;
            }
            // Unbind the current surface and context from the display
            mEgl.eglMakeCurrent(mEglDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE,
                    EGL10.EGL_NO_CONTEXT);

            if (mEglSurface != null) {
                mEgl.eglDestroySurface(mEglDisplay, mEglSurface);
            }

            if (mEglContext != null) {
                mEgl.eglDestroyContext(mEglDisplay, mEglContext);
            }
        }

        private EGLConfig chooseEglConfig() {
            int[] configsCount = new int[1];
            EGLConfig[] configs = new EGLConfig[1];
            if (!mEgl.eglChooseConfig(mEglDisplay, sEglConfig, configs, 1, configsCount)) {
                throw new IllegalArgumentException("eglChooseConfig failed " +
                        GLUtils.getEGLErrorString(mEgl.eglGetError()));
            } else if (configsCount[0] > 0) {
                return configs[0];
            }
            return null;
        }
    }

    private class Firefly {
        static final float TEXTURE_HEIGHT = 30f; // TODO use measurement of texture size
        static final float SPEED = .5f;

        float mX; // between -mDisplayHeight and mDisplayHeight
        float mY; // between -mDisplayWidth and mDisplayWidth
        float mZ; // between 0.0 (near) and 1.0 (far)
        float mZ0;
        float mT;
        float mScale;
        float mAlpha;

        public Firefly() {
        }

        void reset() {
            mX = (float) (Math.random() * mDisplayWidth) * 4 - 2 * mDisplayWidth;
            mY = (float) (Math.random() * mDisplayHeight) * 4 - 2 * mDisplayHeight;
            mZ0 = mZ = (float) (Math.random()) * 2 - 1;
            mT = 0f;
            mScale = 1.5f;
            mAlpha = 0f;
        }

        public void updatePositionAndScale(long timeElapsedMs) {
               mT += timeElapsedMs;
               mZ = mZ0 + mT/1000f * SPEED;
               mAlpha = 1f-mZ;
               if(mZ > 1.0) reset();
        }

        public void draw(GL10 gl) {
            gl.glLoadIdentity();

            // Counter clockwise winding
            gl.glFrontFace(GL10.GL_CCW);

            gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
            gl.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);

            gl.glVertexPointer(3, GL10.GL_FLOAT, 0, mVertexBuffer);
            gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, mTextureBuffer);

            gl.glTranslatef(mX, mY, -NEAR_CLIPPING_PLANE-mZ*(FAR_CLIPPING_PLANE-NEAR_CLIPPING_PLANE));
            gl.glColor4f(1, 1, 1, mAlpha);

            // scale around center
            gl.glTranslatef(TEXTURE_HEIGHT/2, TEXTURE_HEIGHT/2, 0);
            gl.glScalef(mScale, mScale, 0);
            gl.glTranslatef(-TEXTURE_HEIGHT/2, -TEXTURE_HEIGHT/2, 0);

            gl.glDrawElements(GL10.GL_TRIANGLES, mIndices.length, GL10.GL_UNSIGNED_SHORT,
                    mIndexBuffer);

            gl.glColor4f(1, 1, 1, 1);
            gl.glDisableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
            gl.glDisableClientState(GL10.GL_VERTEX_ARRAY);
        }
    }
}
