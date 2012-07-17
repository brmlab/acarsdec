/*
 *  Copyright (c) 2007 by Thierry Leconte (F4DWV)
 *
 *      $Id: serv.c,v 1.2 2007/04/22 16:14:41 f4dwv Exp $
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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include "acarsdec.h"

static int sa, sc;

int init_serv(short port)
{
    struct sockaddr_in locaddr, remaddr;
    socklen_t len;
    char c;
    int res;

    sa = socket(PF_INET, SOCK_STREAM, 0);
    if (sa < 0) {
	fprintf(stderr, "socket : %s\n", strerror(errno));
	return -1;
    }

    memset(&locaddr, 0, sizeof(locaddr));
    locaddr.sin_family = AF_INET;
    locaddr.sin_port = htons(port);
    locaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    len = sizeof(locaddr);
    res = bind(sa, (struct sockaddr *) &locaddr, len);
    if (res) {
	fprintf(stderr, "bind : %s\n", strerror(errno));
	return -1;
    }

    res = listen(sa, 1);
    if (res) {
	fprintf(stderr, "listen : %s\n", strerror(errno));
	return -1;
    }

    memset(&remaddr, 0, sizeof(remaddr));
    len = sizeof(remaddr);
    sc = accept(sa, (struct sockaddr *) &remaddr, &len);
    if (sc < 0) {
	fprintf(stderr, "accept : %s\n", strerror(errno));
	return -1;
    }

    do {
	res = read(sc, &c, 1);
    } while (res == 1 && c != '\n');


    return 0;
}


/* convert ACARS position reports to APRS position */
static void toaprs(int la, char lac, int ln, char lnc, int prec, char *out)
{
    int lad, lnd;
    float lam, lnm;

    lad = la / 10000;
    lnd = ln / 10000;
    lam = (float) (la - (lad * 10000)) * 60.0 / 10000.0;
    lnm = (float) (ln - (lnd * 10000)) * 60.0 / 10000.0;

    switch (prec) {
	case 0:
    		sprintf(out, "%02d%02.0f.  %c/%03d%02.0f.  %c^", lad, lam, lac, lnd, lnm, lnc);
		break;
	case 1:
    		sprintf(out, "%02d%04.1f %c/%03d%04.1f %c^", lad, lam, lac, lnd, lnm, lnc);
		break;
	case 2:
	default:
    		sprintf(out, "%02d%05.2f%c/%03d%05.2f%c^", lad, lam, lac, lnd, lnm, lnc);
		break;
    }
}

int posconv(char *txt, unsigned char *label, char *pos)
{
    char lac, lnc;
    int la, ln;
    char las[7], lns[7];
    int n;
    char *p;

/*try different heuristics */

    n = sscanf(txt, "#M1BPOS%c%05d%c%063d,", &lac, &la, &lnc, &ln);
    if (n == 4 && (lac == 'N' || lac == 'S') && (lnc == 'E' || lnc == 'W')) {
	la *= 10;
	ln *= 10;
	toaprs(la, lac, ln, lnc, 1, pos);
	return 0;;
    }
    n = sscanf(txt, "#M1AAEP%c%06d%c%07d", &lac, &la, &lnc, &ln);
    if (n == 4 && (lac == 'N' || lac == 'S') && (lnc == 'E' || lnc == 'W')) {
	toaprs(la, lac, ln, lnc, 2, pos);
	return 0;;
    }

    if (strncmp(txt, "#M1B", 4) == 0) {
	if ((p = strstr(txt, "/FPO")) != NULL) {
	    n = sscanf(p, "/FPO%c%05d%c%06d", &lac, &la, &lnc, &ln);
	    if (n == 4 && (lac == 'N' || lac == 'S')
		&& (lnc == 'E' || lnc == 'W')) {
		la *= 10;
		ln *= 10;
		toaprs(la, lac, ln, lnc, 1, pos);
		return 0;;
	    }
	}
	if ((p = strstr(txt, "/PS")) != NULL) {
	    n = sscanf(p, "/PS%c%05d%c%06d", &lac, &la, &lnc, &ln);
	    if (n == 4 && (lac == 'N' || lac == 'S')
		&& (lnc == 'E' || lnc == 'W')) {
		la *= 10;
		ln *= 10;
		toaprs(la, lac, ln, lnc, 1, pos);
		return 0;;
	    }
	}
    }

    n = sscanf(txt, "FST01%*8s%c%06d%c%07d", &lac, &la, &lnc, &ln);
    if (n == 4 && (lac == 'N' || lac == 'S') && (lnc == 'E' || lnc == 'W')) {
	toaprs(la, lac, ln, lnc, 2, pos);
	return 0;;
    }

    n = sscanf(txt, "(2%c%5c%c%6c", &lac, las, &lnc, lns);
    if (n == 4 && (lac == 'N' || lac == 'S') && (lnc == 'E' || lnc == 'W')) {
	las[5] = 0;
	lns[6] = 0;
	la = 10 * atoi(las);
	ln = 10 * atoi(lns);
	toaprs(la, lac, ln, lnc, 1, pos);
	return 0;;
    }

    n = sscanf(txt, "(:2%c%5c%c%6c", &lac, las, &lnc, lns);
    if (n == 4 && (lac == 'N' || lac == 'S') && (lnc == 'E' || lnc == 'W')) {
	las[5] = 0;
	lns[6] = 0;
	la = 10 * atoi(las);
	ln = 10 * atoi(lns);
	toaprs(la, lac, ln, lnc, 1, pos);
	return 0;;
    }


    n = sscanf(txt, "(2%*4s%c%5c%c%6c", &lac, las, &lnc, lns);
    if (n == 4 && (lac == 'N' || lac == 'S') && (lnc == 'E' || lnc == 'W')) {
	las[5] = 0;
	lns[6] = 0;
	la = 10 * atoi(las);
	ln = 10 * atoi(lns);
	toaprs(la, lac, ln, lnc, 1, pos);
	return 0;;
    }

    n = sscanf(txt, "LAT %c%3c.%3c/LON %c%3c.%3c", &lac, las, &(las[3]),
	       &lnc, lns, &(lns[3]));
    if (n == 6 && (lac == 'N' || lac == 'S') && (lnc == 'E' || lnc == 'W')) {
	las[6] = 0;
	lns[6] = 0;
	la = 10 * atoi(las);
	ln = 10 * atoi(lns);
	toaprs(la, lac, ln, lnc, 1, pos);
	return 0;;
    }


    n = sscanf(txt, "#DFB(POS-%*6s-%04d%c%05d%c/", &la, &lac, &ln, &lnc);
    if (n == 4 && (lac == 'N' || lac == 'S') && (lnc == 'E' || lnc == 'W')) {
	la *= 100;
	ln *= 100;
	toaprs(la, lac, ln, lnc, 0, pos);
	return 0;;
    }

    n = sscanf(txt, "#DFB*POS\a%*8s%c%04d%c%05d/", &lac, &la, &lnc, &ln);
    if (n == 4 && (lac == 'N' || lac == 'S') && (lnc == 'E' || lnc == 'W')) {
	la *= 100;
	ln *= 100;
	toaprs(la, lac, ln, lnc, 0, pos);
	return 0;;
    }

    n = sscanf(txt, "POS%c%05d%c%06d,", &lac, &la, &lnc, &ln);
    if (n == 4 && (lac == 'N' || lac == 'S') && (lnc == 'E' || lnc == 'W')) {
	la *= 10;
	ln *= 10;
	toaprs(la, lac, ln, lnc, 1, pos);
	return 0;;
    }

    n = sscanf(txt, "POS%*2s,%c%05d%c%06d,", &lac, &la, &lnc, &ln);
    if (n == 4 && (lac == 'N' || lac == 'S') && (lnc == 'E' || lnc == 'W')) {
	la *= 10;
	ln *= 10;
	toaprs(la, lac, ln, lnc, 1, pos);
	return 0;;
    }

    n = sscanf(txt, "RCL%*2s,%c%05d%c%06d,", &lac, &la, &lnc, &ln);
    if (n == 4 && (lac == 'N' || lac == 'S') && (lnc == 'E' || lnc == 'W')) {
	la *= 10;
	ln *= 10;
	toaprs(la, lac, ln, lnc, 1, pos);
	return 0;;
    }

    n = sscanf(txt, "TWX%*2s,%c%05d%c%06d,", &lac, &la, &lnc, &ln);
    if (n == 4 && (lac == 'N' || lac == 'S') && (lnc == 'E' || lnc == 'W')) {
	la *= 10;
	ln *= 10;
	toaprs(la, lac, ln, lnc, 1, pos);
	return 0;;
    }

    n = sscanf(txt, "CLA%*2s,%c%05d%c%06d,", &lac, &la, &lnc, &ln);
    if (n == 4 && (lac == 'N' || lac == 'S') && (lnc == 'E' || lnc == 'W')) {
	la *= 10;
	ln *= 10;
	toaprs(la, lac, ln, lnc, 1, pos);
	return 0;;
    }

    n = sscanf(txt, "%c%05d/%c%06d,", &lac, &la, &lnc, &ln);
    if (n == 4 && (lac == 'N' || lac == 'S') && (lnc == 'E' || lnc == 'W')) {
	la *= 10;
	ln *= 10;
	toaprs(la, lac, ln, lnc, 1, pos);
	return 0;;
    }

    return 1;
}

int send_mesg(msg_t * msg)
{
    char apstr[512];
    char txt[512];
    char pos[64];
    unsigned char *ind;

   if(msg->label[0]=='_' && msg->label[1]==0x7f)
	return 0;

    strcpy(txt,msg->txt);
    for(ind = txt;*ind != 0 ;ind++) {
	if(*ind==0x0a || *ind == 0x0d) *ind=' ';
     }

    ind = msg->addr;
    while (*ind == '.' && *ind != 0)
	ind++;

    if (posconv(msg->txt, msg->label, pos))
	sprintf(apstr, "%s>ACARS:>Fid:%s Lbl:%s %s\n", ind, msg->fid,msg->label,txt);
    else
	sprintf(apstr, "%s>ACARS:!%sFid:%s Lbl:%s %s\n", ind, pos,msg->fid,msg->label,txt);

    write(sc, apstr, strlen(apstr));

    return 0;
}


void end_serv(void)
{
    close(sc);
    close(sa);
}
