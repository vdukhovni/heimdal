/*
 * Copyright (c) 2003 Kungliga Tekniska H�gskolan
 * (Royal Institute of Technology, Stockholm, Sweden). 
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 *
 * 3. Neither the name of KTH nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY KTH AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL KTH OR ITS CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#include "krb5_locl.h"

#ifdef HAVE_OPENSSL
#include <openssl/evp.h>
#endif

RCSID("$Id$");

static int verbose = 0;

static void
hex_dump_data(krb5_data *data)
{
    unsigned char *p = data->data;
    int i, j;

    for (i = j = 0; i < data->length; i++) {
	printf("%02x ", p[i]);
	if (j > 15) {
	    printf("\n");
	    j = 0;
	} else
	    j++;
    }
    if (j != 0)
	printf("\n");
}

struct {
    char *password;
    char *salt;
    int saltlen;
    int iterations;
    krb5_enctype enctype;
    int keylen;
    char *pbkdf2;
    char *key;
} keys[] = {
#ifdef ENABLE_AES
    { 
	"password", "ATHENA.MIT.EDUraeburn", -1,
	1, 
	ETYPE_AES128_CTS_HMAC_SHA1_96, 16,
	"\xcd\xed\xb5\x28\x1b\xb2\xf8\x01\x56\x5a\x11\x22\xb2\x56\x35\x15",
	"\x42\x26\x3c\x6e\x89\xf4\xfc\x28\xb8\xdf\x68\xee\x09\x79\x9f\x15"
    },
    {
	"password", "ATHENA.MIT.EDUraeburn", -1,
	1, 
	ETYPE_AES256_CTS_HMAC_SHA1_96, 32,
	"\xcd\xed\xb5\x28\x1b\xb2\xf8\x01\x56\x5a\x11\x22\xb2\x56\x35\x15"
	"\x0a\xd1\xf7\xa0\x4b\xb9\xf3\xa3\x33\xec\xc0\xe2\xe1\xf7\x08\x37",
	"\xfe\x69\x7b\x52\xbc\x0d\x3c\xe1\x44\x32\xba\x03\x6a\x92\xe6\x5b"
	"\xbb\x52\x28\x09\x90\xa2\xfa\x27\x88\x39\x98\xd7\x2a\xf3\x01\x61"
    },
    {
	"password", "ATHENA.MIT.EDUraeburn", -1,
	2,
	ETYPE_AES128_CTS_HMAC_SHA1_96, 16,
	"\x01\xdb\xee\x7f\x4a\x9e\x24\x3e\x98\x8b\x62\xc7\x3c\xda\x93\x5d",
	"\xc6\x51\xbf\x29\xe2\x30\x0a\xc2\x7f\xa4\x69\xd6\x93\xbd\xda\x13"
    },
    {
	"password", "ATHENA.MIT.EDUraeburn", -1,
	2, 
	ETYPE_AES256_CTS_HMAC_SHA1_96, 32,
	"\x01\xdb\xee\x7f\x4a\x9e\x24\x3e\x98\x8b\x62\xc7\x3c\xda\x93\x5d"
	"\xa0\x53\x78\xb9\x32\x44\xec\x8f\x48\xa9\x9e\x61\xad\x79\x9d\x86",
	"\xa2\xe1\x6d\x16\xb3\x60\x69\xc1\x35\xd5\xe9\xd2\xe2\x5f\x89\x61"
	"\x02\x68\x56\x18\xb9\x59\x14\xb4\x67\xc6\x76\x22\x22\x58\x24\xff"
    },
    {
	"password", "ATHENA.MIT.EDUraeburn", -1,
	1200, 
	ETYPE_AES128_CTS_HMAC_SHA1_96, 16,
	"\x5c\x08\xeb\x61\xfd\xf7\x1e\x4e\x4e\xc3\xcf\x6b\xa1\xf5\x51\x2b",
	"\x4c\x01\xcd\x46\xd6\x32\xd0\x1e\x6d\xbe\x23\x0a\x01\xed\x64\x2a"
    },
    {
	"password", "ATHENA.MIT.EDUraeburn", -1,
	1200, 
	ETYPE_AES256_CTS_HMAC_SHA1_96, 32,
	"\x5c\x08\xeb\x61\xfd\xf7\x1e\x4e\x4e\xc3\xcf\x6b\xa1\xf5\x51\x2b"
	"\xa7\xe5\x2d\xdb\xc5\xe5\x14\x2f\x70\x8a\x31\xe2\xe6\x2b\x1e\x13",
	"\x55\xa6\xac\x74\x0a\xd1\x7b\x48\x46\x94\x10\x51\xe1\xe8\xb0\xa7"
	"\x54\x8d\x93\xb0\xab\x30\xa8\xbc\x3f\xf1\x62\x80\x38\x2b\x8c\x2a"
    },
    {
	"password", "\x12\x34\x56\x78\x78\x56\x34\x12", 8,
	5,
	ETYPE_AES128_CTS_HMAC_SHA1_96, 16,
	"\xd1\xda\xa7\x86\x15\xf2\x87\xe6\xa1\xc8\xb1\x20\xd7\x06\x2a\x49",
	"\xe9\xb2\x3d\x52\x27\x37\x47\xdd\x5c\x35\xcb\x55\xbe\x61\x9d\x8e"
    },
    {
	"password", "\x12\x34\x56\x78\x78\x56\x34\x12", 8,
	5,
	ETYPE_AES256_CTS_HMAC_SHA1_96, 32,
	"\xd1\xda\xa7\x86\x15\xf2\x87\xe6\xa1\xc8\xb1\x20\xd7\x06\x2a\x49"
	"\x3f\x98\xd2\x03\xe6\xbe\x49\xa6\xad\xf4\xfa\x57\x4b\x6e\x64\xee",
	"\x97\xa4\xe7\x86\xbe\x20\xd8\x1a\x38\x2d\x5e\xbc\x96\xd5\x90\x9c"
	"\xab\xcd\xad\xc8\x7c\xa4\x8f\x57\x45\x04\x15\x9f\x16\xc3\x6e\x31"
    },
    {
	"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
	"pass phrase equals block size", -1,
	1200,
	ETYPE_AES128_CTS_HMAC_SHA1_96, 16,
	"\x13\x9c\x30\xc0\x96\x6b\xc3\x2b\xa5\x5f\xdb\xf2\x12\x53\x0a\xc9",
	"\x59\xd1\xbb\x78\x9a\x82\x8b\x1a\xa5\x4e\xf9\xc2\x88\x3f\x69\xed"
    },
    {
	"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
	"pass phrase equals block size", -1,
	1200,
	ETYPE_AES256_CTS_HMAC_SHA1_96, 32,
	"\x13\x9c\x30\xc0\x96\x6b\xc3\x2b\xa5\x5f\xdb\xf2\x12\x53\x0a\xc9"
	"\xc5\xec\x59\xf1\xa4\x52\xf5\xcc\x9a\xd9\x40\xfe\xa0\x59\x8e\xd1",
	"\x89\xad\xee\x36\x08\xdb\x8b\xc7\x1f\x1b\xfb\xfe\x45\x94\x86\xb0"
	"\x56\x18\xb7\x0c\xba\xe2\x20\x92\x53\x4e\x56\xc5\x53\xba\x4b\x34"
    },
    {
	"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
	"pass phrase exceeds block size", -1,
	1200,
	ETYPE_AES128_CTS_HMAC_SHA1_96, 16,
	"\x9c\xca\xd6\xd4\x68\x77\x0c\xd5\x1b\x10\xe6\xa6\x87\x21\xbe\x61",
	"\xcb\x80\x05\xdc\x5f\x90\x17\x9a\x7f\x02\x10\x4c\x00\x18\x75\x1d"
    },
    {
	"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
	"pass phrase exceeds block size", -1,
	1200,
	ETYPE_AES256_CTS_HMAC_SHA1_96, 32,
	"\x9c\xca\xd6\xd4\x68\x77\x0c\xd5\x1b\x10\xe6\xa6\x87\x21\xbe\x61"
	"\x1a\x8b\x4d\x28\x26\x01\xdb\x3b\x36\xbe\x92\x46\x91\x5e\xc8\x2a",
	"\xd7\x8c\x5c\x9c\xb8\x72\xa8\xc9\xda\xd4\x69\x7f\x0b\xb5\xb2\xd2"
	"\x14\x96\xc8\x2b\xeb\x2c\xae\xda\x21\x12\xfc\xee\xa0\x57\x40\x1b"

    },
    {
	"\xf0\x9d\x84\x9e" /* g-clef */, "EXAMPLE.COMpianist", -1,
	50,
	ETYPE_AES128_CTS_HMAC_SHA1_96, 16,
	"\x6b\x9c\xf2\x6d\x45\x45\x5a\x43\xa5\xb8\xbb\x27\x6a\x40\x3b\x39",
	"\xf1\x49\xc1\xf2\xe1\x54\xa7\x34\x52\xd4\x3e\x7f\xe6\x2a\x56\xe5"
    },
    {
	"\xf0\x9d\x84\x9e" /* g-clef */, "EXAMPLE.COMpianist", -1,
	50,
	ETYPE_AES256_CTS_HMAC_SHA1_96, 32,
	"\x6b\x9c\xf2\x6d\x45\x45\x5a\x43\xa5\xb8\xbb\x27\x6a\x40\x3b\x39"
	"\xe7\xfe\x37\xa0\xc4\x1e\x02\xc2\x81\xff\x30\x69\xe1\xe9\x4f\x52",
	"\x4b\x6d\x98\x39\xf8\x44\x06\xdf\x1f\x09\xcc\x16\x6d\xb4\xb8\x3c"
	"\x57\x18\x48\xb7\x84\xa3\xd6\xbd\xc3\x46\x58\x9a\x3e\x39\x3f\x9e"
    },
#endif
    {
	"foo", "", -1, 
	0,
	ETYPE_ARCFOUR_HMAC_MD5, 16,
	NULL,
	"\xac\x8e\x65\x7f\x83\xdf\x82\xbe\xea\x5d\x43\xbd\xaf\x78\x00\xcc"
    },
    {
	"test", "", -1, 
	0,
	ETYPE_ARCFOUR_HMAC_MD5, 16,
	NULL,
	"\x0c\xb6\x94\x88\x05\xf7\x97\xbf\x2a\x82\x80\x79\x73\xb8\x95\x37"
    }
};

static int
string_to_key_test(krb5_context context)
{
    krb5_data password, opaque;
    krb5_error_code ret;
    krb5_keyblock key;
    krb5_salt salt;
    int i, val = 0;
    char iter[4];

    for (i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {

	password.data = keys[i].password;
	password.length = strlen(password.data);

	salt.salttype = KRB5_PW_SALT;
	salt.saltvalue.data = keys[i].salt;
	if (keys[i].saltlen == -1)
	    salt.saltvalue.length = strlen(salt.saltvalue.data);
	else
	    salt.saltvalue.length = keys[i].saltlen;
    
	opaque.data = iter;
	opaque.length = sizeof(iter);
	_krb5_put_int(iter, keys[i].iterations, 4);
	
#ifdef ENABLE_AES
	if (keys[i].pbkdf2) {
#ifdef HAVE_OPENSSL
	    char keyout[32];

	    if (keys[i].keylen > sizeof(keyout))
		abort();

	    PKCS5_PBKDF2_HMAC_SHA1(password.data, password.length,
				   salt.saltvalue.data, salt.saltvalue.length,
				   keys[i].iterations, 
				   keys[i].keylen, keyout);
	    
	    if (memcmp(keyout, keys[i].pbkdf2, keys[i].keylen) != 0) {
		krb5_warnx(context, "%d: openssl key pbkdf2", i);
		val = 1;
		continue;
	    }
#endif
	    
	    ret = _krb5_PKCS5_PBKDF2(context, CKSUMTYPE_SHA1, password, salt, 
				     keys[i].iterations - 1,
				     keys[i].enctype,
				     &key);
	    if (ret) {
		krb5_warn(context, ret, "%d: krb5_PKCS5_PBKDF2", i);
		val = 1;
		continue;
	    }
	    
	    if (key.keyvalue.length != keys[i].keylen) {
		krb5_warnx(context, "%d: size key pbkdf2", i);
		val = 1;
		continue;
	    }

	    if (memcmp(key.keyvalue.data, keys[i].pbkdf2, keys[i].keylen) != 0) {
		krb5_warnx(context, "%d: key pbkdf2 pl %d", 
			   i, password.length);
		val = 1;
		continue;
	    }

	    if (verbose) {
		printf("PBKDF2:\n");
		hex_dump_data(&key.keyvalue);
	    }
	    
	    krb5_free_keyblock_contents(context, &key);
	}
#endif

	ret = krb5_string_to_key_data_salt_opaque (context, keys[i].enctype,
						   password, salt, opaque, 
						   &key);
	if (ret) {
	    krb5_warn(context, ret, "%d: string_to_key_data_salt_opaque", i);
	    val = 1;
	    continue;
	}

	if (key.keyvalue.length != keys[i].keylen) {
	    krb5_warnx(context, "%d: key wrong length (%d/%d)",
		       i, key.keyvalue.length, keys[i].keylen);
	    val = 1;
	    continue;
	}

	if (memcmp(key.keyvalue.data, keys[i].key, keys[i].keylen) != 0) {
	    krb5_warnx(context, "%d: key wrong", i);
	    val = 1;
	    continue;
	}
	
	if (verbose) {
	    printf("key:\n");
	    hex_dump_data(&key.keyvalue);
	}
	krb5_free_keyblock_contents(context, &key);
    }
    return val;
}

#ifdef ENABLE_AES

struct enc_test {
    size_t len;
    char *input;
    char *output;
    char *nextiv;
};

struct enc_test encs1[] = {
    {
	17,
	"\x49\x20\x77\x6f\x75\x6c\x64\x20\x6c\x69\x6b\x65\x20\x74\x68\x65"
	"\x20",
	"\xc6\x35\x35\x68\xf2\xbf\x8c\xb4\xd8\xa5\x80\x36\x2d\xa7\xff\x7f"
	"\x97",
	"\xc6\x35\x35\x68\xf2\xbf\x8c\xb4\xd8\xa5\x80\x36\x2d\xa7\xff\x7f"
    },
    {
	31,
	"\x49\x20\x77\x6f\x75\x6c\x64\x20\x6c\x69\x6b\x65\x20\x74\x68\x65"
	"\x20\x47\x65\x6e\x65\x72\x61\x6c\x20\x47\x61\x75\x27\x73\x20",
	"\xfc\x00\x78\x3e\x0e\xfd\xb2\xc1\xd4\x45\xd4\xc8\xef\xf7\xed\x22"
	"\x97\x68\x72\x68\xd6\xec\xcc\xc0\xc0\x7b\x25\xe2\x5e\xcf\xe5",
	"\xfc\x00\x78\x3e\x0e\xfd\xb2\xc1\xd4\x45\xd4\xc8\xef\xf7\xed\x22"
    },
    {
	32,
	"\x49\x20\x77\x6f\x75\x6c\x64\x20\x6c\x69\x6b\x65\x20\x74\x68\x65"
	"\x20\x47\x65\x6e\x65\x72\x61\x6c\x20\x47\x61\x75\x27\x73\x20\x43",
	"\x39\x31\x25\x23\xa7\x86\x62\xd5\xbe\x7f\xcb\xcc\x98\xeb\xf5\xa8"
	"\x97\x68\x72\x68\xd6\xec\xcc\xc0\xc0\x7b\x25\xe2\x5e\xcf\xe5\x84",
	"\x39\x31\x25\x23\xa7\x86\x62\xd5\xbe\x7f\xcb\xcc\x98\xeb\xf5\xa8"
    },
    {
	47,
	"\x49\x20\x77\x6f\x75\x6c\x64\x20\x6c\x69\x6b\x65\x20\x74\x68\x65"
	"\x20\x47\x65\x6e\x65\x72\x61\x6c\x20\x47\x61\x75\x27\x73\x20\x43"
	"\x68\x69\x63\x6b\x65\x6e\x2c\x20\x70\x6c\x65\x61\x73\x65\x2c",
	"\x97\x68\x72\x68\xd6\xec\xcc\xc0\xc0\x7b\x25\xe2\x5e\xcf\xe5\x84"
	"\xb3\xff\xfd\x94\x0c\x16\xa1\x8c\x1b\x55\x49\xd2\xf8\x38\x02\x9e"
	"\x39\x31\x25\x23\xa7\x86\x62\xd5\xbe\x7f\xcb\xcc\x98\xeb\xf5",
	"\xb3\xff\xfd\x94\x0c\x16\xa1\x8c\x1b\x55\x49\xd2\xf8\x38\x02\x9e"
    },
    {
	48,
	"\x49\x20\x77\x6f\x75\x6c\x64\x20\x6c\x69\x6b\x65\x20\x74\x68\x65"
	"\x20\x47\x65\x6e\x65\x72\x61\x6c\x20\x47\x61\x75\x27\x73\x20\x43"
	"\x68\x69\x63\x6b\x65\x6e\x2c\x20\x70\x6c\x65\x61\x73\x65\x2c\x20",
	"\x97\x68\x72\x68\xd6\xec\xcc\xc0\xc0\x7b\x25\xe2\x5e\xcf\xe5\x84"
	"\x9d\xad\x8b\xbb\x96\xc4\xcd\xc0\x3b\xc1\x03\xe1\xa1\x94\xbb\xd8"
	"\x39\x31\x25\x23\xa7\x86\x62\xd5\xbe\x7f\xcb\xcc\x98\xeb\xf5\xa8",
	"\x9d\xad\x8b\xbb\x96\xc4\xcd\xc0\x3b\xc1\x03\xe1\xa1\x94\xbb\xd8"
    },
    {
	64,
	"\x49\x20\x77\x6f\x75\x6c\x64\x20\x6c\x69\x6b\x65\x20\x74\x68\x65"
	"\x20\x47\x65\x6e\x65\x72\x61\x6c\x20\x47\x61\x75\x27\x73\x20\x43"
	"\x68\x69\x63\x6b\x65\x6e\x2c\x20\x70\x6c\x65\x61\x73\x65\x2c\x20"
	"\x61\x6e\x64\x20\x77\x6f\x6e\x74\x6f\x6e\x20\x73\x6f\x75\x70\x2e",
	"\x97\x68\x72\x68\xd6\xec\xcc\xc0\xc0\x7b\x25\xe2\x5e\xcf\xe5\x84"
	"\x39\x31\x25\x23\xa7\x86\x62\xd5\xbe\x7f\xcb\xcc\x98\xeb\xf5\xa8"
	"\x48\x07\xef\xe8\x36\xee\x89\xa5\x26\x73\x0d\xbc\x2f\x7b\xc8\x40"
	"\x9d\xad\x8b\xbb\x96\xc4\xcd\xc0\x3b\xc1\x03\xe1\xa1\x94\xbb\xd8",
	"\x48\x07\xef\xe8\x36\xee\x89\xa5\x26\x73\x0d\xbc\x2f\x7b\xc8\x40"
    }
};
	

struct enc_test encs2[] = {
    {
	17,
	"\x49\x20\x77\x6f\x75\x6c\x64\x20\x6c\x69\x6b\x65\x20\x74\x68\x65"
	"\x20",
	"\x5c\x13\x26\x27\xc4\xcb\xca\x04\x14\x43\x8a\xb5\x97\x97\x7c\x10"
	"\x16"
    },
    {
	31,
	"\x49\x20\x77\x6f\x75\x6c\x64\x20\x6c\x69\x6b\x65\x20\x74\x68\x65"
	"\x20\x47\x65\x6e\x65\x72\x61\x6c\x20\x47\x61\x75\x27\x73\x20",
	"\x16\xb3\xd8\xe5\xcd\x93\xe6\x2c\x28\x70\xa0\x36\x6e\x9a\xb9\x74"
	"\x16\xc1\xee\xdf\x39\xc8\x3f\xfb\xc5\xf6\x72\xe9\xc1\x6e\x53"
    },
    {
	32,
	"\x49\x20\x77\x6f\x75\x6c\x64\x20\x6c\x69\x6b\x65\x20\x74\x68\x65"
	"\x20\x47\x65\x6e\x65\x72\x61\x6c\x20\x47\x61\x75\x27\x73\x20\x43",
	"\x69\xde\xce\x59\x83\x6a\x82\xe1\xcd\x21\x93\xd0\x9e\x2a\xff\xc8"
	"\x16\xc1\xee\xdf\x39\xc8\x3f\xfb\xc5\xf6\x72\xe9\xc1\x6e\x53\x0c"
    },
    {
	47,
	"\x49\x20\x77\x6f\x75\x6c\x64\x20\x6c\x69\x6b\x65\x20\x74\x68\x65"
	"\x20\x47\x65\x6e\x65\x72\x61\x6c\x20\x47\x61\x75\x27\x73\x20\x43"
	"\x68\x69\x63\x6b\x65\x6e\x2c\x20\x70\x6c\x65\x61\x73\x65\x2c",
	"\x16\xc1\xee\xdf\x39\xc8\x3f\xfb\xc5\xf6\x72\xe9\xc1\x6e\x53\x0c"
	"\xe5\x56\xb4\x88\x41\xb9\xde\x27\xf0\x07\xa1\x6e\x89\x94\x47\xf1"
	"\x69\xde\xce\x59\x83\x6a\x82\xe1\xcd\x21\x93\xd0\x9e\x2a\xff"
    },
    {
	48,
	"\x49\x20\x77\x6f\x75\x6c\x64\x20\x6c\x69\x6b\x65\x20\x74\x68\x65"
	"\x20\x47\x65\x6e\x65\x72\x61\x6c\x20\x47\x61\x75\x27\x73\x20\x43"
	"\x68\x69\x63\x6b\x65\x6e\x2c\x20\x70\x6c\x65\x61\x73\x65\x2c\x20",
	"\x16\xc1\xee\xdf\x39\xc8\x3f\xfb\xc5\xf6\x72\xe9\xc1\x6e\x53\x0c"
	"\xfd\x68\xd1\x56\x32\x23\x7b\xfa\xb0\x09\x86\x3b\x17\x53\xfa\x30"
	"\x69\xde\xce\x59\x83\x6a\x82\xe1\xcd\x21\x93\xd0\x9e\x2a\xff\xc8"
    },
    {
	64,
	"\x49\x20\x77\x6f\x75\x6c\x64\x20\x6c\x69\x6b\x65\x20\x74\x68\x65"
	"\x20\x47\x65\x6e\x65\x72\x61\x6c\x20\x47\x61\x75\x27\x73\x20\x43"
	"\x68\x69\x63\x6b\x65\x6e\x2c\x20\x70\x6c\x65\x61\x73\x65\x2c\x20"
	"\x61\x6e\x64\x20\x77\x6f\x6e\x74\x6f\x6e\x20\x73\x6f\x75\x70\x2e",
	"\x16\xc1\xee\xdf\x39\xc8\x3f\xfb\xc5\xf6\x72\xe9\xc1\x6e\x53\x0c"
	"\x69\xde\xce\x59\x83\x6a\x82\xe1\xcd\x21\x93\xd0\x9e\x2a\xff\xc8"
	"\x70\x29\xf2\x6f\x7c\x79\xc1\x77\x91\xad\x94\xb0\x78\x62\x27\x67"
	"\xfd\x68\xd1\x56\x32\x23\x7b\xfa\xb0\x09\x86\x3b\x17\x53\xfa\x30"
    },
    {
	78,
	"\x49\x20\x77\x6f\x75\x6c\x64\x20\x6c\x69\x6b\x65\x20\x74\x68\x65"
	"\x20\x47\x65\x6e\x65\x72\x61\x6c\x20\x47\x61\x75\x27\x73\x20\x43"
	"\x68\x69\x63\x6b\x65\x6e\x2c\x20\x70\x6c\x65\x61\x73\x65\x2c\x20"
	"\x61\x6e\x64\x20\x77\x6f\x6e\x74\x6f\x6e\x20\x73\x6f\x75\x70\x2e"
	"\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41",
	"\x16\xc1\xee\xdf\x39\xc8\x3f\xfb\xc5\xf6\x72\xe9\xc1\x6e\x53\x0c"
	"\x69\xde\xce\x59\x83\x6a\x82\xe1\xcd\x21\x93\xd0\x9e\x2a\xff\xc8"
	"\xfd\x68\xd1\x56\x32\x23\x7b\xfa\xb0\x09\x86\x3b\x17\x53\xfa\x30"
	"\x73\xfb\x2c\x36\x76\xaf\xcf\x31\xff\xe3\x8a\x89\x0c\x7e\x99\x3f"
	"\x70\x29\xf2\x6f\x7c\x79\xc1\x77\x91\xad\x94\xb0\x78\x62"
    },
    {
	83,
	"\x49\x20\x77\x6f\x75\x6c\x64\x20\x6c\x69\x6b\x65\x20\x74\x68\x65"
	"\x20\x47\x65\x6e\x65\x72\x61\x6c\x20\x47\x61\x75\x27\x73\x20\x43"
	"\x68\x69\x63\x6b\x65\x6e\x2c\x20\x70\x6c\x65\x61\x73\x65\x2c\x20"
	"\x61\x6e\x64\x20\x77\x6f\x6e\x74\x6f\x6e\x20\x73\x6f\x75\x70\x2e"
	"\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41"
	"\x41\x41\x41",
	"\x16\xc1\xee\xdf\x39\xc8\x3f\xfb\xc5\xf6\x72\xe9\xc1\x6e\x53\x0c"
	"\x69\xde\xce\x59\x83\x6a\x82\xe1\xcd\x21\x93\xd0\x9e\x2a\xff\xc8"
	"\xfd\x68\xd1\x56\x32\x23\x7b\xfa\xb0\x09\x86\x3b\x17\x53\xfa\x30"
	"\x70\x29\xf2\x6f\x7c\x79\xc1\x77\x91\xad\x94\xb0\x78\x62\x27\x67"
	"\x65\x39\x3a\xdb\x92\x05\x4d\x4f\x08\xa1\xfa\x59\xda\x56\x58\x0e"
	"\x3b\xac\x12"
    },
    {
	92,
	"\x49\x20\x77\x6f\x75\x6c\x64\x20\x6c\x69\x6b\x65\x20\x74\x68\x65"
	"\x20\x47\x65\x6e\x65\x72\x61\x6c\x20\x47\x61\x75\x27\x73\x20\x43"
	"\x68\x69\x63\x6b\x65\x6e\x2c\x20\x70\x6c\x65\x61\x73\x65\x2c\x20"
	"\x61\x6e\x64\x20\x77\x6f\x6e\x74\x6f\x6e\x20\x73\x6f\x75\x70\x2e"
	"\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41"
	"\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41",
	"\x16\xc1\xee\xdf\x39\xc8\x3f\xfb\xc5\xf6\x72\xe9\xc1\x6e\x53\x0c"
	"\x69\xde\xce\x59\x83\x6a\x82\xe1\xcd\x21\x93\xd0\x9e\x2a\xff\xc8"
	"\xfd\x68\xd1\x56\x32\x23\x7b\xfa\xb0\x09\x86\x3b\x17\x53\xfa\x30"
	"\x70\x29\xf2\x6f\x7c\x79\xc1\x77\x91\xad\x94\xb0\x78\x62\x27\x67"
	"\x0c\xff\xd7\x63\x50\xf8\x4e\xf9\xec\x56\x1c\x79\xc5\xc8\xfe\x50"
	"\x3b\xac\x12\x6e\xd3\x2d\x02\xc4\xe5\x06\x43\x5f"
    },
    {
	96,
	"\x49\x20\x77\x6f\x75\x6c\x64\x20\x6c\x69\x6b\x65\x20\x74\x68\x65"
	"\x20\x47\x65\x6e\x65\x72\x61\x6c\x20\x47\x61\x75\x27\x73\x20\x43"
	"\x68\x69\x63\x6b\x65\x6e\x2c\x20\x70\x6c\x65\x61\x73\x65\x2c\x20"
	"\x61\x6e\x64\x20\x77\x6f\x6e\x74\x6f\x6e\x20\x73\x6f\x75\x70\x2e"
	"\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41"
	"\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41",
	"\x16\xc1\xee\xdf\x39\xc8\x3f\xfb\xc5\xf6\x72\xe9\xc1\x6e\x53\x0c"
	"\x69\xde\xce\x59\x83\x6a\x82\xe1\xcd\x21\x93\xd0\x9e\x2a\xff\xc8"
	"\xfd\x68\xd1\x56\x32\x23\x7b\xfa\xb0\x09\x86\x3b\x17\x53\xfa\x30"
	"\x70\x29\xf2\x6f\x7c\x79\xc1\x77\x91\xad\x94\xb0\x78\x62\x27\x67"
	"\x08\x28\x49\xad\xfc\x2d\x8e\x86\xae\x69\xa5\xa8\xd9\x29\x9e\xe4"
	"\x3b\xac\x12\x6e\xd3\x2d\x02\xc4\xe5\x06\x43\x5f\x4c\x41\xd1\xb8"
    }
};



char *aes_key1 = 
	"\x63\x68\x69\x63\x6b\x65\x6e\x20\x74\x65\x72\x69\x79\x61\x6b\x69";

char *aes_key2 =
	"\x63\x68\x69\x63\x6b\x65\x6e\x20\x74\x65\x72\x69\x79\x61\x6b\x69"
	"\x2c\x20\x79\x75\x6d\x6d\x79\x20\x79\x75\x6d\x6d\x79\x21\x21\x21";


static int
samep(int testn, char *type, const char *p1, const char *p2, size_t len)
{
    size_t i;
    int val = 1;

    for (i = 0; i < len; i++) {
	if (p1[i] != p2[i]) {
	    if (verbose)
		printf("M");
	    val = 0;
	} else {
	    if (verbose)
		printf(".");
	}
    }
    if (verbose)
	printf("\n");
    return val;
}

static int
encryption_test(krb5_context context, char *key, int keylen,
		struct enc_test *enc, int numenc)
{
    char iv[AES_BLOCK_SIZE];
    int i, val, failed = 0;
    AES_KEY ekey, dkey;
    char *p;

    AES_set_encrypt_key(key, keylen, &ekey);
    AES_set_decrypt_key(key, keylen, &dkey);

    for (i = 0; i < numenc; i++) {
	val = 0;

	if (verbose)
	    printf("test: %d\n", i);
	memset(iv, 0, sizeof(iv));

	p = malloc(enc[i].len + 1);
	if (p == NULL)
	    krb5_errx(context, 1, "malloc");

	p[enc[i].len] = '\0';

	memcpy(p, enc[i].input, enc[i].len);

	_krb5_aes_cts_encrypt(p, p, enc[i].len,
			      &ekey, iv, AES_ENCRYPT);

	if (p[enc[i].len] != '\0') {
	    krb5_warnx(context, "%d: encrypt modified off end", i);
	    val = 1;
	}

	if (!samep(i, "cipher", p, enc[i].output, enc[i].len)) {
	    krb5_warnx(context, "%d: cipher", i);
	    val = 1;
	}

	if (enc[i].nextiv && !samep(i, "iv", iv, enc[i].nextiv, 16)){ /*XXX*/ 
	    krb5_warnx(context, "%d: iv", i);
	    val = 1;
	}

	memset(iv, 0, sizeof(iv));

	_krb5_aes_cts_encrypt(p, p, enc[i].len,
			      &dkey, iv, AES_DECRYPT);

	if (p[enc[i].len] != '\0') {
	    krb5_warnx(context, "%d: decrypt modified off end", i);
	    val = 1;
	}

	if (!samep(i, "clear", p, enc[i].input, enc[i].len))
	    val = 1;

	if (enc[i].nextiv && !samep(i, "iv", iv, enc[i].nextiv, 16)){ /*XXX*/ 
	    krb5_warnx(context, "%d: iv", i);
	    val = 1;
	}

	free(p);

	if (val) {
	    printf("test %d failed\n", i);
	    failed = 1;
	}
	val = 0;
    }
    return failed;
}

#endif /* ENABLE_AES */


static int
krb_enc(krb5_context context,
	krb5_crypto crypto, 
	unsigned usage,
	krb5_data *cipher, 
	krb5_data *clear)
{
    krb5_data decrypt;
    krb5_error_code ret;

    krb5_data_zero(&decrypt);

    ret = krb5_decrypt(context,
		       crypto,
		       usage,
		       cipher->data,
		       cipher->length,
		       &decrypt);

    if (ret) {
	krb5_warn(context, ret, "krb5_decrypt");
	return ret;
    }

    if (decrypt.length != clear->length ||
	memcmp(decrypt.data, clear->data, decrypt.length) != 0) {
	krb5_warnx(context, "clear text not same");
	return EINVAL;
    }

    krb5_data_free(&decrypt);

    return 0;
}

static int
krb_enc_mit(krb5_context context,
	    krb5_enctype enctype,
	    krb5_keyblock *key,
	    unsigned usage,
	    krb5_data *cipher, 
	    krb5_data *clear)
{
    krb5_error_code ret;
    krb5_enc_data e;
    krb5_data decrypt;
    size_t len;

    e.kvno = 0;
    e.enctype = enctype;
    e.ciphertext = *cipher;

    ret = krb5_c_decrypt(context, *key, usage, NULL, &e, &decrypt);
    if (ret)
	return ret;

    if (decrypt.length != clear->length ||
	memcmp(decrypt.data, clear->data, decrypt.length) != 0) {
	krb5_warnx(context, "clear text not same");
	return EINVAL;
    }

    krb5_data_free(&decrypt);

    ret = krb5_c_encrypt_length(context, enctype, clear->length, &len);
    if (ret)
	return ret;

    if (len != cipher->length) {
	krb5_warnx(context, "c_encrypt_length wrong %d != %d",
		   len, cipher->length);
	return EINVAL;
    }

    return 0;
}


struct {
    krb5_enctype enctype;
    unsigned usage;
    size_t keylen;
    void *key;
    size_t elen;
    void* edata;
    size_t plen;
    void *pdata;
} krbencs[] =  {
#ifdef ENABLE_AES
    { 
	ETYPE_AES256_CTS_HMAC_SHA1_96,
	7,
	32,   
	"\x47\x75\x69\x64\x65\x6c\x69\x6e\x65\x73\x20\x74\x6f\x20\x41\x75"
	"\x74\x68\x6f\x72\x73\x20\x6f\x66\x20\x49\x6e\x74\x65\x72\x6e\x65",
	44,
	"\xcf\x79\x8f\x0d\x76\xf3\xe0\xbe\x8e\x66\x94\x70\xfa\xcc\x9e\x91"
	"\xa9\xec\x1c\x5c\x21\xfb\x6e\xef\x1a\x7a\xc8\xc1\xcc\x5a\x95\x24"
	"\x6f\x9f\xf4\xd5\xbe\x5d\x59\x97\x44\xd8\x47\xcd",
	16,
	"\x54\x68\x69\x73\x20\x69\x73\x20\x61\x20\x74\x65\x73\x74\x2e\x0a"
    }
#endif
};


static int
krb_enc_test(krb5_context context)
{
    krb5_error_code ret;
    krb5_crypto crypto;
    krb5_keyblock kb;
    krb5_data cipher, plain;
    int i, failed = 0;

    for (i = 0; i < sizeof(krbencs)/sizeof(krbencs[0]); i++) {

	ret = krb5_enctype_to_keytype(context, 
				      krbencs[i].enctype,
				      (krb5_keytype *)&kb.keytype);
	if (ret)
	    return ret;

	kb.keyvalue.length = krbencs[i].keylen;
	kb.keyvalue.data = krbencs[i].key;

	ret = krb5_crypto_init(context, &kb, krbencs[i].enctype, &crypto);

	cipher.length = krbencs[i].elen;
	cipher.data = krbencs[i].edata;
	plain.length = krbencs[i].plen;
	plain.data = krbencs[i].pdata;
	
	ret = krb_enc(context, crypto, krbencs[i].usage, &cipher, &plain);
		       
	if (ret) {
	    failed = 1;
	    printf("krb_enc failed with %d\n", ret);
	}
	krb5_crypto_destroy(context, crypto);

	ret = krb_enc_mit(context, krbencs[i].enctype, &kb, 
			  krbencs[i].usage, &cipher, &plain);
	if (ret) {
	    failed = 1;
	    printf("krb_enc_mit failed with %d\n", ret);
	}

    }

    return failed;
}


static int
random_to_key(krb5_context context)
{
    krb5_error_code ret;
    krb5_keyblock key;

    ret = krb5_random_to_key(context,
			     ETYPE_DES3_CBC_SHA1,
			     "\x21\x39\x04\x58\x6A\xBD\x7F"
			     "\x21\x39\x04\x58\x6A\xBD\x7F"
			     "\x21\x39\x04\x58\x6A\xBD\x7F",
			     21,
			     &key);
    if (ret){
	krb5_warn(context, ret, "random_to_key");
	return 1;
    }
    if (key.keyvalue.length != 24)
	return 1;

    if (memcmp(key.keyvalue.data,
	       "\x20\x38\x04\x58\x6b\xbc\x7f\xc7"
	       "\x20\x38\x04\x58\x6b\xbc\x7f\xc7"
	       "\x20\x38\x04\x58\x6b\xbc\x7f\xc7",
	       24) != 0)
	return 1;

    krb5_free_keyblock_contents(context, &key);

    return 0;
}


int
main(int argc, char **argv)
{
    krb5_error_code ret;
    krb5_context context;
    int val = 0;
    
    ret = krb5_init_context (&context);
    if (ret)
	errx (1, "krb5_init_context failed: %d", ret);

    val |= string_to_key_test(context);

#ifdef ENABLE_AES
    val |= encryption_test(context, aes_key1, 128,
			   encs1, sizeof(encs1)/sizeof(encs1[0]));
    val |= encryption_test(context, aes_key2, 256, 
			   encs2, sizeof(encs2)/sizeof(encs2[0]));
#endif
    val |= krb_enc_test(context);
    val |= random_to_key(context);

    if (verbose && val == 0)
	printf("all ok\n");
    if (val)
	printf("tests failed\n");

    krb5_free_context(context);

    return val;
}
