
#include <jni.h>
#include <string.h>
#include <android/log.h>
#include "libIOTCAPIsT.h"

#define LOG_TAG "IOTCNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Core initialization and cleanup
JNIEXPORT jlong JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Initialize(JNIEnv *env, jclass clazz) {
    return IOTC_Initialize();
}

JNIEXPORT jlong JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1DeInitialize(JNIEnv *env, jclass clazz) {
    return IOTC_DeInitialize();
}

// Session management
JNIEXPORT jlong JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Get_1SessionID(JNIEnv *env, jclass clazz) {
    return IOTC_Get_SessionID();
}

JNIEXPORT jlong JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Set_1Max_1Session_1Number(JNIEnv *env, jclass clazz, jint maxSessions) {
    return IOTC_Set_Max_Session_Number((unsigned int)maxSessions);
}

JNIEXPORT jlong JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Connect_1ByUID(JNIEnv *env, jclass clazz, jstring uid) {
    const char *uid_str = (*env)->GetStringUTFChars(env, uid, NULL);
    jlong result = IOTC_Connect_ByUID(uid_str);
    (*env)->ReleaseStringUTFChars(env, uid, uid_str);
    return result;
}

JNIEXPORT jlong JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Session_1Close(JNIEnv *env, jclass clazz, jint sessionId) {
    return IOTC_Session_Close(sessionId);
}

JNIEXPORT jlong JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Session_1Check(JNIEnv *env, jclass clazz, jint sessionId) {
    return IOTC_Session_Check(sessionId);
}

JNIEXPORT jlong JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Get_1Session_1Status(JNIEnv *env, jclass clazz, jint sessionId) {
    return IOTC_Get_Session_Status(sessionId);
}

// Channel management
JNIEXPORT jlong JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Session_1Channel_1ON(JNIEnv *env, jclass clazz, jint sessionId, jbyte channel) {
    return IOTC_Session_Channel_ON(sessionId, (unsigned char)channel);
}

JNIEXPORT jlong JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Session_1Channel_1OFF(JNIEnv *env, jclass clazz, jint sessionId, jbyte channel) {
    return IOTC_Session_Channel_OFF(sessionId, (unsigned char)channel);
}

JNIEXPORT jlong JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Session_1Channel_1Check_1ON_1OFF(JNIEnv *env, jclass clazz, jint sessionId, jbyte channel) {
    return IOTC_Session_Channel_Check_ON_OFF(sessionId, (unsigned char)channel);
}

JNIEXPORT jint JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Session_1Get_1Free_1Channel(JNIEnv *env, jclass clazz, jint sessionId) {
    return IOTC_Session_Get_Free_Channel(sessionId);
}

JNIEXPORT jint JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Session_1Get_1Channel_1ON_1Count(JNIEnv *env, jclass clazz, jint sessionId) {
    return IOTC_Session_Get_Channel_ON_Count(sessionId);
}

JNIEXPORT jint JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Session_1Get_1Channel_1ON_1Bitmap(JNIEnv *env, jclass clazz, jint sessionId) {
    return IOTC_Session_Get_Channel_ON_Bitmap(sessionId);
}

// Data transmission
JNIEXPORT jlong JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Session_1Write(JNIEnv *env, jclass clazz, jint sessionId, jbyteArray data, jint size, jbyte channel) {
    jbyte *data_ptr = (*env)->GetByteArrayElements(env, data, NULL);
    jlong result = IOTC_Session_Write(sessionId, data_ptr, (unsigned int)size, (unsigned char)channel);
    (*env)->ReleaseByteArrayElements(env, data, data_ptr, JNI_ABORT);
    return result;
}

JNIEXPORT jlong JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Session_1Read(JNIEnv *env, jclass clazz, jint sessionId, jbyteArray buffer, jint size, jint timeout, jint flags) {
    jbyte *buffer_ptr = (*env)->GetByteArrayElements(env, buffer, NULL);
    jlong result = IOTC_Session_Read(sessionId, buffer_ptr, size, timeout, flags);
    (*env)->ReleaseByteArrayElements(env, buffer, buffer_ptr, 0);
    return result;
}

JNIEXPORT jlong JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Session_1Read_1Check_1Lost_1Data_1And_1Datatype(
    JNIEnv *env, jclass clazz, jint sessionId, jbyteArray buffer, jint size, jint timeout,
    jbyteArray lost, jbyteArray datatype, jint flags, jint unused) {
    
    jbyte *buffer_ptr = (*env)->GetByteArrayElements(env, buffer, NULL);
    jbyte *lost_ptr = (*env)->GetByteArrayElements(env, lost, NULL);
    jbyte *datatype_ptr = (*env)->GetByteArrayElements(env, datatype, NULL);
    
    jlong result = IOTC_Session_Read_Check_Lost_Data_And_Datatype(
        sessionId, buffer_ptr, size, timeout,
        (unsigned char*)lost_ptr, (unsigned char*)datatype_ptr, flags, unused);
    
    (*env)->ReleaseByteArrayElements(env, buffer, buffer_ptr, 0);
    (*env)->ReleaseByteArrayElements(env, lost, lost_ptr, 0);
    (*env)->ReleaseByteArrayElements(env, datatype, datatype_ptr, 0);
    
    return result;
}

// Utility functions
JNIEXPORT void JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Get_1Version(JNIEnv *env, jclass clazz, jintArray version) {
    uint32_t ver;
    IOTC_Get_Version(&ver);
    jint ver_int = (jint)ver;
    (*env)->SetIntArrayRegion(env, version, 0, 1, &ver_int);
}

JNIEXPORT jstring JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Get_1Version_1String(JNIEnv *env, jclass clazz) {
    const char *version_str = IOTC_Get_Version_String();
    return (*env)->NewStringUTF(env, version_str);
}

// Endianness conversion helpers
JNIEXPORT jint JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Data_1ntoh(JNIEnv *env, jclass clazz, jint data) {
    return (jint)IOTC_Data_ntoh((uint32_t)data);
}

JNIEXPORT jint JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Data_1hton(JNIEnv *env, jclass clazz, jint data) {
    return (jint)IOTC_Data_hton((uint32_t)data);
}

// SSL/TLS support
JNIEXPORT jlong JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1sCHL_1shutdown(JNIEnv *env, jclass clazz, jlong ssl) {
    return IOTC_sCHL_shutdown(ssl);
}

// Connection functions (stubs)
JNIEXPORT jlong JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Listen(JNIEnv *env, jclass clazz, jstring uid, jshort port, jint timeoutMs) {
    const char *uid_str = (*env)->GetStringUTFChars(env, uid, NULL);
    jlong result = IOTC_Listen(uid_str, (uint16_t)port, (uint32_t)timeoutMs);
    (*env)->ReleaseStringUTFChars(env, uid, uid_str);
    return result;
}

JNIEXPORT jlong JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Connect(JNIEnv *env, jclass clazz, jstring uid, jstring server, jshort port) {
    const char *uid_str = (*env)->GetStringUTFChars(env, uid, NULL);
    const char *server_str = (*env)->GetStringUTFChars(env, server, NULL);
    jlong result = IOTC_Connect(uid_str, server_str, (uint16_t)port);
    (*env)->ReleaseStringUTFChars(env, uid, uid_str);
    (*env)->ReleaseStringUTFChars(env, server, server_str);
    return result;
}

// Information functions (stubs)
JNIEXPORT jlong JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Session_1Get_1Info(JNIEnv *env, jclass clazz, jint sessionId, jbyteArray info) {
    jbyte *info_ptr = (*env)->GetByteArrayElements(env, info, NULL);
    jlong result = IOTC_Session_Get_Info(sessionId, info_ptr);
    (*env)->ReleaseByteArrayElements(env, info, info_ptr, 0);
    return result;
}

JNIEXPORT jlong JNICALL
Java_com_bambulab_iotc_IOTCNative_IOTC_1Get_1Login_1Info(JNIEnv *env, jclass clazz, jint sessionId, jbyteArray loginInfo) {
    jbyte *login_info_ptr = (*env)->GetByteArrayElements(env, loginInfo, NULL);
    jlong result = IOTC_Get_Login_Info(sessionId, login_info_ptr);
    (*env)->ReleaseByteArrayElements(env, loginInfo, login_info_ptr, 0);
    return result;
}
