
#pragma once
// #define WTSERVICE
#include <chrono>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <deque>
#include <queue>
#include <mutex>
// #include "rtc/rtc.h"

#if PLATFORM_WINDOWS || defined(__ANDROID__) || defined(__linux__) || defined(__APPLE__) || defined(__MINGW64__)

#define DEBUG_HASH false
#define sprintf_s(s, l, ...) sprintf(s, __VA_ARGS__)

#undef LOGD
#undef LOGI
#undef LOGV
#undef LOGE
extern std::mutex logMutex;
#define LOGMUTEX std::lock_guard<std::mutex> guard(logMutex);
#define LOG(...) {LOGMUTEX; fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); fflush(stdout);}
#define LOGD(...) {LOGMUTEX; fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); fflush(stdout);}
#define LOGI(...) {LOGMUTEX; fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); fflush(stdout);}
#define LOGV(...) {LOGMUTEX; fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); fflush(stdout);}
#define LOGE(...) {LOGMUTEX; fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); fflush(stdout);}

#ifdef PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include "ext/libquic/picoquicfirst/getopt.h"
// #include <WinSock2.h>
// #include <Windows.h>

#define SERVER_CA_CERT_FILE "certs\\ca-cert.pem"
#define SERVER_CA_KEY_FILE "certs\\ca-key.pem"
#define SERVER_KEY_FILE  "certs\\server-key.pem"

#define SERVER_CERT_FILE "certs\\cert.pem"
#define SERVER_KEY_FILE  "certs\\key.pem"

#else /* Linux */

#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#ifndef __USE_XOPEN2K
#define __USE_XOPEN2K
#endif
#ifndef __USE_POSIX
#define __USE_POSIX
#endif
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>

#define DEBUG_HASH false

#define SERVER_CERT_FILE "certs/cert.pem"
#define SERVER_KEY_FILE "certs/key.pem"

#endif

#include "picotls.h"
#include "picoquic/picoquic.h"
#include "picoquic/picoquic_packet_loop.h"
#include "picohttp/h3zero_common.h"
#include "picoquic/picoquic_config.h"
#include "picohttp/democlient.h"
#include "picohttp/demoserver.h"
#include "picohttp/siduck.h"
#include "picoquic/picoquic_internal.h"
#include "picohttp/pico_webtransport.h"

#define WT_SESSION_ERR_DA_YAMN 0x01 /* There is insufficient stream credit to continue the protocol */
#define WT_SESSION_ERR_BRUH 0x02 /* Received a malformed message */
#define WT_SESSION_ERR_GAME_OVER 0x03 /* All streams have been reset */
#define WT_SESSION_ERR_BORED 0x04 /* Got tired of waiting for the next message */
#define PICOQUIC_CLIENT_TICKET_STORE "wt_ticket_store.bin";
#define PICOQUIC_CLIENT_TOKEN_STORE "wt_token_store.bin";
#define PICOQUIC_CLIENT_QLOG_DIR ".";
#define WT_VERSION 1

typedef void(*wtOpenCallbackFunc)(int id, void *ptr);
typedef void (*wtMessageCallbackFunc)(int id,
				      const char* data,
				      int len,
				      void* ptr,
				      uint64_t startTime,
				      uint64_t endTime);

typedef struct st_wt_app_ctx_t {
  int status;
} wt_app_ctx_t;

typedef struct st_wt_incoming_t {
  int is_receiving;
  uint32_t expected;  /* UINT32_MAX if unknown */
  uint32_t received;
  uint32_t message_id;
  uint64_t startTime;
  bool started;
} wt_incoming_t;

typedef struct st_wt_outgoing_t {
  int is_sending;
  bool started;
  uint32_t required;  /* UINT32_MAX if unknown */
  uint32_t sent;
} wt_outgoing_t;


// #define WT_ERR_SUCCESS 0
// #define WT_ERR_INVALID -1   // invalid argument
// #define WT_ERR_FAILURE -2   // runtime error
// #define WT_ERR_NOT_AVAIL -3 // element not available
// #define WT_ERR_TOO_SMALL -4 // buffer too small

typedef enum { // Similar to libdatachannel errors
  WT_ERR_SUCCESS = 0,
  WT_ERR_INVALID = -1, // an invalid argument was provided
  WT_ERR_FAILURE = -2, // a runtime error happened
  WT_ERR_NOT_AVAIL = -3, // an element is not available at the moment
  WT_ERR_TOO_SMALL = -4, // a user-provided buffer is too small
 } wt_errors;

typedef enum {
  wt_state_none = 0,
  wt_state_ready,
  wt_state_sent,
  wt_state_sending,
  wt_state_done,
  wt_state_error,
  wt_state_closed,
  wt_state_reset
} wt_state_enum;

#define MAX_APPCHAN 256
// extern int appchan_sid_map[MAX_APPCHAN]; // appchan -> channel_id
// extern int sid_appchan_map[MAX_APPCHAN]; // stream_id -> channel_id

typedef struct st_wt_callbacks_t {
  wtOpenCallbackFunc openCallback;
  wtMessageCallbackFunc messageCallback;
  void* userData;
} wt_callbacks_t;

#define MAX_TIMES 36 // Avoid expensive logging
#define TIMES_TIME 5000
#define MAX_WT_CHANNEL 8
// const int MAX_WT_CHANNEL = 8;

const int preamble_size = sizeof(uint32_t) * 3 + sizeof(size_t);

typedef struct st_wt_buffer_t {
  std::vector<uint8_t> bytes;
  bool complete;
  bool ignore; // We know this is a bad message, probably missing start
  uint32_t sent;
  uint8_t preamble[preamble_size];
  size_t hash_value;
  void(*fn)(void*,int);
  void* userData;
  int type;
  uint64_t startTime;
  uint64_t endTime;
  st_wt_buffer_t() : bytes(0), complete(false), ignore(false), sent(0), fn(nullptr), userData(nullptr) { memset(preamble, 0, preamble_size); }
  st_wt_buffer_t(int size) : bytes(size), complete(false), ignore(false), sent(0), fn(nullptr), userData(nullptr) { memset(preamble, 0, preamble_size); }
  st_wt_buffer_t(std::vector<uint8_t>& vec) : bytes(vec), complete(false), ignore(false), sent(0),
					      fn(nullptr), userData(nullptr), type(0) {
    memset(preamble, 0, preamble_size);
  }
  st_wt_buffer_t(std::vector<uint8_t>& vec, void(*fnIn)(void*,int), void* userDataIn, int typeIn) : bytes(vec), complete(false), ignore(false), sent(0),
												    fn(fnIn), userData(userDataIn), type(typeIn) {
    memset(preamble, 0, preamble_size);
  }
  st_wt_buffer_t(const st_wt_buffer_t& buf) {
    bytes = buf.bytes;
    complete = buf.complete;
    sent = buf.sent;
    ignore = buf.ignore;
    fn = buf.fn;
    userData = buf.userData;
    type = buf.type;
    memcpy(preamble, buf.preamble, preamble_size);
  }
} wt_buffer_t;

typedef struct st_wt_channel_t {
  int id; // What channel# are we in the channel[]
  uint32_t message_id; // Message id field
  wt_state_enum state;
  wt_incoming_t incoming;
  /* Stream management */
  h3zero_stream_ctx_t* stream_ctx;
  std::queue<st_wt_buffer_t, std::deque<st_wt_buffer_t>> in;
  std::queue<st_wt_buffer_t, std::deque<st_wt_buffer_t>> out;
  void* userData;
  int maxPending;
  bool* notify;
  bool openCallbackCalled;
  wtOpenCallbackFunc openCallback;
  wtMessageCallbackFunc messageCallback;
  st_wt_channel_t() {
    id = 0;
    message_id = 0;
    state = wt_state_none;
    stream_ctx = nullptr;
    userData = nullptr;
    maxPending = 0;
    notify = nullptr;
    openCallbackCalled = 0;
    openCallback = nullptr;
    messageCallback = nullptr;
    memset(&in, sizeof(in), 0);
    memset(&out, sizeof(out), 0);
  }
} wt_channel_t;

typedef struct st_wt_ctx_t {
  picoquic_cnx_t* cnx;
  h3zero_callback_ctx_t* h3_ctx;
  std::string server_path;
  uint64_t control_stream_id;
  /* Capsule state */
  picowt_capsule_t capsule;
  /* Connection state */
  int is_client;
  int connection_ready;
  int connection_closed;
  int is_upgraded;
  uint64_t version;
  uint64_t count_fin_wait;
  uint64_t inject_error;
  wt_state_enum state;
  int nb_channels;
  wt_channel_t channels[MAX_WT_CHANNEL];
  /* Datagram management */
  int nb_datagrams_received;
  size_t nb_datagram_bytes_received;
  uint8_t datagram_received;
  int nb_datagrams_sent;
  size_t nb_datagram_bytes_sent;
  int is_datagram_ready;
  uint8_t datagram_send_next;
  uint32_t nb_bytes_received;
  uint32_t nb_bytes_sent;
  st_wt_ctx_t() {
    memset(this, sizeof(st_wt_ctx_t), 0);
  }
} wt_ctx_t;

extern char const* qlog_dir;
#define MAX_APPCHAN 256


/* Define a "post" context used to test the "post" function "end to end".
 */
typedef struct st_wt_post_t {
  size_t nb_received;
  size_t nb_sent;
  size_t response_length;
  char posted[256];
} wtserver_post_test_t;


// picohttp_server_path_item_t path_item_list[2];
// static const char* test_scenario_default;

/* Client loop call back management.
 * This is pretty complex, because the wt client is used to test a variety of interop
 * scenarios, for example:
 *
 * Variants of migration:
 * - Basic NAT traversal (1)
 * - Simple CID swap (2)
 * - Organized migration (3)
 * Encryption key rotation, after some number of packets have been sent.
 *
 * The client loop terminates when the client connection is closed.
 */

typedef struct st_client_loop_cb_t {
  picoquic_cnx_t* cnx_client;
  picoquic_demo_callback_ctx_t* wt_callback_ctx;
  siduck_ctx_t* siduck_ctx;
  int notified_ready;
  int established;
  int migration_to_preferred_started;
  int migration_to_preferred_finished;
  int migration_started;
  int address_updated;
  int force_migration;
  int nb_packets_before_key_update;
  int key_update_done;
  int zero_rtt_available;
  int is_siduck;
  int is_quicperf;
  int socket_buffer_size;
  int multipath_probe_done;
  char const* saved_alpn;
  struct sockaddr_storage server_address;
  struct sockaddr_storage client_address;
  struct sockaddr_storage client_alt_address[PICOQUIC_NB_PATH_TARGET];
  int client_alt_if[PICOQUIC_NB_PATH_TARGET];
  int nb_alt_paths;
  picoquic_connection_id_t server_cid_before_migration;
  picoquic_connection_id_t client_cid_before_migration;
} client_loop_cb_t;

char* picoquic_strsep(char** stringp, const char* delim);
int picoquic_parse_client_multipath_config(char* mp_config, int* src_if, struct sockaddr_storage* alt_ip, int* nb_alt_paths);

// class WebTransportHandler {
//  int cb() {
//    return 0;
//  }
//};

class WebTransport {
  bool done = false; // Whether processing is finished.
#if PLATFORM_WINDOWS
  FRunnableThread* wtThread;
#else
  picoquic_thread_t wtThread = {0};
#endif
  int ac = 0;
  int channelReceiveStats[4] = {0};
  uint64_t lastReceiveStatsStart;
  bool ready = false;
  int just_once;
  int first_connection_seen;
  int connection_done;
  
  picoquic_quic_config_t config;
  std::string certPath;  // These are passed via .c_str() so keep them allocated.
  std::string keyPath;
  std::string wwwDir;
  std::string logPath;
  std::string logFile;
  char option_string[512];
  int opt;
  
  std::string server_name;
  int server_port = default_server_port;
  std::string path;
  char default_server_cert_file[512];
  char default_server_key_file[512];
  char* client_scenario = NULL;
  int nb_packets_before_update = 0;
  int force_migration = 0;
  int is_client = 0;
  int ret;
  
  wt_callbacks_t wt_callbacks[MAX_WT_CHANNEL];
  int wt_callbacks_count = 0;
  const int default_server_port = 4443;
  const char* default_server_name = "::";
  const char* ticket_store_filename = "wt_ticket_store.bin";
  const char* token_store_filename = "wt_token_store.bin";
  const char* server_cert_file = "certs/cert.pem";
  const char* server_key_file = "certs/key.pem";
  
  char const* qlog_dir;
  void resetMaps() {
    for (int i = 0; i < MAX_APPCHAN; i++) {
      appchan_sid_map[i] = -1;
      sid_appchan_map[i] = -1;
    }
  }
  static const int trackedTimes = 4;
  int channelSendStats[trackedTimes];
  std::vector<std::vector<int64_t>> msgTimesSend;
  std::vector<std::vector<int64_t>> msgTimesSendEnd;
  std::vector<std::vector<int64_t>> msgTimesRecv;
  uint64_t lastSendStatsStart;
  
  picohttp_server_path_item_t path_item_list[2] =
    {
      {
	"/post",
	5,
	post_callback,
	NULL
      },
      {
	"/baton",
	6,
	service_callback,
	NULL
      }
    };
 
#if ANDROID
  extern JNIEnv* g_env;
  extern android_app* g_app;
#endif
  
  pthread_once_t g_jni_ptr_once;
  
#ifdef PLATFORM_WINDOWS
  // WSADATA wsaData = { 0 };
  // (void)WSA_START(MAKEWORD(2, 2), &wsaData);
#endif
  
public:
  int appchan_sid_map[MAX_APPCHAN] = { 0 };
  int sid_appchan_map[MAX_APPCHAN] = { 0 }; // stream_id -> channel_id
  std::mutex mutex;
  std::mutex vector_mutex;
  wt_ctx_t ctx;
  picoquic_network_thread_ctx_t* thread_ctx;
  picoquic_packet_loop_param_t packet_loop_param;
  WebTransport() {
    lastReceiveStatsStart = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    memset(&channelSendStats, 0, sizeof(channelSendStats));
    msgTimesSend.resize(trackedTimes);
    msgTimesSendEnd.resize(trackedTimes);
    msgTimesRecv.resize(trackedTimes);
    g_jni_ptr_once = PTHREAD_ONCE_INIT;
  }
  int server(std::string server_name, int port, const std::string cloudPluginDir, const std::string logDir);
  bool ctx_ready();
  void ctx_init();
  
  int set_callbacks(int channel, wtOpenCallbackFunc ocb, wtMessageCallbackFunc mcb, void* userData);
  int send_message(int appchan, char const* bytes, int len);
  int send(int appchan, char const* bytes) {
    return send_message(appchan, bytes, -1);
  }
  int send(int appchan, char const* bytes, int len) {
    return send_message(appchan, bytes, len);
  }
  int send_message(int appchan, char const* bytes, int len, bool keep);
  int send_message_full(int appchan, char const* bytes, int len, bool keep, int maxPending, bool* notify,
			void(*fn)(void*,int), void*userData, int type);
  int receive(h3zero_stream_ctx_t* stream_ctx, int appchan, wt_channel_t* channel);
  int receive_message(int appchan, char* bytesOut, int *lenInOut);
  int send_queue_size(int appchan);
  int receive_queue_size(int appchan);
  
  int client(char const * server_name, int server_port, char const * path);
  static int post_callback(picoquic_cnx_t* cnx, uint8_t* bytes, size_t length,
		  picohttp_call_back_event_t event, h3zero_stream_ctx_t* stream_ctx,
		  void* callback_ctx);
  void join() { // Wait for I/O thread to exit.
    // picoquic_wait_thread(wtThread);
    while (!done) sleep(1);
  }
  static int server_loop_cb(picoquic_quic_t* quic, picoquic_packet_loop_cb_enum cb_mode,
			     void* callback_ctx, void* callback_arg);
  static int client_loop_cb(picoquic_quic_t* quic, picoquic_packet_loop_cb_enum cb_mode,
		     void* callback_ctx, void* callback_arg);
  static int client_callback(st_picoquic_cnx_t* cnx, uint8_t* bytes, size_t length,
		      picohttp_call_back_event_t wt_event,
		      struct st_h3zero_stream_ctx_t* stream_ctx,
		      void* path_app_ctx);
  static int service_callback(st_picoquic_cnx_t* cnx, uint8_t* bytes, size_t length,
			      picohttp_call_back_event_t wt_event,
			      struct st_h3zero_stream_ctx_t* stream_ctx,
			      void* path_app_ctx);
  static picoquic_thread_return_t server_thread_loop(void* that);
  // int ctx_init(h3zero_callback_ctx_t* h3_ctx, h3zero_stream_ctx_t* stream_ctx);
  int ctx_path_params(const uint8_t* path, size_t path_length);
  int app_ctx_init_hide(h3zero_callback_ctx_t* h3_ctx, h3zero_stream_ctx_t* stream_ctx);
  int app_ctx_init(h3zero_callback_ctx_t* h3_ctx, h3zero_stream_ctx_t* stream_ctx);
  int accept_client(uint8_t* path, size_t path_length,
		    struct st_h3zero_stream_ctx_t* stream_ctx);
  int accept_service(picoquic_cnx_t* cnx,
		     uint8_t* path, size_t path_length,
		     struct st_h3zero_stream_ctx_t* stream_ctx);
  h3zero_stream_ctx_t* find_stream(uint64_t stream_id);
  int close_session(uint32_t err, char const * err_msg);
  int stream_reset(h3zero_stream_ctx_t* stream_ctx);
  void unlink_context(struct st_h3zero_stream_ctx_t* control_stream_ctx);
  int receive_datagram(const uint8_t* bytes, size_t length,
		       struct st_h3zero_stream_ctx_t* stream_ctx);
  void  print_address(FILE* F_log, struct sockaddr* address, char* label, picoquic_connection_id_t cnx_id);
  int provide_datagram(uint8_t* bytes,size_t space, struct st_h3zero_stream_ctx_t* stream_ctx);
  int connecting(h3zero_stream_ctx_t* stream_ctx);
  int check(h3zero_stream_ctx_t* stream_ctx, uint8_t received);
  int incoming_data(st_h3zero_stream_ctx_t* stream_ctx,
		    int32_t appchan, wt_channel_t* channelp,
		    const uint8_t* messageBytes, size_t messageLength);
  int stream_data(uint8_t* bytes, size_t length, int is_fin,
		  struct st_h3zero_stream_ctx_t* stream_ctx);
  int provide_data(size_t space, struct st_h3zero_stream_ctx_t* stream_ctx);
  int callback(picoquic_cnx_t* cnx,
	       uint8_t* bytes, size_t length,
	       picohttp_call_back_event_t wt_event,
	       struct st_h3zero_stream_ctx_t* stream_ctx,
	       void* path_app_ctx);
  int process_remote_stream(uint64_t stream_id, uint8_t* bytes, size_t length,
			    picoquic_call_back_event_t fin_or_event,
			    h3zero_stream_ctx_t* stream_ctx);
  void set_receive_ready();
  void set_channel_priority(int sid);
  int start_streams();
  // /*picoquic_cnx_t * cnx, wt_ctx_t* ctx, h3zero_callback_ctx_t* h3_ctx*/);
  int connect_to(std::string server);
  int connect_control(h3zero_callback_ctx_t* h3_ctx);
  void update_callbacks();
  int close(int appchan);

};

  size_t quic_server_callback_select_alpn(picoquic_quic_t* quic, ptls_iovec_t* list, size_t count);
  int quic_server_callback(picoquic_cnx_t* cnx,
			   uint64_t stream_id, uint8_t* bytes, size_t length,
			   picoquic_call_back_event_t fin_or_event, void* callback_ctx, void* v_stream_ctx);

#endif
