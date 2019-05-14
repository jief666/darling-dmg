/*
Copyright (C) 2018 Jief Luce
Copyright (C) 2017 Simon Gander

This file is originally based on the vfdecrypt sources.
This file is also based on the work of Simon Gander (https://github.com/sgan81/apfs-fuse)

You should have received a copy of the GNU General Public License
along with hdimount.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <memory> // for shared_ptr
#include "DarlingDMGCrypto.h"

#include "Reader.h"

#define HMACSHA1_KEY_SIZE 20 // from v2 header

class EncryptReader : public Reader
{
public:
	static bool isEncrypted(std::shared_ptr<Reader> reader);

	EncryptReader(std::shared_ptr<Reader> reader, const char* password);
	~EncryptReader();
	
	virtual int32_t read(void* buf, int32_t count, uint64_t offset);
	virtual uint64_t length();

private:
	void compute_iv(uint32_t chunk_no, uint8_t *iv);
	void decrypt_chunk(void *crypted_buffer, void* outputBuffer, uint32_t chunk_no);
	bool SetupEncryptionV2(const char* password);

	std::shared_ptr<Reader> m_reader;

	uint64_t m_crypt_offset;
	uint64_t m_crypt_size;
	int32_t m_crypt_blocksize;

    uint8_t m_hmacsha1_key[HMACSHA1_KEY_SIZE];
	void* m_aes_decrypt_key;
};
