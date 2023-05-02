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

#include "ndef_text.h"



// Determine whether an NDEF record is a text record.
bool ndef_record_is_text(ndef_record ctx) {
	return ctx.tnf == NDEF_TNF_WELL_KNOWN && ctx.type_len == 1 && ctx.type[0] == 'T' && ctx.payload_len >= 4;
}

// Construct a `ndef_text` containing both the text and language from the record.
// Returns NULL when out of memory, or when not a text record.
ndef_text ndef_record_get_text(ndef_record ctx) {
	// Check type.
	if (!ndef_record_is_text(ctx)) return (ndef_text) { NULL, NULL };
	
	// Parse the status byte.
	uint_fast8_t lang_len = ctx.payload[0] & 0x3f;
	// bool         is_utf16 = ctx.payload[0] & 0x80;
	size_t       text_len = ctx.payload_len - lang_len - 1;
	// TODO: UTF-16 to UTF-8 conversion.
	
	// Allocate memory.
	char *lang = malloc(lang_len + 1);
	if (!lang) return (ndef_text) { NULL, NULL };
	char *text = malloc(text_len + 1);
	if (!text) {
		free(lang);
		return (ndef_text) { NULL, NULL };
	}
	
	// Copy strings.
	memcpy(lang, ctx.payload + 1, lang_len);
	memcpy(text, ctx.payload + 1 + lang_len, text_len);
	lang[lang_len] = 0;
	text[text_len] = 0;
	
	// Return the pair of datas.
	return (ndef_text) { lang, text };
}

// Construct an NDEF record containing the given text and language.
ndef_record ndef_record_new_text(ndef_text ctx) {
	size_t lang_len = strlen(ctx.lang);
	size_t text_len = strlen(ctx.text);
	
	// Validity checks.
	if (!ctx.lang || !ctx.text || lang_len < 2 || text_len) {
		return ndef_record_init();
	}
	
	// Allocate memory.
	uint8_t *mem = malloc(1 + lang_len + text_len);
	if (!mem) return ndef_record_init();
	
	// Create payload.
	*mem = 0x00 | lang_len;
	memcpy(mem + 1, ctx.lang, lang_len);
	memcpy(mem + 1 + lang_len, ctx.text, text_len);
	uint8_t *type = malloc(1);
	if (!type) {
		free(mem);
		return ndef_record_init();
	}
	*type = 'U';
	
	// Construct record.
	return (ndef_record) {
		.tnf			= NDEF_TNF_WELL_KNOWN,
		.type_len		= 1,
		.payload_len	= 1 + lang_len + text_len,
		.id_len			= 0,
		.type			= type,
		.payload		= (uint8_t *) mem,
		.id				= NULL,
	};
}
