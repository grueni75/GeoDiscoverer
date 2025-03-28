#ifndef UTF_CONVERSION_GROUP
#define UTF_CONVERSION_GROUP

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

// original copyright notice still applies to low-level conversion code:
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

    Conversions between UTF32, UTF-16, and UTF-8.  Header file.

    Several funtions are included here, forming a complete set of
    conversions between the three formats.  UTF-7 is not included
    here, but is handled in a separate source file.

    Each of these routines takes pointers to input buffers and output
    buffers.  The input buffers are const.

    Each routine converts the text between *sourceStart and sourceEnd,
    putting the result into the buffer between *targetStart and
    targetEnd. Note: the end pointers are *after* the last item: e.g. 
    *(sourceEnd - 1) is the last item.

    The return result indicates whether the conversion was successful,
    and if not, whether the problem was in the source or target buffers.
    (Only the first encountered problem is indicated.)

    After the conversion, *sourceStart and *targetStart are both
    updated to point to the end of last text successfully converted in
    the respective buffers.

    Input parameters:
        sourceStart - pointer to a pointer to the source buffer.
                The contents of this are modified on return so that
                it points at the next thing to be converted.
        targetStart - similarly, pointer to pointer to the target buffer.
        sourceEnd, targetEnd - respectively pointers to the ends of the
                two buffers, for overflow checking only.

    These conversion functions take a ConversionFlags argument. When this
    flag is set to strict, both irregular sequences and isolated surrogates
    will cause an error.  When the flag is set to lenient, both irregular
    sequences and isolated surrogates are converted.

    Whether the flag is strict or lenient, all illegal sequences will cause
    an error return. This includes sequences such as: <F4 90 80 80>, <C0 80>,
    or <A0> in UTF-8, and values above 0x10FFFF in UTF-32. Conformant code
    must check for illegal sequences.

    When the flag is set to lenient, characters over 0x10FFFF are converted
    to the replacement character; otherwise (when the flag is set to strict)
    they constitute an error.

    Output parameters:
        The value "sourceIllegal" is returned from some routines if the input
        sequence is malformed.  When "sourceIllegal" is returned, the source
        value will point to the illegal value that caused the problem. E.g.,
        in UTF-8 when a sequence is malformed, it points to the start of the
        malformed sequence.  

    Author: Mark E. Davis, 1994.
    Rev History: Rick McGowan, fixes & updates May 2001.
        Fixes & updates, Sept 2001.

------------------------------------------------------------------------ */

/* ---------------------------------------------------------------------
    The following 4 definitions are compiler-specific.
    The C standard does not guarantee that wchar_t has at least
    16 bits, so wchar_t is no less portable than unsigned short!
    All should be unsigned values to avoid sign extension during
    bit mask & shift operations.
------------------------------------------------------------------------ */

typedef unsigned long UTF32;  /* at least 32 bits */
typedef unsigned short UTF16;  /* at least 16 bits */
typedef unsigned char UTF8;  /* typically 8 bits */
typedef unsigned char Booleano;  /* 0 or 1 */

/* Some fundamental constants */
#define UNI_REPLACEMENT_CHAR (UTF32)0x0000FFFD
#define UNI_MAX_BMP (UTF32)0x0000FFFF
#define UNI_MAX_UTF16 (UTF32)0x0010FFFF
#define UNI_MAX_UTF32 (UTF32)0x7FFFFFFF
#define UNI_MAX_LEGAL_UTF32 (UTF32)0x0010FFFF

typedef enum {
  conversionOK,     /* conversion successful */
  sourceExhausted,  /* partial character in source, but hit end */
  targetExhausted,  /* insuff. room in target for conversion */
  sourceIllegal  /* source sequence is illegal/malformed */
} ConversionResult;

typedef enum {
  strictConversion = 0,
  lenientConversion
} ConversionFlags;

/* This is for C++ and does no harm in C */
#ifdef __cplusplus

#include "definitions.h"

extern "C" {
#endif

ConversionResult ConvertUTF8toUTF16 (const UTF8** sourceStart,
    const UTF8* sourceEnd, UTF16** targetStart, UTF16* targetEnd,
    ConversionFlags flags);

ConversionResult ConvertUTF16toUTF8 (const UTF16** sourceStart,
    const UTF16* sourceEnd, UTF8** targetStart, UTF8* targetEnd,
    ConversionFlags flags);

ConversionResult ConvertUTF8toUTF32 (const UTF8** sourceStart,
    const UTF8* sourceEnd, UTF32** targetStart, UTF32* targetEnd,
    ConversionFlags flags);

ConversionResult ConvertUTF32toUTF8 (const UTF32** sourceStart,
    const UTF32* sourceEnd, UTF8** targetStart, UTF8* targetEnd,
    ConversionFlags flags);

ConversionResult ConvertUTF16toUTF32 (const UTF16** sourceStart,
    const UTF16* sourceEnd, UTF32** targetStart, UTF32* targetEnd,
    ConversionFlags flags);

ConversionResult ConvertUTF32toUTF16 (const UTF32** sourceStart,
    const UTF32* sourceEnd, UTF16** targetStart, UTF16* targetEnd,
    ConversionFlags flags);

Booleano isLegalUTF8Sequence(const UTF8 *source, const UTF8 *sourceEnd);

#ifdef __cplusplus
} //extern
#endif //cplusplus


#ifdef __cplusplus

// The following types and macros help to make it irrelevant what kind of
// win32 build is being done.  They will adapt as needed to provide the
// types used in system calls.  They are rendered harmless for other operating
// systems or for non-Unicode builds; this is especially useful for POSIX
// compliant functions that required Unicode in win32 but not in Unix systems.

#if defined(UNICODE)



  #define to_unicode_temp(s) transcode_to_utf16(s)


  #define from_unicode_temp(s) transcode_to_utf8(s)


  #define to_unicode_persist(name, s) transcode_to_utf16 name(s)


  #define from_unicode_persist(name, s) transcode_to_utf8 name(s)
#else
  // these versions of the macros simply defang any conversions.
  #define to_unicode_temp(s) null_transcoder(s, false)
  #define from_unicode_temp(s) null_transcoder(s, false)
  #define to_unicode_persist(name, s) null_transcoder name(s, true)
  #define from_unicode_persist(name, s) null_transcoder name(s, true) 
#endif

#ifdef _MSC_VER

  #define TRACE_PRINT(s) TRACE(_T("%s"), to_unicode_temp(s))
#endif


// The next two classes support converting a UTF-8 string into a UTF-16
// string and vice-versa.  They hold onto the converted string and provide
// operators that return it.


class transcode_to_utf16
{
public:
  transcode_to_utf16(const char *utf8_input);

  transcode_to_utf16(const istring &utf8_input);

  ~transcode_to_utf16();

  int length() const;

  operator const UTF16 * () const { return _converted; }
  operator UTF16 * () { return _converted; }
  operator const flexichar * () const { return (const flexichar *)_converted; }
  operator flexichar * () { return (flexichar *)_converted; }

  ConversionResult _result;
private:
  int _orig_length;  
  UTF16 *_converted;  
};



class transcode_to_utf8
{
public:
  transcode_to_utf8(const UTF16 *utf16_input);

  transcode_to_utf8(const wchar_t *utf16_input);

  ~transcode_to_utf8();

  int length() const;

  operator const UTF8 * () const { return _converted; }
  operator UTF8 * () { return _converted; }

  operator istring() const;

  ConversionResult _result;
private:
  int _orig_length;  
  int _new_length;  
  UTF8 *_converted;  
};



class null_transcoder
{
public:
  null_transcoder(const char *utf8_input, bool make_own_copy);
  null_transcoder(const istring &utf8_input, bool make_own_copy);
  ~null_transcoder() {
    if (_make_own_copy) delete [] _converted;
    _converted = NIL;
  }

  int length() const;
  operator char * () { return (char *)_converted; }
  operator const char * () const { return (const char *)_converted; }

private:
  bool _make_own_copy;
  const UTF8 *_converted;
};

#endif //cplusplus

#endif // outer guard.

