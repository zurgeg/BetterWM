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

#include "agent.h"
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

//agent
pthread_t agentthread;
int agent_status = 0;
int agent_done = 0;
int remove_linkkey = 0;
char agentcommand[14];
char devcommand[48];

int l2connect(const char *bdaddr, int psm)
{
  int fd;
  struct sockaddr_l2 addr;
  struct linger l = { .l_onoff = 1, .l_linger = 5 };
  int opt = 0;

  if ( (fd = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP)) == -1 )
    return -1;

  if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) < 0)
  {
    close(fd);
    return -2;
  }

  if (setsockopt(fd, SOL_L2CAP, L2CAP_LM, &opt, sizeof(opt)) < 0)
  {
    close(fd);
    return -3;
  }

  memset(&addr, 0, sizeof(addr));
  addr.l2_family = AF_BLUETOOTH;
  addr.l2_psm    = htobs(psm);
  str2ba(bdaddr, &addr.l2_bdaddr);

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
  {
    close(fd);
    return -4;
  }

  return fd;
}

void connecttowii(int retries)
{
  //implement retries in case of a failure or two

  int tries = 0;

  if (agent_status == 0)
  {
    if ( pthread_create(&agentthread, NULL, agent_run, (void *)NULL) )
    {
      printf("agent thread creation failed\n");
      exit(1);
    }

    while(!agent_status)
    {
      usleep(50*1000);
      tries++;
      if (tries > 200)
    {
        printf("agent timeout\n");
        exit(1);
      }
    }
    printf("agent started.\n");

  }

  tries = 0;
  remove_linkkey = 1;
  agent_status = 2;
  while(agent_status != 1)
  {
      usleep(50*1000);
      tries++;
      if (tries > 200)
      {
          printf("key timeout\n");
          exit(1);
      }
  }
  printf("key removed.\n");

  /* Connect to Wii */

  printf("connecting to %s psm %d\n", bdaddr, CTRL);
  if ( (ctrl = l2connect(bdaddr, CTRL)) < 0 )
  {
    printf("can't connect to %s psm %d", bdaddr, CTRL);
    exit(1);
  }

  printf("connecting to %s psm %d\n", bdaddr, DATA);
  if ( (data = l2connect(bdaddr, DATA)) < 0 )
  {
    shutdown(ctrl, SHUT_RDWR);
    close(ctrl);
    printf("can't connect to %s psm %d", bdaddr, DATA);
    exit(1);
  }

  printf("connected.\n");

}

void disconnectfromwii()
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

  //set up unload signals
  signal(SIGINT, sig_handler);
  signal(SIGHUP, sig_handler);

  //check args
  if (argc >= 1)
  {
    bdaddr = argv[1];
  }

  if (bachk(bdaddr) == -1)
  {
    printf("usage: %s <wii-bdaddr>\n", *argv);
    return 1;
  }

  input_init();

  init_wiimote(&state);

  //agent, system strings
  /*
  char *tempbdaddr = malloc(18 * sizeof(char));
  memset(tempbdaddr, 0, 18 * sizeof(char));
  agentcommand = malloc(14 * sizeof(char));
  sscanf(bdaddr, "%2c:%2c:%2c:%2c:%2c:%2c", &tempbdaddr[0], &tempbdaddr[3], &tempbdaddr[6], &tempbdaddr[9], &tempbdaddr[12], &tempbdaddr[15]);
  strcpy(agentcommand, "$");
  strcat(agentcommand, tempbdaddr + 15);
  strcat(agentcommand, tempbdaddr + 12);
  strcat(agentcommand, tempbdaddr + 9);
  strcat(agentcommand, tempbdaddr + 6);
  strcat(agentcommand, tempbdaddr + 3);
  strcat(agentcommand, tempbdaddr);
  free(tempbdaddr);
  */

  agentcommand[0] = '$';
  sscanf(bdaddr, "%2c:%2c:%2c:%2c:%2c:%2c",
    &agentcommand[11],
    &agentcommand[9],
    &agentcommand[7],
    &agentcommand[5],
    &agentcommand[3],
    &agentcommand[1]
  );
  agentcommand[13] = 0;

  strcpy(devcommand, "./test-device remove ");
  strcat(devcommand, bdaddr);

  //connect
  connecttowii(0);

  while (running)
  {
    // if (SDL_GetTicks() >= report_time)
    // {
    //   send_report_now = 1;
    // }

    memset(&pfd, 0, sizeof(pfd));

    // Listen for data on either fd
    //setting this to zero is not required for every call...
    //... also POLLERR has no effect in the events field
    pfd[0].fd = ctrl;
    pfd[0].events = POLLIN | POLLERR;
    pfd[1].fd = data;
    pfd[1].events = POLLIN | POLLERR;

    // Check data PSM for output if it's time to send a report
    if (send_report_now)
    {
      pfd[1].events |= POLLOUT;
    }

    if (poll(pfd, 2, 0) < 0)
    {
      printf("ppoll");
      break;
    }

    if (pfd[0].revents & POLLERR)
    {
      printf("error on ctrl psm");
      break;
    }
    if (pfd[1].revents & POLLERR)
    {
      printf("error on data psm");
      break;
    }


    if (pfd[1].revents & POLLIN)
    {
      len = recv(data, buf, 32, MSG_DONTWAIT);

      if (len > 0)
      {
        if (process_report(&state, buf, len) == -1)
        {
          printf("error processing data");
          break;

        }
      }
    }

    //send report
    if (send_report_now)
    {
      //process input
      input_update(&state);

      if (pfd[1].revents & POLLOUT)
      {
        len = generate_report(&state, buf);
        if (send(data, buf, len, MSG_DONTWAIT) == -1)
        {
          printf("send error");
        }
        send_report_now = 0;

        //report_time = SDL_GetTicks() + 20;
        failure = 0;
      }
      else
      {
        printf("failure \n");
        failure += 1;
        if (failure >= 4)
        {
          disconnectfromwii();
          destroy_wiimote(&state);
          init_wiimote(&state);
          connecttowii(2);
        }
        send_report_now = 0;

        //report_time = SDL_GetTicks() + 250;
      }

      /* Schedule next report in 20000us = 20ms*/

    }

    usleep(10*1000);

  }

  printf("cleaning up...\n");

  agent_done = 1;
  pthread_join(agentthread, NULL);

  disconnectfromwii();

  destroy_wiimote(&state);

  input_unload();

  return 0;
}
