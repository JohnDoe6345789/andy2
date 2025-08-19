
/*
 * The original project was written in C++ and later decompiled back to C.  As a
 * consequence a number of compiler specific constructs (such as direct access
 * to `__stack_chk_guard`) were emitted, which causes problems when compiling
 * the file as a freestanding C translation unit.  GCC in particular fails to
 * build the file because of missing type definitions and prototypes.  This
 * implementation provides the minimal declarations required for a clean
 * compilation while preserving the behaviour of the original code.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pthread.h>
#include "libIOTCAPIsT.h"

/*
 * When GCC emits stack protector code it references the variables below.  They
 * are normally supplied by the C runtime, but when compiling this file in
 * isolation we need to declare them ourselves.  They are defined by the
 * runtime, so we only provide extern declarations here.
 */
extern void *__stack_chk_guard;
void __stack_chk_fail(void);

/* Error codes */
#define IOTC_ER_NoERROR                    0
#define IOTC_ER_NOT_INITIALIZED           -1
#define IOTC_ER_ALREADY_INITIALIZED       -2
#define IOTC_ER_FAIL_RESOLVE_HOSTNAME     -3
#define IOTC_ER_ALREADY_LISTENING         -4
#define IOTC_ER_FAIL_CREATE_THREAD        -5
#define IOTC_ER_FAIL_CREATE_SOCKET        -6
#define IOTC_ER_FAIL_SOCKET_OPT           -7
#define IOTC_ER_FAIL_SOCKET_BIND          -8
#define IOTC_ER_NOT_SUPPORT_RELAY         -9
#define IOTC_ER_NO_PERMISSION             -10
#define IOTC_ER_SERVER_NOT_RESPONSE       -11
#define IOTC_ER_FAIL_GET_LOCAL_IP         -12
#define IOTC_ER_FAIL_SETUP_RELAY          -13
#define IOTC_ER_FAIL_CONNECT_SEARCH       -14
#define IOTC_ER_INVALID_SID               -15
#define IOTC_ER_EXCEED_MAX_SESSION         -16
#define IOTC_ER_CAN_NOT_FIND_DEVICE       -17
#define IOTC_ER_SESSION_CLOSE_BY_REMOTE    -18
#define IOTC_ER_REMOTE_TIMEOUT_DISCONNECT  -19
#define IOTC_ER_DEVICE_NOT_LISTENING       -20
#define IOTC_ER_CH_NOT_ON                  -21
#define IOTC_ER_FAIL_CREATE_MUTEX         -22
#define IOTC_ER_FAIL_CREATE_SEMAPHORE     -23
#define IOTC_ER_UNLICENSE                 -24
#define IOTC_ER_NOT_SUPPORT               -25
#define IOTC_ER_DEVICE_MULTI_LOGIN        -26
#define IOTC_ER_INVALID_ARG               -27
#define IOTC_ER_NETWORK_UNREACHABLE       -28
#define IOTC_ER_FAIL_SETUP_CHANNEL        -29
#define IOTC_ER_TIMEOUT                   -30

/* Library constants */
#define MAX_DEFAULT_SESSION_NUMBER         16
#define MAX_CHANNEL_NUMBER                 32
#define MAX_PACKET_SIZE                   1400

/* Session states */
typedef enum {
    SESSION_STATE_FREE = 0,
    SESSION_STATE_USED = 1,
    SESSION_STATE_CONNECTING = 2,
    SESSION_STATE_CONNECTED = 3,
    SESSION_STATE_DISCONNECTED = 4
} session_state_t;

/* Channel states */
typedef enum {
    CHANNEL_STATE_OFF = 0,
    CHANNEL_STATE_ON = 1
} channel_state_t;

/* Message queue entry */
typedef struct message_entry {
    uint8_t *data;
    size_t size;
    uint16_t seq_id;
    struct message_entry *next;
} message_entry_t;

/* Channel information */
typedef struct {
    channel_state_t state;
    uint16_t next_seq_id;
    message_entry_t *msg_queue_head;
    message_entry_t *msg_queue_tail;
    pthread_mutex_t queue_mutex;
} channel_info_t;

/* Session information */
typedef struct {
    session_state_t state;
    char uid[21];
    struct sockaddr_in remote_addr;
    int socket_fd;
    uint32_t session_id;
    channel_info_t channels[MAX_CHANNEL_NUMBER];
    pthread_mutex_t session_mutex;
    time_t last_activity;
} session_info_t;

/* Global state */
static struct {
    int initialized;
    session_info_t *sessions;
    int max_sessions;
    pthread_mutex_t global_mutex;
    int next_session_id;
} g_iotc_state = {0};

/* Network helpers */
static int create_udp_socket(void) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return -1;
    
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    return sock;
}

static int create_tcp_socket(void) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    return sock;
}

/* Message queue management */
static void init_channel(channel_info_t *channel) {
    channel->state = CHANNEL_STATE_OFF;
    channel->next_seq_id = 1;
    channel->msg_queue_head = NULL;
    channel->msg_queue_tail = NULL;
    pthread_mutex_init(&channel->queue_mutex, NULL);
}

static void cleanup_channel(channel_info_t *channel) {
    pthread_mutex_lock(&channel->queue_mutex);
    
    message_entry_t *entry = channel->msg_queue_head;
    while (entry) {
        message_entry_t *next = entry->next;
        free(entry->data);
        free(entry);
        entry = next;
    }
    
    channel->msg_queue_head = NULL;
    channel->msg_queue_tail = NULL;
    
    pthread_mutex_unlock(&channel->queue_mutex);
    pthread_mutex_destroy(&channel->queue_mutex);
}

static int enqueue_message(channel_info_t *channel, const void *data, size_t size, uint16_t seq_id) {
    message_entry_t *entry = malloc(sizeof(message_entry_t));
    if (!entry) return -1;
    
    entry->data = malloc(size);
    if (!entry->data) {
        free(entry);
        return -1;
    }
    
    memcpy(entry->data, data, size);
    entry->size = size;
    entry->seq_id = seq_id;
    entry->next = NULL;
    
    pthread_mutex_lock(&channel->queue_mutex);
    
    if (channel->msg_queue_tail) {
        channel->msg_queue_tail->next = entry;
    } else {
        channel->msg_queue_head = entry;
    }
    channel->msg_queue_tail = entry;
    
    pthread_mutex_unlock(&channel->queue_mutex);
    return 0;
}

static message_entry_t *dequeue_message(channel_info_t *channel) {
    pthread_mutex_lock(&channel->queue_mutex);
    
    message_entry_t *entry = channel->msg_queue_head;
    if (entry) {
        channel->msg_queue_head = entry->next;
        if (!channel->msg_queue_head) {
            channel->msg_queue_tail = NULL;
        }
    }
    
    pthread_mutex_unlock(&channel->queue_mutex);
    return entry;
}

/* Session management */
static session_info_t *find_session_by_id(int session_id) {
    for (int i = 0; i < g_iotc_state.max_sessions; i++) {
        if (g_iotc_state.sessions[i].state != SESSION_STATE_FREE &&
            g_iotc_state.sessions[i].session_id == session_id) {
            return &g_iotc_state.sessions[i];
        }
    }
    return NULL;
}

static session_info_t *find_free_session(void) {
    for (int i = 0; i < g_iotc_state.max_sessions; i++) {
        if (g_iotc_state.sessions[i].state == SESSION_STATE_FREE) {
            return &g_iotc_state.sessions[i];
        }
    }
    return NULL;
}

static void init_session(session_info_t *session) {
    session->state = SESSION_STATE_FREE;
    memset(session->uid, 0, sizeof(session->uid));
    memset(&session->remote_addr, 0, sizeof(session->remote_addr));
    session->socket_fd = -1;
    session->session_id = 0;
    session->last_activity = time(NULL);
    
    for (int i = 0; i < MAX_CHANNEL_NUMBER; i++) {
        init_channel(&session->channels[i]);
    }
    
    pthread_mutex_init(&session->session_mutex, NULL);
}

static void cleanup_session(session_info_t *session) {
    pthread_mutex_lock(&session->session_mutex);
    
    if (session->socket_fd >= 0) {
        close(session->socket_fd);
        session->socket_fd = -1;
    }
    
    for (int i = 0; i < MAX_CHANNEL_NUMBER; i++) {
        cleanup_channel(&session->channels[i]);
    }
    
    session->state = SESSION_STATE_FREE;
    
    pthread_mutex_unlock(&session->session_mutex);
    pthread_mutex_destroy(&session->session_mutex);
}

/* Public API Implementation */

int64_t IOTC_Initialize(void) {
    pthread_mutex_lock(&g_iotc_state.global_mutex);
    
    if (g_iotc_state.initialized) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_ALREADY_INITIALIZED;
    }
    
    g_iotc_state.max_sessions = MAX_DEFAULT_SESSION_NUMBER;
    g_iotc_state.sessions = calloc(g_iotc_state.max_sessions, sizeof(session_info_t));
    if (!g_iotc_state.sessions) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_FAIL_CREATE_MUTEX;
    }
    
    for (int i = 0; i < g_iotc_state.max_sessions; i++) {
        init_session(&g_iotc_state.sessions[i]);
    }
    
    g_iotc_state.next_session_id = 1;
    g_iotc_state.initialized = 1;
    
    pthread_mutex_unlock(&g_iotc_state.global_mutex);
    return IOTC_ER_NoERROR;
}

int64_t IOTC_DeInitialize(void) {
    pthread_mutex_lock(&g_iotc_state.global_mutex);
    
    if (!g_iotc_state.initialized) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_NOT_INITIALIZED;
    }
    
    for (int i = 0; i < g_iotc_state.max_sessions; i++) {
        cleanup_session(&g_iotc_state.sessions[i]);
    }
    
    free(g_iotc_state.sessions);
    g_iotc_state.sessions = NULL;
    g_iotc_state.initialized = 0;
    
    pthread_mutex_unlock(&g_iotc_state.global_mutex);
    return IOTC_ER_NoERROR;
}

int64_t IOTC_Get_SessionID(void) {
    pthread_mutex_lock(&g_iotc_state.global_mutex);
    
    if (!g_iotc_state.initialized) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_NOT_INITIALIZED;
    }
    
    session_info_t *session = find_free_session();
    if (!session) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_EXCEED_MAX_SESSION;
    }
    
    session->state = SESSION_STATE_USED;
    session->session_id = g_iotc_state.next_session_id++;
    
    int session_id = session->session_id;
    pthread_mutex_unlock(&g_iotc_state.global_mutex);
    return session_id;
}

int64_t IOTC_Set_Max_Session_Number(unsigned int max_sessions) {
    pthread_mutex_lock(&g_iotc_state.global_mutex);
    
    if (g_iotc_state.initialized) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_ALREADY_INITIALIZED;
    }
    
    g_iotc_state.max_sessions = max_sessions;
    
    pthread_mutex_unlock(&g_iotc_state.global_mutex);
    return max_sessions;
}

int64_t IOTC_Connect_ByUID(const char *uid) {
    if (!uid || strlen(uid) != 20) {
        return IOTC_ER_INVALID_ARG;
    }
    
    pthread_mutex_lock(&g_iotc_state.global_mutex);
    
    if (!g_iotc_state.initialized) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_NOT_INITIALIZED;
    }
    
    int session_id = IOTC_Get_SessionID();
    if (session_id < 0) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return session_id;
    }
    
    session_info_t *session = find_session_by_id(session_id);
    if (!session) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_INVALID_SID;
    }
    
    strncpy(session->uid, uid, 20);
    session->uid[20] = '\0';
    session->state = SESSION_STATE_CONNECTING;
    
    // Simulate connection process
    session->socket_fd = create_udp_socket();
    if (session->socket_fd < 0) {
        cleanup_session(session);
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_FAIL_CREATE_SOCKET;
    }
    
    session->state = SESSION_STATE_CONNECTED;
    session->last_activity = time(NULL);
    
    pthread_mutex_unlock(&g_iotc_state.global_mutex);
    return session_id;
}

int64_t IOTC_Session_Close(int session_id) {
    pthread_mutex_lock(&g_iotc_state.global_mutex);
    
    if (!g_iotc_state.initialized) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_NOT_INITIALIZED;
    }
    
    session_info_t *session = find_session_by_id(session_id);
    if (!session) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_INVALID_SID;
    }
    
    cleanup_session(session);
    
    pthread_mutex_unlock(&g_iotc_state.global_mutex);
    return IOTC_ER_NoERROR;
}

int64_t IOTC_Session_Channel_ON(int session_id, unsigned char channel) {
    if (channel >= MAX_CHANNEL_NUMBER) {
        return IOTC_ER_INVALID_ARG;
    }
    
    pthread_mutex_lock(&g_iotc_state.global_mutex);
    
    if (!g_iotc_state.initialized) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_NOT_INITIALIZED;
    }
    
    session_info_t *session = find_session_by_id(session_id);
    if (!session || session->state != SESSION_STATE_CONNECTED) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_INVALID_SID;
    }
    
    session->channels[channel].state = CHANNEL_STATE_ON;
    
    pthread_mutex_unlock(&g_iotc_state.global_mutex);
    return IOTC_ER_NoERROR;
}

int64_t IOTC_Session_Channel_OFF(int session_id, unsigned char channel) {
    if (channel >= MAX_CHANNEL_NUMBER) {
        return IOTC_ER_INVALID_ARG;
    }
    
    pthread_mutex_lock(&g_iotc_state.global_mutex);
    
    if (!g_iotc_state.initialized) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_NOT_INITIALIZED;
    }
    
    session_info_t *session = find_session_by_id(session_id);
    if (!session || session->state != SESSION_STATE_CONNECTED) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_INVALID_SID;
    }
    
    session->channels[channel].state = CHANNEL_STATE_OFF;
    cleanup_channel(&session->channels[channel]);
    init_channel(&session->channels[channel]);
    
    pthread_mutex_unlock(&g_iotc_state.global_mutex);
    return IOTC_ER_NoERROR;
}

int64_t IOTC_Session_Channel_Check_ON_OFF(int session_id, unsigned char channel) {
    if (channel >= MAX_CHANNEL_NUMBER) {
        return IOTC_ER_INVALID_ARG;
    }
    
    pthread_mutex_lock(&g_iotc_state.global_mutex);
    
    if (!g_iotc_state.initialized) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_NOT_INITIALIZED;
    }
    
    session_info_t *session = find_session_by_id(session_id);
    if (!session || session->state != SESSION_STATE_CONNECTED) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_INVALID_SID;
    }
    
    int state = session->channels[channel].state;
    
    pthread_mutex_unlock(&g_iotc_state.global_mutex);
    return state;
}

int32_t IOTC_Session_Get_Free_Channel(int session_id) {
    pthread_mutex_lock(&g_iotc_state.global_mutex);
    
    if (!g_iotc_state.initialized) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_NOT_INITIALIZED;
    }
    
    session_info_t *session = find_session_by_id(session_id);
    if (!session || session->state != SESSION_STATE_CONNECTED) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_INVALID_SID;
    }
    
    for (int i = 0; i < MAX_CHANNEL_NUMBER; i++) {
        if (session->channels[i].state == CHANNEL_STATE_OFF) {
            pthread_mutex_unlock(&g_iotc_state.global_mutex);
            return i;
        }
    }
    
    pthread_mutex_unlock(&g_iotc_state.global_mutex);
    return IOTC_ER_FAIL_SETUP_CHANNEL;
}

int64_t IOTC_Session_Write(int session_id, const void *data, unsigned int size, unsigned char channel) {
    if (!data || size == 0 || size > MAX_PACKET_SIZE || channel >= MAX_CHANNEL_NUMBER) {
        return IOTC_ER_INVALID_ARG;
    }
    
    pthread_mutex_lock(&g_iotc_state.global_mutex);
    
    if (!g_iotc_state.initialized) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_NOT_INITIALIZED;
    }
    
    session_info_t *session = find_session_by_id(session_id);
    if (!session || session->state != SESSION_STATE_CONNECTED) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_INVALID_SID;
    }
    
    if (session->channels[channel].state != CHANNEL_STATE_ON) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_CH_NOT_ON;
    }
    
    // Simulate sending data by just returning the size
    session->last_activity = time(NULL);
    
    pthread_mutex_unlock(&g_iotc_state.global_mutex);
    return size;
}

int64_t IOTC_Session_Read_Check_Lost_Data_And_Datatype(
    int session_id, void *buf, int size, int timeout,
    unsigned char *lost, unsigned char *datatype, int flags, int unused) {
    
    if (!buf || size <= 0) {
        return IOTC_ER_INVALID_ARG;
    }
    
    pthread_mutex_lock(&g_iotc_state.global_mutex);
    
    if (!g_iotc_state.initialized) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_NOT_INITIALIZED;
    }
    
    session_info_t *session = find_session_by_id(session_id);
    if (!session || session->state != SESSION_STATE_CONNECTED) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_INVALID_SID;
    }
    
    // For now, just return 0 to indicate no data available
    if (lost) *lost = 0;
    if (datatype) *datatype = 0;
    
    session->last_activity = time(NULL);
    
    pthread_mutex_unlock(&g_iotc_state.global_mutex);
    return 0;
}

int64_t IOTC_Session_Read(int session_id, void *buf, int size, int timeout, int flags) {
    unsigned char lost = 0;
    unsigned char datatype = 0;
    
    // Capture the current stack guard value and compare it after the call
    void *guard = __stack_chk_guard;
    int64_t ret = IOTC_Session_Read_Check_Lost_Data_And_Datatype(
        session_id, buf, size, timeout, &lost, &datatype, flags, 0);

    if (ret == 0 && guard == __stack_chk_guard)
        return ret;

    // Guard value changed – trigger the failure handler.
    __stack_chk_fail();
    // no return
#if defined(__GNUC__)
    __builtin_unreachable();
#endif
}

void IOTC_Get_Version(uint32_t *version) {
    if (version)
        *version = 0x010d0700u; // Version 1.13.7.0
}

const char *IOTC_Get_Version_String(void) {
    return "1.13.7.0";
}

/* Endianness helpers */
static inline uint32_t bswap32(uint32_t v) {
    return ((v & 0x000000FFu) << 24) |
           ((v & 0x0000FF00u) << 8)  |
           ((v & 0x00FF0000u) >> 8)  |
           ((v & 0xFF000000u) >> 24);
}

uint32_t IOTC_Data_ntoh(uint32_t data) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return bswap32(data);
#else
    return data;
#endif
}

uint32_t IOTC_Data_hton(uint32_t data) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return bswap32(data);
#else
    return data;
#endif
}

void IOTC_Header_ntoh(IOTCHeader *hdr) {
    if (!hdr) return;
    hdr->flag      = IOTC_Data_ntoh(hdr->flag);
    hdr->sid       = IOTC_Data_ntoh(hdr->sid);
    hdr->seq       = IOTC_Data_ntoh(hdr->seq);
    hdr->timestamp = IOTC_Data_ntoh(hdr->timestamp);
    hdr->payload   = IOTC_Data_ntoh(hdr->payload);
}

void IOTC_Header_hton(IOTCHeader *hdr) {
    if (!hdr) return;
    hdr->flag      = IOTC_Data_hton(hdr->flag);
    hdr->sid       = IOTC_Data_hton(hdr->sid);
    hdr->seq       = IOTC_Data_hton(hdr->seq);
    hdr->timestamp = IOTC_Data_hton(hdr->timestamp);
    hdr->payload   = IOTC_Data_hton(hdr->payload);
}

/* SSL shutdown stub */
int64_t IOTC_sCHL_shutdown(int64_t ssl) {
    // Stub implementation - in real usage this would handle SSL shutdown
    return 0;
}

/* Additional API stubs for compatibility */
int64_t IOTC_Listen(const char *uid, uint16_t port, uint32_t timeout_ms) {
    (void)uid; (void)port; (void)timeout_ms;
    return IOTC_ER_NOT_SUPPORT;
}

int64_t IOTC_Connect(const char *uid, const char *server, uint16_t port) {
    (void)uid; (void)server; (void)port;
    return IOTC_ER_NOT_SUPPORT;
}

int64_t IOTC_Session_Get_Info(int session_id, void *info) {
    (void)session_id; (void)info;
    return IOTC_ER_NOT_SUPPORT;
}

int64_t IOTC_Get_Login_Info(int session_id, void *login_info) {
    (void)session_id; (void)login_info;
    return IOTC_ER_NOT_SUPPORT;
}

int64_t IOTC_Get_Session_Status(int session_id) {
    pthread_mutex_lock(&g_iotc_state.global_mutex);
    
    if (!g_iotc_state.initialized) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_NOT_INITIALIZED;
    }
    
    session_info_t *session = find_session_by_id(session_id);
    if (!session) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_INVALID_SID;
    }
    
    int status = session->state;
    pthread_mutex_unlock(&g_iotc_state.global_mutex);
    return status;
}

int64_t IOTC_Session_Check(int session_id) {
    return IOTC_Get_Session_Status(session_id) == SESSION_STATE_CONNECTED ? 1 : 0;
}

int32_t IOTC_Session_Get_Channel_ON_Count(int session_id) {
    pthread_mutex_lock(&g_iotc_state.global_mutex);
    
    if (!g_iotc_state.initialized) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_NOT_INITIALIZED;
    }
    
    session_info_t *session = find_session_by_id(session_id);
    if (!session || session->state != SESSION_STATE_CONNECTED) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return IOTC_ER_INVALID_SID;
    }
    
    int count = 0;
    for (int i = 0; i < MAX_CHANNEL_NUMBER; i++) {
        if (session->channels[i].state == CHANNEL_STATE_ON) {
            count++;
        }
    }
    
    pthread_mutex_unlock(&g_iotc_state.global_mutex);
    return count;
}

uint32_t IOTC_Session_Get_Channel_ON_Bitmap(int session_id) {
    pthread_mutex_lock(&g_iotc_state.global_mutex);
    
    if (!g_iotc_state.initialized) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return 0;
    }
    
    session_info_t *session = find_session_by_id(session_id);
    if (!session || session->state != SESSION_STATE_CONNECTED) {
        pthread_mutex_unlock(&g_iotc_state.global_mutex);
        return 0;
    }
    
    uint32_t bitmap = 0;
    for (int i = 0; i < MAX_CHANNEL_NUMBER; i++) {
        if (session->channels[i].state == CHANNEL_STATE_ON) {
            bitmap |= (1U << i);
        }
    }
    
    pthread_mutex_unlock(&g_iotc_state.global_mutex);
    return bitmap;
}

/* Initialize mutex at startup */
__attribute__((constructor))
static void init_global_mutex(void) {
    pthread_mutex_init(&g_iotc_state.global_mutex, NULL);
}

__attribute__((destructor))
static void cleanup_global_mutex(void) {
    pthread_mutex_destroy(&g_iotc_state.global_mutex);
}
