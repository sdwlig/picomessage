
#include "WebTransport.hpp"

void ocbs(int chan, void *ptr) {
  WebTransport* wt = (WebTransport*)ptr;
  LOG("server: Opened: %d", chan);
}
void mcbs(int chan,
	 const char* data,
	 int len,
	 void* ptr,
	 uint64_t startTime,
	 uint64_t endTime) {
  WebTransport* wt = (WebTransport*)ptr;
  LOG("server: Message on chan:%d len:%d", chan, len);
  wt->send(1, "Hello, world!", -1);
  //  wt.close();
}

void ocbc(int chan, void *ptr) {
  WebTransport* wt = (WebTransport*)ptr;
  LOG("client: Opened: %d", chan);
}
void mcbc(int chan,
	 const char* data,
	 int len,
	 void* ptr,
	 uint64_t startTime,
	 uint64_t endTime) {
  WebTransport* wt = (WebTransport*)ptr;
  LOG("client: Message on chan:%d len:%d", chan, len);
  wt->send(1, "Hello, world!", -1);
  //  wt.close();
}


int main() {
  WebTransport wt;
  wt.set_callbacks(0, ocbs, mcbs, &wt);
  wt.set_callbacks(1, ocbs, mcbs, &wt);
  wt.set_callbacks(2, ocbs, mcbs, &wt);
  wt.server("localhost", 4433, "./", "./");

  WebTransport wtc;
  wtc.set_callbacks(0, ocbc, mcbc, &wt);
  wtc.set_callbacks(1, ocbc, mcbc, &wt);
  wtc.set_callbacks(2, ocbc, mcbc, &wt);
  wtc.client("localhost", 4433, "/baton");
  // wtc.send(0, "test", -1);

  wt.join();
  wtc.join();

  return 0;
}
