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


// Possible prepended URI values enum.
typedef enum {
	NDEF_URI_ABBREVNONE = 0x00,
	NDEF_URI_ABBREV_http_www = 0x01,
	NDEF_URI_ABBREV_https_www = 0x02,
	NDEF_URI_ABBREV_http = 0x03,
	NDEF_URI_ABBREV_https = 0x04,
	NDEF_URI_ABBREV_tel = 0x05,
	NDEF_URI_ABBREV_mailto = 0x06,
	NDEF_URI_ABBREV_ftp_anonymous_anonymous = 0x07,
	NDEF_URI_ABBREV_ftp_ftp = 0x08,
	NDEF_URI_ABBREV_ftps = 0x09,
	NDEF_URI_ABBREV_sftp = 0x0A,
	NDEF_URI_ABBREV_smb = 0x0B,
	NDEF_URI_ABBREV_nfs = 0x0C,
	NDEF_URI_ABBREV_ftp = 0x0D,
	NDEF_URI_ABBREV_dav = 0x0E,
	NDEF_URI_ABBREV_news = 0x0F,
	NDEF_URI_ABBREV_telnet = 0x10,
	NDEF_URI_ABBREV_imap = 0x11,
	NDEF_URI_ABBREV_rtsp = 0x12,
	NDEF_URI_ABBREV_urn = 0x13,
	NDEF_URI_ABBREV_pop = 0x14,
	NDEF_URI_ABBREV_sip = 0x15,
	NDEF_URI_ABBREV_sips = 0x16,
	NDEF_URI_ABBREV_tftp = 0x17,
	NDEF_URI_ABBREV_btspp = 0x18,
	NDEF_URI_ABBREV_btl2cap = 0x19,
	NDEF_URI_ABBREV_btgoep = 0x1A,
	NDEF_URI_ABBREV_tcpobex = 0x1B,
	NDEF_URI_ABBREV_irdaobex = 0x1C,
	NDEF_URI_ABBREV_file = 0x1D,
	NDEF_URI_ABBREV_urn_epc_id = 0x1E,
	NDEF_URI_ABBREV_urn_epc_tag = 0x1F,
	NDEF_URI_ABBREV_urn_epc_pat = 0x20,
	NDEF_URI_ABBREV_urn_epc_raw = 0x21,
	NDEF_URI_ABBREV_urn_epc = 0x22,
	NDEF_URI_ABBREV_urn_nfc = 0x23,
	// Number of URI abbreviations
	NDEF_URI_ABBREVMAX
} ndef_uri_abbrev;

// Table containing string values of URI abbreviations.
extern const char *ndef_uri_abbrev_table[NDEF_URI_ABBREVMAX];

// Determine whether an NDEF record is a URI record.
bool ndef_record_is_uri(ndef_record ctx);
// Construct a string containing the full URI from a URI record.
// Returns NULL when out of memory, or when not a URI record.
char *ndef_record_get_uri(ndef_record ctx);
// Construct an NDEF record containing the given URI.
// The internal data may be abbreviated according to `ndef_uri_abbrev`.
ndef_record ndef_record_new_uri(const char *uri);
// Construct an NDEF record containing the given URI.
// The internal data will not be abbreviated.
ndef_record ndef_record_new_raw_uri(const char *uri);


#ifdef __cplusplus
} // extern "C"
#endif
