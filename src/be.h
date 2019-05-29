#ifndef BENDIAN_H
#define BENDIAN_H
#include <stdint.h>
#ifdef __FreeBSD__
#include <sys/endian.h>
#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#include <TargetConditionals.h>

	#if TARGET_RT_LITTLE_ENDIAN
	#define __BYTE_ORDER __LITTLE_ENDIAN

		#define be16toh(x) OSSwapInt16(x)
		#define be32toh(x) OSSwapInt32(x)
		#define be64toh(x) OSSwapInt64(x)

		#define htobe16(x) OSSwapInt16(x)
		#define htobe32(x) OSSwapInt32(x)
		#define htobe64(x) OSSwapInt64(x)

		#define le16toh(x) (x)
		#define le32toh(x) (x)
		#define le64toh(x) (x)

	#else
	#define __BYTE_ORDER __BIG_ENDIAN

		#define le16toh(x) OSSwapInt16(x)
		#define le32toh(x) OSSwapInt32(x)
		#define le64toh(x) OSSwapInt64(x)

		#define htobe16(x) (x)
		#define htobe32(x) (x)
		#define htobe64(x) (x)

		#define be16toh(x) (x)
		#define be32toh(x) (x)
		#define be64toh(x) (x)

	#endif

#elif defined(_WIN32)
	static uint16_t htobe16(uint16_t x) {
		union { uint16_t u16; uint8_t v[2]; } ret;
		ret.v[0] = (uint8_t)(x >> 8);
		ret.v[1] = (uint8_t)x;
		return ret.u16;
	}

	static uint32_t htobe32(uint32_t x) {
		union { uint32_t u32; uint8_t v[4]; } ret;
		ret.v[0] = (uint8_t)(x >> 24);
		ret.v[1] = (uint8_t)(x >> 16);
		ret.v[2] = (uint8_t)(x >> 8);
		ret.v[3] = (uint8_t)x;
		return ret.u32;
	}

	static uint64_t htobe64(uint64_t x) {
		union { uint64_t u64; uint8_t v[8]; } ret;
		ret.v[0] = (uint8_t)(x >> 56);
		ret.v[1] = (uint8_t)(x >> 48);
		ret.v[2] = (uint8_t)(x >> 40);
		ret.v[3] = (uint8_t)(x >> 32);
		ret.v[4] = (uint8_t)(x >> 24);
		ret.v[5] = (uint8_t)(x >> 16);
		ret.v[6] = (uint8_t)(x >> 8);
		ret.v[7] = (uint8_t)x;
		return ret.u64;
	}

	// windows can be only LE
	#define __BYTE_ORDER __LITTLE_ENDIAN // this define is required in HFSCatalogBTree.cpp

	#define be16toh(x)	htobe16(x)
	#define be32toh(x)	htobe32(x)
	#define be64toh(x)	htobe64(x)

	#define le16toh(x)	x
	#define le32toh(x)	x
	#define le64toh(x)	x

#else
#include <endian.h>
#endif


inline uint16_t bswap_le_to_h(const uint8_t &x) { return x; }
inline uint16_t bswap_le_to_h(const uint16_t &x) { return le16toh(x); }
inline uint32_t bswap_le_to_h(const uint32_t &x) { return le32toh(x); }
inline uint64_t bswap_le_to_h(const uint64_t &x) { return le64toh(x); }

inline uint16_t bswap_be_to_h(const uint8_t &x) { return x; }
inline uint16_t bswap_be_to_h(const uint16_t &x) { return be16toh(x); }
inline uint32_t bswap_be_to_h(const uint32_t &x) { return be32toh(x); }
inline uint64_t bswap_be_to_h(const uint64_t &x) { return be64toh(x); }




// Little-endian template that does automatic swapping if necessary.
template<typename T>
struct le_jief
{
public:
	void operator=(T v) { val = bswap_le_to_h(v); }
	operator T() const { return bswap_le_to_h(val); }
	T get() const { return bswap_le_to_h(val); }
private:
	T val;
};


// Little-endian template that does automatic swapping if necessary.
template<typename T>
struct be
{
public:
	void operator=(T v) { val = bswap_be_to_h(v); }
	operator T() const { return bswap_be_to_h(val); }
	T get() const { return bswap_be_to_h(val); }
private:
	T val;
};


#endif
