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

#include "ndef_uri.h"



// Table containing string values of URI abbreviations.
const char *ndef_uri_abbrev_table[NDEF_URI_ABBREVMAX] = {
	"",
	"http://www.",
	"https://www.",
	"http://",
	"https://",
	"tel:",
	"mailto:",
	"ftp://anonymous:anonymous@",
	"ftp://ftp.",
	"ftps://",
	"sftp://",
	"smb://",
	"nfs://",
	"ftp://",
	"dav://",
	"news:",
	"telnet://",
	"imap:",
	"rtsp://",
	"urn:",
	"pop:",
	"sip:",
	"sips:",
	"tftp:",
	"btspp://",
	"btl2cap://",
	"btgoep://",
	"tcpobex://",
	"irdaobex://",
	"file://",
	"urn:epc:id:",
	"urn:epc:tag:",
	"urn:epc:pat:",
	"urn:epc:raw:",
	"urn:epc:",
	"urn:nfc:",
};



// Determine whether an NDEF record is a URI record.
bool ndef_record_is_uri(ndef_record ctx) {
	return ctx.tnf == NDEF_TNF_WELL_KNOWN && ctx.type_len == 1 && ctx.type[0] == 'U' && ctx.payload_len >= 2;
}

// Construct a string containing the full URI from a URI record.
// Returns NULL when out of memory, or when not a URI record.
char *ndef_record_get_uri(ndef_record ctx) {
	if (ndef_record_is_uri(ctx)) {
		if (ctx.payload[0]) {
			// Decode abbreviation.
			if (ctx.payload[0] >= NDEF_URI_ABBREVMAX) {
				return NULL;
			}
			const char *abbrev = ndef_uri_abbrev_table[ctx.payload[0]];
			// Allocate memory.
			size_t cap = strlen(abbrev) + strnlen((char *) ctx.payload + 1, ctx.payload_len - 1);
			char *mem = malloc(cap + 1);
			if (!mem) return NULL;
			// Copy string data.
			strcpy(mem, abbrev);
			strncat(mem, (char *) ctx.payload + 1, ctx.payload_len - 1);
			mem[cap] = 0;
			// Return buffer.
			return mem;
		} else {
			// Just clone the string.
			return strndup((char *) ctx.payload + 1, ctx.payload_len - 1);
		}
	} else {
		return NULL;
	}
}

// Common URI record creator.
static ndef_record new_uri(uint8_t abbrev, const char *uri) {
	// Allocate memory.
	char *mem = malloc(strlen(uri) + 2);
	if (!mem) {
		return ndef_record_init();
	}
	
	// Create payload.
	*mem = abbrev;
	strcpy(mem + 1, uri);
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
		.payload_len	= strlen(uri) + 1,
		.id_len			= 0,
		.type			= type,
		.payload		= (uint8_t *) mem,
		.id				= NULL,
	};
}

// Construct an NDEF record containing the given URI.
// The internal data may be abbreviated according to `ndef_uri_abbrev`.
ndef_record ndef_record_new_uri(const char *uri) {
	// Check abbreviation table.
	uint_fast8_t match     = 0;
	size_t       match_len = 0;
	for (uint_fast8_t i = 1; i < NDEF_URI_ABBREVMAX; i++) {
		size_t len = strlen(ndef_uri_abbrev_table[i]);
		if (len > match_len && !strncmp(ndef_uri_abbrev_table[i], uri, len)) {
			match     = i;
			match_len = len;
		}
	}
	
	// Create with the abbreviation.
	return new_uri(match, uri + match_len);
}

// Construct an NDEF record containing the given URI.
// The internal data will not be abbreviated.
ndef_record ndef_record_new_raw_uri(const char *uri) {
	return new_uri(0, uri);
}
