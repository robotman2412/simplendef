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

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif



// Flag bitmask for NDEF MB flag.
#define NDEF_FLAG_MB	0x80
// Flag bitmask for NDEF ME flag.
#define NDEF_FLAG_ME	0x40
// Flag bitmask for NDEF CR flag.
#define NDEF_FLAG_CF	0x20
// Flag bitmask for NDEF SR flag.
#define NDEF_FLAG_SR	0x10
// Flag bitmask for NDEF IL flag.
#define NDEF_FLAG_IL	0x08
// Bitmask for getting TNF field.
#define NDEF_FLAG_TNF	0x07

// Type name format for an NDEF record.
typedef enum {
	// Empty record.
	NDEF_TNF_EMPTY		= 0x00,
	// NFC-specific type.
	NDEF_TNF_WELL_KNOWN	= 0x01,
	// One of the MIME types.
	NDEF_TNF_MIME		= 0x02,
	// URI (link) type.
	NDEF_TNF_URI		= 0x03,
	// Nonstandard type.
	NDEF_TNF_EXTERNAL	= 0x04,
	// Uknown type.
	NDEF_TNF_UNKNOWN	= 0x05,
	// Magic value for chunked records.
	NDEF_TNF_UNCHANGED	= 0x06,
} ndef_tnf;

// LUT from ndef_tnf to name.
extern const char *ndef_tnf_names[8];


// Abstract NDEF record without encoding details.
typedef struct {
	// Index of corresponding raw record.
	size_t   raw_index;
	// Number of raw records used to make this record.
	size_t   raw_len;
	
	// Type of data in this record.
	ndef_tnf tnf;
	
	// Length of the type field.
	uint8_t  type_len;
	// Length of the payload field.
	size_t   payload_len;
	// Length of the ID field.
	uint8_t  id_len;
	
	// User-specified payload type.
	uint8_t *type;
	// User-specified payload.
	uint8_t *payload;
	// User-specified ID.
	uint8_t *id;
} ndef_record;


// Encoding details for an NDEF record.
typedef struct {
	// Index of corresponding abstract record.
	size_t   abs_index;
	
	// First record flag.
	bool     flag_begin;
	// Last record flag.
	bool     flag_end;
	// Chunked data flag.
	bool     flag_chunked;
	// Short record flag.
	bool     flag_short_record;
	// Includes ID length flag.
	bool     flag_include_id_len;
} ndef_enc_detail;


// Representation of the data in an NDEF record.
typedef struct {
	/* ==== Abstract ==== */
	union {
		// Catch-all for abstract information.
		ndef_record abstract;
		
		struct {
			// Index of corresponding raw record.
			size_t   raw_index;
			// Number of raw records used to make this record.
			size_t   raw_len;
			
			// Type of data in this record.
			ndef_tnf tnf;
			
			// Length of the type field.
			uint8_t  type_len;
			// Length of the payload field.
			size_t   payload_len;
			// Length of the ID field.
			uint8_t  id_len;
			
			// User-specified payload type.
			uint8_t *type;
			// User-specified payload.
			uint8_t *payload;
			// User-specified ID.
			uint8_t *id;
		};
	};
	
	/* ==== Encoding details ==== */
	union {
		// Catch-all for encoding details.
		ndef_enc_detail enc_detail;
		
		struct {
			// Index of corresponding abstract record.
			size_t   abs_index;
			
			// First record flag.
			bool     flag_begin;
			// Last record flag.
			bool     flag_end;
			// Chunked data flag.
			bool     flag_chunked;
			// Short record flag.
			bool     flag_short_record;
			// Includes ID length flag.
			bool     flag_include_id_len;
		};
	};
} ndef_raw_record;



#ifdef NDEF_REVEAL_PRIVATE

// Magic value as in ndef_ctx.
#define NDEF_MAGIC 0xdeadbeef

// All context required to read and write NDEF messages.
typedef struct {
	// Magic value.
	uint32_t     magic;
	
	// Number of NDEF records.
	size_t       raw_records_len;
	// Capacity for NDEF record pointers.
	size_t       raw_records_cap;
	// NDEF record pointers.
	ndef_raw_record *raw_records;
	
	// Number of abstract NDEF records.
	size_t       abs_records_len;
	// Capacity for abstract NDEF record pointers.
	size_t       abs_records_cap;
	// Abstract NDEF record pointers.
	ndef_record *abs_records;
} ndef_ctx_s;

// All context required to read and write NDEF messages.
typedef ndef_ctx_s *ndef_ctx;

// Append a raw NDEF record.
bool		ndef_raw_append		(ndef_ctx ctx, ndef_raw_record record);
// Clone an ndef_raw_record.
bool		ndef_record_clone	(ndef_record in, ndef_record *out);
// Parse a single NDEF record from a blob of data.
// Sets `len` to the amount of successfully decoded data when finished.
bool		ndef_record_parse	(ndef_raw_record *out, uint8_t *data, size_t *len);

#else

// All context required to read and write NDEF messages.
typedef void *ndef_ctx;

#endif


// Create an empty NDEF codec context.
ndef_ctx	ndef_init			();
// Create a clone of an NDEF codec context.
ndef_ctx	ndef_clone			(ndef_ctx ctx);
// Destroy an NDEF codec context.
void		ndef_destroy		(ndef_ctx ctx);
// Print a hierarchical info about this NDEF message.
void		ndef_print_info		(ndef_ctx ctx);

// Parse a blob of NDEF data.
// Sets `len` to the amount of successfully decoded data when finished.
ndef_ctx	ndef_decode			(uint8_t *data, size_t *len);
// Encode the NDEF data into a new blob.
bool		ndef_encode			(ndef_ctx ctx, uint8_t **out_data, size_t *out_len);

// Get the number of raw NDEF records.
size_t		ndef_raw_records_len(ndef_ctx ctx);
// Get a pointer to the raw NDEF records.
const ndef_raw_record *
			ndef_raw_records	(ndef_ctx ctx);
// Delete all raw records but keep abstract ones.
void		ndef_raw_clear		(ndef_ctx ctx);

// Get the number of abstract NDEF records.
size_t		ndef_records_len	(ndef_ctx ctx);
// Get a pointer to the abstract NDEF records.
const ndef_record *
			ndef_records		(ndef_ctx ctx);
// Delete all records.
void		ndef_clear			(ndef_ctx ctx);

// Delete an NDEF record in the message.
void		ndef_splice			(ndef_ctx ctx, size_t index);
// Delete one or more NDEF records in the message.
void		ndef_splice_n		(ndef_ctx ctx, size_t index, size_t len);

// Insert an NDEF record in an arbitrary index in the message.
// Does not create a corresponding raw record.
bool		ndef_insert			(ndef_ctx ctx, size_t index, ndef_record record);
// Insert one or more NDEF records in an arbitrary index in the message.
// Does not create a corresponding raw record.
bool		ndef_insert_n		(ndef_ctx ctx, size_t index, const ndef_record *records, size_t len);
// Append an NDEF record to the message.
// Does not create a corresponding raw record.
bool		ndef_append			(ndef_ctx ctx, ndef_record record);
// Append an NDEF record to the message.
// Does not create a corresponding raw record.
bool		ndef_append_n		(ndef_ctx ctx, size_t index, const ndef_record *records, size_t len);

// Insert an NDEF record in an arbitrary index in the message.
// Does not create a corresponding raw record.
// Moves the input data; `ctx` shall take ownership of contained resources if and only if the operation is successful.
bool		ndef_insert_mv		(ndef_ctx ctx, size_t index, ndef_record record);
// Insert one or more NDEF records in an arbitrary index in the message.
// Does not create a corresponding raw record.
// Moves the input data; `ctx` shall take ownership of contained resources if and only if the operation is successful.
bool		ndef_insert_n_mv	(ndef_ctx ctx, size_t index, ndef_record *records, size_t len);
// Append an NDEF record to the message.
// Does not create a corresponding raw record.
// Moves the input data; `ctx` shall take ownership of contained resources if and only if the operation is successful.
bool		ndef_append_mv		(ndef_ctx ctx, ndef_record record);
// Append an NDEF record to the message.
// Does not create a corresponding raw record.
// Moves the input data; `ctx` shall take ownership of contained resources if and only if the operation is successful.
bool		ndef_append_n_mv	(ndef_ctx ctx, size_t index, ndef_record *records, size_t len);


// Create an ndef_raw_record with all ZERO / NULL.
static inline ndef_raw_record ndef_raw_record_init() {
	return (ndef_raw_record) { 0 };
}
// Free resources from an ndef_raw_record (call `free` on `type`, `payload` and `id`).
static inline void ndef_raw_record_destroy(ndef_raw_record ctx) {
	if (ctx.type)    free(ctx.type);
	if (ctx.payload) free(ctx.payload);
	if (ctx.id)      free(ctx.id);
}


// Create an ndef_record with all ZERO / NULL.
static inline ndef_record ndef_record_init() {
	return (ndef_record) { 0 };
}
// Free resources from an ndef_record (call `free` on `type`, `payload` and `id`).
static inline void ndef_record_destroy(ndef_record ctx) {
	if (ctx.type)    free(ctx.type);
	if (ctx.payload) free(ctx.payload);
	if (ctx.id)      free(ctx.id);
}


#ifdef __cplusplus
} // extern "C"
#endif
