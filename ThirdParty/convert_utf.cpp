#ifndef CONVERT_UTF_IMPLEMENTATION_FILE
#define CONVERT_UTF_IMPLEMENTATION_FILE

/*****************************************************************************\
*                                                                             *
*  Name   : convert_utf                                                       *
*  Author : Unicode, Inc. (C conversion functions)                            *
*  Author : Chris Koeritz (C++ conversion classes)                            *
*                                                                             *
*******************************************************************************
* Copyright (c) 2006-$now By Author.  This program is free software; you can  *
* redistribute it and/or modify it under the terms of the GNU General Public  *
* License as published by the Free Software Foundation; either version 2 of   *
* the License or (at your option) any later version.  This is online at:      *
*     http://www.fsf.org/copyleft/gpl.html                                    *
* Please send any updates to: fred@gruntose.com                               *
\*****************************************************************************/

//copyright below is relevant to UTF conversion methods only.
/*
 * Copyright 2001-$now Unicode, Inc.
 * 
 * Disclaimer
 * 
 * This source code is provided as is by Unicode, Inc. No claims are
 * made as to fitness for any particular purpose. No warranties of any
 * kind are expressed or implied. The recipient agrees to determine
 * applicability of information provided. If this file has been
 * purchased on magnetic or optical media from Unicode, Inc., the
 * sole remedy for any claim will be exchange of defective media
 * within 90 days of receipt.
 * 
 * Limitations on Rights to Redistribute This Code
 * 
 * Unicode, Inc. hereby grants the right to freely use the information
 * supplied in this file in the creation of products supporting the
 * Unicode Standard, and to make copies of this file in any form
 * for internal or external distribution as long as this notice
 * remains attached.
 */

/* ---------------------------------------------------------------------

    Conversions between UTF32, UTF-16, and UTF-8. Source code file.
    Author: Mark E. Davis, 1994.
    Rev History: Rick McGowan, fixes & updates May 2001.
    Sept 2001: fixed const & error conditions per
        mods suggested by S. Parent & A. Lillich.
    June 2002: Tim Dodd added detection and handling of incomplete
        source sequences, enhanced error detection, added casts
        to eliminate compiler warnings.
    July 2003: slight mods to back out aggressive FFFE detection.
    Jan 2004: updated switches in from-UTF8 conversions.
    Oct 2004: updated to use UNI_MAX_LEGAL_UTF32 in UTF-32 conversions.

    See the header file "ConvertUTF.h" for complete documentation.

------------------------------------------------------------------------ */

#include "convert_utf.h"
#ifdef CVTUTF_DEBUG
  #include <stdio.h>
#endif
#ifdef __cplusplus
  #include "istring.h"
  #include <string.h>
  #include <wchar.h>
#endif

static const int halfShift  = 10; /* used for shifting by 10 bits */

static const UTF32 halfBase = 0x0010000UL;
static const UTF32 halfMask = 0x3FFUL;

#define UNI_SUR_HIGH_START  (UTF32)0xD800
#define UNI_SUR_HIGH_END    (UTF32)0xDBFF
#define UNI_SUR_LOW_START   (UTF32)0xDC00
#define UNI_SUR_LOW_END     (UTF32)0xDFFF

/* --------------------------------------------------------------------- */

ConversionResult ConvertUTF32toUTF16 (
  const UTF32** sourceStart, const UTF32* sourceEnd, 
  UTF16** targetStart, UTF16* targetEnd, ConversionFlags flags) {
    ConversionResult result = conversionOK;
    const UTF32* source = *sourceStart;
    UTF16* target = *targetStart;
    while (source < sourceEnd) {
  UTF32 ch;
  if (target >= targetEnd) {
      result = targetExhausted; break;
  }
  ch = *source++;
  if (ch <= UNI_MAX_BMP) { /* Target is a character <= 0xFFFF */
      /* UTF-16 surrogate values are illegal in UTF-32; 0xffff or 0xfffe are both reserved values */
      if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END) {
    if (flags == strictConversion) {
        --source; /* return to the illegal value itself */
        result = sourceIllegal;
        break;
    } else {
        *target++ = UNI_REPLACEMENT_CHAR;
    }
      } else {
    *target++ = (UTF16)ch; /* normal case */
      }
  } else if (ch > UNI_MAX_LEGAL_UTF32) {
      if (flags == strictConversion) {
    result = sourceIllegal;
      } else {
    *target++ = UNI_REPLACEMENT_CHAR;
      }
  } else {
      /* target is a character in range 0xFFFF - 0x10FFFF. */
      if (target + 1 >= targetEnd) {
    --source; /* Back up source pointer! */
    result = targetExhausted; break;
      }
      ch -= halfBase;
      *target++ = (UTF16)((ch >> halfShift) + UNI_SUR_HIGH_START);
      *target++ = (UTF16)((ch & halfMask) + UNI_SUR_LOW_START);
  }
    }
    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* --------------------------------------------------------------------- */

ConversionResult ConvertUTF16toUTF32 (
  const UTF16** sourceStart, const UTF16* sourceEnd, 
  UTF32** targetStart, UTF32* targetEnd, ConversionFlags flags) {
    ConversionResult result = conversionOK;
    const UTF16* source = *sourceStart;
    UTF32* target = *targetStart;
    UTF32 ch, ch2;
    while (source < sourceEnd) {
  const UTF16* oldSource = source; /*  In case we have to back up because of target overflow. */
  ch = *source++;
  /* If we have a surrogate pair, convert to UTF32 first. */
  if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END) {
      /* If the 16 bits following the high surrogate are in the source buffer... */
      if (source < sourceEnd) {
    ch2 = *source;
    /* If it's a low surrogate, convert to UTF32. */
    if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) {
        ch = ((ch - UNI_SUR_HIGH_START) << halfShift)
      + (ch2 - UNI_SUR_LOW_START) + halfBase;
        ++source;
    } else if (flags == strictConversion) { /* it's an unpaired high surrogate */
        --source; /* return to the illegal value itself */
        result = sourceIllegal;
        break;
    }
      } else { /* We don't have the 16 bits following the high surrogate. */
    --source; /* return to the high surrogate */
    result = sourceExhausted;
    break;
      }
  } else if (flags == strictConversion) {
      /* UTF-16 surrogate values are illegal in UTF-32 */
      if (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END) {
    --source; /* return to the illegal value itself */
    result = sourceIllegal;
    break;
      }
  }
  if (target >= targetEnd) {
      source = oldSource; /* Back up source pointer! */
      result = targetExhausted; break;
  }
  *target++ = ch;
    }
    *sourceStart = source;
    *targetStart = target;
#ifdef CVTUTF_DEBUG
if (result == sourceIllegal) {
    fprintf(stderr, "ConvertUTF16toUTF32 illegal seq 0x%04x,%04x\n", ch, ch2);
    fflush(stderr);
}
#endif
    return result;
}

/* --------------------------------------------------------------------- */

/*
 * Index into the table below with the first byte of a UTF-8 sequence to
 * get the number of trailing bytes that are supposed to follow it.
 * Note that *legal* UTF-8 values can't have 4 or 5-bytes. The table is
 * left as-is for anyone who may want to do such conversion, which was
 * allowed in earlier algorithms.
 */
static const char trailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

/*
 * Magic values subtracted from a buffer value during UTF8 conversion.
 * This table contains as many values as there might be trailing bytes
 * in a UTF-8 sequence.
 */
static const UTF32 offsetsFromUTF8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL, 
         0x03C82080UL, 0xFA082080UL, 0x82082080UL };

/*
 * Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
 * into the first byte, depending on how many bytes follow.  There are
 * as many entries in this table as there are UTF-8 sequence types.
 * (I.e., one byte sequence, two byte... etc.). Remember that sequencs
 * for *legal* UTF-8 will be 4 or fewer bytes total.
 */
static const UTF8 firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

/* --------------------------------------------------------------------- */

/* The interface converts a whole buffer to avoid function-call overhead.
 * Constants have been gathered. Loops & conditionals have been removed as
 * much as possible for efficiency, in favor of drop-through switches.
 * (See "Note A" at the bottom of the file for equivalent code.)
 * If your compiler supports it, the "isLegalUTF8" call can be turned
 * into an inline function.
 */

/* --------------------------------------------------------------------- */

ConversionResult ConvertUTF16toUTF8 (
  const UTF16** sourceStart, const UTF16* sourceEnd, 
  UTF8** targetStart, UTF8* targetEnd, ConversionFlags flags) {
    ConversionResult result = conversionOK;
    const UTF16* source = *sourceStart;
    UTF8* target = *targetStart;
    while (source < sourceEnd) {
  UTF32 ch;
  unsigned short bytesToWrite = 0;
  const UTF32 byteMask = 0xBF;
  const UTF32 byteMark = 0x80; 
  const UTF16* oldSource = source; /* In case we have to back up because of target overflow. */
  ch = *source++;
  /* If we have a surrogate pair, convert to UTF32 first. */
  if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END) {
      /* If the 16 bits following the high surrogate are in the source buffer... */
      if (source < sourceEnd) {
    UTF32 ch2 = *source;
    /* If it's a low surrogate, convert to UTF32. */
    if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) {
        ch = ((ch - UNI_SUR_HIGH_START) << halfShift)
      + (ch2 - UNI_SUR_LOW_START) + halfBase;
        ++source;
    } else if (flags == strictConversion) { /* it's an unpaired high surrogate */
        --source; /* return to the illegal value itself */
        result = sourceIllegal;
        break;
    }
      } else { /* We don't have the 16 bits following the high surrogate. */
    --source; /* return to the high surrogate */
    result = sourceExhausted;
    break;
      }
  } else if (flags == strictConversion) {
      /* UTF-16 surrogate values are illegal in UTF-32 */
      if (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END) {
    --source; /* return to the illegal value itself */
    result = sourceIllegal;
    break;
      }
  }
  /* Figure out how many bytes the result will require */
  if (ch < (UTF32)0x80) {       bytesToWrite = 1;
  } else if (ch < (UTF32)0x800) {     bytesToWrite = 2;
  } else if (ch < (UTF32)0x10000) {   bytesToWrite = 3;
  } else if (ch < (UTF32)0x110000) {  bytesToWrite = 4;
  } else {          bytesToWrite = 3;
              ch = UNI_REPLACEMENT_CHAR;
  }

  target += bytesToWrite;
  if (target > targetEnd) {
      source = oldSource; /* Back up source pointer! */
      target -= bytesToWrite; result = targetExhausted; break;
  }
  switch (bytesToWrite) { /* note: everything falls through. */
      case 4: *--target = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
      case 3: *--target = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
      case 2: *--target = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
      case 1: *--target =  (UTF8)(ch | firstByteMark[bytesToWrite]);
  }
  target += bytesToWrite;
    }
    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* --------------------------------------------------------------------- */

/*
 * Utility routine to tell whether a sequence of bytes is legal UTF-8.
 * This must be called with the length pre-determined by the first byte.
 * If not calling this from ConvertUTF8to*, then the length can be set by:
 *  length = trailingBytesForUTF8[*source]+1;
 * and the sequence is illegal right away if there aren't that many bytes
 * available.
 * If presented with a length > 4, this returns false.  The Unicode
 * definition of UTF-8 goes up to 4-byte sequences.
 */

static Booleano isLegalUTF8(const UTF8 *source, int length) {
    UTF8 a;
    const UTF8 *srcptr = source+length;
    switch (length) {
    default: return false;
  /* Everything else falls through when "true"... */
    case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 2: if ((a = (*--srcptr)) > 0xBF) return false;

  switch (*source) {
      /* no fall-through in this inner switch */
      case 0xE0: if (a < 0xA0) return false; break;
      case 0xED: if (a > 0x9F) return false; break;
      case 0xF0: if (a < 0x90) return false; break;
      case 0xF4: if (a > 0x8F) return false; break;
      default:   if (a < 0x80) return false;
  }

    case 1: if (*source >= 0x80 && *source < 0xC2) return false;
    }
    if (*source > 0xF4) return false;
    return true;
}

/* --------------------------------------------------------------------- */

/*
 * Exported function to return whether a UTF-8 sequence is legal or not.
 * This is not used here; it's just exported.
 */
Booleano isLegalUTF8Sequence(const UTF8 *source, const UTF8 *sourceEnd) {
    int length = trailingBytesForUTF8[*source]+1;
    if (source+length > sourceEnd) {
  return false;
    }
    return isLegalUTF8(source, length);
}

/* --------------------------------------------------------------------- */

ConversionResult ConvertUTF8toUTF16 (
  const UTF8** sourceStart, const UTF8* sourceEnd, 
  UTF16** targetStart, UTF16* targetEnd, ConversionFlags flags) {
    ConversionResult result = conversionOK;
    const UTF8* source = *sourceStart;
    UTF16* target = *targetStart;
    while (source < sourceEnd) {
      UTF32 ch = 0;
      unsigned short extraBytesToRead = trailingBytesForUTF8[*source];
      if (source + extraBytesToRead >= sourceEnd) {
          result = sourceExhausted; break;
      }
      /* Do this check whether lenient or strict */
      if (! isLegalUTF8(source, extraBytesToRead+1)) {
          result = sourceIllegal;
          break;
      }
      /*
       * The cases all fall through. See "Note A" below.
       */
      switch (extraBytesToRead) {
          case 5: ch += *source++; ch <<= 6; /* remember, illegal UTF-8 */
          case 4: ch += *source++; ch <<= 6; /* remember, illegal UTF-8 */
          case 3: ch += *source++; ch <<= 6;
          case 2: ch += *source++; ch <<= 6;
          case 1: ch += *source++; ch <<= 6;
          case 0: ch += *source++;
      }
      ch -= offsetsFromUTF8[extraBytesToRead];

      if (target >= targetEnd) {
          source -= (extraBytesToRead+1); /* Back up source pointer! */
          result = targetExhausted; break;
      }
      if (ch <= UNI_MAX_BMP) { /* Target is a character <= 0xFFFF */
          /* UTF-16 surrogate values are illegal in UTF-32 */
          if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END) {
        if (flags == strictConversion) {
            source -= (extraBytesToRead+1); /* return to the illegal value itself */
            result = sourceIllegal;
            break;
        } else {
            *target++ = UNI_REPLACEMENT_CHAR;
        }
          } else {
        *target++ = (UTF16)ch; /* normal case */
          }
      } else if (ch > UNI_MAX_UTF16) {
          if (flags == strictConversion) {
        result = sourceIllegal;
        source -= (extraBytesToRead+1); /* return to the start */
        break; /* Bail out; shouldn't continue */
          } else {
        *target++ = UNI_REPLACEMENT_CHAR;
          }
      } else {
          /* target is a character in range 0xFFFF - 0x10FFFF. */
          if (target + 1 >= targetEnd) {
        source -= (extraBytesToRead+1); /* Back up source pointer! */
        result = targetExhausted; break;
          }
          ch -= halfBase;
          *target++ = (UTF16)((ch >> halfShift) + UNI_SUR_HIGH_START);
          *target++ = (UTF16)((ch & halfMask) + UNI_SUR_LOW_START);
      }
    }
    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* --------------------------------------------------------------------- */

ConversionResult ConvertUTF32toUTF8 (
  const UTF32** sourceStart, const UTF32* sourceEnd, 
  UTF8** targetStart, UTF8* targetEnd, ConversionFlags flags) {
    ConversionResult result = conversionOK;
    const UTF32* source = *sourceStart;
    UTF8* target = *targetStart;
    while (source < sourceEnd) {
  UTF32 ch;
  unsigned short bytesToWrite = 0;
  const UTF32 byteMask = 0xBF;
  const UTF32 byteMark = 0x80; 
  ch = *source++;
  if (flags == strictConversion ) {
      /* UTF-16 surrogate values are illegal in UTF-32 */
      if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END) {
    --source; /* return to the illegal value itself */
    result = sourceIllegal;
    break;
      }
  }
  /*
   * Figure out how many bytes the result will require. Turn any
   * illegally large UTF32 things (> Plane 17) into replacement chars.
   */
  if (ch < (UTF32)0x80) {       bytesToWrite = 1;
  } else if (ch < (UTF32)0x800) {     bytesToWrite = 2;
  } else if (ch < (UTF32)0x10000) {   bytesToWrite = 3;
  } else if (ch <= UNI_MAX_LEGAL_UTF32) {  bytesToWrite = 4;
  } else {          bytesToWrite = 3;
              ch = UNI_REPLACEMENT_CHAR;
              result = sourceIllegal;
  }
  
  target += bytesToWrite;
  if (target > targetEnd) {
      --source; /* Back up source pointer! */
      target -= bytesToWrite; result = targetExhausted; break;
  }
  switch (bytesToWrite) { /* note: everything falls through. */
      case 4: *--target = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
      case 3: *--target = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
      case 2: *--target = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
      case 1: *--target = (UTF8) (ch | firstByteMark[bytesToWrite]);
  }
  target += bytesToWrite;
    }
    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* --------------------------------------------------------------------- */

ConversionResult ConvertUTF8toUTF32 (
  const UTF8** sourceStart, const UTF8* sourceEnd, 
  UTF32** targetStart, UTF32* targetEnd, ConversionFlags flags) {
    ConversionResult result = conversionOK;
    const UTF8* source = *sourceStart;
    UTF32* target = *targetStart;
    while (source < sourceEnd) {
  UTF32 ch = 0;
  unsigned short extraBytesToRead = trailingBytesForUTF8[*source];
  if (source + extraBytesToRead >= sourceEnd) {
      result = sourceExhausted; break;
  }
  /* Do this check whether lenient or strict */
  if (! isLegalUTF8(source, extraBytesToRead+1)) {
      result = sourceIllegal;
      break;
  }
  /*
   * The cases all fall through. See "Note A" below.
   */
  switch (extraBytesToRead) {
      case 5: ch += *source++; ch <<= 6;
      case 4: ch += *source++; ch <<= 6;
      case 3: ch += *source++; ch <<= 6;
      case 2: ch += *source++; ch <<= 6;
      case 1: ch += *source++; ch <<= 6;
      case 0: ch += *source++;
  }
  ch -= offsetsFromUTF8[extraBytesToRead];

  if (target >= targetEnd) {
      source -= (extraBytesToRead+1); /* Back up the source pointer! */
      result = targetExhausted; break;
  }
  if (ch <= UNI_MAX_LEGAL_UTF32) {
      /*
       * UTF-16 surrogate values are illegal in UTF-32, and anything
       * over Plane 17 (> 0x10FFFF) is illegal.
       */
      if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END) {
    if (flags == strictConversion) {
        source -= (extraBytesToRead+1); /* return to the illegal value itself */
        result = sourceIllegal;
        break;
    } else {
        *target++ = UNI_REPLACEMENT_CHAR;
    }
      } else {
    *target++ = ch;
      }
  } else { /* i.e., ch > UNI_MAX_LEGAL_UTF32 */
      result = sourceIllegal;
      *target++ = UNI_REPLACEMENT_CHAR;
  }
    }
    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* ---------------------------------------------------------------------

    Note A.
    The fall-through switches in UTF-8 reading code save a
    temp variable, some decrements & conditionals.  The switches
    are equivalent to the following loop:
  {
      int tmpBytesToRead = extraBytesToRead+1;
      do {
    ch += *source++;
    --tmpBytesToRead;
    if (tmpBytesToRead) ch <<= 6;
      } while (tmpBytesToRead > 0);
  }
    In UTF-8 writing code, the switches on "bytesToWrite" are
    similarly unrolled loops.

   --------------------------------------------------------------------- */


#ifdef __cplusplus

transcode_to_utf16::transcode_to_utf16(const char *utf8_input)
: _orig_length(int(strlen(utf8_input)) + 1),
  _converted(new UTF16[_orig_length + 1])
    // we don't ever expect the string to get longer going to the larger data
    // type, so the current length should be enough.
{
  _result = conversionOK;
  if (_orig_length == 1) {
    // no length, so only provide a blank string.
    _converted[0] = 0;
    return;
  }
  memset((byte *)_converted, 0, 2 * _orig_length);
  // we use these temporary pointers since the converter resets the source
  // and target pointers to the end of the conversion.  the same pattern
  // is used in the code below.
  const UTF8 *temp_in = (const UTF8 *)utf8_input;
  UTF16 *temp_out = _converted;
  _result = ConvertUTF8toUTF16(&temp_in, temp_in + _orig_length,
      &temp_out, temp_out + _orig_length, lenientConversion);
}

transcode_to_utf16::transcode_to_utf16(const istring &utf8_input)
: _orig_length(utf8_input.length() + 1),
  _converted(new UTF16[_orig_length])
{
  _result = conversionOK;
  if (_orig_length == 1) {
    // no length, so only provide a blank string.
    _converted[0] = 0;
    return;
  }
  memset((byte *)_converted, 0, 2 * _orig_length);
  const UTF8 *temp_in = (const UTF8 *)utf8_input.observe();
  UTF16 *temp_out = _converted;
  _result = ConvertUTF8toUTF16(&temp_in, temp_in + _orig_length,
      &temp_out, temp_out + _orig_length, lenientConversion);
}

transcode_to_utf16::~transcode_to_utf16()
{
  delete [] _converted;
  _converted = NIL;
}

int transcode_to_utf16::length() const
{ return int(wcslen((wchar_t *)_converted)); }


transcode_to_utf8::transcode_to_utf8(const UTF16 *utf16_input)
: _orig_length(int(wcslen((const wchar_t *)utf16_input))),
  _new_length(_orig_length * 2 + _orig_length / 2 + 1),
    // this is just an estimate.  it may be appropriate most of the time.
    // whatever doesn't fit will get truncated.
  _converted(new UTF8[_new_length])
{
  _result = conversionOK;
  if (_orig_length == 0) {
    // no length, so only provide a blank string.
    _converted[0] = 0;
    return;
  }
  memset(_converted, 0, _new_length);
  const UTF16 *temp_in = (const UTF16 *)utf16_input;
  UTF8 *temp_out = _converted;
  _result = ConvertUTF16toUTF8(&temp_in, temp_in + _orig_length,
      &temp_out, temp_out + _new_length, lenientConversion);
}

transcode_to_utf8::transcode_to_utf8(const wchar_t *utf16_input)
: _orig_length(int(wcslen(utf16_input))),
  _new_length(_orig_length * 2 + _orig_length / 2 + 1),
    // this is just an estimate.  it may be appropriate most of the time.
    // whatever doesn't fit will get truncated.
    _converted(new UTF8[_new_length > 0 ? _new_length : 1])
{
  _result = conversionOK;
  if (_orig_length == 0) {
    // no length, so only provide a blank string.
    _converted[0] = 0;
    return;
  }
  memset(_converted, 0, _new_length);
  const UTF16 *temp_in = (const UTF16 *)utf16_input;
  UTF8 *temp_out = _converted;
  _result = ConvertUTF16toUTF8(&temp_in, temp_in + _orig_length,
      &temp_out, temp_out + _new_length, lenientConversion);
}

transcode_to_utf8::~transcode_to_utf8()
{
  delete [] _converted;
  _converted = NIL;
}

int transcode_to_utf8::length() const
{ return int(strlen((char *)_converted)); }

transcode_to_utf8::operator istring() const
{ return istring((char *)_converted); }


null_transcoder::null_transcoder(const char *utf8_input, bool make_own_copy)
: _make_own_copy(make_own_copy),
  _converted(make_own_copy? new UTF8[strlen(utf8_input) + 1]
      : (const UTF8 *)utf8_input)
{
  if (_make_own_copy) {
    strcpy((char *)_converted, utf8_input);
  }
}

null_transcoder::null_transcoder(const istring &utf8_input, bool make_own_copy)
: _make_own_copy(make_own_copy),
  _converted(make_own_copy? new UTF8[utf8_input.length() + 1]
      : (const UTF8 *)utf8_input.s())
{
  if (_make_own_copy) {
    strcpy((char *)_converted, utf8_input.s());
  }
}

int null_transcoder::length() const
{ return int(strlen((char *)_converted)); }

#endif //_cplusplus


#endif //CONVERT_UTF_IMPLEMENTATION_FILE

