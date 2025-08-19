
package com.bambulab.iotc;

public class IOTCNative {
    static {
        System.loadLibrary("IOTCAPIsT");
    }

    // Core initialization and cleanup
    public static native long IOTC_Initialize();
    public static native long IOTC_DeInitialize();

    // Session management
    public static native long IOTC_Get_SessionID();
    public static native long IOTC_Set_Max_Session_Number(int maxSessions);
    public static native long IOTC_Connect_ByUID(String uid);
    public static native long IOTC_Session_Close(int sessionId);
    public static native long IOTC_Session_Check(int sessionId);
    public static native long IOTC_Get_Session_Status(int sessionId);

    // Channel management
    public static native long IOTC_Session_Channel_ON(int sessionId, byte channel);
    public static native long IOTC_Session_Channel_OFF(int sessionId, byte channel);
    public static native long IOTC_Session_Channel_Check_ON_OFF(int sessionId, byte channel);
    public static native int IOTC_Session_Get_Free_Channel(int sessionId);
    public static native int IOTC_Session_Get_Channel_ON_Count(int sessionId);
    public static native int IOTC_Session_Get_Channel_ON_Bitmap(int sessionId);

    // Data transmission
    public static native long IOTC_Session_Write(int sessionId, byte[] data, int size, byte channel);
    public static native long IOTC_Session_Read(int sessionId, byte[] buffer, int size, int timeout, int flags);
    public static native long IOTC_Session_Read_Check_Lost_Data_And_Datatype(
        int sessionId, byte[] buffer, int size, int timeout,
        byte[] lost, byte[] datatype, int flags, int unused);

    // Utility functions
    public static native void IOTC_Get_Version(int[] version);
    public static native String IOTC_Get_Version_String();

    // Endianness conversion helpers
    public static native int IOTC_Data_ntoh(int data);
    public static native int IOTC_Data_hton(int data);

    // SSL/TLS support
    public static native long IOTC_sCHL_shutdown(long ssl);

    // Connection functions
    public static native long IOTC_Listen(String uid, short port, int timeoutMs);
    public static native long IOTC_Connect(String uid, String server, short port);

    // Information functions
    public static native long IOTC_Session_Get_Info(int sessionId, byte[] info);
    public static native long IOTC_Get_Login_Info(int sessionId, byte[] loginInfo);
}
