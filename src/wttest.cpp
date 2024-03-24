#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
  // wt->send(1, "Hello, world!", -1);
  //  wt.close();
}


int main(int argc, char* argv[]) {
  bool client = false;
  bool server = false;
  int port = 4433;
  std::string host = "localhost";
  int opt;
  while ((opt = getopt(argc, argv, "csp:h:")) != -1) {
    switch (opt) {
    case 'c':
      client = true;
      break;
    case 's':
      server = true;
      break;
    case 'p':
      port = atoi(optarg);
      break;
    case 'h':
      host = optarg;
      break;
    default:
      break;
    }
  }
  

    WebTransport wt;
  if (server) {
    wt.set_callbacks(0, ocbs, mcbs, &wt);
    wt.set_callbacks(1, ocbs, mcbs, &wt);
    wt.set_callbacks(2, ocbs, mcbs, &wt);
    wt.set_callbacks(3, ocbs, mcbs, &wt);
    wt.server(host.c_str(), port, "./", "./");
  }
  
  WebTransport wtc;
  if (client) {
    wtc.set_callbacks(0, ocbc, mcbc, &wtc);
    wtc.set_callbacks(1, ocbc, mcbc, &wtc);
    wtc.set_callbacks(2, ocbc, mcbc, &wtc);
    wtc.set_callbacks(3, ocbc, mcbc, &wtc);
    wtc.client(host.c_str(), port, "/baton");
  }

  if (client) {
    sleep(1);
    LOG("wttest: sending test");
    wtc.send(0, "client test", -1);
    sleep(1);
    LOG("wttest: sending test2");
    wtc.send(0, "client test2", -1);
    sleep(1);
    LOG("wttest: sending test3");
    wtc.send(0, "client test3 ", -1);
    sleep(1);
    wtc.send(0, "client test4  ", -1);
  }
  if (server) {
    sleep(1);
    wt.send(0, "server test", -1);
  wt.join();
  }
  if (client) {
    wtc.join();
  }

  LOG("WTTEST ending");

  return 0;
}
