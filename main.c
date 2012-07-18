/*
 *  Copyright (c) 2007 by Thierry Leconte (F4DWV)
 *
 *      $Id: main.c,v 1.5 2007/04/22 16:14:41 f4dwv Exp $
 *
 *   This code is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2
 *   published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "version.h"
#include "acarsdec.h"
#include "acars_labels.h"
#include "acars_aircrafts.h"

int posconv(char *txt, unsigned char *label, char *pos);
extern int optind, opterr;
extern char *optarg;
long rx_idx;

static void usage(void)
{
	fprintf(stderr, "Usage: acarsdec [-LR][-s noport] -d alsapcmdevice | -f sndfile \n");
	fprintf(stderr, " -f sndfile :\t\tdecode from file sndfile (ie: a .wav file)\n");
	fprintf(stderr, " -d alsapcmdevice :\tdecode from soundcard input alsapcmdevice (ie: hw:0,0)\n");
	fprintf(stderr, " [-LR] :\t\tdiseable left or right channel decoding of stereo signal\n");
	fprintf(stderr, " [-s noport ] :\t\tact as an APRS local server, on port : noport\n");
	fprintf(stderr, "Input could be mono or stereo but with 48Khz sampling frequency.\nIf stereo, acarsdec will demod the 2 channels independantly (if no L ou R options specified)\n\n");
	exit(1);
}

void print_mesg(msg_t * msg)
{
	time_t t;
	struct tm *tmp;
	char pos[128];

	long i=0;

	printf("RX_IDX: %ld\n", rx_idx);
	printf("ACARS mode: %c, ", msg->mode);
	printf("message label: %s\n", msg->label);
	while(acars_labels[i][0]){
		if(!strcmp(acars_labels[i][0],(const char*)msg->label)){
			printf("ACARS ML description: %s\n",acars_labels[i][1]);
			break;
		}
		i++;
	}

	printf("Aircraft reg: %s, ", msg->addr);
	printf("flight id: %s\n", msg->fid);
	i=0;
	while(acars_aircrafts[i].reg){
		const char *regtmp = (const char *) msg->addr;
		if (regtmp[0] == '.')
			regtmp++;
		if(!strcmp(acars_aircrafts[i].reg, regtmp)){
			printf("Aircraft type: %s, ",acars_aircrafts[i].type);
			printf("carrier: %s, ",acars_aircrafts[i].carrier_icao);
			printf("cn: %s\n",acars_aircrafts[i].cn);
			break;
		}
		i++;
	}

	printf("Block id: %d, ", (int) msg->bid);
	printf(" msg. no: %s\n", msg->no);
	printf("Message content:-\n%s", msg->txt);

	rx_idx++;

    if (posconv(msg->txt, msg->label, pos)==0)
        printf("\nAPRS : Addr:%s Fid:%s Lbl:%s pos:%s\n", msg->addr, msg->fid,msg->label,pos);
 
	t = time(NULL);
	tmp = gmtime(&t);
	printf
	    ("\n----------------------------------------------------------[%02d/%02d/%04d %02d:%02d]\n\n",
	     tmp->tm_mday, tmp->tm_mon + 1, tmp->tm_year + 1900,
	     tmp->tm_hour, tmp->tm_min);

}

int main(int argc, char **argv)
{
	int c;
	msg_t msgl, msgr;
	unsigned char rl, rr;
	int nbitl, nbitr;
	int nrbitl, nrbitr;
	int nbch=0;
	int el=1,er=1;
	short port=0;


	while ((c = getopt(argc, argv, "d:f:RLs:")) != EOF) {
		switch (c) {
		case 'd':
			nbch = initsample(optarg, 0);
			break;
		case 'f':
			nbch = initsample(optarg, 1);
			break;
		case 'L':
			el=0;
			break;
		case 'R':
			er=0;
			break;
		case 's':
			port=atoi(optarg);
			break;
		default:
			usage();
			exit(1);
		}
	}

	if (nbch == 0) {
		usage();
		exit(1);
	}

	if(port) 
	 if(init_serv(port)) {
		exit(1);
	 }
		

/* main loop */

	init_bits();
	init_mesg();

	nbitl = nbitr = 0;
	nrbitl = nrbitr = 8;

	do {
		short sample[4096];
		int ind, len;

		len = getsample(sample, 4096);
		if (len < 0)
			break;

		for (ind = 0; ind < len;) {

			if(el) {
			nbitl += getbit(sample[ind], &rl, 0);
			if (nbitl >= nrbitl) {
				nrbitl = getmesg(rl, &msgl, 0);
				nbitl = 0;
				if (nrbitl == 0) {
					if(port)
						send_mesg(&msgl);
					else
						print_mesg(&msgl);
					nrbitl = 8;
				}
			}
			}
			ind++;

			if (nbch >= 2) {
				if(er) {
				nbitr += getbit(sample[ind], &rr, 1);
				if (nbitr >= nrbitr) {
					nrbitr = getmesg(rr, &msgr, 1);
					nbitr = 0;
					if (nrbitr == 0) {
						if(port)
							send_mesg(&msgl);
						else
							print_mesg(&msgl);
						nrbitr = 8;
					}
				}
				}
				ind++;
			}
		}
	} while (1);


	if(port)
		end_serv();

	endsample();

	exit(0);
}
