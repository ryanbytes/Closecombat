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
    private static final int UNIT_STRIDE = 9;
    private static final float COMMAND_BAR_HEIGHT = 76f;

    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final GestureDetector gestureDetector;
    private final ScaleGestureDetector scaleDetector;

    private float cameraX = 0f;
    private float cameraY = 0f;
    private float zoom = 0.75f;
    private long previousFrameNs = 0L;
    private int activeMoveMode = 0;

    public BattlefieldView(Context context) {
        super(context);
        setFocusable(true);
        NativeEngine.reset();
        NativeEngine.setMoveMode(activeMoveMode);

        gestureDetector = new GestureDetector(context, new GestureDetector.SimpleOnGestureListener() {
            @Override
            public boolean onDown(MotionEvent e) {
                return true;
            }

            @Override
            public boolean onSingleTapUp(MotionEvent e) {
                if (handleCommandBarTap(e.getX(), e.getY())) {
                    performClick();
                    invalidate();
                    return true;
                }

                float worldX = e.getX() / zoom + cameraX;
                float worldY = e.getY() / zoom + cameraY;
                NativeEngine.tap(worldX, worldY);
                performClick();
                return true;
            }

            @Override
            public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
                if (!scaleDetector.isInProgress()
                        && e1.getY() < getHeight() - COMMAND_BAR_HEIGHT
                        && e2.getY() < getHeight() - COMMAND_BAR_HEIGHT) {
                    cameraX += distanceX / zoom;
                    cameraY += distanceY / zoom;
                    clampCamera();
                    invalidate();
                }
                return true;
            }

            @Override
            public boolean onDoubleTap(MotionEvent e) {
                if (e.getY() >= getHeight() - COMMAND_BAR_HEIGHT) {
                    return false;
                }
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
        scaleDetector.onTouchEvent(event);
        gestureDetector.onTouchEvent(event);
        return true;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        long now = System.nanoTime();
        if (previousFrameNs != 0L) {
            float dt = (now - previousFrameNs) / 1_000_000_000f;
            NativeEngine.step(Math.min(dt, 0.25f));
        }
        previousFrameNs = now;

        drawBattlefield(canvas);
        drawCommandBar(canvas);
        postInvalidateOnAnimation();
    }

    private boolean handleCommandBarTap(float x, float y) {
        if (getHeight() <= 0 || y < getHeight() - COMMAND_BAR_HEIGHT) {
            return false;
        }

        float buttonWidth = getWidth() / 4f;
        int button = Math.min(3, Math.max(0, (int) (x / buttonWidth)));

        if (button <= 2) {
            activeMoveMode = button;
            NativeEngine.setMoveMode(activeMoveMode);
        } else {
            NativeEngine.stopSelected();
        }
        return true;
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
            int moveMode = (int) units[i + 8];

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

                paint.setTextSize(12f / zoom);
                paint.setColor(Color.WHITE);
                String mode = moveMode == 1 ? "FAST" : (moveMode == 2 ? "SNEAK" : "MOVE");
                canvas.drawText(mode, x - 19f, y + 44f, paint);
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
        canvas.drawText("Tap blue unit to select • choose order • tap destination", 28f, 42f, paint);
        paint.setTextSize(15f);
        canvas.drawText("Drag: pan   Pinch: zoom   Double-tap: reset camera", 28f, 68f, paint);
    }

    private void drawCommandBar(Canvas canvas) {
        float top = getHeight() - COMMAND_BAR_HEIGHT;
        float buttonWidth = getWidth() / 4f;

        paint.setStyle(Paint.Style.FILL);
        paint.setColor(Color.rgb(24, 25, 22));
        canvas.drawRect(0f, top, getWidth(), getHeight(), paint);

        String[] labels = {"MOVE", "FAST", "SNEAK", "STOP"};
        paint.setTextAlign(Paint.Align.CENTER);
        paint.setTextSize(19f);

        for (int i = 0; i < labels.length; ++i) {
            float left = i * buttonWidth;
            if (i == activeMoveMode && i < 3) {
                paint.setColor(Color.rgb(82, 88, 72));
                canvas.drawRect(left + 4f, top + 6f, left + buttonWidth - 4f, getHeight() - 6f, paint);
            }

            paint.setColor(Color.rgb(70, 72, 66));
            paint.setStrokeWidth(1f);
            if (i > 0) {
                canvas.drawLine(left, top + 10f, left, getHeight() - 10f, paint);
            }

            paint.setColor(Color.WHITE);
            canvas.drawText(labels[i], left + buttonWidth / 2f, top + 47f, paint);
        }

        paint.setTextAlign(Paint.Align.LEFT);
    }

    private void clampCamera() {
        float visibleW = getWidth() > 0 ? getWidth() / zoom : 1f;
        float usableHeight = Math.max(1f, getHeight() - COMMAND_BAR_HEIGHT);
        float visibleH = usableHeight / zoom;
        float maxX = Math.max(0f, NativeEngine.getWorldWidth() - visibleW);
        float maxY = Math.max(0f, NativeEngine.getWorldHeight() - visibleH);
        cameraX = Math.max(0f, Math.min(maxX, cameraX));
        cameraY = Math.max(0f, Math.min(maxY, cameraY));
    }
}
