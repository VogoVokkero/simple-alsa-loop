/*
 * Copyright (c) 2012 Daniel Mack
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */

/*
 * See README
 */
#include "esg-bsp-test.h"



snd_pcm_t *playback_handle, *capture_handle;

uint8_t buf[BUFFER_SZ_BYTES];
void *ch_bufs[CHANNELS] = {0};

static unsigned int rate = RATE;
static unsigned int format = SND_PCM_FORMAT_S32_LE;

static unsigned long int buffer_sz_frames = BUFFER_SZ_FRAMES;

static int open_stream(snd_pcm_t **handle, const char *name, int dir, int mode)
{
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t *sw_params;
	const char *dirname = (dir == SND_PCM_STREAM_PLAYBACK) ? "PLAYBACK" : "CAPTURE";
	int err;
	int dummy;

	if ((err = snd_pcm_open(handle, name, dir, mode)) < 0)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("cannot open audio device "));
		return err;
	}

	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("cannot allocate hardware parameter structure"));
		return err;
	}

	if ((err = snd_pcm_hw_params_any(*handle, hw_params)) < 0)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("cannot initialize hardware parameter structure"));
		return err;
	}

	if ((err = snd_pcm_hw_params_set_access(*handle, hw_params, SAMPLE_ACCESS)) < 0)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("cannot set access type"));
		return err;
	}

	if ((err = snd_pcm_hw_params_set_format(*handle, hw_params, format)) < 0)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("cannot set sample format"));
		return err;
	}

	if ((err = snd_pcm_hw_params_set_rate_near(*handle, hw_params, &rate, NULL)) < 0)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("cannot set sample rate"));
		return err;
	}

	if ((err = snd_pcm_hw_params_set_channels(*handle, hw_params, CHANNELS)) < 0)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("cannot set channel count"));
		return err;
	}

	if ((err = snd_pcm_hw_params_set_buffer_size_near(*handle, hw_params, &buffer_sz_frames)) < 0)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("set_buffer_time"));
		return err;
	}

	if ((err = snd_pcm_hw_params(*handle, hw_params)) < 0)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("cannot set parameters"));
		return err;
	}

	snd_pcm_hw_params_free(hw_params);

	if ((err = snd_pcm_sw_params_malloc(&sw_params)) < 0)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("cannot allocate software parameters structure"));
		return err;
	}
	if ((err = snd_pcm_sw_params_current(*handle, sw_params)) < 0)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("cannot initialize software parameters structure"));
		return err;
	}
	if ((err = snd_pcm_sw_params_set_avail_min(*handle, sw_params, PERIOD_SZ_FRAMES)) < 0)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("cannot set minimum available count"));
		return err;
	}

	if ((err = snd_pcm_sw_params_set_start_threshold(*handle, sw_params, 0U)) < 0)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("cannot set start mode"));
		return err;
	}
	if ((err = snd_pcm_sw_params(*handle, sw_params)) < 0)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("cannot set software parameters"));
		return err;
	}

	return 0;
}

int audio_init(void)
{
	int err;
	unsigned int period_cnt = 0;

	printf("using " ALSA_DEVICE "\n");
	printf("Using %s access\n", (SAMPLE_ACCESS == SND_PCM_ACCESS_RW_NONINTERLEAVED) ? "non-interleaved" : "interleaved");

	if ((err = open_stream(&playback_handle, ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0)
		return err;

	if ((err = open_stream(&capture_handle, ALSA_DEVICE, SND_PCM_STREAM_CAPTURE, 0)) < 0)
		return err;

	if ((err = snd_pcm_prepare(playback_handle)) < 0)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("cannot prepare audio interface for use"));
		return err;
	}

	if ((err = snd_pcm_prepare(capture_handle)) < 0)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("cannot prepare audio interface for use"));
		return err;
	}

	if ((err = snd_pcm_start(capture_handle)) < 0)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("cannot start audio interface for use"));
		return err;
	}

	/* actual sample buffer */
	memset(buf, 0, sizeof(buf));

	/* non-interleaved channel buffer offets */
	for (int c = 0; c < CHANNELS; c++)
	{
		ch_bufs[c] = (void *)((unsigned char *)buf + (c * SAMPLE_SZ_BYTES * PERIOD_SZ_FRAMES));
	}

	while (1)
	{
		int avail;
		snd_pcm_sframes_t r = 0;

		period_cnt++;

		if ((err = snd_pcm_wait(playback_handle, 1000)) < 0)
		{
			DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("poll failed"));
			break;
		}

		avail = snd_pcm_avail_update(capture_handle);
		if (avail > 0)
		{
			if (avail > BUFFER_SZ_BYTES)
				avail = BUFFER_SZ_BYTES;

			if (SAMPLE_ACCESS == SND_PCM_ACCESS_RW_NONINTERLEAVED)
			{
				r = snd_pcm_readn(capture_handle, ch_bufs, avail);
			}
			else
			{
				r = snd_pcm_readi(capture_handle, buf, avail);
			}

			if (0 == (period_cnt & 0xF))
			{
				unsigned long *pbuf = (unsigned long*)(buf);
				printf("r = %04ld:%08lx-%08lx-%08lx-%08lx\n", r, pbuf[0], pbuf[1], pbuf[2], pbuf[3]);
			}
		}

		avail = snd_pcm_avail_update(playback_handle);
		if (avail > 0)
		{
			if (avail > BUFFER_SZ_BYTES)
				avail = BUFFER_SZ_BYTES;

			if (SAMPLE_ACCESS == SND_PCM_ACCESS_RW_NONINTERLEAVED)
			{
				r = snd_pcm_writen(playback_handle, ch_bufs, avail);
			}
			else
			{
				r = snd_pcm_writei(playback_handle, buf, avail);
			}
		}
	}

	snd_pcm_close(playback_handle);
	snd_pcm_close(capture_handle);
	return 0;
}
