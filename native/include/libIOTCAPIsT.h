
#ifndef LIBIOTCAPIST_H
#define LIBIOTCAPIST_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Minimal representation of the packet header used by the
 * IOTC library.  Only the fields required by the exported
 * byte order conversion helpers are modelled here.
 */
typedef struct {
    uint32_t flag;      /* packet flags and version */
    uint32_t sid;       /* session identifier */
    uint32_t seq;       /* sequence number */
    uint32_t timestamp; /* timestamp or similar */
    uint32_t payload;   /* payload length */
} IOTCHeader;

/* Device information structure for LAN search */
typedef struct {
    char UID[20];        /* Device UID */
    char IP[16];         /* IP address in dotted decimal format */
    uint16_t port;       /* Port number */
    char reserved[2];    /* Reserved for alignment */
} IOTCDevInfo;

/* Core initialization and cleanup */
int64_t IOTC_Initialize(void);
int64_t IOTC_DeInitialize(void);

/* Session management */
int64_t IOTC_Get_SessionID(void);
int64_t IOTC_Set_Max_Session_Number(unsigned int max_sessions);
int64_t IOTC_Connect_ByUID(const char *uid);
int64_t IOTC_Session_Close(int session_id);
int64_t IOTC_Session_Check(int session_id);
int64_t IOTC_Get_Session_Status(int session_id);

/* Channel management */
int64_t IOTC_Session_Channel_ON(int session_id, unsigned char channel);
int64_t IOTC_Session_Channel_OFF(int session_id, unsigned char channel);
int64_t IOTC_Session_Channel_Check_ON_OFF(int session_id, unsigned char channel);
int32_t IOTC_Session_Get_Free_Channel(int session_id);
int32_t IOTC_Session_Get_Channel_ON_Count(int session_id);
uint32_t IOTC_Session_Get_Channel_ON_Bitmap(int session_id);

/* Data transmission */
int64_t IOTC_Session_Write(int session_id, const void *data, unsigned int size, unsigned char channel);
int64_t IOTC_Session_Read(int session_id, void *buf, int size, int timeout, int flags);
int64_t IOTC_Session_Read_Check_Lost_Data_And_Datatype(
    int session_id, void *buf, int size, int timeout,
    unsigned char *lost, unsigned char *datatype, int flags, int unused);

/* SSL/TLS support */
int64_t IOTC_sCHL_shutdown(int64_t ssl);

/* Utility functions */
void IOTC_Get_Version(uint32_t *version);
const char *IOTC_Get_Version_String(void);

/* Endianness conversion helpers */
uint32_t IOTC_Data_ntoh(uint32_t data);
uint32_t IOTC_Data_hton(uint32_t data);
void IOTC_Header_ntoh(IOTCHeader *hdr);
void IOTC_Header_hton(IOTCHeader *hdr);

/* Connection functions (limited implementation) */
int64_t IOTC_Listen(const char *uid, uint16_t port, uint32_t timeout_ms);
int64_t IOTC_Connect(const char *uid, const char *server, uint16_t port);

/* Information functions (stubs) */
int64_t IOTC_Session_Get_Info(int session_id, void *info);
int64_t IOTC_Get_Login_Info(int session_id, void *login_info);

#ifdef __cplusplus
}
#endif

#endif /* LIBIOTCAPIST_H */
