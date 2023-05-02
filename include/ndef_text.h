/*
	MIT License

	Copyright (c) 2023 Julian Scheffers

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#pragma once

#include "ndef.h"

#ifdef __cplusplus
extern "C" {
#endif


// A struct for holding a language code and a string.
typedef struct {
	// ISO/IANA language code.
	char *lang;
	// UTF-8 text data.
	char *text;
} ndef_text;

// Determine whether an NDEF record is a text record.
bool ndef_record_is_text(ndef_record ctx);
// Construct a `ndef_text` containing both the text and language from the record.
// Returns NULL when out of memory, or when not a text record.
ndef_text ndef_record_get_text(ndef_record ctx);
// Construct an NDEF record containing the given text and language.
ndef_record ndef_record_new_text(ndef_text ctx);

// Calls `free` on `lang` and `text` in an `ndef_text` struct.
static inline void ndef_text_destroy(ndef_text ctx) {
	if (ctx.lang) free(ctx.lang);
	if (ctx.text) free(ctx.text);
}


#ifdef __cplusplus
} // extern "C"
#endif
