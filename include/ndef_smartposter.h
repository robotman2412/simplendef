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
#include "ndef_text.h"

#ifdef __cplusplus
extern "C" {
#endif


// A struct for holding the inner NDEF, URI (if present) and text (if present) of a smart poster record.
typedef struct {
	// All inner NDEF data.
	ndef_ctx  ndef;
	// First found URI.
	char     *uri;
	// First found text entry.
	ndef_text text;
} ndef_smartposter;

// Determine whether an NDEF record is a text record.
bool ndef_record_is_smartposter(ndef_record ctx);
// Construct a `ndef_smartposter` containing a summary containing the payload NDEF, URI (if present) and text (if present).
// Returns NULL when out of memory, or when not a smart poster record.
ndef_smartposter ndef_record_get_smartposter(ndef_record ctx);
// Construct an NDEF record containing the given smart poster data.
// If not already present, the URI and text are added to the NDEF data.
// All fields are optional, but at least one is required.
ndef_record ndef_record_new_smartposter(ndef_smartposter ctx);

// Create an empty smart poster.
static inline ndef_smartposter ndef_smartposter_init() {
	return (ndef_smartposter) {
		NULL, NULL, (ndef_text) { NULL, NULL },
	};
}
// Calls `free` on `lang` and `text` in an `ndef_text` struct.
static inline void ndef_smartposter_destroy(ndef_smartposter ctx) {
	if (ctx.ndef) ndef_destroy(ctx.ndef);
	if (ctx.uri) free(ctx.uri);
	ndef_text_destroy(ctx.text);
}


#ifdef __cplusplus
} // extern "C"
#endif
