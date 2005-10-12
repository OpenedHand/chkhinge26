/*
 * Chech Hinge26 - Daemon to monitor for keyboard switch events and run 
 * a command when detected.
 *
 * Copyright 2005 Openedhand Ltd.
 *
 * Author: Richard Purdie <rpurdie@openedhand.com>
 *
 * Based on bits of evtext.c by Vojtech Pavlik and kbdd by Nils Faerber
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "input.h"

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)	((array[LONG(bit)] >> OFF(bit)) & 1)

void do_switch_event(int status, char *cmd) 
{
  char cmd_with_args[256];

  snprintf(cmd_with_args, 256, "%s %i", cmd, status);

  /* 3 = closed, 0 = landscape, 2 = portrait. Ignore 1 */

  switch (fork())
    {
    case 0:
      execlp ("/bin/sh", "sh", "-c", cmd_with_args, (char *)NULL);
      printf ("Error: Exec of '%s' failed.\n", cmd);
      exit (0);
      break;
    case -1:
      printf ("Error: Fork failed.\n");
      break;
    }
}

void
cleanup_children(int s)
{
        kill(-getpid(), 15);  /* kill every one in our process group  */
        exit(0);
}

void 
install_signal_handlers(void)
{
        signal (SIGCHLD, SIG_IGN);  /* kernel can deal with zombies  */
        signal (SIGINT, cleanup_children);
        signal (SIGQUIT, cleanup_children);
        signal (SIGTERM, cleanup_children);
}


int main (int argc, char **argv)
{
	int fd, rd, i, j, k;
	struct input_event ev[64];
	int version;
	unsigned short id[4];
	unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
	char name[256] = "Unknown";
	int abs[5];
	int uindev;
	int stroke_active=0;
	signed long xpos=0, ypos=0;

	int switch_bits[NBITS(SW_MAX)];

	if (argc < 3) {
		printf ("Usage: chhing26 /dev/inputX <cmd to run>\n");
		printf ("Where X is the corgi keyboard to monitor\n");
		exit (1);
	}

	if ((fd = open(argv[1], O_RDONLY)) < 0) {
		perror("chkhinge86");
		exit(1);
	}

	/* Find supported Events */
	memset(bit, 0, sizeof(bit));
	ioctl(fd, EVIOCGBIT(0, EV_MAX), bit[0]);
	
	/* Check Switches are present */
	if (!test_bit(EV_SW, bit[0])) {
		printf("Error: This device doesn't have any switches!\n");
		exit(1);
	}

	/* Check have correct switches */
	ioctl(fd, EVIOCGBIT(EV_SW, SW_MAX), bit[EV_SW]);
	
	if ((!test_bit(0, bit[EV_SW]) || !test_bit(1, bit[EV_SW]))) {
		printf("Error: An incorrect number of switches were found!\n");
		exit(1);
	}

	install_signal_handlers();

	/* Read Current Switch Status */
	memset(switch_bits, 0, sizeof(switch_bits));
	ioctl(fd, EVIOCGSW(SW_MAX), switch_bits);
	do_switch_event(test_bit(0,switch_bits) | (test_bit(1,switch_bits) << 1), argv[2] );

	while (1) 
	{
		int pressed=0;
		rd = read(fd, ev, sizeof(struct input_event) * 64);

		if (rd < (int) sizeof(struct input_event)) 
		{
			printf("yyy\n");
			perror("\nchkhinge26: Error reading input device!\n");
			exit (1);
		}

		for (i = 0; i < rd / sizeof(struct input_event); i++)
		{

			if ((ev[i].type == EV_SW) && ((ev[i].code == 0) || (ev[i].code == 1))) {
				ioctl(fd, EVIOCGSW(SW_MAX), switch_bits);
				do_switch_event(test_bit(0,switch_bits) | (test_bit(1,switch_bits) << 1), argv[2] );
			}

			/*if (ev[i].type == EV_SW) 
					printf("Event: time %ld.%06ld, type %d (%s), code %d (%s), value %d\n",
				ev[i].time.tv_sec, 
				ev[i].time.tv_usec, 
				ev[i].type,
				events[ev[i].type] ? events[ev[i].type] : "?",
				ev[i].code,
				names[ev[i].type] ? (names[ev[i].type][ev[i].code] ? names[ev[i].type][ev[i].code] : "?") : "?",
				ev[i].value);*/
		}
	}
}
