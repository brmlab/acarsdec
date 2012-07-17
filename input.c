/*
 *  Copyright (c) 2007 by Thierry Leconte (F4DWV)
 *
 *      $Id: input.c,v 1.3 2007/03/29 16:21:49 f4dwv Exp $
 *
 *   This library is free software; you can redistribute it and/or modify
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
#include <sndfile.h>
#include <alsa/asoundlib.h>

#include "acarsdec.h"

static int source = 0;
static int nbch = 0;

static SNDFILE *inwav;
static int initsnd(char *filename)
{
	SF_INFO infwav;

/* open wav input file */
	infwav.format = 0;
	inwav = sf_open(filename, SFM_READ, &infwav);
	if (inwav == NULL) {
		fprintf(stderr, "could not open %s\n", filename);
		return (0);
	}
	if (infwav.samplerate != 48000) {
		fprintf(stderr,
			"Bad Input File sample rate: %d. Must be 48000\n",
			infwav.samplerate);
		return (0);
	}
	nbch=infwav.channels;
	return (infwav.channels);
}

static snd_pcm_t *capture_handle;
static int initalsa(char *filename)
{
	snd_pcm_hw_params_t *hw_params;
	int err;

	if ((err =
	     snd_pcm_open(&capture_handle, filename,
			  SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		fprintf(stderr, "cannot open audio device %s (%s)\n",
			filename, snd_strerror(err));
		return 0;
	}

	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		fprintf(stderr,
			"cannot allocate hardware parameter structure (%s)\n",
			snd_strerror(err));
		return 0;
	}

	if ((err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0) {
		fprintf(stderr,
			"cannot initialize hardware parameter structure (%s)\n",
			snd_strerror(err));
		return 0;
	}

	if ((err =
	     snd_pcm_hw_params_set_access(capture_handle, hw_params,
					  SND_PCM_ACCESS_RW_INTERLEAVED)) <
	    0) {
		fprintf(stderr, "cannot set access type (%s)\n",
			snd_strerror(err));
		return 0;
	}

	if ((err =
	     snd_pcm_hw_params_set_format(capture_handle, hw_params,
					  SND_PCM_FORMAT_S16)) < 0) {
		fprintf(stderr, "cannot set sample format (%s)\n",
			snd_strerror(err));
		return 0;
	}

	if ((err =
	     snd_pcm_hw_params_set_rate(capture_handle, hw_params, 48000,
					0)) < 0) {
		fprintf(stderr, "cannot set sample rate (%s)\n",
			snd_strerror(err));
		return 0;
	}

	for(nbch=2;nbch>0;nbch--)  {
		if (snd_pcm_hw_params_set_channels(capture_handle, hw_params, nbch)==0)	
			break;
	}

	if (nbch ==0) {
		fprintf(stderr, "cannot set number of channels\n");
		return 0;
	}

        if ((err = snd_pcm_hw_params(capture_handle, hw_params)) < 0) {
                fprintf(stderr, "cannot set parameters (%s)\n",
                        snd_strerror(err));
		return 0;
	}
        snd_pcm_hw_params_free(hw_params);

	if ((err = snd_pcm_prepare(capture_handle)) < 0) {
		fprintf(stderr,
			"cannot prepare audio interface for use (%s)\n",
			snd_strerror(err));
		return 0;
	}
	return nbch;
}

/* open input source*/
int initsample(char *sourcename, int src)
{
	source = src;
	if (src)
		return initsnd(sourcename);
	else
		return initalsa(sourcename);
}

int getsample(short *sample, int nb)
{
	int r;

	if (source) {
		r = sf_read_short(inwav, sample, nb);
		if (r == 0)
			r = -1;	/* this is the end */
	} else {
		r = snd_pcm_readi(capture_handle, sample, nb/nbch);
		if (r <= 0)
			fprintf(stderr,
				"cannot read from interface (%s)\n",
				snd_strerror(r));
		r=r*nbch;
	}
	return r;
}

void endsample(void)
{
	if (source)
		sf_close(inwav);
	else
		snd_pcm_close(capture_handle);
}
