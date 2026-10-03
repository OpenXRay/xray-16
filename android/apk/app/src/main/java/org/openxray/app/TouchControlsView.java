package org.openxray.app;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.view.View;

/**
 * Resource-independent Android input overlay.
 *
 * The view never edits or depends on game UI assets. It translates touches
 * into engine actions and relative mouse input, so original PC resources and
 * modded menus remain usable without Android-specific copies.
 */
final class TouchControlsView extends View {
    static final int CONTROL_FORWARD = 0;
    static final int CONTROL_BACK = 1;
    static final int CONTROL_LEFT = 2;
    static final int CONTROL_RIGHT = 3;
    static final int CONTROL_INVENTORY = 4;
    static final int CONTROL_USE = 5;
    static final int CONTROL_FIRE = 6;
    static final int CONTROL_ESCAPE = 7;

    private static final int NO_POINTER = -1;

    private final Paint fillPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint strokePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final float density;

    private int joystickPointer = NO_POINTER;
    private int firePointer = NO_POINTER;
    private int usePointer = NO_POINTER;
    private int inventoryPointer = NO_POINTER;
    private int escapePointer = NO_POINTER;
    private int mousePointer = NO_POINTER;
    private float joystickX;
    private float joystickY;
    private float mouseX;
    private float mouseY;
    private float mouseDownX;
    private float mouseDownY;
    private long mouseDownTime;
    private int activeMask;

    TouchControlsView(Context context) {
        super(context);
        density = getResources().getDisplayMetrics().density;
        setBackgroundColor(Color.TRANSPARENT);
        setFocusable(false);
        setClickable(true);

        fillPaint.setColor(Color.argb(76, 18, 22, 28));
        strokePaint.setStyle(Paint.Style.STROKE);
        strokePaint.setStrokeWidth(dp(2));
        strokePaint.setColor(Color.argb(185, 235, 239, 245));
        textPaint.setColor(Color.WHITE);
        textPaint.setTextAlign(Paint.Align.CENTER);
        textPaint.setTextSize(dp(13));
        textPaint.setFakeBoldText(true);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        final float joyRadius = dp(78);
        final float joyCx = dp(102);
        final float joyCy = getHeight() - dp(100);
        drawCircle(canvas, joyCx, joyCy, joyRadius, "");

        float knobX = joyCx;
        float knobY = joyCy;
        if (joystickPointer != NO_POINTER) {
            float dx = joystickX - joyCx;
            float dy = joystickY - joyCy;
            float length = (float)Math.hypot(dx, dy);
            if (length > joyRadius && length > 0) {
                dx *= joyRadius / length;
                dy *= joyRadius / length;
            }
            knobX += dx;
            knobY += dy;
        }
        drawCircle(canvas, knobX, knobY, dp(31), "");

        RectF fire = fireBounds();
        drawCircle(canvas, fire.centerX(), fire.centerY(), fire.width() / 2, "ОГОНЬ");
        RectF use = useBounds();
        drawCircle(canvas, use.centerX(), use.centerY(), use.width() / 2, "ВЗЯТЬ");
        RectF inventory = inventoryBounds();
        drawCircle(canvas, inventory.centerX(), inventory.centerY(), inventory.width() / 2, "ИНВ");
        RectF escape = escapeBounds();
        drawCircle(canvas, escape.centerX(), escape.centerY(), escape.width() / 2, "ESC");
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        final int action = event.getActionMasked();
        final int index = event.getActionIndex();

        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
            capturePointer(event.getPointerId(index), event.getX(index), event.getY(index));
        } else if (action == MotionEvent.ACTION_MOVE) {
            updatePointers(event);
        } else if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP) {
            releasePointer(event.getPointerId(index), event.getX(index), event.getY(index), false);
        } else if (action == MotionEvent.ACTION_CANCEL) {
            releaseAllControls();
        }

        invalidate();
        return true;
    }

    void releaseAllControls() {
        for (int control = CONTROL_FORWARD; control <= CONTROL_ESCAPE; ++control)
            setControl(control, false);
        if (mousePointer != NO_POINTER)
            XRayActivity.clickTouchMouse(false);
        joystickPointer = firePointer = usePointer = inventoryPointer = escapePointer = mousePointer = NO_POINTER;
        invalidate();
    }

    private void capturePointer(int pointerId, float x, float y) {
        if (escapePointer == NO_POINTER && escapeBounds().contains(x, y)) {
            escapePointer = pointerId;
            setControl(CONTROL_ESCAPE, true);
        } else if (firePointer == NO_POINTER && fireBounds().contains(x, y)) {
            firePointer = pointerId;
            setControl(CONTROL_FIRE, true);
        } else if (usePointer == NO_POINTER && useBounds().contains(x, y)) {
            usePointer = pointerId;
            setControl(CONTROL_USE, true);
        } else if (inventoryPointer == NO_POINTER && inventoryBounds().contains(x, y)) {
            inventoryPointer = pointerId;
            setControl(CONTROL_INVENTORY, true);
        } else if (joystickPointer == NO_POINTER && x < getWidth() * 0.42f && y > getHeight() * 0.42f) {
            joystickPointer = pointerId;
            updateJoystick(x, y);
        } else if (mousePointer == NO_POINTER) {
            mousePointer = pointerId;
            mouseX = mouseDownX = x;
            mouseY = mouseDownY = y;
            mouseDownTime = SystemClock.uptimeMillis();
        }
    }

    private void updatePointers(MotionEvent event) {
        for (int index = 0; index < event.getPointerCount(); ++index) {
            int pointerId = event.getPointerId(index);
            float x = event.getX(index);
            float y = event.getY(index);
            if (pointerId == joystickPointer) {
                updateJoystick(x, y);
            } else if (pointerId == mousePointer) {
                XRayActivity.moveTouchMouse(x - mouseX, y - mouseY);
                mouseX = x;
                mouseY = y;
            }
        }
    }

    private void releasePointer(int pointerId, float x, float y, boolean cancelled) {
        if (pointerId == joystickPointer) {
            joystickPointer = NO_POINTER;
            setControl(CONTROL_FORWARD, false);
            setControl(CONTROL_BACK, false);
            setControl(CONTROL_LEFT, false);
            setControl(CONTROL_RIGHT, false);
        } else if (pointerId == firePointer) {
            firePointer = NO_POINTER;
            setControl(CONTROL_FIRE, false);
        } else if (pointerId == usePointer) {
            usePointer = NO_POINTER;
            setControl(CONTROL_USE, false);
        } else if (pointerId == inventoryPointer) {
            inventoryPointer = NO_POINTER;
            setControl(CONTROL_INVENTORY, false);
        } else if (pointerId == escapePointer) {
            escapePointer = NO_POINTER;
            setControl(CONTROL_ESCAPE, false);
        } else if (pointerId == mousePointer) {
            mousePointer = NO_POINTER;
            float travel = (float)Math.hypot(x - mouseDownX, y - mouseDownY);
            long duration = SystemClock.uptimeMillis() - mouseDownTime;
            if (!cancelled && travel <= dp(14) && duration <= 350) {
                XRayActivity.clickTouchMouse(true);
                XRayActivity.clickTouchMouse(false);
            }
        }
    }

    private void updateJoystick(float x, float y) {
        joystickX = x;
        joystickY = y;
        float dx = x - dp(102);
        float dy = y - (getHeight() - dp(100));
        float deadZone = dp(18);
        setControl(CONTROL_LEFT, dx < -deadZone);
        setControl(CONTROL_RIGHT, dx > deadZone);
        setControl(CONTROL_FORWARD, dy < -deadZone);
        setControl(CONTROL_BACK, dy > deadZone);
    }

    private void setControl(int control, boolean pressed) {
        int bit = 1 << control;
        boolean wasPressed = (activeMask & bit) != 0;
        if (wasPressed == pressed)
            return;
        if (pressed)
            activeMask |= bit;
        else
            activeMask &= ~bit;
        XRayActivity.setTouchControl(control, pressed);
    }

    private RectF fireBounds() {
        return circleBounds(getWidth() - dp(89), getHeight() - dp(92), dp(55));
    }

    private RectF useBounds() {
        return circleBounds(getWidth() - dp(202), getHeight() - dp(151), dp(39));
    }

    private RectF inventoryBounds() {
        return circleBounds(getWidth() - dp(207), getHeight() - dp(55), dp(39));
    }

    private RectF escapeBounds() {
        return circleBounds(getWidth() - dp(66), dp(66), dp(34));
    }

    private RectF circleBounds(float x, float y, float radius) {
        return new RectF(x - radius, y - radius, x + radius, y + radius);
    }

    private void drawCircle(Canvas canvas, float x, float y, float radius, String label) {
        canvas.drawCircle(x, y, radius, fillPaint);
        canvas.drawCircle(x, y, radius, strokePaint);
        if (!label.isEmpty()) {
            Paint.FontMetrics metrics = textPaint.getFontMetrics();
            float baseline = y - (metrics.ascent + metrics.descent) / 2;
            canvas.drawText(label, x, baseline, textPaint);
        }
    }

    private float dp(float value) {
        return value * density;
    }
}
