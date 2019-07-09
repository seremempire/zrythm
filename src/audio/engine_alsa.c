/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef __linux__

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "audio/engine.h"
#include "audio/engine_alsa.h"
#include "audio/midi.h"
#include "audio/port.h"
#include "project.h"

#include <alsa/asoundlib.h>
#include <pthread.h>

#define POLY 10

static int
set_sw_params (AudioEngine * self)
{
  int err;

  /* get current swparams */
  err =
    snd_pcm_sw_params_current (
      self->playback_handle, self->sw_params);
  if (err < 0)
    {
      g_warning (
        "Cannot init sw params: %s",
        snd_strerror (err));
      return err;
    }

  /* allow the transfer when at least period_size
   * samples can be processed */
  g_warn_if_fail (self->block_length > 0);
  err =
    snd_pcm_sw_params_set_avail_min (
      self->playback_handle, self->sw_params,
      self->block_length);
  if (err < 0)
    {
      g_warning (
        "Cannot set avail min: %s",
        snd_strerror (err));
      return err;
    }

  /* start the transfer whena period is full */
  err =
    snd_pcm_sw_params_set_start_threshold (
      self->playback_handle,
      self->sw_params,
      self->block_length);
  if (err < 0)
    {
      g_warning (
        "Cannot set start threshold: %s",
        snd_strerror (err));
      return err;
    }

  /* write the parameters to the playback device */
  err =
    snd_pcm_sw_params (
      self->playback_handle, self->sw_params);
  if (err < 0)
    {
      g_warning (
        "Cannot set sw params: %s",
        snd_strerror (err));
      return err;
    }

  return 0;
}

static void
set_hw_params (
  AudioEngine * self)
{
  int err, dir;

  /* choose all parameters */
  err =
    snd_pcm_hw_params_any (
      self->playback_handle, self->hw_params);
  if (err < 0)
    g_warning (
      "Failed to choose all parameters: %s",
      snd_strerror (err));

  /* set interleaved read/write format */
  err =
    snd_pcm_hw_params_set_access (
      self->playback_handle, self->hw_params,
      SND_PCM_ACCESS_RW_INTERLEAVED);
  if (err < 0)
    g_warning (
      "Cannot set access type: %s",
      snd_strerror (err));

  /* set the sample format */
  err =
    snd_pcm_hw_params_set_format (
      self->playback_handle, self->hw_params,
      SND_PCM_FORMAT_FLOAT_LE);
  if (err < 0)
    g_warning (
      "Cannot set format: %s",
      snd_strerror (err));

  /* set num channels */
  err =
    snd_pcm_hw_params_set_channels (
      self->playback_handle, self->hw_params, 2);
  if (err < 0)
    g_warning (
      "Cannot set channels: %s",
      snd_strerror (err));

  /* set sample rate */
  err =
    snd_pcm_hw_params_set_rate_near (
      self->playback_handle, self->hw_params,
      &self->sample_rate, 0);
  if (err < 0)
    g_warning (
      "Cannot set sample rate: %s",
      snd_strerror (err));

  /* set periods */
  snd_pcm_hw_params_set_periods (
    self->playback_handle, self->hw_params, 2, 0);

  snd_pcm_uframes_t period_size =
    self->block_length;
  dir = 0;
  err =
    snd_pcm_hw_params_set_period_size_near (
      self->playback_handle, self->hw_params,
      &period_size, &dir);
  if (err < 0)
    g_warning (
      "Unable to set perriod size %lu for playback: "
      "%s",
      period_size,
      snd_strerror (err));
  dir = 0;
  err =
    snd_pcm_hw_params_get_period_size (
      self->hw_params, &period_size, &dir);
  if (err < 0)
    g_warning (
      "Unable to perriod size %lu for playback: %s",
      period_size,
      snd_strerror (err));
  self->block_length = period_size;

  snd_pcm_uframes_t buffer_size =
    period_size * 2; /* 2 channels */
  err =
    snd_pcm_hw_params_set_buffer_size_near (
      self->playback_handle, self->hw_params,
      &buffer_size);
  if (err < 0)
    g_warning (
      "Unable to set buffer size %lu for playback: %s",
      buffer_size,
      snd_strerror (err));
  err =
    snd_pcm_hw_params_get_buffer_size (
      self->hw_params, &buffer_size);

  if (2 * period_size  > buffer_size)
    g_warning ("Buffer %lu too small",
               buffer_size);

	// write the parameters to device
  err =
    snd_pcm_hw_params (
      self->playback_handle, self->hw_params);
  if (err < 0)
    g_warning (
      "Cannot set hw params: %s",
      snd_strerror (err));
}

static inline int
process_cb (
  AudioEngine* self,
  snd_pcm_sframes_t nframes)
{
  self->nframes = nframes;

  memset(self->alsa_out_buf, 0, nframes * 2);
  engine_process (self, nframes);

  return
    snd_pcm_writei (
      self->playback_handle,
      self->alsa_out_buf,
      nframes);
}

static inline int
midi_callback (
  AudioEngine* self)
{
  snd_seq_event_t *ev;

  zix_sem_wait (
    &self->midi_in->midi_events->access_sem);

  int time = 0;

  do
    {
      snd_seq_event_input (
        self->seq_handle, &ev);
      switch (ev->type)
        {
        /* see https://www.alsa-project.org/alsa-doc/alsa-lib/group___seq_events.html for more */
        case SND_SEQ_EVENT_PITCHBEND:
          g_message ("pitch %d",
                     ev->data.control.value);
          midi_events_add_pitchbend (
            self->midi_in->midi_events, 1,
            ev->data.control.value,
            time++, 1);
          break;
        case SND_SEQ_EVENT_CONTROLLER:
          g_message ("modulation %d",
                     ev->data.control.value);
          midi_events_add_control_change (
            self->midi_in->midi_events,
            1, ev->data.control.param,
            ev->data.control.value,
            time++, 1);
          break;
        case SND_SEQ_EVENT_NOTEON:
          g_message ("note on: note %d vel %d",
                     ev->data.note.note,
                     ev->data.note.velocity);
          /*g_message ("time %u:%u",*/
                     /*ev->time.time.tv_sec,*/
                     /*ev->time.time.tv_nsec);*/
          midi_events_add_note_on (
            self->midi_in->midi_events,
            1, ev->data.note.note,
            ev->data.note.velocity,
            time++, 1);
          break;
        case SND_SEQ_EVENT_NOTEOFF:
          g_message ("note off: note %d",
                     ev->data.note.note);
          midi_events_add_note_off (
            self->midi_in->midi_events,
            1, ev->data.note.note,
            time++, 1);
          /* FIXME passing ticks, should pass
           * frames */
          break;
        default:
          g_message ("Unknown MIDI event received");
          break;
      }
      snd_seq_free_event(ev);
    } while (
        snd_seq_event_input_pending (
          self->seq_handle, 0) > 0);

  zix_sem_post (
    &self->midi_in->midi_events->access_sem);

  return 0;
}

static void *
audio_thread (void * _self)
{
  AudioEngine * self = (AudioEngine *) _self;

  int err;
  err =
    snd_pcm_open(
      &self->playback_handle, "default",
      SND_PCM_STREAM_PLAYBACK, 0);
  if (err < 0)
    g_warning (
      "Cannot open audio device: %s",
      snd_strerror (err));

  /* set hw params */
  err =
    snd_pcm_hw_params_malloc (&self->hw_params);
  if (err < 0)
    g_warning (
      "Cannot allocate hw params: %s",
      snd_strerror (err));
  set_hw_params (self);

  /* set sw params */
  err = snd_pcm_sw_params_malloc (&self->sw_params);
  if (err < 0)
    g_warning (
      "Cannot allocate sw params: %s",
      snd_strerror (err));
  set_sw_params (self);
  g_warn_if_fail (self->block_length > 0);

  engine_realloc_port_buffers (
    self, self->block_length);
  engine_update_frames_per_tick (
    TRANSPORT->beats_per_bar,
    TRANSPORT->bpm,
    self->sample_rate);

  g_usleep (8000);

  struct pollfd *pfds;
  int l1, nfds =
    snd_pcm_poll_descriptors_count (
      self->playback_handle);
  pfds =
    (struct pollfd *)
    alloca(nfds * sizeof(struct pollfd));
  snd_pcm_poll_descriptors (
    self->playback_handle, pfds, nfds);
  int frames_processed;
  while (1)
    {
      if (poll (pfds, nfds, 1000) > 0)
        {
          for (l1 = 0; l1 < nfds; l1++)
            {
              while (pfds[l1].revents > 0)
                {
                  frames_processed =
                    process_cb (
                      self, self->block_length);
                  if (frames_processed <
                        self->block_length)
                    {
                      g_warning ("XRUN");
                      snd_pcm_prepare (
                        self->playback_handle);
                    }
                }
            }
        }
    }
  return NULL;
}

static void *
midi_thread (void * _self)
{
  AudioEngine * self = (AudioEngine *) _self;

  int err;

  if ((err = snd_seq_open (
        &self->seq_handle,
        "default", SND_SEQ_OPEN_DUPLEX, 0)) < 0)
    g_warning ("Error opening ALSA sequencer: %s",
               snd_strerror (err));

  snd_seq_set_client_name (
    self->seq_handle, "Zrythm");
  if (snd_seq_create_simple_port (
        self->seq_handle, "Zrythm MIDI",
      SND_SEQ_PORT_CAP_WRITE |
        SND_SEQ_PORT_CAP_SUBS_WRITE,
      SND_SEQ_PORT_TYPE_APPLICATION) < 0)
    g_warning ("Error creating sequencer port");

  int seq_nfds, l1;
  struct pollfd *pfds;
  seq_nfds =
    snd_seq_poll_descriptors_count (
      self->seq_handle, POLLIN);
  pfds =
    (struct pollfd *)
    alloca (
      sizeof (struct pollfd) * seq_nfds);
  snd_seq_poll_descriptors (
    self->seq_handle, pfds, seq_nfds, POLLIN);
  while (1)
    {
      if (poll (pfds, seq_nfds, 1000) > 0)
        {
          for (l1 = 0; l1 < seq_nfds; l1++)
            {
               if (pfds[l1].revents > 0)
                 midi_callback (self);
            }
        }
    }

  return NULL;
}

/**
 * Copy the cached MIDI events to the MIDI events
 * in the MIDI in port, used at the start of each
 * cycle. */
void
engine_alsa_receive_midi_events (
  AudioEngine * self,
  int           print)
{
  midi_events_dequeue (
    self->midi_in->midi_events);
}

int
alsa_setup (
  AudioEngine *self, int loading)
{
  self->block_length = 512;
  self->sample_rate = 44100;
  self->midi_buf_size = 4096;
  self->alsa_out_buf =
    (float *) malloc (
      2 * sizeof (float) * self->block_length);

  Port *stereo_out_l, *stereo_out_r,
      *stereo_in_l, *stereo_in_r;

  stereo_out_l =
    port_new_with_type (
      TYPE_AUDIO, FLOW_OUTPUT,
      "ALSA Stereo Out / L");
  stereo_out_r =
    port_new_with_type (
      TYPE_AUDIO, FLOW_OUTPUT,
      "ALSA Stereo Out / R");
  stereo_in_l =
    port_new_with_type (
      TYPE_AUDIO, FLOW_INPUT,
      "ALSA Stereo In / L");
  stereo_in_r =
    port_new_with_type (
      TYPE_AUDIO, FLOW_INPUT,
      "ALSA Stereo In / R");

  stereo_in_l->identifier.owner_type =
    PORT_OWNER_TYPE_BACKEND;
  stereo_in_r->identifier.owner_type =
    PORT_OWNER_TYPE_BACKEND;
  stereo_out_l->identifier.owner_type =
    PORT_OWNER_TYPE_BACKEND;
  stereo_out_r->identifier.owner_type =
    PORT_OWNER_TYPE_BACKEND;

  self->stereo_out =
    stereo_ports_new (stereo_out_l, stereo_out_r);
  self->stereo_in =
    stereo_ports_new (stereo_in_l, stereo_in_r);

  pthread_t thread_id;
  pthread_create(
    &thread_id, NULL,
    &audio_thread, self);

  g_message ("ALSA setup complete");

  return 0;
}

int
alsa_midi_setup (
  AudioEngine * self,
  int           loading)
{
  if (loading)
    {
    }
  else
    {
      self->midi_in =
        port_new_with_type (
          TYPE_EVENT,
          FLOW_INPUT,
          "ALSA MIDI In");
      self->midi_in->identifier.owner_type =
        PORT_OWNER_TYPE_BACKEND;
    }

  /* init queue */
  self->midi_in->midi_events =
    midi_events_new (
      self->midi_in);

  pthread_t thread_id;
  pthread_create(
    &thread_id, NULL,
    &midi_thread, self);

  return 0;
}

#endif