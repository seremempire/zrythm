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

#ifndef __AUDIO_EXT_PORT_H__
#define __AUDIO_EXT_PORT_H__

/**
 * \file
 *
 * External ports.
 */

#include "config.h"

#include "audio/automatable.h"
#include "audio/fader.h"
#include "audio/passthrough_processor.h"
#include "plugins/plugin.h"
#include "utils/audio.h"
#include "utils/yaml.h"

#include <gdk/gdk.h>

#ifdef HAVE_JACK
#include <jack/jack.h>
#endif

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * External port type.
 */
typedef enum ExtPortType
{
  EXT_PORT_TYPE_JACK,
  EXT_PORT_TYPE_ALSA,
} ExtPortType;

static const cyaml_strval_t
ext_port_type_strings[] =
{
	{ "jack",   EXT_PORT_TYPE_JACK    },
	{ "alsa",   EXT_PORT_TYPE_ALSA   },
};

/**
 * External port.
 */
typedef struct ExtPort
{
  /** JACK port. */
  jack_port_t *    jport;

  /** Full port name. */
  char *           full_name;

  /** Short port name. */
  char *           short_name;

  /** Alias #1 if any. */
  char *           alias1;

  /** Alias #2 if any. */
  char *           alias2;

  int              num_aliases;

  ExtPortType      type;
} ExtPort;

static const cyaml_schema_field_t
ext_port_fields_schema[] =
{
  CYAML_FIELD_STRING_PTR (
    "full_name", CYAML_FLAG_POINTER,
    ExtPort, full_name,
   	0, CYAML_UNLIMITED),
  CYAML_FIELD_STRING_PTR (
    "short_name", CYAML_FLAG_POINTER,
    ExtPort, short_name,
   	0, CYAML_UNLIMITED),
  CYAML_FIELD_STRING_PTR (
    "alias1", CYAML_FLAG_POINTER,
    ExtPort, alias1,
   	0, CYAML_UNLIMITED),
  CYAML_FIELD_STRING_PTR (
    "alias2", CYAML_FLAG_POINTER,
    ExtPort, alias2,
   	0, CYAML_UNLIMITED),
  CYAML_FIELD_INT (
    "num_aliases", CYAML_FLAG_DEFAULT,
    ExtPort, num_aliases),
  CYAML_FIELD_ENUM (
    "type", CYAML_FLAG_DEFAULT,
    ExtPort, type, ext_port_type_strings,
    CYAML_ARRAY_LEN (ext_port_type_strings)),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
ext_port_schema =
{
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
	  ExtPort, ext_port_fields_schema),
};

/**
 * Inits the ExtPort after loading a project.
 */
void
ext_port_init_loaded (
  ExtPort * ext_port);

/**
 * Returns the buffer of the external port.
 */
float *
ext_port_get_buffer (
  ExtPort * ext_port,
  int       nframes);

/**
 * Clears the buffer of the external port.
 */
void
ext_port_clear_buffer (
  ExtPort * ext_port,
  int       nframes);

/**
 * Exposes the given Port if not exposed and makes
 * the connection from the Port to the ExtPort (eg in
 * JACK) or backwards.
 *
 * @param src 1 if the ext_port is the source, 0 if it
 *   is the destination.
 */
void
ext_port_connect (
  ExtPort * ext_port,
  Port *    port,
  int       src);

/**
 * Disconnects the Port from the ExtPort.
 *
 * @param src 1 if the ext_port is the source, 0 if it
 *   is the destination.
 */
void
ext_port_disconnect (
  ExtPort * ext_port,
  Port *    port,
  int       src);

#ifdef HAVE_JACK
/**
 * Creates an ExtPort from a JACK port.
 */
ExtPort *
ext_port_from_jack_port (
  jack_port_t * jport);
#endif

/**
 * Collects external ports of the given type.
 *
 * @param hw Hardware or not.
 * @param ports An array of ExtPort pointers to fill
 *   in. The array should be preallocated.
 * @param size Size of the array to fill in.
 */
void
ext_ports_get (
  PortType   type,
  PortFlow   flow,
  int        hw,
  ExtPort ** ports,
  int *      size);

/**
 * Creates a shallow clone of the port.
 */
ExtPort *
ext_port_clone (
  ExtPort * ext_port);

/**
 * Frees an array of ExtPort pointers.
 */
void
ext_ports_free (
  ExtPort ** ext_port,
  int        size);

/**
 * Frees the ext_port.
 */
void
ext_port_free (
  ExtPort * ext_port);

/**
 * @}
 */

#endif