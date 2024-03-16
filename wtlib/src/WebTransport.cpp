#if PLATFORM_WINDOWS || defined(__ANDROID__) || defined(__linux__) || defined(__APPLE__) || defined(__MINGW64__)

#define __STDC_FORMAT_MACROS

#include <chrono>
#include <cstring>
#include <cstddef>
// #include <format>
#include <functional>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <mutex>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <unordered_set>

#if PLATFORM_WINDOWS
#include <uLog.h>
#include <filesystem>
#include "HAL/RunnableThread.h"
#include "GenericPlatform/GenericPlatformAffinity.h"
#endif

using namespace std::chrono;

// #include "common.h"
// #include "stream.h"

#include "WebTransport.hpp"

#ifndef PRIu64
#define PRIu64 "lu"
#endif
#ifndef PRIu32
#define PRIu32 "u"
#endif

#include "loglib/autoqlog.h"
#include "picoquic/picoquic_utils.h"
#include "picohttp/h3zero.h"
// #include "picoquic/picosocks.h"
#include "picohttp/pico_webtransport.h"
#include "picohttp/quicperf.h"
#include "picoquic/picoquic_unified_log.h"
#include "picoquic/picoquic_logger.h"
#include "picoquic/picoquic_binlog.h"
#include "picoquic/performance_log.h"
#include "picoquic/picoquic_lb.h"
#include "picoquic/tls_api.h"
#include "picohttp/h3zero_uri.h"

#if ANDROID
#include <android_native_app_glue.h>
#include "AndroidHelper.hpp"
#include "AndroidLogging.hpp"
#include "jni.h"
#include "AndroidJNIHelper.hpp"

#define LOGD LOG
#define LOGE LOG

#undef VNLogDebug
#undef VNLogError
#undef VNLogInfo
#define VNLogDebug LOGD
#define VNLogError LOGE
#define VNLogInfo  LOGI
#endif

#ifndef PICOQUIC_VERSION
#define PICOQUIC_VERSION  "1.1.14.0"
#endif

int WebTransport::close(int appchan) {
}

bool wt_zeros(const char* bytes, int len);
void dumpBytes(const char* msg, int appchan, int length, int size, int offset, const uint8_t* bp, bool all);
int isFileReadable(std::string filename);
bool wt_zeros(const char* bytes, int len);

int WebTransport::set_callbacks(int channel, wtOpenCallbackFunc ocb, wtMessageCallbackFunc mcb, void* userData) {
  if (channel < 0 || channel > 8) return -1;
  wt_callbacks[channel].openCallback = ocb;
  wt_callbacks[channel].messageCallback = mcb;
  wt_callbacks[channel].userData = userData;
  update_callbacks();
  return 0;
}

int WebTransport::send_queue_size(int appchan) {
  wt_channel_t& channel = ctx.channels[appchan];
  // if (channel.state == wt_state_none) return -1;
  std::lock_guard<std::mutex> guard(vector_mutex);
  return channel.out.size();
}

int WebTransport::receive_queue_size(int appchan) {
  wt_channel_t& channel = ctx.channels[appchan];
  // if (channel.state == wt_state_none) return -1;
  std::lock_guard<std::mutex> guard(vector_mutex);
  return channel.in.size();
}

int WebTransport::send_message(int appchan, char const* bytes, int len) {
  return send_message_full(appchan, bytes, len, true, 0, nullptr, nullptr, nullptr, 0);
}
int WebTransport::send_message(int appchan, char const* bytes, int len, bool keep) {
  return send_message_full(appchan, bytes, len, keep, 0, nullptr, nullptr, nullptr, 0);
}

int WebTransport::send_message_full(int appchan, char const* bytes, int len, bool keep,
				    int maxPending, bool* notify,
				    void (*fn)(void*,int), void* userData, int type) {
  std::lock_guard<std::mutex> guard(vector_mutex);
  if (wt_zeros(bytes, len)) {
    LOGE("wt: wt_send_message_full chan:%d Sending all zeros requested.", appchan);
    return 0;
  }
  uint64_t now = duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
  if (appchan < 4) {
    channelSendStats[appchan]++;
    msgTimesSend[appchan].push_back(now);
  }
  // gac = ac;
  if (appchan > MAX_WT_CHANNEL) return 0;
  if (len < 0) len = strlen(bytes);
  wt_channel_t& channel = ctx.channels[appchan];
  if (channel.state == wt_state_none) return -1;
  if (channel.state != wt_state_sending && channel.state != wt_state_ready) {
    LOGE("wt: wt_send_message: Invalid state %d", (int)(len < 0 ? strlen(bytes) : len));
    return 0;
  }
  static uint64_t last = 0;
  channel.maxPending = maxPending;
  channel.notify = notify;
  if (maxPending) while (channel.out.size() >= maxPending) {
      if (appchan == 1 && channel.out.size() > 5) {
	LOG("wt_send_message: dropping part of: %d", (int)channel.out.size());
      }
      // LOG("wt: Popping excess message from channel:%d", appchan);
      // channel.out.pop();
      if (notify) *notify = true;
      if (now - last > 1000)
	{
	  if (appchan == 2)
	    LOG("wt: wt_send_message: chan:%d output full, skipping message", appchan);
	  last = now;
	}
      if (appchan < 4) { // Dropping msg, so fix up stats.
	msgTimesSend[appchan].push_back(-now);
	msgTimesSendEnd[appchan].push_back(now);
      }
      return 0; // Just don't add the new one.  Can't safely interrupt a message being sent.
    }
  if (appchan < 4)
    msgTimesSend[appchan].push_back(now);
  if (len < 0) {
    // LOGD("wt: wt_send_message appchan: %d asking to send: %d %s", appchan, (int)strlen(bytes), bytes);
  }
  else {
    // LOGD("wt: wt_send_message appchan: %d asking to send: %d", appchan, (int)len);
  }
  
  // std::lock_guard<std::mutex> guard(vector_mutex);
  if (channel.out.size() > 40) {
    static uint64_t last = 0;
    if (now - last > 1000)
      {
	if (appchan == 2)
	  LOGD("wt_send_message: chan:%d output full, skipping message", appchan);
	last = now;
      }
    return 0;
    LOGD("wt_send_message: %d messages in out queue for channel %d",
	 (int)channel.out.size(), appchan);
  }
  // int sid = appchan_sid_map[appchan];
  // LOGD("wt: wt_send_message asking to send: %d bytes for appchan %d to index %d via sid %d",
  //		 (int)len, appchan, chan, sid);
  if (true || keep) {
    uint32_t size = len;
    std::vector<uint8_t> vec(0);
    wt_buffer_t buffer(vec, fn, userData, type);
    channel.out.push(buffer);
    wt_buffer_t& buff = channel.out.back();
    buff.bytes.resize(size);
    memcpy(buff.bytes.data(), bytes, len);
    // dumpBytes("out", appchan, len, size, vec.data(), false);
    buff.complete = true;
  }
  return 0;
}

bool WebTransport::ctx_ready() {
  if (!ready || !ctx.nb_channels /* || !ctx->is_upgraded */
      || (ctx.state != wt_state_ready && ctx.state != wt_state_sending)) return false;
  return true;
}

int WebTransport::receive_message(int appchan, char* bytesOut, int *lenInOut) {
  if (!ctx_ready()) return WT_ERR_NOT_AVAIL;
  std::lock_guard<std::mutex> guard(vector_mutex);
  wt_channel_t& channel = ctx.channels[appchan];
  if (channel.in.size() > 1000000) {
    LOG("wt_receive_message: channel.in bad, resetting");
    memset(&channel.in, 0, sizeof(channel.in));
  }
  if (channel.in.size() == 0 || !channel.in.front().complete) { // Not ready yet.
    *lenInOut = 0;
    return WT_ERR_NOT_AVAIL;
  }
  std::vector<uint8_t> buffer = channel.in.front().bytes;
  int count = buffer.size();
  if (count > *lenInOut) count = *lenInOut;
  memcpy(bytesOut, buffer.data(), count);
  channel.in.pop();
  *lenInOut = count;
  if (wt_zeros(bytesOut, count)) {
    LOGE("wt: wt_receive_message chan:%d Received all zeros.", appchan);
    return 0;
  }
  
  return 0;
}

/*
 * Receive data from a WebTransport channel
 */
int WebTransport::receive(h3zero_stream_ctx_t* stream_ctx, int appchan, wt_channel_t* channel)
{
  // gac=ac;
  if (appchan < 4) {
    channelReceiveStats[appchan]++;
    uint64_t now = duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
    msgTimesRecv[appchan].push_back(now);
    if (MAX_TIMES > 0 && now - lastReceiveStatsStart > TIMES_TIME) {
      lastReceiveStatsStart = now;
      lastSendStatsStart = now;
      LOGI("wt: wt_receive last second: Received: control:%d video:%d audio:%d hmd:%d   Sent: control:%d video:%d audio:%d hmd:%d"
	   , (int)(channelReceiveStats[0] / (TIMES_TIME/1000.0))
	   , (int)(channelReceiveStats[1] / (TIMES_TIME/1000.0))
	   , (int)(channelReceiveStats[2] / (TIMES_TIME/1000.0))
	   , (int)(channelReceiveStats[3] / (TIMES_TIME/1000.0))
	   , (int)(channelSendStats[0] / (TIMES_TIME/1000.0))
	   , (int)(channelSendStats[1] / (TIMES_TIME/1000.0))
	   , (int)(channelSendStats[2] / (TIMES_TIME/1000.0))
	   , (int)(channelSendStats[3] / (TIMES_TIME/1000.0)));
      for (int ch = 0; ch < 4; ch++)
	{
	  if (msgTimesSend[ch].size() == 0) continue;
	  if (msgTimesSend[ch].size() - msgTimesSendEnd[ch].size() > 4) {
	    LOGD("wt: send:%d length mismatch: Sent:%d Ended:%d", ch,
		 (int)msgTimesSend[ch].size(), (int)msgTimesSendEnd[ch].size());
	  }
	  std::string s = "";
	  for (int i = 1; i < msgTimesSend[ch].size() && i < MAX_TIMES; i++)
	    {
	      char sb[64];
	      bool dropped = false;
	      int64_t send = msgTimesSend[ch][i];
	      if (send < 0) {
		send = -send;
		dropped = true;
	      }
	      int64_t lastSend = msgTimesSend[ch][i - 1] < 0 ? -msgTimesSend[ch][i - 1] : msgTimesSend[ch][i - 1];
	      sprintf(sb, "%2u%s", (uint32_t)(send - lastSend), dropped ? "#" : " ");
	      s += sb;
	    }
	  LOGD("wt: send:%d %s", ch, s.c_str());
	  s = "";
	  for (int i = 1; i < msgTimesSendEnd[ch].size() && i < MAX_TIMES; i++)
	    {
	      char sb[64];
	      int64_t send = msgTimesSend[ch][i] < 0 ? -msgTimesSend[ch][i] : msgTimesSend[ch][i];
	      sprintf_s(sb, sizeof(sb), "%2u ", (uint32_t)(msgTimesSendEnd[ch][i] - send));
	      s += sb;
	    }
	  LOGD("wt: sEnd:%d %s", ch, s.c_str());
	  msgTimesSend[ch].resize(0);
	  msgTimesSendEnd[ch].resize(0);
	}
      for (int ch = 0; ch < 4; ch++)
	{
	  if (msgTimesRecv[ch].size() == 0) continue;
	  std::string s = "";
	  for (int i = 1; i < msgTimesRecv[ch].size() && i < MAX_TIMES; i++)
	    {
	      char sb[64];
	      sprintf_s(sb, sizeof(sb), "%2lu ", msgTimesRecv[ch][i] - msgTimesRecv[ch][i-1]);
	      s += sb;
	    }
	  LOGD("wt: recv:%d %s", ch, s.c_str());
	  msgTimesRecv[ch].resize(0);
	}
      
      memset(&channelReceiveStats, 0, sizeof(channelReceiveStats));
      memset(&channelSendStats, 0, sizeof(channelSendStats));
    }
  }

  // std::lock_guard<std::mutex> guard(vector_mutex);
  uint8_t* buffer = channel->in.front().bytes.data();
  int len = channel->in.front().bytes.size();
  if (!channel->in.size() || !channel->in.front().complete) return 0; // Not ready yet.
  if (appchan == 2) { // Audio: Temporarily prevent build up of messages when waiting for video.
    // while (channel->in.size() > 10) channel->in.pop();
  }
  
  int ret = 0;
  if (len) {
    if (len == 4 && !strncmp((const char*)buffer, "open", 4)) {
      // pc_opened(appchan, NULL);
      if (channel->openCallback && !channel->openCallbackCalled) {
	channel->openCallbackCalled = true;
	(*channel->openCallback)(appchan, channel->userData);
      }
      send_message(appchan, "open", 4);
      channel->in.pop();
      return 0;
    }
    else
      {
	// if (appchan == 0 && !strstr((char*)buffer, "\"times\""))
	//	LOGD("msg: %s", buffer);
	if (channel->messageCallback)
	  {
	    wt_buffer_t& buff = channel->in.front();
	    (*channel->messageCallback)(appchan, (const char*)buffer, len, channel->userData,
					buff.startTime, buff.endTime);
	    channel->in.pop();
	  } // else: data is already in queue
      }
  }
  return ret;
}

// Fast hash function
uint64_t fnv64(const uint8_t* pb, const uint8_t* pbend)
{
  const uint64_t MagicPrime = UINT64_C(0x00000100000001B3);
  uint64_t Hash = UINT64_C(0xCBF29CE484222325);
  for (; pb < pbend; pb++)
    Hash = (Hash ^ (*pb)) * MagicPrime;
  return Hash;
}

/*
 * Send data to a WebTransport channel.
 *
 * The provide data function assumes that the wt header has been sent already.
 * Process the FIN of a stream.
 */

int WebTransport::provide_data(size_t space, struct st_h3zero_stream_ctx_t* stream_ctx)
{
  bool debugBytes = false;
  static bool debugOnce = true;
  if (debugOnce) {
    debugBytes = true;
    debugOnce = false;
  }
  int ret = 0;
  if (sid_appchan_map[stream_ctx->stream_id] < 0) return 0; // Too early to send data.
  int32_t appchan = sid_appchan_map[stream_ctx->stream_id];
  if (appchan < 0) {
    LOGE("wt: wt_provide_data appchan not known for stream_id:%d", (int)stream_ctx->stream_id);
    (void)picoquic_provide_stream_data_buffer(&stream_ctx, 0, 0, 0);
    return 0;
  }
  wt_channel_t& channel = ctx.channels[appchan];
  
  std::lock_guard<std::mutex> guard(vector_mutex);
  if (channel.out.empty()) {
    (void)picoquic_provide_stream_data_buffer(&stream_ctx, 0, 0, 0);
    return 0;
  }
  if ((ret == 0) && channel.out.front().complete && (channel.state == wt_state_sending || channel.state == wt_state_ready)) {
    wt_buffer_t& buffer = channel.out.front();
    std::vector<uint8_t>& outData = channel.out.front().bytes;
    uint32_t required = outData.size();
    uint32_t sent = buffer.sent;
    uint32_t header = preamble_size;
    int offset = sent; // Remember where we started
    if (sent == 0) {
      memcpy(buffer.preamble, &appchan, sizeof(uint32_t)); // appchan:32, totalSize:32
      memcpy(buffer.preamble + sizeof(uint32_t), &required, sizeof(uint32_t));
      memcpy(buffer.preamble + sizeof(uint32_t) * 2, &channel.message_id, sizeof(channel.message_id));
      channel.message_id++;
      buffer.hash_value = DEBUG_HASH ? fnv64(outData.data(), outData.data() + outData.size()) : 0;
      memcpy(buffer.preamble + sizeof(uint32_t) * 3, &buffer.hash_value, sizeof(buffer.hash_value));
    }
    int more_to_send = channel.out.size() > 1 ? 1 : 0;
    size_t use = required + header - sent;
    if (use > space) {
      more_to_send = 1;
      use = space;
    }
    int sending = use;
    uint8_t* output = picoquic_provide_stream_data_buffer(&stream_ctx, use, 0, more_to_send);
    if (!output) {
      LOGE("provide_data: provide_stream_data_buffer returned null");
      ret = 1;
    }
    int headerConsumed = 0;
    if (!ret && sent < header) {
      int pre = header - sent;
      if (pre > use) pre = use;
      memcpy(output, buffer.preamble + sent, pre);
      sent += pre;
      headerConsumed += pre;
      use -= pre;
    }
    int consumed = 0;
    if (!ret && sent >= header && use > 0) {
      memcpy(output + headerConsumed, outData.data() + sent - header, use);
      consumed += use;
      sent += use;
      if (debugBytes && appchan == 1) {
	LOG("wt: wt_provide_data Sending %d at offset %d of %d = %d on chan:message_id: %d %d hash:%zu",
	    (int)sending - headerConsumed, offset, (int)outData.size(), sent, appchan, channel.message_id - 1, buffer.hash_value);
	// dumpBytes("out start", appchan, required, outData.size(), offset, outData.data(), true);
	// dumpBytes("out", appchan, required, outData.size(), offset, output, true);
      }
    }
    buffer.sent = sent;
    ctx.nb_bytes_sent += headerConsumed + consumed;
    
    if (!ret && sent == required + header) {
      uint64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
      if (msgTimesSend[appchan].size())
	msgTimesSendEnd[appchan].push_back(now);
      wt_buffer_t& buff = channel.out.front();
      if (buff.fn) buff.fn(buff.userData, buff.type);
      channel.out.pop();
    }
    
  }
  else {
    (void)picoquic_provide_stream_data_buffer(&stream_ctx, 0, 0, 0);
  }
  
  return ret;
}


/* Initialize the content of a wt context.
 * TODO: replace internal pointers by pointer to h3zero context
 */
int WebTransport::app_ctx_init(h3zero_callback_ctx_t* h3_ctx, h3zero_stream_ctx_t* stream_ctx)
{
  int ret = 0;
  ctx.nb_channels = 0;
  for (int i = 0; i < MAX_WT_CHANNEL; i++) {
    ctx.channels[i].id = i;
    if (!ctx.channels[i].openCallback)
      ctx.channels[i].openCallback = [](int id, void* st) {
	LOGD("wt: channel %d open", id);
      };
    if (!ctx.channels[i].messageCallback)
      ctx.channels[i].messageCallback = [](int id, const char* message, int size, void* st,
					    uint64_t startTime, uint64_t endTime) {
	LOGD("wt: message received on channel %d", id);
      };
  }
  /* Init the stream tree */
  /* Do we use the path table for the client? or the web folder? */
  /* connection wide tracking of stream prefixes */
  if (h3_ctx == NULL) {
    ret = -1;
  }
  else {
    ctx.h3_ctx = h3_ctx;
    
    /* Connection flags connection_ready and connection_closed are left
     * to zero by default. */
    /* init the protocol will be done in the "accept" call for server */
    
    if (stream_ctx != NULL) {
      /* Register the control stream and the stream id */
      ctx.control_stream_id = stream_ctx->stream_id;
      stream_ctx->ps.stream_state.control_stream_id = stream_ctx->stream_id;
      stream_ctx->path_callback_ctx = this;
      ret = h3zero_declare_stream_prefix(ctx.h3_ctx, stream_ctx->stream_id, client_callback, this);
    }
    else {
      /* Poison the control stream ID field so errors can be detected. */
      ctx.control_stream_id = UINT64_MAX;
    }
  }
  
  if (ret != 0) {
    /* Todo: undo init. */
  }
  return ret;
}


int WebTransport::app_ctx_init_hide(h3zero_callback_ctx_t* h3_ctx, h3zero_stream_ctx_t* stream_ctx)
{
  int ret = 0;
  
  /* Init the stream tree */
  /* Do we use the path table for the client? or the web folder? */
  /* connection wide tracking of stream prefixes */
  if (h3_ctx == NULL) {
    ret = -1;
  }
  else {
    ctx.h3_ctx = h3_ctx;
    
    /* Connection flags connection_ready and connection_closed are left
     * to zero by default. */
    /* init the protocol will be done in the "accept" call for server */
    
    if (stream_ctx != NULL) {
      /* Register the control stream and the stream id */
      ctx.control_stream_id = stream_ctx->stream_id;
      stream_ctx->ps.stream_state.control_stream_id = stream_ctx->stream_id;
      stream_ctx->path_callback_ctx = this;
      ret = h3zero_declare_stream_prefix(ctx.h3_ctx, stream_ctx->stream_id, client_callback, &ctx);
    }
    else {
      /* Poison the control stream ID field so errors can be detected. */
      ctx.control_stream_id = UINT64_MAX;
    }
  }
  
  if (ret != 0) {
    /* Todo: undo init. */
  }
  return ret;
}


/* Client:
 * - Create the QUIC context.
 * - Open the sockets
 * - Find the server's address
 * - Create a client context and a client connection.
 * - On a forever loop:
 *     - get the next wakeup time
 *     - wait for arrival of message on sockets until that time
 *     - if a message arrives, process it.
 *     - else, check whether there is something to send.
 *       if there is, send it.
 * - The loop breaks if the client connection is finished.
 */


int WebTransport::ctx_path_params(const uint8_t* path, size_t path_length)
{
  int ret = 0;
  size_t query_offset = h3zero_query_offset(path, path_length);
  if (query_offset < path_length) {
    const uint8_t* queries = path + query_offset;
    size_t queries_length = path_length - query_offset;
    
    if (h3zero_query_parameter_number(queries, queries_length, "version", 5, &ctx.version, 0) != 0 ||
	h3zero_query_parameter_number(queries, queries_length, "inject", 6, &ctx.inject_error, 0) != 0) {
      ret = -1;
    }
    else if ( ctx.version != WT_VERSION ) {
      ret = -1;
    }
  } else {
    /* Set parameters to default values */
  }
  
  return ret;
}

void WebTransport::set_channel_priority(int sid) {
  int chan = sid_appchan_map[sid];
  const int priorities[] = { 2, 6, 1, 1 };
  if (chan < sizeof(priorities) / sizeof(int))
    picoquic_set_stream_priority(ctx.cnx, sid, priorities[chan]);
}

/* Accept an incoming connection */
int WebTransport::accept_client(uint8_t* path, size_t path_length,
				struct st_h3zero_stream_ctx_t* stream_ctx)
{
  int ret = 0;
  h3zero_callback_ctx_t* h3_ctx = (h3zero_callback_ctx_t*)picoquic_get_callback_context(ctx.cnx);
  {
    update_callbacks();
    /* register the incoming stream ID */
    ret = app_ctx_init(h3_ctx, stream_ctx);
    
    /* init the global parameters */
    if (path != NULL && path_length > 0) {
      ret = ctx_path_params(path, path_length);
    }
    stream_ctx->ps.stream_state.is_web_transport = 1;
    stream_ctx->path_callback = client_callback;
    stream_ctx->path_callback_ctx = this;
    ctx.connection_ready = 1;
    for (int i = 0; i < MAX_WT_CHANNEL; i++) {
      ctx.channels[i].openCallback = wt_callbacks[i].openCallback;
      ctx.channels[i].messageCallback = wt_callbacks[i].messageCallback;
      ctx.channels[i].userData = wt_callbacks[i].userData;
      ctx.channels[i].message_id = 0;
      ctx.channels[i].id = i;
    }
    if (ret == 0) {
      for (int chan = 0; chan < MAX_WT_CHANNEL; chan++) {
	if ((stream_ctx =
	     picowt_create_local_stream(ctx.cnx, 1, ctx.h3_ctx,
					ctx.control_stream_id)) == NULL) {
	  ret = -1;
	  break;
	}
	int32_t sid = stream_ctx->stream_id;
	appchan_sid_map[chan] = sid;
	sid_appchan_map[sid] = chan;
	set_channel_priority(sid);
	ctx.nb_channels += 1;
	auto& channel = ctx.channels[chan];
	channel.id = chan;
	channel.message_id = 0;
	channel.incoming.message_id = 0;
	channel.state = wt_state_sending;
	channel.stream_ctx = stream_ctx;
	if (ctx.channels[chan].openCallback) {
	  (*ctx.channels[chan].openCallback)(
					     chan, ctx.channels[chan].userData);
	  channel.openCallbackCalled = true;
	}
	stream_ctx->path_callback = client_callback;
	stream_ctx->path_callback_ctx = this;
	send_message(chan, "open", 4, true);
	ret = picoquic_mark_active_stream(ctx.cnx, sid, 1, stream_ctx);
      }
    }
    ready = true;
  }
  return ret;
}

h3zero_stream_ctx_t* WebTransport::find_stream(uint64_t stream_id)
{
  h3zero_stream_ctx_t* stream_ctx = h3zero_find_stream(ctx.h3_ctx, stream_id);
  return stream_ctx;
}

/* Close the session. */
int WebTransport::close_session(uint32_t err, char const * err_msg)
{
  int ret = 0;
  
  h3zero_stream_ctx_t* stream_ctx = find_stream(ctx.control_stream_id);
  
  LOGD("wt: Closing session control stream %" PRIu64, ctx.control_stream_id);
  
  if (stream_ctx != NULL && !stream_ctx->ps.stream_state.is_fin_sent) {
    if (err_msg == NULL) {
      switch (err) {
      case 0:
	err_msg = "Have a nice day";
	break;
      case WT_SESSION_ERR_DA_YAMN:
	err_msg = "There is insufficient stream credit to continue the protocol";
	break;
      case WT_SESSION_ERR_BRUH:
	err_msg = "Received a malformed message";
	break;
      case WT_SESSION_ERR_GAME_OVER:
	err_msg = "All streams have been reset";
	break;
      case WT_SESSION_ERR_BORED:
	err_msg = "Got tired of waiting for the next message";
	break;
      default:
	break;
      }
    }
    LOGD("wt: close_session: %s", err_msg);
    ret = picowt_send_close_session_message(ctx.cnx, stream_ctx, err, err_msg);
    ctx.state = wt_state_closed;
    // pc_close_all();
  }
  return(ret);
}

int WebTransport::stream_reset(h3zero_stream_ctx_t* stream_ctx)
{
  int ret = 0;
  if (sid_appchan_map[stream_ctx->stream_id] < 0) return 0; // Not set up yet.
  int32_t appchan = sid_appchan_map[stream_ctx->stream_id];
  wt_channel_t* channel = appchan >= 0 ? &ctx.channels[appchan] : NULL;
  if (channel) {
    channel->state = wt_state_none;
    // while (channel->in.size()) channel->in.pop();
    // while (channel->out.size()) channel->out.pop();
    // channel->openCallback = NULL;
    // channel->messageCallback = NULL;
    channel->openCallbackCalled = false;
  }
  
  picoquic_log_app_message(ctx.cnx, "wt: Received reset on stream %" PRIu64 ", closing the session", stream_ctx->stream_id);
  LOGD("wt: Received reset on stream %" PRIu64 ", closing the session", stream_ctx->stream_id);
  
  if (&ctx != NULL) {
    LOGD("wt: stream_reset closing stream");
    ret = close_session(WT_SESSION_ERR_GAME_OVER, NULL);
    /* Any reset results in the abandon of the context */
    ctx.state = wt_state_closed;
    if (ctx.is_client) {
      ret = picoquic_close(ctx.cnx, 0);
    }
    h3zero_delete_stream_prefix(ctx.cnx, ctx.h3_ctx, ctx.control_stream_id);
  }
  
  return ret;
}

void WebTransport::unlink_context(struct st_h3zero_stream_ctx_t* control_stream_ctx)
{
  h3zero_callback_ctx_t* h3_ctx = (h3zero_callback_ctx_t*)picoquic_get_callback_context(ctx.cnx);
  picosplay_node_t* previous = NULL;
  
  /* dereference the control stream ID */
  picoquic_log_app_message(ctx.cnx, "wt: Prefix for control stream %" PRIu64 " was unregistered", control_stream_ctx->stream_id);
  LOGD("wt: Prefix for control stream %" PRIu64 " was unregistered", control_stream_ctx->stream_id);
  std::lock_guard<std::mutex> guard(vector_mutex);
  for (int i = 0; i < ctx.nb_channels; i++) {
    wt_channel_t& channel = ctx.channels[i];
    channel.openCallbackCalled = false;
    channel.state = wt_state_none;
    while (channel.in.size()) channel.in.pop();
    while (channel.out.size()) channel.out.pop();
  }
  /* Free the streams created for this session */
  while (1) {
    picosplay_node_t* next = (previous == NULL) ? picosplay_first(&h3_ctx->h3_stream_tree) : picosplay_next(previous);
    if (next == NULL) {
      break;
    }
    else {
      h3zero_stream_ctx_t* stream_ctx =
	(h3zero_stream_ctx_t*)picohttp_stream_node_value(next);
      
      if (control_stream_ctx->stream_id == stream_ctx->ps.stream_state.control_stream_id &&
	  control_stream_ctx->stream_id != stream_ctx->stream_id) {
	stream_ctx->ps.stream_state.control_stream_id = UINT64_MAX;
	stream_ctx->path_callback = NULL;
	stream_ctx->path_callback_ctx = NULL;
	picoquic_set_app_stream_ctx(ctx.cnx, stream_ctx->stream_id, NULL);
	h3zero_forget_stream(ctx.cnx, stream_ctx);
	picosplay_delete_hint(&h3_ctx->h3_stream_tree, next);
      }
      else {
	previous = next;
      }
    }
  }
  /* Then free the context, if this is not a client */
  picoquic_set_app_stream_ctx(ctx.cnx, control_stream_ctx->stream_id, NULL);
  // picowt_release_capsule(&ctx.capsule);
  memset(&ctx.capsule, sizeof(ctx.capsule), 0);
  if (!ctx.cnx->client_mode) {
    // free(ctx);
  }
  else {
    ctx.connection_closed = 1;
  }
}

/* Management of datagrams
 */
int WebTransport::receive_datagram(const uint8_t* bytes, size_t length,
				   struct st_h3zero_stream_ctx_t* stream_ctx)
{
  int ret = 0;
  const uint8_t* bytes_max = bytes + length;
  uint64_t len;
  uint8_t next_data = 0; // Todo
  
  if (stream_ctx != NULL && stream_ctx->stream_id != ctx.control_stream_id) {
    /* error, unexpected datagram on this stream */
  }
  else if ((bytes = picoquic_frames_varint_decode(bytes, bytes_max, &len)) != NULL &&
	   (bytes = picoquic_frames_fixed_skip(bytes, bytes_max, len)) != NULL &&
	   (bytes = picoquic_frames_uint8_decode(bytes, bytes_max, &next_data)) != NULL &&
	   bytes == bytes_max){
    ctx.datagram_received = next_data;
    ctx.nb_datagrams_received += 1;
    ctx.nb_datagram_bytes_received += length;
  }
  else {
    /* error, badly coded datagram */
  }
  return ret;
}

int WebTransport::provide_datagram(size_t space, struct st_h3zero_stream_ctx_t* stream_ctx)
{
  int ret = 0;
  if (ctx.is_datagram_ready) {
    if (space > 1536) {
      space = 1536;
    }
    if (space < 3) {
      /* Not enough space to send anything */
    }
    else {
      uint8_t* buffer = h3zero_provide_datagram_buffer(&ctx, space, 0);
      if (buffer == NULL) {
	ret = -1;
      }
      else {
	size_t length = space - 3;
	uint8_t* bytes = buffer;
	*bytes++ = 0x40 | (uint8_t)((length >> 8) & 0x3F);
	*bytes++ = (uint8_t)(length & 0xFF);
	memset(bytes, 0, length);
	bytes += length;
	*bytes = ctx.datagram_send_next;
	ctx.is_datagram_ready = 0;
	ctx.datagram_send_next = 0;
	ctx.nb_datagrams_sent += 1;
	ctx.nb_datagram_bytes_sent += space;
      }
    }
  }
  
  return ret;
}

/* Update context when sending a connect request */
int WebTransport::connecting(h3zero_stream_ctx_t* stream_ctx)
{
  
  picoquic_log_app_message(ctx.cnx, "wt: Outgoing connect on stream: %" PRIu64, stream_ctx->stream_id);
  LOGD("wt: Outgoing connect on stream: %" PRIu64, stream_ctx->stream_id);
  ctx.state = wt_state_ready; // wt_state_sending
  ctx.control_stream_id = stream_ctx->stream_id;
  
  return 0;
}

/*
 * Process incoming raw packet data into messages.
 * This gets called one or more times to receive all message data.
 * The offer may be for a full net packet size (1400+) or as little as 1 byte.
 */
int WebTransport::incoming_data( st_h3zero_stream_ctx_t* stream_ctx,
				int32_t appchan, wt_channel_t* channelp, const uint8_t* messageBytes,
				size_t messageLength)
{
  bool debugBytes = false;
  static bool debugOnce = true;
  if (debugOnce) {
    debugBytes = true;
    debugOnce = false;
  }
  int ret = 0;
  int pass = 0;
  int messageConsumed = 0; // Handle multiple messages: keep track of how much prev used.
  std::lock_guard<std::mutex> guard(vector_mutex);
  do {
    const size_t length = messageLength - messageConsumed;
    const uint8_t* bytes = messageBytes + messageConsumed;
    if (messageConsumed > 0) {
      LOGD("wt: wt_incoming_data Processing additional message at:%d", (int)messageConsumed);
      // dumpBytes("in extra before", appchan, length, length, 8, bytes-16, true);
      dumpBytes("in extra ", appchan, length, length, 0, bytes, true);
    }
    auto& channel  = *channelp;
    auto& incoming = channel.incoming;
    ctx.nb_bytes_received += length;
    const int header        = preamble_size;
    int headerConsumed = 0;
    // Every message starts: uint32_t appchan, uint32_t size
    
    if (!incoming.started)
      {
	incoming.is_receiving = true;
	incoming.received     = 0;
	incoming.started      = true;
	incoming.expected     = 0; // We don't know size yet.
	uint64_t now = duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
	incoming.startTime    = now; // Note when we started receiving this message.
	// If the queue is empty or it has items and the back() isn't empty, push new.
	if (channel.in.size() == 0 || (channel.in.size() && channel.in.back().bytes.size()))
	  {
	    wt_buffer_t buffer(0);
	    channel.in.push(buffer);
	  }
      }
    if (debugBytes)
      dumpBytes("in raw", appchan, length, length, incoming.received, bytes, true);
    uint32_t msgSize = 0;
    wt_buffer_t& buffer = channel.in.back();
    if (incoming.received < header)
      {
	int use = header - incoming.received;
	if (use > length)
	  use = length;
	memcpy(buffer.preamble + incoming.received, bytes, use);
	headerConsumed += use;
	incoming.received += use;
	if (incoming.received >= header) {
	  uint32_t new_message_id = ((uint32_t*)buffer.preamble)[2];
	  if (new_message_id != channel.incoming.message_id) {
	    LOGE("wt: wt_incoming_data chan: %d message_id value unexpected: new:%d old %d",
		 appchan, new_message_id, channel.incoming.message_id);
	    dumpBytes("bad mid1 ", appchan, length, length, 0, bytes, true);
	    dumpBytes("bad mid2 ", appchan, length, length, 16, bytes+16, true);
	  }
	  channel.incoming.message_id = new_message_id + 1;
	}
      }
    if (incoming.received >= header)
      {
	uint32_t msgAppchan = ((uint32_t*)buffer.preamble)[0];
	msgSize             = ((uint32_t*)buffer.preamble)[1];
	incoming.expected   = msgSize;
	// This may happen if we get a fragment of a message from a prior session.
	if (msgAppchan != appchan || msgSize == 0)
	  {
	    LOGE("wt: wt_incoming_data corrupted packet");
	    incoming.started      = false;
	    incoming.is_receiving = 0;
	    incoming.received     = 0;
	    incoming.expected     = UINT32_MAX;
	    return 0;
	  }
      }
    if (incoming.received >= header && buffer.bytes.size() != incoming.expected) {
      if (incoming.expected > 0)
	{
	  buffer.bytes.resize(incoming.expected);
	}
      else
	LOGE("wt: wt_incoming_data msgSize == 0");
    }
    msgSize = incoming.expected; // Only valid when received >= header
    int consumed = 0;
    if (incoming.received >= header)
      {
	msgSize = ((uint32_t*)buffer.preamble)[1];
	uint8_t* bufferData = buffer.bytes.data();
	int      copyNow = incoming.expected + header - incoming.received;
	if (copyNow > length - headerConsumed)
	  copyNow = length - headerConsumed;
	if (debugBytes)
	  {
	    LOGD("wt: wt_incoming_data Receiving %d at offset %d of %d = %d on chan: %d", (int)copyNow,
		 incoming.received - header, (int)msgSize, (int)length + incoming.received - header, appchan);
	  }
	memcpy(bufferData + incoming.received - header, bytes + consumed + headerConsumed, copyNow);
	consumed += copyNow;
	incoming.received += copyNow;
	ctx.nb_bytes_received += copyNow;
      }
    
    messageConsumed += consumed + headerConsumed;
    if (messageConsumed < messageLength && incoming.received != incoming.expected + header)
      LOGE("wt: wt_incoming_data Data leftover %d pass:%d", (int) messageLength - messageConsumed, pass);
    
    if (incoming.received == incoming.expected + header)
      {
	uint64_t new_hash = DEBUG_HASH ? fnv64(buffer.bytes.data(),
					       buffer.bytes.data() +
					       buffer.bytes.size()) : 0;
	uint64_t given_hash = *(uint64_t*) (& (((uint32_t*)buffer.preamble)[3]));
	if (given_hash != new_hash) {
	  LOGE("wt: wt_incoming_data chan:message_id: %d %d hash doesn't match: given:%lu vs. computed:%lu",
	       appchan, incoming.message_id, given_hash, new_hash);
	  debugBytes = true;
	}
	if (debugBytes)
	  {
	    uint8_t* bufferData = buffer.bytes.data();
	    int      bufferSize = buffer.bytes.size();
	    dumpBytes("in preamble", appchan, length, bufferSize, 0, buffer.preamble, true);
	  }
	incoming.started      = false;
	incoming.is_receiving = 0;
	incoming.received     = 0;
	incoming.expected     = UINT32_MAX;
	uint64_t now = duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
	buffer.startTime      = incoming.startTime;
	buffer.endTime        = now;
	buffer.complete       = true;
      }
    pass++;
  } while (messageConsumed < messageLength);
  return ret;
}

int WebTransport::stream_data(uint8_t* bytes, size_t length, int is_fin,
			      struct st_h3zero_stream_ctx_t* stream_ctx)
{
  int ret = 0;
  if (is_fin) {
    LOGD("wt: Fin received for stream: %lu", stream_ctx->stream_id);
  }
  
  size_t channel_id = SIZE_MAX;
  
  if (stream_ctx->stream_id == ctx.control_stream_id) {
    return 0;
    stream_ctx->ps.stream_state.is_fin_received = 1;
    if (ctx.is_client) {
      ret = picoquic_close(ctx.cnx, 0);
      return 0;
    }
    else {
      if (!stream_ctx->ps.stream_state.is_fin_sent) {
	picoquic_add_to_stream(ctx.cnx, stream_ctx->stream_id, NULL, 0, 1);
      }
      h3zero_delete_stream_prefix(ctx.cnx, ctx.h3_ctx, stream_ctx->stream_id);
    }
    return 0;
  }
  else if (stream_ctx->ps.stream_state.control_stream_id == UINT64_MAX) {
    picoquic_log_app_message(ctx.cnx, "Received FIN after close on stream %" PRIu64, stream_ctx->stream_id);
  }
  else {
    int32_t appchan = sid_appchan_map[stream_ctx->stream_id];
    bool sendOpen = false;
    if (appchan < 0)
      { // We are sure to be at the start of a message which has appchan.
	memcpy(&appchan, bytes, sizeof(uint32_t));
	if (appchan > MAX_WT_CHANNEL) return 0;
	appchan_sid_map[appchan]               = stream_ctx->stream_id;
	sid_appchan_map[stream_ctx->stream_id] = appchan;
	wt_channel_t& channel = ctx.channels[appchan];
	set_channel_priority(stream_ctx->stream_id);
	sendOpen = true;
	channel.stream_ctx = stream_ctx;
	channel.id = appchan;
	channel.message_id = 0;
	channel.incoming.message_id = 0;
	stream_ctx->ps.stream_state.is_web_transport = 1;
	stream_ctx->path_callback = service_callback;
	stream_ctx->path_callback_ctx = this;
	ctx.connection_ready = 1;
	ctx.state = wt_state_ready;
	wt_incoming_t& incoming = channel.incoming;
	incoming.is_receiving = 1;
	incoming.expected = UINT32_MAX;
	incoming.received = 0;
	incoming.started = false;
	channel.state = wt_state_sending;
	send_message(appchan, "open", 4);
      }
    wt_channel_t& channel = ctx.channels[appchan];
    channel.stream_ctx = stream_ctx;
    stream_ctx->ps.stream_state.is_web_transport = 1;
    stream_ctx->path_callback = service_callback;
    stream_ctx->path_callback_ctx = this;
    ctx.connection_ready = 1;
    ctx.state = wt_state_ready;
    channel.state = wt_state_sending;
    if (sendOpen) send_message(appchan, "open", 4, true);
    wt_incoming_t& incoming = channel.incoming;
    wt_channel_t* channelp = &channel;
    
    /* Process to receive the stream */
    if (ret == 0) {
      if (length > 0) {
	ret = incoming_data(stream_ctx, appchan, channelp, bytes, length);
      }
      /* process FIN, including doing the check */
      if (is_fin) {
	if (ctx.state != wt_state_closed) {
	  wt_incoming_t& incoming = ctx.channels[appchan].incoming;
	  if (incoming.is_receiving) {
	    if (IS_BIDIR_STREAM_ID(stream_ctx->stream_id) &&
		IS_LOCAL_STREAM_ID(stream_ctx->stream_id, ctx.is_client) &&
		length == 0 &&
		ctx.count_fin_wait > 0){
	      ctx.count_fin_wait--;
	    }
	    else {
	      picoquic_log_app_message(ctx.cnx, "Error: FIN before on data stream %" PRIu64 "\n",
				       stream_ctx->stream_id);
	      ret = close_session(WT_SESSION_ERR_BRUH, "Fin stream before ");
	    }
	  }
	  else if (ret == 0) {
	    std::lock_guard<std::mutex> guard(vector_mutex);
	    ret = receive(stream_ctx, appchan, channelp);
	    incoming.started = false;
	    incoming.expected = 0;
	  }
	}
	if (stream_ctx->ps.stream_state.is_fin_sent == 1 &&
	    (stream_ctx->ps.stream_state.is_fin_received || stream_ctx->stream_id != ctx.control_stream_id)) {
	  h3zero_callback_ctx_t* h3_ctx = (h3zero_callback_ctx_t*)picoquic_get_callback_context(ctx.cnx);
	  picoquic_set_app_stream_ctx(ctx.cnx, stream_ctx->stream_id, NULL);
	  if (h3_ctx != NULL) {
	    h3zero_delete_stream(ctx.cnx, ctx.h3_ctx, stream_ctx);
	  }
	}
      } else {
	if ((ret == 0) && channelp->in.size() && channelp->in.front().complete) {
	  if (channelp->in.front().ignore) {
	    channelp->in.pop();
	  } else
	    ret = receive(stream_ctx, appchan, channelp);
	  incoming.started = false;
	  incoming.expected = 0;
	  // channelp->sent = 0;
	}
      }
    }
  }
  
  return ret;
}


int WebTransport::start_streams() {
  int ret = 0;
  bool once = false;
  if (!once)
    {
      h3zero_stream_ctx_t* stream_ctx;
      once = false; // true;
      for (int chan = 0; chan < MAX_WT_CHANNEL; chan++)
	{
	  if ((stream_ctx = picowt_create_local_stream(ctx.cnx, 1, ctx.h3_ctx,
						       ctx.control_stream_id))
	      == NULL)
	    {
	      ret = -1;
	      break;
	    }
	  LOGD("wt: PICOQUIC creating channel: %d with streamId: %d", chan, (int)stream_ctx->stream_id);
	  ctx.nb_channels += 1;
	  int32_t sid = stream_ctx->stream_id;
	  appchan_sid_map[chan] = sid;
	  sid_appchan_map[sid] = chan;
	  auto& channel = ctx.channels[chan];
	  channel.id = chan;
	  channel.message_id = 0;
	  channel.incoming.message_id = 0;
	  channel.stream_ctx        = stream_ctx;
	  channel.state = wt_state_sending;
	  // if (cb_ctx->channels[chan].openCallback) {
	  //	(*cb_ctx->channels[chan].openCallback)(
	  //		chan, cb_ctx->channels[chan].userData);
	  //}
	  // channel.openCallbackCalled = false;
	  stream_ctx->path_callback = client_callback;
	  stream_ctx->path_callback_ctx = this;
	  send_message(chan, "open", 4, true);
	  ret = picoquic_mark_active_stream(ctx.cnx, sid, 1, stream_ctx);
	  
	  stream_ctx->path_callback     = client_callback;
	  stream_ctx->path_callback_ctx = this;
	  // ret = picoquic_mark_active_stream(cb_ctx->cnx, stream_ctx->stream_id, 1, stream_ctx);
	  send_message(chan, "open", 4, true);
	}
    }
  return ret;
}

/* Web transport callback. This will be called from the web server
 * when the path points to a web transport callback.
 * Discuss: is the stream context needed? Should it be a wt_stream_context?
 */

int WebTransport::client_callback(picoquic_cnx_t* cnx,
				  uint8_t* bytes, size_t length,
				  picohttp_call_back_event_t wt_event,
				  struct st_h3zero_stream_ctx_t* stream_ctx,
				  void* path_app_ctx)
{
  int ret = 0;
  WebTransport* wt = (WebTransport*)path_app_ctx; // callback_ctx;
  LOGD("wt_client_callback: %d, %" PRIi64 "\n", (int)wt_event, (stream_ctx == NULL)?(int64_t)-1:(int64_t)stream_ctx->stream_id);
  switch (wt_event) {
  case picohttp_callback_connecting:
    {
      LOGD("wt: client_callback connecting");
      ret       = wt->connecting(stream_ctx);
    }
    break;
  case picohttp_callback_connect:
    /* A connect has been received on this stream, and could be accepted.
     */
    /* The web transport should create a web transport connection context,
     * and also register the stream ID as identifying this context.
     * Then, callback the application. That means the WT app context
     * should be obtained from the path app context, etc.
     */
    {
      LOGD("wt: wt_client_callback connect");
      ret = wt->accept_client(bytes, length, stream_ctx);
    }
    break;
  case picohttp_callback_connect_refused:
    /* The response from the server has arrived and it is negative. The
     * application needs to close that stream.
     * Do we need an error code? Maybe pass as bytes + length.
     * Application should clean up the app context.
     */
    LOGE("wt: connect_refused");
    break;
  case picohttp_callback_connect_accepted: /* Connection request was accepted by peer */
    /* The response from the server has arrived and it is positive.
     * The application can start sending data.
     */
    {
      LOGD("wt: wt_client_callback accepted");
      if (stream_ctx != NULL)
	{
	  stream_ctx->is_upgraded = 1;
	  wt->ctx.is_upgraded = 1;
	  // if (stream_ctx.stream_id > 0)
	  //	picoquic_mark_active_stream(cnx, stream_ctx->stream_id, 1, stream_ctx);
	}
    }
    break;
    
  case picohttp_callback_post_fin:
  case picohttp_callback_post_data:
    /* Data received on a stream for which the per-app stream context is known.
     * the app just has to process the data, and process the fin bit if present.
     */
    {
      ret = wt->stream_data(bytes, length, (wt_event == picohttp_callback_post_fin),
			    stream_ctx);
    }
    break;
  case picohttp_callback_provide_data: /* Stack is ready to send chunk of response */
    /* We assume that the required stream headers have already been pushed,
     * and that the stream context is already set. Just send the data.
     */
    {
      ret = wt->provide_data(length, stream_ctx);
    }
    break;
  case picohttp_callback_post_datagram:
    /* Data received on a stream for which the per-app stream context is known.
     * the app just has to process the data.
     */
    {
      ret = wt->receive_datagram(bytes, length, stream_ctx);
    }
    break;
  case picohttp_callback_provide_datagram: /* Stack is ready to send a datagram */
    {
      ret = wt->provide_datagram(length, stream_ctx);
    }
    break;
  case picohttp_callback_reset: /* Stream has been abandoned. */
    /* If control stream: abandon the whole connection. */
    {
      ret = wt->stream_reset(stream_ctx);
    }
    break;
  case picohttp_callback_free: /* Used during clean up the stream. Only cause the freeing of memory. */
    /* Free the memory attached to the stream */
    break;
  case picohttp_callback_deregister:
    /* The app context has been removed from the registry.
     * Its references should be removed from streams belonging to this session.
     * On the client, the memory should be freed.
     */
    {
      // wt->ctx_init();
      wt->unlink_context(stream_ctx);
    }
    break;
  default:
    /* protocol error */
    ret = -1;
    break;
  }
  return ret;
}


int WebTransport::process_remote_stream(uint64_t stream_id, uint8_t* bytes, size_t length,
					picoquic_call_back_event_t fin_or_event,
					h3zero_stream_ctx_t* stream_ctx)
{
  int ret = 0;
  
  if (stream_ctx == NULL) {
    stream_ctx = h3zero_find_or_create_stream(ctx.cnx, stream_id, ctx.h3_ctx, 1, 1);
    picoquic_set_app_stream_ctx(ctx.cnx, stream_id, stream_ctx);
  }
  if (stream_ctx == NULL) {
    ret = -1;
  }
  else {
    uint8_t* bytes_max = bytes + length;
    
    bytes = h3zero_parse_incoming_remote_stream(bytes, bytes_max, stream_ctx, ctx.h3_ctx);
    
    if (bytes == NULL) {
      picoquic_log_app_message(ctx.cnx, "wt: Cannot parse incoming stream: %" PRIu64, stream_id);
      LOGE("wt: Cannot parse incoming stream: %" PRIu64, stream_id);
      ret = -1;
    }
    else {
      ret = h3zero_post_data_or_fin(ctx.cnx, bytes, length, fin_or_event, stream_ctx);
    }
  }
  return ret;
}

/* Ready to receive */
void WebTransport::set_receive_ready()
{
  ctx.is_upgraded = 0;
  for (size_t i = 0; i < ctx.nb_channels; i++) {
    ctx.channels[i].state = wt_state_ready;
    ctx.channels[i].incoming.is_receiving = 0;
    ctx.channels[i].incoming.expected = UINT32_MAX;
  }
}

int WebTransport::connect_to(std::string server) {
  return 0;
}
int WebTransport::connect_control(h3zero_callback_ctx_t* h3_ctx)
{
  int ret = 0;
  
  /* Create a stream context for the connect call. */
  h3zero_stream_ctx_t* stream_ctx = picowt_set_control_stream(ctx.cnx, h3_ctx);
  if (stream_ctx == NULL) {
    ret = -1;
  }
  else {
    ctx.connection_ready = 1;
    ctx.is_client = 1;
    
    if (ctx.server_path.length()) {
      ret = ctx_path_params((const uint8_t*)ctx.server_path.c_str(), strlen(ctx.server_path.c_str()));
    }
    if (ret == 0) {
      /* send the WT CONNECT */
      stream_ctx->path_callback_ctx = this;
      ret = picowt_connect(ctx.cnx, h3_ctx, stream_ctx, ctx.server_path.c_str(), client_callback, (void*)this);
    }
    if (ret == 0) {
      set_receive_ready();
    }
  }
  return ret;
}

void WebTransport::update_callbacks()
{
  for (int i = 0; /*i < wt_callback_count &&*/ i < MAX_WT_CHANNEL; i++)
    {
      if (&ctx) {
	ctx.channels[i].openCallback    = wt_callbacks[i].openCallback;
	ctx.channels[i].messageCallback = wt_callbacks[i].messageCallback;
	ctx.channels[i].userData        = wt_callbacks[i].userData;
      }
    }
}

void WebTransport::ctx_init() {
  for (int i = 0; i < MAX_WT_CHANNEL; i++) {
    ctx.channels[i].openCallback = wt_callbacks[i].openCallback;
    ctx.channels[i].messageCallback = wt_callbacks[i].messageCallback;
    ctx.channels[i].userData = wt_callbacks[i].userData;
  }
  set_receive_ready();
  resetMaps();
}

#if ANDROID
#define RTC_CHECK(condition) (condition) ? static_cast<void>(0) :	\
  LOGE("wt: Error: %s:%d %s", __FILE__, __LINE__, #condition)

JavaVM* GetJVM() {
  // RTC_CHECK(g_jvm); // << "JNI_OnLoad failed to run?";
  return g_app->activity->vm;
}

// Return a |JNIEnv*| usable on this thread or NULL if this thread is detached.
/*JNIEnv* GetEnv() {
  void* env = nullptr;
  jint status = g_app->activity->vm->GetEnv(&env, JNI_VERSION_1_6);
  RTC_CHECK(((env != nullptr) && (status == JNI_OK)) ||
  ((env == nullptr) && (status == JNI_EDETACHED)));
  //	<< "Unexpected GetEnv return: " << status << ":" << env;
  return reinterpret_cast<JNIEnv*>(env);
  }*/

// calls android.os.Process.setThreadPriority()
static void setProcessThreadPriority(JNIEnv* env,int wantedPriority){
  jclass jcProcess = env->FindClass("android/os/Process");
  jmethodID jmSetThreadPriority=env->GetStaticMethodID(jcProcess,"setThreadPriority","(I)V");
  env->CallStaticVoidMethod(jcProcess,jmSetThreadPriority,(jint)wantedPriority);
}
// calls android.os.Process.getThreadPriority(android.os.Process.myTid())
static void printProcessThreadPriority(JNIEnv* env){
  jclass jcProcess = env->FindClass("android/os/Process");
  jmethodID jmGetThreadPriority=env->GetStaticMethodID(jcProcess,"getThreadPriority","(I)I");
  jmethodID jmMyTid=env->GetStaticMethodID(jcProcess,"myTid","()I");
  jint myTid=env->CallStaticIntMethod(jcProcess,jmMyTid);
  jint currentPrio=env->CallStaticIntMethod(jcProcess,jmGetThreadPriority,(jint)myTid);
  LOGD("wt: printProcessThreadPriority %d",currentPrio);
}

JNIEnv* getEnv() {
  JNIEnv* env;
  g_app->activity->vm->AttachCurrentThread(&env, nullptr);
  return env;
}

static void updateThreadPriority(JNIEnv* env) {
  setProcessThreadPriority(env,-20);
}

#endif

// client("10.0.0.1", 4343, "/baton");
int WebTransport::client(char const * server_nameIn, int server_portIn, char const * pathIn)
{
  server_name = server_nameIn;
  server_port = server_portIn;
  path = pathIn;
  memset(&ctx, sizeof(ctx), 0);
  
  if (server_nameIn == NULL || server_name.length() == 0) {
    LOGE("wt: wt_client - The server name was not provided.");
    return -1;
  }
  
  if (server_portIn == 0) {
    LOGE("wt: wt_client - The server port was not provided.");
    return -1;
  }
  
  if (pathIn == NULL || strlen(pathIn) == 0) {
    LOGE("wt: wt_client - The path was not provided.");
    return -1;
  }
  
#ifdef ANDROID_hide
  JNIEnv* env = getEnv();
  printProcessThreadPriority(env);
  updateThreadPriority(env);
  printProcessThreadPriority(env);
#endif

  static struct sockaddr_storage server_address;
  static picoquic_quic_t* quic = NULL;
  static h3zero_callback_ctx_t* h3_ctx = NULL;
  
  char const* sni = "test";
  uint64_t current_time = picoquic_current_time();
  
  /* Get the server's address */
  if (ret == 0) {
    int is_name = 0;
    ret = picoquic_get_server_address(server_name.c_str(), server_port, &server_address, &is_name);
    
    if (ret != 0) {
      LOGE("Cannot get the IP address for <%s> port <%d>", server_name.c_str(), server_port);
    }
    else if (is_name) {
      sni = server_name.c_str();
    }
  }
  
  if (ret == 0) {
    quic = picoquic_create(
			   8,
			   NULL, /* Cert */
			   NULL, /* Key file */
			   NULL, /* trust file */
			   "h3",
			   NULL, /* default_callback_fn */
			   NULL, /* default_callback_ctx */
			   NULL,
			   NULL,
			   NULL, /* Reset seed is only for servers */
			   current_time,
			   NULL, /* Not using simulated time */
			   ticket_store_filename,
			   NULL, /* Only server need the ticket_encryption_key */
			   0 /* Only server need the ticket_encryption_key length */ );
    
    if (quic == NULL) {
      LOGE("wt: Could not create quic context\n");
      ret = -1;
    }
    else {
      picoquic_set_textlog(quic, "-");
      if (picoquic_load_retry_tokens(quic, token_store_filename) != 0) {
	LOGE("wt: No token file present. Will create one as <%s>.\n", token_store_filename);
      }
      
      // picoquic_set_default_congestion_algorithm(quic, picoquic_bbr_algorithm);
      // picoquic_set_key_log_file_from_env(quic);
      // picoquic_set_qlog(quic, qlog_dir);
      // picoquic_set_log_level(quic, 1);
    }
  }
  // picoquic_set_default_bbr_quantum_ratio(quic, 0.05);
  
  if (ret == 0) {
    h3_ctx = h3zero_callback_create_context(NULL);
    if (h3_ctx == NULL) {
      ret = 1;
    }
  }
  if (ret == 0) {
    LOGD("wt: Starting connection to %s, port %d\n", server_name.c_str(), server_port);
    
    /* Create a client connection */
    picoquic_cnx_t* cnx = cnx = picoquic_create_cnx(quic, picoquic_null_connection_id,
						    picoquic_null_connection_id,
						    (struct sockaddr*) & server_address,
						    current_time, 0, sni, "h3", 1);
    if (cnx == NULL) {
      LOGE("wt: Could not create connection context\n");
      ret = -1;
    }
    else {
      ctx.h3_ctx = h3_ctx;
      ctx.cnx = cnx;
      ctx.is_client = 1;
      ctx.server_path = path;
      picowt_set_transport_parameters(cnx);
      picoquic_set_callback(cnx, h3zero_callback, h3_ctx);
      /* Initialize the callback context. First, create a bidir stream */
      app_ctx_init(h3_ctx, NULL);
      
      /* Create a stream context for the connect call. */
      ret = connect_control(h3_ctx);
      
      /* Client connection parameters could be set here, before starting the connection. */
      if (ret == 0) {
	ret = picoquic_start_client_cnx(cnx);
      }
      if (ret < 0) {
	LOGE("wt: Could not activate connection\n");
      } else {
	/* Printing out the initial CID, which is used to identify log files */
	picoquic_connection_id_t icid = picoquic_get_initial_cnxid(cnx);
	LOGD("wt: Initial connection ID: ");
	
	for (uint8_t i = 0; i < icid.id_len; i++) {
	  printf("%02x", icid.id[i]);
	}
	printf("\n");
	// Perform the initialization, settings and QPACK streams
	ret = h3zero_protocol_init(cnx);
      }
    }
  }
  if (ret == 0) {
    /* Wait for packets */
    packet_loop_param.local_port = 0;
    packet_loop_param.local_af = AF_INET; // AF_UNSPEC, AF_INET, AF_INET6
    packet_loop_param.dest_if = 0;
    packet_loop_param.socket_buffer_size = 0;
    packet_loop_param.do_not_use_gso = 0;
    packet_loop_param.extra_socket_required = 0;
    packet_loop_param.simulate_eio = 0;
    packet_loop_param.send_length_max = 0;
    thread_ctx = picoquic_start_custom_network_thread(quic, &packet_loop_param,
						      nullptr, nullptr, nullptr,
						      "wtclient", client_loop_cb, this, &ret);
  }
  return wtThread;
}

#ifdef OLD
  /* Done. At this stage, we print out statistics, etc. */
  printf("Final wt state: %d %d\n", ctx.state, ret);
  /* print statistics per lane */
  printf("Wt bytes received: %" PRIu32 "\n", ctx.nb_bytes_received);
  printf("Wt bytes sent: %" PRIu32 "\n", ctx.nb_bytes_sent);
  printf("wt: datagrams sent: %d\n", ctx.nb_datagrams_sent);
  printf("wt: datagrams received: %d\n", ctx.nb_datagrams_received);
  printf("wt: datagrams bytes sent: %zu\n", ctx.nb_datagram_bytes_sent);
  printf("wt: datagrams bytes received: %zu\n", ctx.nb_datagram_bytes_received);
  printf("wt: Last sent datagram: 0x%02x\n", ctx.datagram_send_next);
  printf("wt: Last received datagram : 0x%02x\n", ctx.datagram_received);
  // if (ctx.capsule.h3_capsule.is_stored) {
  // char log_text[256];
  // printf("wt: Capsule received.\n");
  // printf("wt: Error code: %lu\n", (unsigned long) ctx.capsule.error_code);
  // LOGE("wt: Error code: %lu\n", (unsigned long) ctx.capsule.error_code);
  // printf("wt: Error message: %s\n",
  // 	   picoquic_uint8_to_str(log_text, sizeof(log_text), ctx.capsule.error_msg,
  // 				 ctx.capsule.error_msg_len));
  // LOGE("wt: Error message: %s\n",
  // 	 picoquic_uint8_to_str(log_text, sizeof(log_text), ctx.capsule.error_msg,
  // 			       ctx.capsule.error_msg_len));
  // }
  
  if (h3_ctx != NULL) {
    if (ctx.cnx) h3zero_callback_delete_context(ctx.cnx, h3_ctx);
  }
  
  /* Save tickets and tokens, and free the QUIC context */
  if (quic != NULL) {
    if (picoquic_save_session_tickets(quic, ticket_store_filename) != 0) {
      LOGE("wt: Could not store the saved session tickets.\n");
    }
    if (picoquic_save_retry_tokens(quic, token_store_filename) != 0) {
      LOGE("wt: Could not save tokens to <%s>.\n", token_store_filename);
    }
    picoquic_free(quic);
  }
#endif


/* Client socket loop */

/* Client loop call back management.
 * The function "picoquic_packet_loop" will call back the application when it is ready to
 * receive or send packets, after receiving a packet, and after sending a packet.
 * We implement here a minimal callback that instruct  "picoquic_packet_loop" to exit
 * when the connection is complete.
 */

int WebTransport::client_loop_cb(picoquic_quic_t* quic, picoquic_packet_loop_cb_enum cb_mode,
				 void* callback_ctx, void * callback_arg)
{
  int ret = 0;
  WebTransport* wt = (WebTransport*)callback_ctx;
  wt_ctx_t* ctx = &wt->ctx;
  picoquic_cnx_t* cnx = wt->ctx.cnx;
#if 0
  if (exitRequested) {
    return PICOQUIC_NO_ERROR_TERMINATE_PACKET_LOOP;
  }
#endif
  if (ctx == NULL) {
    ret = PICOQUIC_ERROR_UNEXPECTED_ERROR;
  }
  else {
    switch (cb_mode) {
    case picoquic_packet_loop_ready:
      {
	LOGD("wt: client: PICOQUIC waiting for packets");
	auto options           = (picoquic_packet_loop_options_t*)callback_arg;
	options->do_time_check = true;
	wt->start_streams();
      }
      break;
    case picoquic_packet_loop_time_check:
      { // set time = 0 when active
	auto time_check_arg = (packet_loop_time_check_arg_t*)callback_arg;
	time_check_arg->delta_t = 5 * 1000; // 1ms
	if (/*g_wt_ready &&*/ cnx && (ctx->state == wt_state_sending || ctx->state == wt_state_ready)) {
	  // LOGD("wt: time_check: %lu", time_check_arg->delta_t);
	  static int count = 0;
	  std::lock_guard<std::mutex> guard(wt->vector_mutex);
	  if (ctx->nb_channels == 0)
	    ctx->nb_channels = 8;
	  for (int i = 0; i < ctx->nb_channels; i++)
	    {
	      static int ccount = 0;
	      wt_channel_t* channel = &ctx->channels[i];
	      if (!channel->stream_ctx || !channel->stream_ctx->cnx) continue;
	      cnx = channel->stream_ctx->cnx;
	      if (channel->out.size() && channel->state == wt_state_sending) {
		auto stream_ctx_check = picoquic_find_stream(cnx, wt->appchan_sid_map[i]);
		if (cnx && stream_ctx_check) {
		  if (ctx->state != wt_state_sending) ctx->state = wt_state_sending;
		  picoquic_mark_active_stream(cnx, wt->appchan_sid_map[i], 1,
					      channel->stream_ctx);
		  time_check_arg->delta_t = 0;
		}
	      }
	    }
	}
      }
      break;
    case picoquic_packet_loop_after_receive:
      if (picoquic_get_cnx_state(cnx) == picoquic_state_disconnected) {
	ret = PICOQUIC_NO_ERROR_TERMINATE_PACKET_LOOP;
	LOGD("wt: PICOQUIC_NO_ERROR_TERMINATE");
      }
      break;
    case picoquic_packet_loop_after_send:
      if (picoquic_get_cnx_state(cnx) == picoquic_state_disconnected) {
	ret = PICOQUIC_NO_ERROR_TERMINATE_PACKET_LOOP;
	LOGD("wt: PICOQUIC_NO_ERROR_TERMINATE");
      }
      break;
    case picoquic_packet_loop_port_update:
      break;
    default:
      ret = PICOQUIC_ERROR_UNEXPECTED_ERROR;
      LOGD("wt: PICOQUIC_ERROR_UNEXPECTED_ERROR");
      break;
    }
  }
  return ret;
}

int WebTransport::server(std::string serverName, int port, const std::string configDir, const std::string logDir) {
#if PLATFORM_WINDOWS
  WSADATA wsaData = { 0 };
  (void)WSA_START(MAKEWORD(2, 2), &wsaData);
#endif
  memset(&ctx, sizeof(ctx), 0);
  certPath = server_cert_file;
  keyPath = server_key_file;
  wwwDir = "";
  logPath = "log";
  logFile = "log/wt.log";
  
  if (!configDir.empty()) {
    std::string cpd = configDir;
    wwwDir = cpd;
    wwwDir.append("www");
    certPath = cpd;
    certPath.append("/certs/cert.pem");
    keyPath = cpd;
    keyPath.append("/certs/key.pem");
  }
  if (!logDir.empty()) {
    std::string lp = logDir;
    logPath = lp;
    logFile = lp.append("/wt.log");
  }
  
  WebTransport* wt = (WebTransport*)this;
  picoquic_quic_config_t* config = &this->config;
  picoquic_config_init(config);
  picoquic_config_set_option(config, picoquic_option_CERT, certPath.c_str());
  picoquic_config_set_option(config, picoquic_option_KEY, keyPath.c_str());
  picoquic_config_set_option(config, picoquic_option_LOG_FILE, logFile.c_str());
  picoquic_config_set_option(config, picoquic_option_QLOG_DIR, logPath.c_str());
  picoquic_config_set_option(config, picoquic_option_WWWDIR, wwwDir.c_str());
  if (isFileReadable(certPath.c_str())) {
    picoquic_config_set_option(config, picoquic_option_CERT, certPath.c_str());
  }
  else {
    LOG("server_cert_file %s is not readable.", certPath.c_str());
  }
  if (isFileReadable(keyPath.c_str())) {
    picoquic_config_set_option(config, picoquic_option_KEY, keyPath.c_str());
  }
  else {
    LOG("server_key_file %s is not readable.", keyPath.c_str());
  }
  picoquic_config_set_option(config, picoquic_option_WWWDIR, wwwDir.c_str());
  // picoquic_config_set_option(config, picoquic_option_ROOT_TRUST_FILE);
  
  server_name = serverName;
  config->server_port = port ? port : default_server_port;
    
  /* Run as server */
  {LOG("wt: Starting Picoquic server (v%s) on port %d, server name = %s, just_once = %d, do_retry = %d\n",
       PICOQUIC_VERSION, wt->server_port, wt->server_name.c_str(), wt->just_once, 1); }
  // wt->quic_server(wt->server_name.c_str(), 0);

  picoquic_quic_t* quic = NULL;
  uint64_t current_time = 0;
  picohttp_server_parameters_t picoquic_file_param;
  
  memset(&picoquic_file_param, 0, sizeof(picohttp_server_parameters_t));
  picoquic_file_param.web_folder = config->www_dir;
  picoquic_file_param.path_table = wt->path_item_list;
  wt->path_item_list[0].path_app_ctx = wt;
  wt->path_item_list[1].path_app_ctx = wt;
  picoquic_file_param.path_table_nb = 2;
  
  /* Setup the server context */
  if (ret == 0) {
    current_time = picoquic_current_time();
    /* Create QUIC context */
    ret = picoquic_config_set_option(config, picoquic_option_Idle_Timeout, "300000");
    if (config->ticket_file_name == NULL) {
      ret = picoquic_config_set_option(config, picoquic_option_Ticket_File_Name, wt->ticket_store_filename);
    }
    if (ret == 0 && config->token_file_name == NULL) {
      ret = picoquic_config_set_option(config, picoquic_option_Token_File_Name, wt->token_store_filename);
    }
    // ret = picoquic_config_set_option(config, picoquic_option_LOG_FILE, "C:/c/log/wt.log");
    if (ret == 0) {
      quic = picoquic_create_and_configure(config, picoquic_demo_server_callback, &picoquic_file_param, current_time, NULL);
      static struct sockaddr_storage server_address;
      picoquic_cnx_t* cnx = cnx = picoquic_create_cnx(quic, picoquic_null_connection_id,
						      picoquic_null_connection_id,
						      nullptr, // (struct sockaddr*) & server_address,
						      current_time, 0, "localhost", "h3", 1);
      wt->ctx.cnx = cnx;
      // picoquic_set_default_bbr_quantum_ratio(quic, 0.01);
      if (quic == NULL) {
	ret = -1;
      }
      else {
	picoquic_set_default_wifi_shadow_rtt(quic, 150 * 1000);
	// picoquic_set_key_log_file_from_env(quic);
	
	picoquic_set_alpn_select_fn(quic, picoquic_demo_server_callback_select_alpn);
	
	// picoquic_use_unique_log_names(quic, 1);
	
	if (config->qlog_dir != NULL) {
	    picoquic_set_qlog(quic, config->qlog_dir);
	  }
	if (config->performance_log != NULL) {
	    ret = picoquic_perflog_setup(quic, config->performance_log);
	  }
	if (ret == 0 && config->cnx_id_cbdata != NULL) {
	  picoquic_load_balancer_config_t lb_config;
	  ret = picoquic_lb_compat_cid_config_parse(&lb_config, config->cnx_id_cbdata, strlen(config->cnx_id_cbdata));
	  if (ret != 0) {
	    fprintf(stdout, "wt: Cannot parse the CNX_ID config policy: %s.\n", config->cnx_id_cbdata);
	    LOG("wt: Cannot parse the CNX_ID config policy: %s.\n", config->cnx_id_cbdata);
	  }
	  else {
	    ret = picoquic_lb_compat_cid_config(quic, &lb_config);
	    if (ret != 0) {
	      fprintf(stdout, "wt: Cannot set the CNX_ID config policy: %s.\n", config->cnx_id_cbdata);
	      LOG("wt: Cannot set the CNX_ID config policy: %s.\n", config->cnx_id_cbdata);
	    }
	  }
	}
	if (ret == 0) {
	  fprintf(stdout, "wt: Accept enable multipath: %d.\n", quic->default_multipath_option);
	  LOG("wt: Accept enable multipath: %d.\n", quic->default_multipath_option);
	}
      }
    }
  }
  
  picoquic_set_textlog(quic, "-");
  if (ret == 0) {
    /* Wait for packets */
    packet_loop_param.local_port = config->server_port;
    packet_loop_param.local_af = AF_INET; // AF_UNSPEC, AF_INET, AF_INET6
    packet_loop_param.dest_if = config->dest_if;
    packet_loop_param.socket_buffer_size = config->socket_buffer_size;
    packet_loop_param.do_not_use_gso = 0;
    packet_loop_param.extra_socket_required = 0;
    packet_loop_param.simulate_eio = 0;
    packet_loop_param.send_length_max = 0;
    thread_ctx = picoquic_start_custom_network_thread(quic, &packet_loop_param,
						      nullptr, nullptr, nullptr,
						      "wtserver", server_loop_cb, this, &ret);
  }
  return ret;
};

#ifdef hide_cleanup
    /* Clean up */
      wt->ctx.nb_channels = 0;
    if (config->cnx_id_cbdata != NULL) {
      picoquic_lb_compat_cid_config_free(quic);
    }
    if (quic != NULL) {
      picoquic_free(quic);
    }
    {LOG("wt: Server exit with code = %d\n", true ? 0 : 1); }
    
    picoquic_config_clear(config);
#endif

#endif

void WebTransport::print_address(FILE* F_log, struct sockaddr* address, char* label, picoquic_connection_id_t cnx_id)
{
  char hostname[256];
  
  const char* x = inet_ntop(address->sa_family,
			    (address->sa_family == AF_INET) ? (void*)&(((struct sockaddr_in*)address)->sin_addr) : (void*)&(((struct sockaddr_in6*)address)->sin6_addr),
			    hostname, sizeof(hostname));
  
  fprintf(F_log, "%016llx : ", (unsigned long long)picoquic_val64_connection_id(cnx_id));
  LOG("wt: %016llx : ", (unsigned long long)picoquic_val64_connection_id(cnx_id));
  
  if (x != NULL) {
    fprintf(F_log, "%s %s, port %d\n", label, x,
	    (address->sa_family == AF_INET) ? ((struct sockaddr_in*)address)->sin_port : ((struct sockaddr_in6*)address)->sin6_port);
    LOG("wt: %s %s, port %d\n", label, x,
	(address->sa_family == AF_INET) ? ((struct sockaddr_in*)address)->sin_port : ((struct sockaddr_in6*)address)->sin6_port);
  }
  else {
    fprintf(F_log, "%s: inet_ntop failed with error # %ld\n", label, WSA_LAST_ERROR(errno));
    LOG("wt: %s: inet_ntop failed with error # %ld\n", label, (long)WSA_LAST_ERROR(errno));
  }
}

int WebTransport::server_loop_cb(picoquic_quic_t* quic, picoquic_packet_loop_cb_enum cb_mode,
				 void* callback_ctx, void* callback_arg)
{
  // LOGD("wt: server: server_loop_cb called");
  WebTransport* wt = (WebTransport*)callback_ctx;
  wt_ctx_t* ctx = &wt->ctx;
  picoquic_cnx_t* cnx = wt->ctx.cnx;
  int ret = 0;
  
  if (wt == NULL) {
    ret = PICOQUIC_ERROR_UNEXPECTED_ERROR;
  }
  else {
    switch (cb_mode) {
    case picoquic_packet_loop_ready:
      {
	LOGD("wt: server: PICOQUIC waiting for packets");
	auto options = (picoquic_packet_loop_options_t*)callback_arg;
	options->do_time_check = true;
	// wt_start_streams(ctx);
      }
      break;
    case picoquic_packet_loop_time_check:
      {
	// {LOG("wt: server_loop loop_time_check");}
	auto time_check_arg = (packet_loop_time_check_arg_t*)callback_arg;
	time_check_arg->delta_t = 1 * 1000; // 2ms
	if (cnx && ctx && (ctx->state == wt_state_sending || ctx->state == wt_state_ready)) {
	  // LOGD("wt: time_check: %lu", time_check_arg->delta_t);
	  static int count = 0;
	  std::lock_guard<std::mutex> guard(wt->vector_mutex);
	  if (ctx->nb_channels == 0)
	    ctx->nb_channels = 8;
	  for (int i = 0; i < ctx->nb_channels; i++)
	    {
	      static int ccount = 0;
	      if ((ccount++ % 40) == 0) {
		// LOG("wt: time_check found data waiting on: %d queued:%d",
		//	i, (int)channel->out.size());
	      }
	      wt_channel_t* channel = &ctx->channels[i];
	      if (channel->out.size() && channel->state == wt_state_sending) {
		auto stream_ctx_check = picoquic_find_stream(cnx, wt->appchan_sid_map[i]);
		if (cnx && stream_ctx_check) {
		  picoquic_mark_active_stream(cnx, wt->appchan_sid_map[i], 1,
					      channel->stream_ctx);
		  time_check_arg->delta_t = 0;
		}
	      }
	    }
	}
      }
      break;
    case picoquic_packet_loop_after_receive:
      {LOG("wt: server_loop after_receive");}
      break;
    case picoquic_packet_loop_after_send:
      // {LOG("wt: server_loop after_send");}
      break;
    case picoquic_packet_loop_port_update:
      {LOG("wt: server_loop port_update");}
      break;
    default:
      ret = PICOQUIC_ERROR_UNEXPECTED_ERROR;
      {LOG("wt: server_loop Unexpected Error");}
      break;
    }
  }
  return ret;
}


int WebTransport::post_callback(picoquic_cnx_t* cnx, uint8_t* bytes, size_t length,
				picohttp_call_back_event_t event, h3zero_stream_ctx_t* stream_ctx,
				void* callback_ctx)
{
  WebTransport* wt = (WebTransport*)callback_ctx;
  cnx->client_mode = false;
  int ret = 0;
  wtserver_post_test_t* ctx = (wtserver_post_test_t*)stream_ctx->path_callback_ctx;
  
  switch (event) {
  case picohttp_callback_get: /* Received a get command */
    break;
  case picohttp_callback_post: /* Received a post command */
    {LOG("wt: post post");}
    if (ctx == NULL) {
      ctx = (wtserver_post_test_t*)malloc(sizeof(wtserver_post_test_t));
      if (ctx == NULL) {
	/* cannot handle the stream -- TODO: reset stream? */
	return -1;
      }
      else {
	memset(ctx, 0, sizeof(wtserver_post_test_t));
	stream_ctx->path_callback_ctx = (void*)ctx;
      }
    }
    else {
      /* unexpected. Should not have a context here */
      return -1;
    }
    break;
  case picohttp_callback_post_data: /* Data received from peer on stream N */
  case picohttp_callback_post_fin: /* All posted data have been received, prepare the response now. */
    {LOG("wt: post data");}
    /* Add data to echo size */
    if (ctx == NULL) {
      ret = -1;
    }
    else {
      ctx->nb_received += length;
      if (event == picohttp_callback_post_fin) {
	if (ctx != NULL) {
	  size_t nb_chars = 0;
	  if (picoquic_sprintf(ctx->posted, sizeof(ctx->posted), &nb_chars, "Received %zu bytes.\n", ctx->nb_received) >= 0) {
	    ctx->response_length = nb_chars;
	    ret = (int)nb_chars;
	    if (ctx->response_length <= length) {
	      memcpy(bytes, ctx->posted, ctx->response_length);
	    }
	  }
	  else {
	    ret = -1;
	  }
	}
	else {
	  ret = -1;
	}
      }
    }
    break;
  case picohttp_callback_provide_data:
    {LOG("wt: post provide");}
    if (ctx == NULL || ctx->nb_sent > ctx->response_length) {
      ret = -1;
    }
    else
      {
	/* Provide data. */
	uint8_t* buffer;
	size_t available = ctx->response_length - ctx->nb_sent;
	int is_fin = 1;
	
	if (available > length) {
	  available = length;
	  is_fin = 0;
	}
	
	buffer = picoquic_provide_stream_data_buffer(bytes, available, is_fin, !is_fin);
	if (buffer != NULL) {
	  memcpy(buffer, ctx->posted + ctx->nb_sent, available);
	  ctx->nb_sent += available;
	  ret = 0;
	}
	else {
	  ret = -1;
	}
      }
    break;
  case picohttp_callback_reset: /* stream is abandoned */
    stream_ctx->path_callback = NULL;
    stream_ctx->path_callback_ctx = NULL;
    
    if (ctx != NULL) {
      free(ctx);
    }
    break;
  default:
    ret = -1;
    break;
  }
  
  return ret;
}


/* Accept an incoming connection */
int WebTransport::accept_service(picoquic_cnx_t* cnxIn,
				 uint8_t* path, size_t path_length,
				 struct st_h3zero_stream_ctx_t* stream_ctx)
{
  LOG("wt_accept_service called for new connection");
  int ret = 0;
  ctx.cnx = cnxIn;
  h3zero_callback_ctx_t* h3_ctx = ctx.h3_ctx; // (h3zero_callback_ctx_t*)picoquic_get_callback_context(ctx.cnx);
  {
    LOG("wt: wt_accept_service Accepting incoming connection");
    /* register the incoming stream ID */
    ret = app_ctx_init(h3_ctx, stream_ctx);
    
    /* init the global parameters */
    if (path != NULL && path_length > 0) {
      ret = ctx_path_params(path, path_length);
    }
    
    if (ret == 0) {
      stream_ctx->ps.stream_state.is_web_transport = 1;
      stream_ctx->path_callback = service_callback;
      stream_ctx->path_callback_ctx = this;
      ctx.connection_ready = 1;
      // ctx.nb_channels = 0; // Reset so new channels are accepted.
      ctx.state = wt_state_sending; // wt_state_ready;
      if (stream_ctx->stream_id > 0) {
	ret = picoquic_mark_active_stream(ctx.cnx, stream_ctx->stream_id, 1, stream_ctx);
      }
      for (int i = 0; i < MAX_WT_CHANNEL; i++) {
	ctx.channels[i].openCallback = wt_callbacks[i].openCallback;
	ctx.channels[i].messageCallback = wt_callbacks[i].messageCallback;
	ctx.channels[i].userData = wt_callbacks[i].userData;
	ctx.channels[i].state = wt_state_sending;
	ctx.channels[i].id = i;
	ctx.channels[i].message_id = 0;
	ctx.channels[i].incoming.message_id = 0;
      }
    }
  }
  ready = true;
  return ret;
}



/* Web transport callback. This will be called from the web server
 * when the path points to a web transport callback.
 */

int WebTransport::service_callback(st_picoquic_cnx_t* cnx, uint8_t* bytes, size_t length,
				   picohttp_call_back_event_t wt_event,
				   struct st_h3zero_stream_ctx_t* stream_ctx,
				   void* path_app_ctx)
{
  WebTransport* wt = (WebTransport*)path_app_ctx;
  int ret = 0;
  DBG_PRINTF("service_callback: %d, %" PRIi64 "\n", (int)wt_event, (stream_ctx == NULL)?(int64_t)-1:(int64_t)stream_ctx->stream_id);
  switch (wt_event) {
  case picohttp_callback_connecting:
    ret = wt->connecting(stream_ctx);
    {LOG("wt: service connecting");}
    break;
  case picohttp_callback_connect:
    /* A connect has been received on this stream, and could be accepted.
     */
    /* The web transport should create a web transport connection context,
     * and also register the stream ID as identifying this context.
     * Then, callback the application. That means the WT app context
     * should be obtained from the path app context, etc.
     */
    ret = wt->accept_service(cnx, bytes, length, stream_ctx);
    {LOG("wt: service connect h3 WT");}
    break;
  case picohttp_callback_connect_refused:
    /* The response from the server has arrived and it is negative. The
     * application needs to close that stream.
     * Do we need an error code? Maybe pass as bytes + length.
     * Application should clean up the app context.
     */
    {LOG("wt: service connect refused"); }
    break;
  case picohttp_callback_connect_accepted: /* Connection request was accepted by peer */
    /* The response from the server has arrived and it is positive.
     * The application can start sending data.
     */
    if (stream_ctx != NULL) {
      stream_ctx->is_upgraded = 1;
    }
    {LOG("wt: service connect accepted");}
    break;
    
  case picohttp_callback_post_fin:
  case picohttp_callback_post_data:
    {LOG("wt: service data");}
    /* Data received on a stream for which the per-app stream context is known.
     * the app just has to process the data, and process the fin bit if present.
     */
    ret = wt->stream_data(bytes, length, (wt_event == picohttp_callback_post_fin), stream_ctx);
    break;
  case picohttp_callback_provide_data: /* Stack is ready to send chunk of response */
    /* We assume that the required stream headers have already been pushed,
     * and that the stream context is already set. Just send the data.
     */
    ret = wt->provide_data(length, stream_ctx);
    break;
  case picohttp_callback_post_datagram:
    /* Data received on a stream for which the per-app stream context is known.
     * the app just has to process the data.
     */
    ret = wt->receive_datagram(bytes, length, stream_ctx);
    break;
  case picohttp_callback_provide_datagram: /* Stack is ready to send a datagram */
    ret = wt->provide_datagram(length, stream_ctx);
    break;
  case picohttp_callback_reset: /* Stream has been abandoned. */
    /* If control stream: abandon the whole connection. */
    ret = wt->stream_reset(stream_ctx);
    break;
  case picohttp_callback_free: /* Used during clean up the stream. Only cause the freeing of memory. */
    /* Free the memory attached to the stream */
    break;
  case picohttp_callback_deregister:
    /* The app context has been removed from the registry.
     * Its references should be removed from streams belonging to this session.
     * On the client, the memory should be freed.
     */
    {LOG("wt: service deregister");}
    wt->unlink_context(stream_ctx);
    {
      for (int i = 0; wt->ctx.nb_channels; i++) {
	wt_channel_t* channel = &wt->ctx.channels[i];
	memset(&channel, 0, sizeof(channel));
      }
      ret = PICOQUIC_NO_ERROR_TERMINATE_PACKET_LOOP;
    }
    break;
  default:
    /* protocol error */
    {LOG("wt: protocol error");}
    ret = -1;
    break;
  }
  return ret;
}


  
#ifdef server_thread
#if PLATFORM_WINDOWS
  class FQuicRunnable : public FRunnable {
  public:
    FQuicRunnable() {}
    virtual bool Init() override {
      return true;
    }
    virtual uint32 Run() override {
      server_thread_loop(this);
      return 0;
    }
    virtual void Exit() override {
      
    }
    virtual void Stop() override {
      
    }
  };
  FQuicRunnable quicRun;
  wtThread = FRunnableThread::Create(&quicRun, TEXT("WebTransport"), 1 * 1024 * 1024, TPri_TimeCritical, 0, EThreadCreateFlags::None);
  SetThreadPriority(wThread, TPri_TimeCritical); // TPri_Highest);
#else
  ret = picoquic_create_thread(&wtThread, server_thread_loop, this);
#endif
#endif

char* picoquic_strsep(char** stringp, const char* delim)
{
#if PLATFORM_WINDOWS
  if (*stringp == NULL) {
    return NULL;
  }
  char* token_start = *stringp;
  *stringp = strpbrk(token_start, delim);
  if (*stringp) {
    **stringp = '\0';
    (*stringp)++;
  }
  return token_start;
#else
  return strsep(stringp, delim);
#endif
}

int isFileReadable(std::string filename) {
  std::ifstream file;
  file.open(filename);
  int ret = 0;
  if (file) {
    ret = 1;
    file.close();
  }
  return ret;
}

void dumpBytes(const char* msg, int appchan, int length, int size, int offset, const uint8_t* bp, bool all) {
  char tst[256];
  sprintf(tst, "wt: %s chan:%d len:%6d size:%6d offset:%6d [%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x] ",
	  msg, appchan, (int)length, (int)size, offset,
	  bp[0], bp[1], bp[2], bp[3], bp[4], bp[5], bp[6], bp[7],
	  bp[8], bp[9], bp[10], bp[11], bp[12], bp[13], bp[14], bp[15],
	  bp[16], bp[17], bp[18], bp[19]);
  std::string s;
  int idx = all ? 0 : 12;
  while (idx < 30) {
    s += isprint(bp[idx]) ? (char)bp[idx] : '.';
    if (idx > 20) break;
    idx++;
  }
  strcat(tst, s.c_str());
  LOGD("%s", tst);
}

bool wt_zeros(const char* bytes, int len) {
  bool ret = false;
  const char* be = bytes + len;
  while (bytes < be) {
    if (!*bytes++) {
      ret = false;
      break;
    }
  }
  return ret;
}
