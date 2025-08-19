
package com.bambulab.iotc;

import android.util.Log;

public class IOTCManager {
    private static final String TAG = "IOTCManager";
    private static boolean isInitialized = false;
    
    public static class ErrorCodes {
        public static final int IOTC_ER_NoERROR = 0;
        public static final int IOTC_ER_NOT_INITIALIZED = -1;
        public static final int IOTC_ER_ALREADY_INITIALIZED = -2;
        public static final int IOTC_ER_INVALID_ARG = -27;
        public static final int IOTC_ER_EXCEED_MAX_SESSION = -16;
        public static final int IOTC_ER_INVALID_SID = -15;
        public static final int IOTC_ER_CH_NOT_ON = -21;
    }
    
    public static boolean initialize() {
        if (isInitialized) {
            Log.w(TAG, "IOTC already initialized");
            return true;
        }
        
        long result = IOTCNative.IOTC_Initialize();
        if (result == ErrorCodes.IOTC_ER_NoERROR) {
            isInitialized = true;
            Log.i(TAG, "IOTC initialized successfully");
            String version = IOTCNative.IOTC_Get_Version_String();
            Log.i(TAG, "IOTC Version: " + version);
            return true;
        } else {
            Log.e(TAG, "Failed to initialize IOTC: " + result);
            return false;
        }
    }
    
    public static void deinitialize() {
        if (!isInitialized) {
            Log.w(TAG, "IOTC not initialized");
            return;
        }
        
        long result = IOTCNative.IOTC_DeInitialize();
        if (result == ErrorCodes.IOTC_ER_NoERROR) {
            isInitialized = false;
            Log.i(TAG, "IOTC deinitialized successfully");
        } else {
            Log.e(TAG, "Failed to deinitialize IOTC: " + result);
        }
    }
    
    public static int connectByUID(String uid) {
        if (!isInitialized) {
            Log.e(TAG, "IOTC not initialized");
            return ErrorCodes.IOTC_ER_NOT_INITIALIZED;
        }
        
        if (uid == null || uid.length() != 20) {
            Log.e(TAG, "Invalid UID: " + uid);
            return ErrorCodes.IOTC_ER_INVALID_ARG;
        }
        
        long result = IOTCNative.IOTC_Connect_ByUID(uid);
        if (result >= 0) {
            Log.i(TAG, "Connected to UID " + uid + ", session ID: " + result);
            return (int)result;
        } else {
            Log.e(TAG, "Failed to connect to UID " + uid + ": " + result);
            return (int)result;
        }
    }
    
    public static boolean closeSession(int sessionId) {
        long result = IOTCNative.IOTC_Session_Close(sessionId);
        if (result == ErrorCodes.IOTC_ER_NoERROR) {
            Log.i(TAG, "Session " + sessionId + " closed successfully");
            return true;
        } else {
            Log.e(TAG, "Failed to close session " + sessionId + ": " + result);
            return false;
        }
    }
    
    public static boolean isSessionConnected(int sessionId) {
        return IOTCNative.IOTC_Session_Check(sessionId) == 1;
    }
    
    public static boolean turnOnChannel(int sessionId, byte channel) {
        long result = IOTCNative.IOTC_Session_Channel_ON(sessionId, channel);
        if (result == ErrorCodes.IOTC_ER_NoERROR) {
            Log.i(TAG, "Channel " + channel + " turned on for session " + sessionId);
            return true;
        } else {
            Log.e(TAG, "Failed to turn on channel " + channel + " for session " + sessionId + ": " + result);
            return false;
        }
    }
    
    public static boolean turnOffChannel(int sessionId, byte channel) {
        long result = IOTCNative.IOTC_Session_Channel_OFF(sessionId, channel);
        if (result == ErrorCodes.IOTC_ER_NoERROR) {
            Log.i(TAG, "Channel " + channel + " turned off for session " + sessionId);
            return true;
        } else {
            Log.e(TAG, "Failed to turn off channel " + channel + " for session " + sessionId + ": " + result);
            return false;
        }
    }
    
    public static int writeData(int sessionId, byte[] data, byte channel) {
        if (data == null || data.length == 0) {
            Log.e(TAG, "Invalid data");
            return ErrorCodes.IOTC_ER_INVALID_ARG;
        }
        
        long result = IOTCNative.IOTC_Session_Write(sessionId, data, data.length, channel);
        if (result >= 0) {
            Log.d(TAG, "Wrote " + result + " bytes to session " + sessionId + ", channel " + channel);
            return (int)result;
        } else {
            Log.e(TAG, "Failed to write data to session " + sessionId + ", channel " + channel + ": " + result);
            return (int)result;
        }
    }
    
    public static int readData(int sessionId, byte[] buffer, int timeout) {
        if (buffer == null || buffer.length == 0) {
            Log.e(TAG, "Invalid buffer");
            return ErrorCodes.IOTC_ER_INVALID_ARG;
        }
        
        long result = IOTCNative.IOTC_Session_Read(sessionId, buffer, buffer.length, timeout, 0);
        if (result >= 0) {
            Log.d(TAG, "Read " + result + " bytes from session " + sessionId);
            return (int)result;
        } else {
            Log.e(TAG, "Failed to read data from session " + sessionId + ": " + result);
            return (int)result;
        }
    }
}
