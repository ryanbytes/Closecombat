package com.ryanbytes.closecombat;

public final class NativeEngine {
    static {
        System.loadLibrary("closecombat");
    }

    private NativeEngine() {}

    public static native void reset();
    public static native void step(float dtSeconds);
    public static native void tap(float worldX, float worldY);
    public static native void setMoveMode(int mode);
    public static native void stopSelected();
    public static native float[] getUnits();
    public static native float[] getObstacles();
    public static native float getWorldWidth();
    public static native float getWorldHeight();
}
