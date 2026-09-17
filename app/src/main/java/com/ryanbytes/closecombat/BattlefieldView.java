package com.ryanbytes.closecombat;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.View;

public final class BattlefieldView extends View {
    private static final int UNIT_STRIDE = 8;

    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final GestureDetector gestureDetector;
    private final ScaleGestureDetector scaleDetector;

    private float cameraX = 0f;
    private float cameraY = 0f;
    private float zoom = 0.75f;
    private long previousFrameNs = 0L;

    public BattlefieldView(Context context) {
        super(context);
        setFocusable(true);
        NativeEngine.reset();

        gestureDetector = new GestureDetector(context, new GestureDetector.SimpleOnGestureListener() {
            @Override
            public boolean onDown(MotionEvent e) {
                return true;
            }

            @Override
            public boolean onSingleTapUp(MotionEvent e) {
                float worldX = e.getX() / zoom + cameraX;
                float worldY = e.getY() / zoom + cameraY;
                NativeEngine.tap(worldX, worldY);
                performClick();
                return true;
            }

            @Override
            public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
                if (!scaleDetector.isInProgress()) {
                    cameraX += distanceX / zoom;
                    cameraY += distanceY / zoom;
                    clampCamera();
                    invalidate();
                }
                return true;
            }

            @Override
            public boolean onDoubleTap(MotionEvent e) {
                zoom = 0.75f;
                cameraX = 0f;
                cameraY = 0f;
                clampCamera();
                return true;
            }
        });

        scaleDetector = new ScaleGestureDetector(context, new ScaleGestureDetector.SimpleOnScaleGestureListener() {
            @Override
            public boolean onScale(ScaleGestureDetector detector) {
                float oldZoom = zoom;
                float focusWorldX = detector.getFocusX() / oldZoom + cameraX;
                float focusWorldY = detector.getFocusY() / oldZoom + cameraY;

                zoom = Math.max(0.40f, Math.min(2.75f, oldZoom * detector.getScaleFactor()));
                cameraX = focusWorldX - detector.getFocusX() / zoom;
                cameraY = focusWorldY - detector.getFocusY() / zoom;
                clampCamera();
                invalidate();
                return true;
            }
        });
    }

    @Override
    public boolean performClick() {
        super.performClick();
        return true;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        boolean scaled = scaleDetector.onTouchEvent(event);
        boolean gestured = gestureDetector.onTouchEvent(event);
        return scaled || gestured || true;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        long now = System.nanoTime();
        if (previousFrameNs != 0L) {
            float dt = (now - previousFrameNs) / 1_000_000_000f;
            NativeEngine.step(Math.min(dt, 0.05f));
        }
        previousFrameNs = now;

        drawBattlefield(canvas);
        postInvalidateOnAnimation();
    }

    private void drawBattlefield(Canvas canvas) {
        canvas.drawColor(Color.rgb(76, 87, 61));

        canvas.save();
        canvas.scale(zoom, zoom);
        canvas.translate(-cameraX, -cameraY);

        float worldW = NativeEngine.getWorldWidth();
        float worldH = NativeEngine.getWorldHeight();

        paint.setStyle(Paint.Style.FILL);
        paint.setColor(Color.rgb(104, 116, 79));
        canvas.drawRect(0f, 0f, worldW, worldH, paint);

        paint.setStrokeWidth(1f / zoom);
        paint.setColor(Color.argb(45, 20, 20, 20));
        for (float x = 0; x <= worldW; x += 100f) {
            canvas.drawLine(x, 0f, x, worldH, paint);
        }
        for (float y = 0; y <= worldH; y += 100f) {
            canvas.drawLine(0f, y, worldW, y, paint);
        }

        float[] obstacles = NativeEngine.getObstacles();
        paint.setColor(Color.rgb(92, 76, 57));
        for (int i = 0; i + 3 < obstacles.length; i += 4) {
            canvas.drawRect(
                    obstacles[i],
                    obstacles[i + 1],
                    obstacles[i] + obstacles[i + 2],
                    obstacles[i + 1] + obstacles[i + 3],
                    paint);
        }

        float[] units = NativeEngine.getUnits();
        for (int i = 0; i + UNIT_STRIDE - 1 < units.length; i += UNIT_STRIDE) {
            float x = units[i + 1];
            float y = units[i + 2];
            int side = (int) units[i + 3];
            boolean selected = units[i + 4] > 0.5f;
            float morale = units[i + 5];
            float suppression = units[i + 6];
            float health = units[i + 7];

            if (health <= 0f) {
                paint.setColor(Color.rgb(55, 50, 45));
                paint.setStrokeWidth(4f / zoom);
                canvas.drawLine(x - 10f, y - 10f, x + 10f, y + 10f, paint);
                canvas.drawLine(x - 10f, y + 10f, x + 10f, y - 10f, paint);
                continue;
            }

            paint.setStyle(Paint.Style.FILL);
            paint.setColor(side == 0 ? Color.rgb(63, 101, 153) : Color.rgb(155, 67, 60));
            canvas.drawCircle(x, y, 18f, paint);

            if (selected) {
                paint.setStyle(Paint.Style.STROKE);
                paint.setStrokeWidth(3f / zoom);
                paint.setColor(Color.WHITE);
                canvas.drawCircle(x, y, 25f, paint);
                paint.setStyle(Paint.Style.FILL);
            }

            float barW = 42f;
            paint.setColor(Color.rgb(30, 30, 30));
            canvas.drawRect(x - barW / 2f, y + 24f, x + barW / 2f, y + 29f, paint);
            paint.setColor(Color.rgb(80, 180, 80));
            canvas.drawRect(x - barW / 2f, y + 24f, x - barW / 2f + barW * health, y + 29f, paint);

            paint.setColor(Color.argb((int) (190f * suppression), 230, 190, 55));
            canvas.drawCircle(x, y, 23f, paint);

            paint.setTextSize(13f / zoom);
            paint.setColor(Color.WHITE);
            canvas.drawText(String.valueOf((int) (morale * 100f)), x - 10f, y - 25f, paint);
        }

        canvas.restore();

        paint.setStyle(Paint.Style.FILL);
        paint.setColor(Color.argb(180, 0, 0, 0));
        canvas.drawRoundRect(new RectF(14f, 14f, 530f, 82f), 10f, 10f, paint);
        paint.setColor(Color.WHITE);
        paint.setTextSize(18f);
        canvas.drawText("Tap blue unit to select • tap ground to move", 28f, 42f, paint);
        paint.setTextSize(15f);
        canvas.drawText("Drag: pan   Pinch: zoom   Double-tap: reset camera", 28f, 68f, paint);
    }

    private void clampCamera() {
        float visibleW = getWidth() > 0 ? getWidth() / zoom : 1f;
        float visibleH = getHeight() > 0 ? getHeight() / zoom : 1f;
        float maxX = Math.max(0f, NativeEngine.getWorldWidth() - visibleW);
        float maxY = Math.max(0f, NativeEngine.getWorldHeight() - visibleH);
        cameraX = Math.max(0f, Math.min(maxX, cameraX));
        cameraY = Math.max(0f, Math.min(maxY, cameraY));
    }
}
