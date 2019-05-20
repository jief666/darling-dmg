/*
 *  Created by jief on 28/08/2018.
 *  Copyright © 2018 jf-luce.
 *  This program is free software; you can redistribute it
 *    and/or modify it under the terms of the GNU General Public License version 3.0 as published by the Free Software Foundation.
 */

#ifndef __DARLING_DMG__CRYPTO_H__
#define __DARLING_DMG__CRYPTO_H__

#include <stdio.h>
#include <stdint.h>

#include <string>
#include <vector>
//#include "crypto/TripleDes.h"

//#ifdef __cplusplus
//extern "C" {
//#endif

int DarlingDMGCrypto_PKCS5_PBKDF2_HMAC_SHA1(const char *pass, int passlen,
                           const unsigned char *salt, int saltlen, int iter,
                           int keylen, unsigned char *out);

void DarlingDMGCrypto_DES_CBC(const unsigned char *key, const unsigned char *iv,
			   unsigned char *out,
			   const unsigned char *in, int inl);

unsigned char* DarlingDMGCrypto_HMAC(const uint8_t *key, int key_len,
                        const unsigned char *d, size_t n,
                        unsigned char *md, unsigned int *md_len);

int DarlingDMGCrypto_set_aes_decrypt_key(const unsigned char *userKey, const int bits, void **key);
void DarlingDMGCrypto_aes_cbc_decrypt(const unsigned char *in, unsigned char *out,
                            size_t length, const void *key,
							unsigned char *ivec);

bool base64Decode(const std::string& input, std::vector<uint8_t>& output);

//#ifdef __cplusplus
//}
//#endif

#endif /* __DARLING_DMG__CRYPTO_H__ */
