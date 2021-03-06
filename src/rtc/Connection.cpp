#include <rtc/Connection.h>
#include <string.h>

/* ----------------------------------------------------------------- */

static void rtc_connection_udp_alloc_cb(uv_handle_t* handle, size_t nsize, uv_buf_t* buf);
static void rtc_connection_udp_recv_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned int flags);
static void rtc_connection_udp_send_cb(uv_udp_send_t* req, int status);

/* ----------------------------------------------------------------- */

namespace rtc {
  
  ConnectionUDP::ConnectionUDP() 
    :loop(NULL)
     //    ,saddr(NULL)
    ,user(NULL)
    ,on_data(NULL)
    ,port(0)
  {

    loop = uv_default_loop();
    if (!loop) {
      printf("rtc::ConnectionUDP - error: ConnectionUDP() cannot get the default uv loop.\n");
      ::exit(1);
    }

  }

  bool ConnectionUDP::bind(std::string sip, uint16_t sport) {

    int r;
    ip = sip;
    port = sport;

    /* create sockaddr */
    r = uv_ip4_addr(ip.c_str(), port, &raddr);
    if (r != 0) {
      printf("rtc::ConnectionUDP - error: cannot create sockaddr_in: %s\n", uv_strerror(r));
      return false;
    }

    /* initialize the socket */
    r = uv_udp_init(loop, &sock);
    if (r != 0) {
      printf("rtc::ConnectionUDP - error: cannot initialize the UDP socket in ConnectionUDP: %s\n", uv_strerror(r));
      return false;
    }

    /* bind */
    r  = uv_udp_bind(&sock, (const struct sockaddr*)&raddr, 0);
    if (r != 0) {
      printf("rtc::ConnectionUDP - error: cannot bind the UDP socket in ConnectionUDP: %s\n", uv_strerror(r));
      return false;
    }

    sock.data = (void*) this;

    /* start receiving */
    r = uv_udp_recv_start(&sock, 
                          rtc_connection_udp_alloc_cb,
                          rtc_connection_udp_recv_cb);

    if (r != 0) {
      printf("rtc::ConnectionUDP - error: cannot start receiving in ConnectionUDP: %s\n", uv_strerror(r));
      return false;
    }

    return true;
  }

#if 0
  void ConnectionUDP::send(uint8_t* data, uint32_t nbytes) {

    if (!saddr) {
      printf("rtc::ConnectionUDP - error: cannot send data in ConnectionUDP(); no data received yet, so we don't know where to send to.\n");
      return;
    }

    uv_udp_send_t* req = (uv_udp_send_t*)malloc(sizeof(uv_udp_send_t));
    if (!req) {
      printf("rtc::ConnectionUDP - error: cannot allocate a send request in ConnectionUDP.\n");
      return;
    }

    printf("rtc::ConnectionUDP - verbose: sending the following data.\n");
    printf("-----------------------------------");
    int nl = 0;
    int c = 0;
    for(uint32_t i = 0; i < nbytes; ++i, ++nl) {
      if (nl == 32 || c == 0) {
        printf("\n\t%02D: ", c);
        c++;
        nl = 0;
      }
      printf("%02X ", data[i]);
    }
    printf("\n-----------------------------------\n");

    /* @todo check nbytes size in ConnectionUDP::send */
    /* @todo we def. don't want to allocate everytime when we need to sentin ConnectionUDP. */

    char* buffer_copy = new char[nbytes];
    if (!buffer_copy) {
      printf("rtc::ConnectionUDP - error: cannot allocate a copy for the send buffer in ConnectionUDP.\n");
      free(req);
      req = NULL;
      return;
    }

    memcpy(buffer_copy, data, nbytes);
    uv_buf_t buf = uv_buf_init(buffer_copy, nbytes);
    
    req->data = buffer_copy;

    int r = uv_udp_send(req, 
                        &sock, 
                        &buf, 
                        1, 
                        (const struct sockaddr*)saddr, 
                        rtc_connection_udp_send_cb);

    if (r != 0) {
      printf("rtc:::ConnectionUDP - error: cannot send udp data in ConnectionUDP: %s.\n", uv_strerror(r));
      free(req);
      free(buffer_copy);
      req = NULL;
      buffer_copy = NULL;
    }
  }
#endif

  void ConnectionUDP::sendTo(std::string rip, uint16_t rport, uint8_t* data, uint32_t nbytes) {
    uv_udp_send_t* req = (uv_udp_send_t*)malloc(sizeof(uv_udp_send_t));
    if (!req) {
      printf("rtc::ConnectionUDP - error: cannot allocate a send request in ConnectionUDP.\n");
      return;
    }


    printf("rtc::ConnectionUDP - verbose: sending the following data (%u bytes) form %s:%u to %s:%u.\n", nbytes, ip.c_str(), port, rip.c_str(), rport);

#if 0
    printf("-----------------------------------");

    int c = 0;
    for(uint32_t i = 0; i < nbytes; ++i) {
      if (i == 0 || i % 16 == 0) {
        printf("\n\t");
        c++;

      }
      printf("%02X ", data[i]);
    }
    printf("\n-----------------------------------\n");
#endif

    /* @todo check nbytes size in ConnectionUDP::send */
    /* @todo we def. don't want to allocate everytime when we need to sentin ConnectionUDP. */

    char* buffer_copy = new char[nbytes];
    if (!buffer_copy) {
      printf("rtc::ConnectionUDP - error: cannot allocate a copy for the send buffer in ConnectionUDP.\n");
      free(req);
      req = NULL;
      return;
    }

    memcpy(buffer_copy, data, nbytes);
    uv_buf_t buf = uv_buf_init(buffer_copy, nbytes);
    
    req->data = buffer_copy;

    struct sockaddr_in send_addr;
    uv_ip4_addr(rip.c_str(), rport, &send_addr);
    int r = uv_udp_send(req, 
                        &sock, 
                        &buf, 
                        1, 
                        (const struct sockaddr*)&send_addr, 
                        rtc_connection_udp_send_cb);

    if (r != 0) {
      printf("rtc:::ConnectionUDP - error: cannot send udp data in ConnectionUDP: %s.\n", uv_strerror(r));
      free(req);
      free(buffer_copy);
      req = NULL;
      buffer_copy = NULL;
    }
  }

  void ConnectionUDP::update() {
    uv_run(loop, UV_RUN_NOWAIT);
  }

} /* namespace rtc */

/* ----------------------------------------------------------------- */

static void rtc_connection_udp_recv_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned int flags) {

  /* do nothing when we receive 0 as nread. */
  if (nread == 0) {
    printf("rtc::ConnectionUDP - verbose: received 0 bytes, flags: %d.\n", flags);
    return;
  }

  rtc::ConnectionUDP* udp = static_cast<rtc::ConnectionUDP*>(handle->data);
  /*
  if (!udp->saddr) {
    udp->saddr = (struct sockaddr*)malloc(sizeof(struct sockaddr));
    if (!udp->saddr) {
      printf("rtc::ConnectionUDP - error: cannot allocate the `struct sockaddr*` in ConnectionUDP. Out of mem?\n");
      exit(1);
    }
    memcpy(udp->saddr, addr, sizeof(struct sockaddr));
  }
  */

  if (addr) {
    char src_ip[20];
    uint16_t port;
    const struct sockaddr_in* src_addr = (const struct sockaddr_in*)addr;
    uv_ip4_name(src_addr, src_ip, sizeof(src_ip));
    port = ntohs(src_addr->sin_port);
    printf("rtc::ConnectionUDP - verbose: received from: %s:%d, number of bytes: %ld\n", src_ip, port, nread);
    if (udp->on_data) {
      udp->on_data(src_ip, port, udp->ip, udp->port, (uint8_t*)buf->base, nread, udp->user);
    }
  }
}

static void rtc_connection_udp_alloc_cb(uv_handle_t* handle, size_t nsize,  uv_buf_t* buf) {
  static char slab[65536];
 
  if (nsize > sizeof(slab)) {
    printf("rtc::ConnectionUDP - error: requested receiver size to large. @todo - this is just a quick implementation.\n");
    exit(1);
  }
  
  buf->base = slab;
  buf->len = sizeof(slab);
}

static void rtc_connection_udp_send_cb(uv_udp_send_t* req, int status) {
  printf("rtc::ConnectionUDP - ready sending some data, status: %d\n", status);

  /* @todo rtc_connection_udp_send_cb needs to handle the status value.*/
  char* ptr = (char*)req->data;
  delete[] ptr;
  delete req;
  req = NULL;
  ptr = NULL;
}
