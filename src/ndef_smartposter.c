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

#include "ndef_smartposter.h"
#include "ndef_uri.h"



// Determine whether an NDEF record is a text record.
bool ndef_record_is_smartposter(ndef_record ctx) {
	return ctx.tnf == NDEF_TNF_WELL_KNOWN && ctx.type_len == 2 && ctx.payload_len && ctx.type[0] == 'S' && ctx.type[1] == 'p';
}

// Construct a `ndef_smartposter` containing a summary containing the payload NDEF, URI (if present) and text (if present).
// Returns NULL when out of memory, or when not a smart poster record.
ndef_smartposter ndef_record_get_smartposter(ndef_record ctx) {
	ndef_smartposter out = ndef_smartposter_init();
	
	// Decode inner NDEF.
	size_t payload_len = ctx.payload_len;
	out.ndef = ndef_decode(ctx.payload, &payload_len);
	
	// Look for URI record.
	for (size_t i = 0; i < ndef_records_len(out.ndef); i++) {
		ndef_record record = ndef_records(out.ndef)[i];
		out.uri = ndef_record_get_uri(record);
		if (out.uri) break;
	}
	
	// Look for text record.
	for (size_t i = 0; i < ndef_records_len(out.ndef); i++) {
		ndef_record record = ndef_records(out.ndef)[i];
		out.text = ndef_record_get_text(record);
		if (out.text.lang) break;
	}
	
	return out;
}

// Construct an NDEF record containing the given smart poster data.
// If not already present, the URI and text are added to the NDEF data.
// All fields are optional, but at least one is required.
ndef_record ndef_record_new_smartposter(ndef_smartposter ctx) {
	if (!ctx.ndef) {
		ctx.ndef = ndef_init();
	} else {
		ctx.ndef = ndef_clone(ctx.ndef);
	}
	
	// Check for URI and text records.
	bool has_uri = false, has_text = false;
	for (size_t i = 0; i < ndef_records_len(ctx.ndef); i++) {
		ndef_record record = ndef_records(ctx.ndef)[i];
		has_uri  |= ndef_record_is_uri (record);
		has_text |= ndef_record_is_text(record);
	}
	
	// Append URI record.
	if (!has_uri && ctx.uri) {
		ndef_append_mv(ctx.ndef, ndef_record_new_uri(ctx.uri));
	}
	
	// Append text record.
	if (!has_text && ctx.text.lang && ctx.text.text) {
		ndef_append_mv(ctx.ndef, ndef_record_new_text(ctx.text));
	}
	
	// Encode NDEF data.
	size_t len;
	uint8_t *data;
	ndef_encode(ctx.ndef, &data, &len);
	ndef_destroy(ctx.ndef);
	
	// Construct type.
	uint8_t *type = malloc(2);
	if (!type) {
		free(data);
		return ndef_record_init();
	}
	type[0] = 'S';
	type[1] = 'p';
	
	// Construct record.
	return (ndef_record) {
		.tnf			= NDEF_TNF_WELL_KNOWN,
		.type_len		= 1,
		.payload_len	= len,
		.id_len			= 0,
		.type			= type,
		.payload		= (uint8_t *) data,
		.id				= NULL,
	};
}
