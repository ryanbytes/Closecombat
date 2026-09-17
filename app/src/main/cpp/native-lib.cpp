#include <jni.h>

#include "Engine.h"

namespace {
cc::Engine gEngine;

jfloatArray toJavaFloatArray(JNIEnv* env, const std::vector<float>& values) {
    jfloatArray result = env->NewFloatArray(static_cast<jsize>(values.size()));
    if (result == nullptr || values.empty()) {
        return result;
    }
    env->SetFloatArrayRegion(result, 0, static_cast<jsize>(values.size()), values.data());
    return result;
}
} // namespace

extern "C" JNIEXPORT void JNICALL
Java_com_ryanbytes_closecombat_NativeEngine_reset(JNIEnv*, jclass) {
    gEngine.reset();
}

extern "C" JNIEXPORT void JNICALL
Java_com_ryanbytes_closecombat_NativeEngine_step(JNIEnv*, jclass, jfloat dtSeconds) {
    gEngine.step(dtSeconds);
}

extern "C" JNIEXPORT void JNICALL
Java_com_ryanbytes_closecombat_NativeEngine_tap(JNIEnv*, jclass, jfloat worldX, jfloat worldY) {
    gEngine.tap(worldX, worldY);
}

extern "C" JNIEXPORT void JNICALL
Java_com_ryanbytes_closecombat_NativeEngine_setOrderMode(JNIEnv*, jclass, jint mode) {
    gEngine.setOrderMode(static_cast<int>(mode));
}

extern "C" JNIEXPORT void JNICALL
Java_com_ryanbytes_closecombat_NativeEngine_stopSelected(JNIEnv*, jclass) {
    gEngine.stopSelected();
}

extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_ryanbytes_closecombat_NativeEngine_getUnits(JNIEnv* env, jclass) {
    return toJavaFloatArray(env, gEngine.unitSnapshot());
}

extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_ryanbytes_closecombat_NativeEngine_getSoldiers(JNIEnv* env, jclass) {
    return toJavaFloatArray(env, gEngine.soldierSnapshot());
}

extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_ryanbytes_closecombat_NativeEngine_getObstacles(JNIEnv* env, jclass) {
    return toJavaFloatArray(env, gEngine.obstacleSnapshot());
}

extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_ryanbytes_closecombat_NativeEngine_getCoverZones(JNIEnv* env, jclass) {
    return toJavaFloatArray(env, gEngine.coverSnapshot());
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_ryanbytes_closecombat_NativeEngine_getWorldWidth(JNIEnv*, jclass) {
    return gEngine.worldWidth();
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_ryanbytes_closecombat_NativeEngine_getWorldHeight(JNIEnv*, jclass) {
    return gEngine.worldHeight();
}
