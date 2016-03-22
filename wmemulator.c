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
#include <pthread.h>

#include "wiimote.h"
#include "input.h"

#define CTRL 17
#define DATA 19

//address, sockets
char * bdaddr = NULL;
int ctrl, data;

//signal handler to break out of main loop
static int running = 1;
void sig_handler(int sig)
{
  running = 0;
}

int createsocket()
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

int l2connect(const char *bdaddr, int psm)
{
  int fd;
  struct sockaddr_l2 addr;

  fd = createsocket();
  if (fd < 0)
  {
    return -1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.l2_family = AF_BLUETOOTH;
  addr.l2_psm    = htobs(psm);
  str2ba(bdaddr, &addr.l2_bdaddr);

  if ( connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 )
  {
    close(fd);
    return -1;
  }

  return fd;
}

int l2accept(int psm)
{
  int fd, wiifd;
  struct sockaddr_l2 addr, wiiaddr;
  socklen_t opt = sizeof(wiiaddr);
  char buf[18];

  fd = createsocket();
  if (fd < 0)
  {
    return -1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.l2_family = AF_BLUETOOTH;
  addr.l2_psm = htobs(psm);
  addr.l2_bdaddr = *BDADDR_ANY;

  if ( bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 )
  {
    close(fd);
    return -1;
  }

  if ( listen(fd, 1) < 0 )
  {
    close(fd);
    return -1;
  }

  wiifd = accept(fd, (struct sockaddr *)&wiiaddr, &opt);
  ba2str( &wiiaddr.l2_bdaddr, buf );
  printf("accepted connection from %s.\n", buf);

  close(fd);

  return wiifd;
}

void waitforwii()
{

  printf("waiting for connection on psm %d...\n", CTRL);
  ctrl = l2accept(CTRL);

  if ( ctrl < 0 )
  {
    printf("can't connect to psm %d\n", CTRL);
    running = 0;
    return;
  }

  printf("waiting for connection on psm %d...\n", DATA);
  data = l2accept(DATA);

  if (data < 0)
  {
    printf("can't connection to psm %d\n", DATA);
    running = 0;
    return;
  }

  printf("connected.\n");
}


void connecttowii()
{

  printf("connecting to %s psm %d\n", bdaddr, CTRL);
  ctrl = l2connect(bdaddr, CTRL);

  if (ctrl < 0)
  {
    printf("can't connect to %s psm %d\n", bdaddr, CTRL);
    running = 0;
    return;
  }

  printf("connecting to %s psm %d\n", bdaddr, DATA);
  data = l2connect(bdaddr, DATA);

  if (data < 0)
  {
    printf("can't connect to %s psm %d\n", bdaddr, DATA);
    running = 0;
    return;
  }

  printf("connected.\n");

}

void disconnect()
{
  shutdown(ctrl, SHUT_RDWR);
  shutdown(data, SHUT_RDWR);
  close(ctrl);
  close(data);
}

int main(int argc, char *argv[])
{
  struct pollfd pfd[2];
  unsigned char buf[32];
  ssize_t len;

  //struct timespec timeout;
  //timeout.tv_sec = 0;
  //timeout.tv_nsec = 0;

  struct wiimote_state state;

  int send_report_now = 0;
  int failure = 0;

  if (argc > 1)
  {
    bdaddr = argv[1];

    if (bachk(bdaddr) < 0)
    {
      printf("usage: %s <wii-bdaddr>\n", *argv);
      return 1;
    }
  }

  //set up unload signals
  signal(SIGINT, sig_handler);
  signal(SIGHUP, sig_handler);

  input_init();

  init_wiimote(&state);

  if (bdaddr == NULL)
  {
    waitforwii();
  }
  else
  {
    connecttowii();
  }


  while (running)
  {

    memset(&pfd, 0, sizeof(pfd));

    // Listen for data on either fd
    //setting this to zero is not required for every call...
    //... also POLLERR has no effect in the events field
    pfd[0].fd = ctrl;
    pfd[0].events = POLLIN | POLLERR;
    pfd[1].fd = data;
    pfd[1].events = POLLIN | POLLERR;

    // Check data PSM for output if it's time to send a report
    if (1 || send_report_now)
    {
      pfd[1].events |= POLLOUT;
    }

    if (poll(pfd, 2, 0) < 0)
    {
      printf("ppoll\n");
      break;
    }

    if (pfd[0].revents & POLLERR)
    {
      printf("error on ctrl psm\n");
      break;
    }
    if (pfd[1].revents & POLLERR)
    {
      printf("error on data psm\n");
      break;
    }


    if (pfd[1].revents & POLLIN)
    {
      len = recv(data, buf, 32, MSG_DONTWAIT);

      if (len > 0)
      {
        process_report(&state, buf, len);
      }
    }

    //send report
    if (1 || send_report_now)
    {
      //process input
      input_update(&state);

      if (pfd[1].revents & POLLOUT)
      {
        len = generate_report(&state, buf);
        send(data, buf, len, MSG_DONTWAIT);
        send_report_now = 0;

        failure = 0;
      }
      else
      {
        printf("failure \n");
        failure += 1;
        if (failure >= 4)
        {
          disconnect();
          destroy_wiimote(&state);
          init_wiimote(&state);
          connecttowii();
        }
        send_report_now = 0;

        usleep(200*1000);
      }

      /* Schedule next report in 20000us = 20ms*/

    }

    usleep(20*1000);

  }

  printf("cleaning up...\n");

  disconnect();

  destroy_wiimote(&state);

  input_unload();

  return 0;
}
