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

#define NDEF_REVEAL_PRIVATE
#include "ndef.h"
#include "ndef_record_types.h"

#define MAGIC_CHECK if (!ctx || ctx->magic != NDEF_MAGIC) { printf("NDEF: Fatal error: Invalid context\n"); abort(); }


// LUT from ndef_tnf to name.
const char *ndef_tnf_names[8] = {
	"EMPTY (0)",
	"WELL_KNOWN (1)",
	"MIME (2)",
	"URI (3)",
	"EXTERNAL (4)",
	"UNKNOWN (5)",
	"UNCHANGED (6)",
	"Reserved (7)",
};



// BOX used for output streaming.
typedef struct {
	// Byte buffer.
	uint8_t *buf;
	// Capacity of buffer.
	size_t   buf_cap;
	// Length of buffer.
	size_t   buf_len;
} ndef_ostream;

// Make an `ndef_ostream`.
static inline ndef_ostream ndef_ostream_init() {
	return (ndef_ostream) { NULL, 0, 0 };
}

// Destroy an `ndef_ostream`.
static inline void ndef_ostream_destroy(ndef_ostream ctx) {
	if (ctx.buf) free(ctx.buf);
}

// Append one (1) byte (eight (8) bit (either one (1) or zero (0)) quantity (amount)) to the output stream.
static bool ndef_ostream_append(ndef_ostream *ctx, uint8_t data) {
	if (ctx->buf_len >= ctx->buf_cap) {
		size_t cap = ctx->buf_cap * 2;
		if (!cap) cap = 1;
		void *mem = realloc(ctx->buf, cap);
		if (!mem) {
			printf("NDEF: Error: Out of memory (allocating %zu byte%s)\n", cap, cap == 1 ? "" : "s");
			return false;
		}
		ctx->buf = mem;
		ctx->buf_cap = cap;
	}
	ctx->buf[ctx->buf_len++] = data;
	return true;
}

// Append MULTI-BYTE DATA to the output stream.
static bool ndef_ostream_append_n(ndef_ostream *ctx, uint8_t *data, size_t len) {
	size_t cap = ctx->buf_cap;
	if (!cap) cap = 1;
	if (cap < ctx->buf_len + len) {
		while (cap < ctx->buf_len + len) cap *= 2;
		void *mem = realloc(ctx->buf, cap);
		if (!mem) {
			printf("NDEF: Error: Out of memory (allocating %zu byte%s)\n", cap, cap == 1 ? "" : "s");
			return false;
		}
		ctx->buf = mem;
		ctx->buf_cap = cap;
	}
	memcpy(ctx->buf + ctx->buf_len, data, len);
	ctx->buf_len += len;
	return true;
}



// Prints a simple hexdump.
static void hexdump(const void *datas, size_t size, int indent) {
	const size_t cols = 16;
	const uint8_t *arr = datas;
	for (size_t y = 0; y * cols < size; y++) {
		// Print indentation.
		for (int x = 0; x < indent; x++) putc(' ', stdout);
		
		// Print hex chars.
		uint_fast8_t x;
		for (x = 0; x < cols && x+y*cols < size; x++) {
			if (x) putc(' ', stdout);
			printf("%02x", arr[y*cols+x]);
		}
		// Padding.
		if (size > 16) {
			if (!x) { x++; fputs("  ", stdout); }
			for (; x < cols; x++) fputs("   ", stdout);
		}
		
		// Print ASCII chars.
		fputs("  ", stdout);
		for (x = 0; x < cols && x+y*cols < size; x++) {
			char c = arr[y*cols+x];
			if (c >= 0x20 && c <= 0x7e) putc(c, stdout);
			else putc('.', stdout);
		}
		putc('\n', stdout);
	}
}

// Append a raw NDEF record.
bool ndef_raw_append(ndef_ctx ctx, ndef_raw_record record) {
	MAGIC_CHECK
	
	// Determine new capacity.
	size_t cap = ctx->raw_records_cap;
	if (ctx->raw_records_len >= cap) {
		if (!cap) cap = 1;
		cap *= 2;
		void *mem = realloc(ctx->raw_records, sizeof(ndef_raw_record) * cap);
		if (!mem) {
			printf("NDEF: Error: Out of memory (allocating %zu bytes)\n", sizeof(ndef_raw_record) * cap);
			return false;
		}
		ctx->raw_records     = mem;
		ctx->raw_records_cap = cap;
	}
	
	// Copy an element.
	ctx->raw_records[ctx->raw_records_len] = record;
	ctx->raw_records_len ++;
	
	return true;
}

// Clone an ndef_raw_record.
// This method is an excellent example of something C++ is way better at than C is.
bool ndef_record_clone(ndef_record in, ndef_record *out) {
	ndef_record tmp = in;
	
	if (in.payload) {
		tmp.payload = malloc(in.payload_len);
		if (!tmp.payload) {
			printf("NDEF: Error: Out of memory (allocating %zu byte%s)\n", in.payload_len, in.payload_len == 1 ? "" : "s");
			return false;
		}
		memcpy(tmp.payload, in.payload, in.payload_len);
	}
	
	if (in.type) {
		tmp.type = malloc(in.type_len);
		if (!tmp.type) {
			printf("NDEF: Error: Out of memory (allocating %zu byte%s)\n", in.type_len, in.type_len == 1 ? "" : "s");
			if (tmp.payload) free(tmp.payload);
			return false;
		}
		memcpy(tmp.type, in.type, in.type_len);
	}
	
	if (in.id) {
		tmp.id = malloc(in.id_len);
		if (!tmp.type) {
			printf("NDEF: Error: Out of memory (allocating %zu byte%s)\n", in.id_len, in.id_len == 1 ? "" : "s");
			if (tmp.payload) free(tmp.type);
			if (tmp.payload) free(tmp.payload);
			return false;
		}
		memcpy(tmp.id, in.id, in.id_len);
	}
	
	*out = tmp;
	return true;
}

// Decode a single NDEF record from a blob of data.
// Sets `len` to the amount of successfully decoded data when finished.
bool ndef_raw_record_decode(ndef_raw_record *out, uint8_t *data, size_t *len) {
	ndef_raw_record tmp;
	
	// Minimum length check.
	if (*len < 3) {
		printf("NDEF: Decode error: Not enough data (%zu byte%s; expected 3+ bytes)\n", *len, *len == 1 ? "" : "s");
		return false;
	}
	
	// Decode flags byte.
	size_t pos = 0;
	tmp.flag_begin          = data[pos] & NDEF_FLAG_MB;
	tmp.flag_end            = data[pos] & NDEF_FLAG_ME;
	tmp.flag_chunked        = data[pos] & NDEF_FLAG_CF;
	tmp.flag_short_record   = data[pos] & NDEF_FLAG_SR;
	tmp.flag_include_id_len = data[pos] & NDEF_FLAG_IL;
	tmp.tnf                 = data[pos] & NDEF_FLAG_TNF;
	pos++;
	
	// Minimum length check.
	if (tmp.flag_short_record && *len < 3 + tmp.flag_include_id_len) {
		printf("NDEF: Decode error: Not enough data (%zu bytes; expected %zu+ bytes)\n", *len, 3 + tmp.flag_include_id_len);
		return false;
	} else if (!tmp.flag_short_record && *len < 5 + tmp.flag_include_id_len) {
		printf("NDEF: Decode error: Not enough data (%zu bytes; expected %zu+ bytes)\n", *len, 5 + tmp.flag_include_id_len);
		return false;
	}
	
	// Type length.
	tmp.type_len = data[pos++];
	
	// Payload length.
	if (tmp.flag_short_record) {
		tmp.payload_len = data[pos++];
	} else {
		tmp.payload_len = 0;
		tmp.payload_len |= data[pos++] << 24;
		tmp.payload_len |= data[pos++] << 16;
		tmp.payload_len |= data[pos++] <<  8;
		tmp.payload_len |= data[pos++] <<  0;
	}
	
	// ID length.
	if (tmp.flag_include_id_len) {
		tmp.id_len = data[pos++];
	} else {
		tmp.id_len = 0;
	}
	
	// Minimum length check.
	if (*len < pos + tmp.type_len + tmp.payload_len + tmp.id_len) {
		printf("NDEF: Debug: 0x%02x %zu %zu %zu %zu\n", data[0], pos, tmp.type_len, tmp.payload_len, tmp.id_len);
		printf("NDEF: Decode error: Not enough data (%zu bytes; expected %zu bytes)\n", *len, pos + tmp.type_len + tmp.payload_len + tmp.id_len);
		return false;
	}
	
	// Type field.
	if (tmp.type_len) {
		tmp.type = malloc(tmp.type_len);
		if (!tmp.type) {
			printf("NDEF: Error: Out of memory (allocating %zu bytes)\n", (size_t) tmp.type_len);
			return false;
		}
		memcpy(tmp.type, data + pos, tmp.type_len);
		pos += tmp.type_len;
	} else {
		tmp.type = NULL;
	}
	
	// Payload field.
	if (tmp.payload_len) {
		tmp.payload = malloc(tmp.payload_len);
		if (!tmp.payload) {
			if (tmp.type) free(tmp.type);
			printf("NDEF: Error: Out of memory (allocating %zu bytes)\n", (size_t) tmp.payload_len);
			return false;
		}
		memcpy(tmp.payload, data + pos, tmp.payload_len);
		pos += tmp.payload_len;
	} else {
		tmp.payload = NULL;
	}
	
	// ID field.
	if (tmp.id_len) {
		tmp.id = malloc(tmp.id_len);
		if (!tmp.id) {
			if (tmp.type) free(tmp.type);
			if (tmp.payload) free(tmp.payload);
			printf("NDEF: Error: Out of memory (allocating %zu bytes)\n", (size_t) tmp.id_len);
			return false;
		}
		memcpy(tmp.id, data + pos, tmp.id_len);
		pos += tmp.id_len;
	} else {
		tmp.id = NULL;
	}
	
	// SUCCESSFUL DECODE YAY!
	*out = tmp;
	*len = pos;
	return true;
}

// Encode a single NDEF record.
bool ndef_raw_record_encode(ndef_ostream *out, const ndef_raw_record *data) {
	size_t len0 = out->buf_len;
	
	// Create flags field.
	uint8_t flags = 0;
	flags |= NDEF_FLAG_MB  * data->flag_begin;
	flags |= NDEF_FLAG_ME  * data->flag_end;
	flags |= NDEF_FLAG_CF  * data->flag_chunked;
	flags |= NDEF_FLAG_SR  * data->flag_short_record;
	flags |= NDEF_FLAG_IL  * data->flag_include_id_len;
	flags |= NDEF_FLAG_TNF & data->tnf;
	bool res = ndef_ostream_append(out, flags);
	if (!res) { out->buf_len = len0; return false; }
	
	// Type length.
	res = ndef_ostream_append(out, data->type_len);
	if (!res) { out->buf_len = len0; return false; }
	
	// Payload length.
	if (data->flag_short_record) {
		res = ndef_ostream_append(out, data->payload_len);
	} else {
		res  = ndef_ostream_append(out, data->payload_len >> 24);
		res &= ndef_ostream_append(out, data->payload_len >> 16);
		res &= ndef_ostream_append(out, data->payload_len >>  8);
		res &= ndef_ostream_append(out, data->payload_len >>  0);
		if (!res) { out->buf_len = len0; return false; }
	}
	
	// ID length.
	if (data->flag_include_id_len) {
		res = ndef_ostream_append(out, data->id_len);
		if (!res) { out->buf_len = len0; return false; }
	}
	
	// Type.
	if (data->type_len) {
		res = ndef_ostream_append_n(out, data->type, data->type_len);
		if (!res) { out->buf_len = len0; return false; }
	}
	
	// Payload.
	if (data->payload_len) {
		res = ndef_ostream_append_n(out, data->payload, data->payload_len);
		if (!res) { out->buf_len = len0; return false; }
	}
	
	// ID.
	if (data->flag_include_id_len && data->id_len) {
		res = ndef_ostream_append_n(out, data->id, data->id_len);
		if (!res) { out->buf_len = len0; return false; }
	}
	
	return true;
}



// Create an empty NDEF codec context.
ndef_ctx ndef_init() {
	// Make new memory.
	ndef_ctx out = malloc(sizeof(ndef_ctx_s));
	
	// Fill with placeholder values.
	if (out) *out = (ndef_ctx_s) {
		NDEF_MAGIC,
		0, 0, NULL,
		0, 0, NULL,
	};
	
	return out;
}

// Create a clone of an NDEF codec context.
ndef_ctx ndef_clone(ndef_ctx ctx) {
	MAGIC_CHECK
	
	// Make new memory.
	ndef_ctx out = malloc(sizeof(ndef_ctx));
	if (out) {
		// Copy values.
		*out = *ctx;
		
		// Clone arrays.
		out->abs_records = malloc(sizeof(ndef_record) * out->abs_records_cap);
		memcpy(out->abs_records, ctx->abs_records, sizeof(ndef_record) * out->abs_records_cap);
		out->raw_records = malloc(sizeof(ndef_raw_record) * out->raw_records_cap);
		memcpy(out->raw_records, ctx->raw_records, sizeof(ndef_raw_record) * out->raw_records_cap);
	}
	
	return out;
}

// Destroy an NDEF codec context.
void ndef_destroy(ndef_ctx ctx) {
	ndef_clear(ctx);
	ctx->magic = 0;
}


// Print a hierarchical info about this NDEF record.
static void ndef_record_print_info_r(ndef_record ctx, size_t recursion_limit, size_t indent);
// Print a hierarchical info about this NDEF message.
static void ndef_print_info_r(ndef_ctx ctx, size_t recursion_limit, size_t indent);

// Print a hierarchical info about this NDEF record.
static void ndef_record_print_info_r(ndef_record ctx, size_t recursion_limit, size_t indent) {
	// Check recursion limit.
	if (!recursion_limit) {
		for (int x = 0; x < indent; x++) putc(' ', stdout);
		printf("(recursion limited)\n");
		return;
	}
	
	// Check empty record.
	if ((!ctx.id_len && !ctx.payload_len && !ctx.type_len) || ctx.tnf == NDEF_TNF_EMPTY) {
		for (int x = 0; x < indent; x++) putc(' ', stdout);
		printf("(empty record)\n");
	} else {
		for (int x = 0; x < indent; x++) putc(' ', stdout);
		printf("NDEF record:\n");
		indent += 2;
	}
	
	// ID field.
	if (ctx.id_len) {
		for (int x = 0; x < indent; x++) putc(' ', stdout);
		printf("ID:    %zu byte", ctx.id_len);
		if (ctx.id_len <= 16) {
			if (ctx.id_len != 1) putc('s', stdout);
			hexdump(ctx.id, ctx.id_len, 2);
			
		} else /* ctx.id_len > 16 */ {
			printf("s:\n");
			hexdump(ctx.id, ctx.id_len, indent + 2);
		}
		
	}
	
	// Type field.
	if (ctx.type_len) {
		for (int x = 0; x < indent; x++) putc(' ', stdout);
		printf("Type:  %zu byte", ctx.type_len);
		if (ctx.type_len <= 16) {
			if (ctx.type_len != 1) putc('s', stdout);
			hexdump(ctx.type, ctx.type_len, 2);
			
		} else /* ctx.type_len > 16 */ {
			printf("s:\n");
			hexdump(ctx.type, ctx.type_len, indent + 2);
		}
		
	}
	
	// Check type.
	bool do_hexdump = false;
	if (ndef_record_is_smartposter(ctx)) {
		// Smart poster type.
		for (int x = 0; x < indent; x++) putc(' ', stdout);
		printf("Note:  Record is smart poster\n");
		
		// Try to show smart poster info.
		ndef_smartposter sp = ndef_record_get_smartposter(ctx);
		if (!sp.uri && !sp.text.lang) {
			do_hexdump = true;
		} else {
			ndef_print_info_r(sp.ndef, recursion_limit - 1, indent);
		}
		ndef_smartposter_destroy(sp);
		
	} else if (ndef_record_is_uri(ctx)) {
		// URI type.
		for (int x = 0; x < indent; x++) putc(' ', stdout);
		printf("Note:  Record is URI\n");
		
		// Try to show URI.
		char *uri = ndef_record_get_uri(ctx);
		if (uri) {
			for (int x = 0; x < indent; x++) putc(' ', stdout);
			printf("URI:   %s\n", uri);
			free(uri);
		} else {
			do_hexdump = true;
		}
		
	} else if (ndef_record_is_text(ctx)) {
		// Text type.
		for (int x = 0; x < indent; x++) putc(' ', stdout);
		printf("Note:  Record is text\n");
		
		// Try to show text.
		ndef_text text = ndef_record_get_text(ctx);
		if (text.lang) {
			for (int x = 0; x < indent; x++) putc(' ', stdout);
			printf("Lang:  %s\n", text.lang);
			for (int x = 0; x < indent; x++) putc(' ', stdout);
			printf("Text:  %s\n", text.text);
			
		} else {
			do_hexdump = true;
		}
		ndef_text_destroy(text);
	}
	
	if (do_hexdump) {
		// Default: Simple info dump.
		if (ctx.payload_len) {
			for (int x = 0; x < indent; x++) putc(' ', stdout);
			printf("Payload: %zu byte", ctx.payload_len);
			if (ctx.payload_len <= 16) {
				if (ctx.payload_len != 1) putc('s', stdout);
				hexdump(ctx.payload, ctx.payload_len, 2);
				
			} else /* ctx.payload_len > 16 */ {
				printf("s:\n");
				hexdump(ctx.payload, ctx.payload_len, indent + 2);
			}
			
		} else {
			for (int x = 0; x < indent; x++) putc(' ', stdout);
			printf("Payload: empty\n");
		}
	}
}

// Print a hierarchical info about this NDEF message.
static void ndef_print_info_r(ndef_ctx ctx, size_t recursion_limit, size_t indent) {
	// Check recursion limit.
	if (!recursion_limit) {
		for (int x = 0; x < indent; x++) putc(' ', stdout);
		printf("(recursion limited)\n");
		return;
	}
	
	// Print number of records.
	for (int x = 0; x < indent; x++) putc(' ', stdout);
	if (ctx->abs_records_len) {
		printf("NDEF message: %zu record%s", ctx->abs_records_len, ctx->abs_records_len == 1 ? "\n" : "s\n");
	} else {
		printf("NDEF message: empty\n");
	}
	
	// Do the print on EACH ONE.
	for (size_t i = 0; i < ctx->abs_records_len; i++) {
		ndef_record_print_info_r(ctx->abs_records[i], recursion_limit-1, indent + 2);
	}
}

// Print a hierarchical info about this NDEF message.
void ndef_print_info(ndef_ctx ctx) {
	ndef_print_info_r(ctx, 8, 0);
}


// Decode a blob of NDEF data.
ndef_ctx ndef_decode(uint8_t *data, size_t *len) {
	bool partial = false;
	
	// NULL checks.
	if (!data || !len || !*len) return NULL;
	
	// Make a context for APPENDING to.
	ndef_ctx ctx = ndef_init();
	
	// Continuous parsing time!
	size_t pos = 0;
	while (*len > pos) {
		// Decode one record.
		ndef_raw_record record;
		size_t record_len = *len - pos;
		bool res = ndef_raw_record_decode(&record, data + pos, &record_len);
		if (!res) { partial = true; break; }
		pos += record_len;
		
		// Append raw record.
		res = ndef_raw_append(ctx, record);
		if (!res) { partial = true; break; }
	}
	
	// Decode raw records into full records.
	for (size_t i = 0; i < ctx->raw_records_len; i++) {
		// TODO: Support for chunked records.
		ctx->raw_records[i].abs_index = i;
		ctx->raw_records[i].raw_index = i;
		ctx->raw_records[i].raw_len   = 1;
		ndef_append(ctx, ctx->raw_records[i].abstract);
	}
	
	if (partial) {
		printf("NDEF: Note: Decoding is partial\n");
	}
	return ctx;
}

// Encode the NDEF data into a new blob.
bool ndef_encode(ndef_ctx ctx, uint8_t **out_data, size_t *out_len) {
	MAGIC_CHECK
	
	// Make stream to output to.
	ndef_ostream out = ndef_ostream_init();
	
	// Add some chunks.
	ndef_raw_clear(ctx);
	for (size_t i = 0; i < ctx->abs_records_len; i++) {
		// Make a raw record.
		ndef_raw_record raw = { .raw_index = i, .raw_len = 0, .abs_index = i };
		// Think up some flags for it.
		raw.abstract			= ctx->abs_records[i];
		raw.flag_begin			= i == 0;
		raw.flag_end			= i == ctx->abs_records_len - 1;
		raw.flag_chunked		= false;
		raw.flag_short_record	= !(raw.payload_len & 0xffffff00);
		raw.flag_include_id_len	= raw.id_len;
		// Write out the data.
		bool res = ndef_raw_record_encode(&out, &raw);
		if (!res) {
			ndef_ostream_destroy(out);
			return false;
		}
	}
	
	// Output the final data.
	*out_data = out.buf;
	*out_len  = out.buf_len;
	return true;
}


// Get the number of raw NDEF records.
size_t ndef_raw_records_len(ndef_ctx ctx) {
	MAGIC_CHECK
	return ctx->raw_records_len;
}

// Get a pointer to the raw NDEF records.
const ndef_raw_record *ndef_raw_records(ndef_ctx ctx) {
	return ctx->raw_records;
}

// Delete all raw records but keep abstract ones.
void ndef_raw_clear(ndef_ctx ctx) {
	MAGIC_CHECK
	
	// Clear raw records.
	if (ctx->raw_records) free(ctx->raw_records);
	ctx->raw_records_cap = 0;
	ctx->raw_records_len = 0;
	
	// Remove pointers from abstract records.
	for (size_t i = 0; i < ctx->abs_records_len; i++) {
		ctx->abs_records[i].raw_index = 0;
		ctx->abs_records[i].raw_len   = 0;
	}
}


// Get the number of abstract NDEF records.
size_t ndef_records_len(ndef_ctx ctx) {
	MAGIC_CHECK
	return ctx->abs_records_len;
}

// Get a pointer to the abstract NDEF records.
const ndef_record *ndef_records(ndef_ctx ctx) {
	MAGIC_CHECK
	return ctx->abs_records;
}

// Delete all records.
void ndef_clear(ndef_ctx ctx) {
	if (ctx->raw_records) free(ctx->raw_records);
	if (ctx->abs_records) free(ctx->abs_records);
	ctx->raw_records_cap = 0;
	ctx->raw_records_len = 0;
	ctx->abs_records_cap = 0;
	ctx->abs_records_len = 0;
}


// Delete an abstract NDEF record in the message.
void ndef_splice(ndef_ctx ctx, size_t index) {
	ndef_splice_n(ctx, index, 1);
}

// Delete one or more NDEF records in the message.
void ndef_splice_n(ndef_ctx ctx, size_t index, size_t len) {
	MAGIC_CHECK
}


// Insert one or more NDEF records in an arbitrary index in the message.
// Does not create a corresponding raw record.
bool insert_n(ndef_ctx ctx, size_t index, const ndef_record *records, size_t len, bool is_move) {
	MAGIC_CHECK
	
	// Bounds check.
	if (index > ctx->abs_records_len) index = ctx->abs_records_len;
	
	// Allocate memories.
	size_t old_len = ctx->abs_records_len;
	size_t new_len = ctx->abs_records_len + len;
	if (new_len > ctx->abs_records_cap) {
		// Determine new capacity.
		size_t cap = ctx->abs_records_cap;
		if (!cap) cap = 1;
		while (new_len > cap) cap *= 2;
		
		// Allocate new memory.
		void *mem = realloc(ctx->abs_records, cap * sizeof(ndef_record));
		if (!mem) {
			printf("NDEF: Error: Out of memory (allocating %zu bytes)\n", (size_t) cap);
			return false;
		}
		ctx->abs_records     = mem;
		ctx->abs_records_cap = cap;
	}
	
	// Relocate objects.
	for (ssize_t i = old_len - index - 1; i >= 0; i--) {
		ctx->abs_records[old_len + i] = ctx->abs_records[index + i];
	}
	
	// Fill in new objects.
	size_t i;
	for (i = 0; i < len; i ++) {
		ndef_record *ptr = ctx->abs_records + index + i;
		*ptr = records[i];
		if (!is_move) {
			if (ptr->type_len) {
				ptr->type    = malloc(ptr->type_len);
				if (!ptr->type) break;
				memcpy(ptr->type, records[i].type, ptr->type_len);
			}
			if (ptr->payload_len) {
				ptr->payload = malloc(ptr->payload_len);
				if (!ptr->payload) {
					if (ptr->type) free(ptr->type);
					break;
				}
				memcpy(ptr->payload, records[i].payload, ptr->payload_len);
			}
			if (ptr->id_len) {
				ptr->id      = malloc(ptr->id_len);
				if (!ptr->id) {
					if (ptr->type) free(ptr->type);
					if (ptr->payload) free(ptr->payload);
					break;
				}
				memcpy(ptr->id, records[i].id, ptr->id_len);
			}
		}
	}
	
	// Excessively complicated error handling.
	if (!is_move && i < len) {
		// Free memory after unsuccessful allocation.
		for (size_t x = 0; x < i; x++) {
			ndef_record_destroy(ctx->abs_records[index + i]);
		}
		
		// Undo the shifting operation too.
		for (size_t i = 0; i < old_len - index; i++) {
			ctx->abs_records[index + i] = ctx->abs_records[old_len + i];
		}
		
		return false;
	}
	
	ctx->abs_records_len = new_len;
	return true;
}


// Insert an NDEF record in an arbitrary index in the message.
// Does not create a corresponding raw record.
bool ndef_insert(ndef_ctx ctx, size_t index, ndef_record record) {
	return ndef_insert_n(ctx, index, &record, 1);
}

// Insert one or more NDEF records in an arbitrary index in the message.
// Does not create a corresponding raw record.
bool ndef_insert_n(ndef_ctx ctx, size_t index, const ndef_record *records, size_t len) {
	return insert_n(ctx, index, records, len, false);
}

// Append an NDEF record to the message.
// Does not create a corresponding raw record.
bool ndef_append(ndef_ctx ctx, ndef_record record) {
	return ndef_insert_n(ctx, SIZE_MAX, &record, 1);
}

// Append an NDEF record to the message.
// Does not create a corresponding raw record.
bool ndef_append_n(ndef_ctx ctx, size_t index, const ndef_record *records, size_t len) {
	return ndef_insert_n(ctx, SIZE_MAX, records, len);
}


// Insert an NDEF record in an arbitrary index in the message.
// Does not create a corresponding raw record.
// Moves the input data; `ctx` shall take ownership of contained resources.
bool ndef_insert_mv(ndef_ctx ctx, size_t index, ndef_record record) {
	return ndef_insert_n_mv(ctx, index, &record, 1);
}

// Insert one or more NDEF records in an arbitrary index in the message.
// Does not create a corresponding raw record.
// Moves the input data; `ctx` shall take ownership of contained resources.
bool ndef_insert_n_mv(ndef_ctx ctx, size_t index, ndef_record *records, size_t len) {
	return insert_n(ctx, index, records, len, true);
}

// Append an NDEF record to the message.
// Does not create a corresponding raw record.
// Moves the input data; `ctx` shall take ownership of contained resources.
bool ndef_append_mv(ndef_ctx ctx, ndef_record record) {
	return ndef_insert_n_mv(ctx, SIZE_MAX, &record, 1);
}

// Append an NDEF record to the message.
// Does not create a corresponding raw record.
// Moves the input data; `ctx` shall take ownership of contained resources.
bool ndef_append_n_mv(ndef_ctx ctx, size_t index, ndef_record *records, size_t len) {
	return ndef_insert_n_mv(ctx, SIZE_MAX, records, len);
}
