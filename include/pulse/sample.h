#ifndef foosamplehfoo
#define foosamplehfoo

/* $Id: sample.h 1449 2007-05-23 16:57:35Z lennart $ */

/***
  This file is part of PulseAudio.

  Copyright 2004-2006 Lennart Poettering
  Copyright 2006 Pierre Ossman <ossman@cendio.se> for Cendio AB

  PulseAudio is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 2 of the License,
  or (at your option) any later version.

  PulseAudio is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with PulseAudio; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#include <inttypes.h>
#include <sys/types.h>
#include <math.h>

#include <pulse/cdecl.h>

/** \page sample Sample Format Specifications
 *
 * \section overv_sec Overview
 *
 * PulseAudio is capable of handling a multitude of sample formats, rates
 * and channels, transparently converting and mixing them as needed.
 *
 * \section format_sec Sample Format
 *
 * PulseAudio supports the following sample formats:
 *
 * \li PA_SAMPLE_U8 - Unsigned 8 bit PCM.
 * \li PA_SAMPLE_S16LE - Signed 16 bit PCM, little endian.
 * \li PA_SAMPLE_S16BE - Signed 16 bit PCM, big endian.
 * \li PA_SAMPLE_FLOAT32LE - 32 bit IEEE floating point PCM, little endian.
 * \li PA_SAMPLE_FLOAT32BE - 32 bit IEEE floating point PCM, big endian.
 * \li PA_SAMPLE_ALAW - 8 bit a-Law.
 * \li PA_SAMPLE_ULAW - 8 bit mu-Law.
 *
 * The floating point sample formats have the range from -1 to 1.
 *
 * The sample formats that are sensitive to endianness have convenience
 * macros for native endian (NE), and reverse endian (RE).
 *
 * \section rate_sec Sample Rates
 *
 * PulseAudio supports any sample rate between 1 Hz and 4 GHz. There is no
 * point trying to exceed the sample rate of the output device though as the
 * signal will only get downsampled, consuming CPU on the machine running the
 * server.
 *
 * \section chan_sec Channels
 *
 * PulseAudio supports up to 16 individiual channels. The order of the
 * channels is up to the application, but they must be continous. To map
 * channels to speakers, see \ref channelmap.
 *
 * \section calc_sec Calculations
 *
 * The PulseAudio library contains a number of convenience functions to do
 * calculations on sample formats:
 *
 * \li pa_bytes_per_second() - The number of bytes one second of audio will
 *                             take given a sample format.
 * \li pa_frame_size() - The size, in bytes, of one frame (i.e. one set of
 *                       samples, one for each channel).
 * \li pa_sample_size() - The size, in bytes, of one sample.
 * \li pa_bytes_to_usec() - Calculate the time it would take to play a buffer
 *                          of a certain size.
 *
 * \section util_sec Convenience Functions
 *
 * The library also contains a couple of other convenience functions:
 *
 * \li pa_sample_spec_valid() - Tests if a sample format specification is
 *                              valid.
 * \li pa_sample_spec_equal() - Tests if the sample format specifications are
 *                              identical.
 * \li pa_sample_format_to_string() - Return a textual description of a
 *                                    sample format.
 * \li pa_parse_sample_format() - Parse a text string into a sample format.
 * \li pa_sample_spec_snprint() - Create a textual description of a complete
 *                                 sample format specification.
 * \li pa_bytes_snprint() - Pretty print a byte value (e.g. 2.5 MiB).
 */

/** \file
 * Constants and routines for sample type handling */

PA_C_DECL_BEGIN

/** Maximum number of allowed channels */
#define PA_CHANNELS_MAX 32

/** Maximum allowed sample rate */
#define PA_RATE_MAX (48000*4)

/** Sample format */
typedef enum pa_sample_format {
    PA_SAMPLE_U8,              /**< Unsigned 8 Bit PCM */
    PA_SAMPLE_ALAW,            /**< 8 Bit a-Law */
    PA_SAMPLE_ULAW,            /**< 8 Bit mu-Law */
    PA_SAMPLE_S16LE,           /**< Signed 16 Bit PCM, little endian (PC) */
    PA_SAMPLE_S16BE,           /**< Signed 16 Bit PCM, big endian */
    PA_SAMPLE_FLOAT32LE,       /**< 32 Bit IEEE floating point, little endian, range -1 to 1 */
    PA_SAMPLE_FLOAT32BE,       /**< 32 Bit IEEE floating point, big endian, range -1 to 1 */
    PA_SAMPLE_MAX,             /**< Upper limit of valid sample types */
    PA_SAMPLE_INVALID = -1     /**< An invalid value */
} pa_sample_format_t;

#ifdef WORDS_BIGENDIAN
/** Signed 16 Bit PCM, native endian */
#define PA_SAMPLE_S16NE PA_SAMPLE_S16BE
/** 32 Bit IEEE floating point, native endian */
#define PA_SAMPLE_FLOAT32NE PA_SAMPLE_FLOAT32BE
/** Signed 16 Bit PCM reverse endian */
#define PA_SAMPLE_S16RE PA_SAMPLE_S16LE
/** 32 Bit IEEE floating point, reverse endian */
#define PA_SAMPLE_FLOAT32RE PA_SAMPLE_FLOAT32LE
#else
/** Signed 16 Bit PCM, native endian */
#define PA_SAMPLE_S16NE PA_SAMPLE_S16LE
/** 32 Bit IEEE floating point, native endian */
#define PA_SAMPLE_FLOAT32NE PA_SAMPLE_FLOAT32LE
/** Signed 16 Bit PCM reverse endian */
#define PA_SAMPLE_S16RE PA_SAMPLE_S16BE
/** 32 Bit IEEE floating point, reverse endian */
#define PA_SAMPLE_FLOAT32RE PA_SAMPLE_FLOAT32BE
#endif

/** A Shortcut for PA_SAMPLE_FLOAT32NE */
#define PA_SAMPLE_FLOAT32 PA_SAMPLE_FLOAT32NE

/** A sample format and attribute specification */
typedef struct pa_sample_spec {
    pa_sample_format_t format;     /**< The sample format */
    uint32_t rate;                 /**< The sample rate. (e.g. 44100) */
    uint8_t channels;              /**< Audio channels. (1 for mono, 2 for stereo, ...) */
} pa_sample_spec;

/** Type for usec specifications (unsigned). May be either 32 or 64 bit, depending on the architecture */
typedef uint64_t pa_usec_t;

/** Return the amount of bytes playback of a second of audio with the specified sample type takes */
size_t pa_bytes_per_second(const pa_sample_spec *spec);

/** Return the size of a frame with the specific sample type */
size_t pa_frame_size(const pa_sample_spec *spec);

/** Return the size of a sample with the specific sample type */
size_t pa_sample_size(const pa_sample_spec *spec);

/** Calculate the time the specified bytes take to play with the specified sample type */
pa_usec_t pa_bytes_to_usec(uint64_t length, const pa_sample_spec *spec);

/** Calculates the number of bytes that are required for the specified time. \since 0.9 */
size_t pa_usec_to_bytes(pa_usec_t t, const pa_sample_spec *spec);

/** Return non-zero when the sample type specification is valid */
int pa_sample_spec_valid(const pa_sample_spec *spec);

/** Return non-zero when the two sample type specifications match */
int pa_sample_spec_equal(const pa_sample_spec*a, const pa_sample_spec*b);

/** Return a descriptive string for the specified sample format. \since 0.8 */
const char *pa_sample_format_to_string(pa_sample_format_t f);

/** Parse a sample format text. Inverse of pa_sample_format_to_string() */
pa_sample_format_t pa_parse_sample_format(const char *format);

/** Maximum required string length for pa_sample_spec_snprint() */
#define PA_SAMPLE_SPEC_SNPRINT_MAX 32

/** Pretty print a sample type specification to a string */
char* pa_sample_spec_snprint(char *s, size_t l, const pa_sample_spec *spec);

/** Pretty print a byte size value. (i.e. "2.5 MiB") */
char* pa_bytes_snprint(char *s, size_t l, unsigned v);

PA_C_DECL_END

#endif
