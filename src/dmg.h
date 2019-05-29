#ifndef DMG_H
#define DMG_H
#include "be.h"
#pragma pack(1)
#define UDIF_SIGNATURE 0x6B6F6C79 // aka koly

enum
{
	kUDIFFlagsFlattened = 1
};

enum
{
	kUDIFDeviceImageType = 1,
	kUDIFPartitionImageType = 2
};

struct UDIFChecksum
{
	be<uint32_t> type;
	be<uint32_t> size;
	be<uint32_t> data[0x20];
};

struct UDIFID
{
	be<uint32_t> data1; /* smallest */
	be<uint32_t> data2;
	be<uint32_t> data3;
	be<uint32_t> data4; /* largest */
};

struct UDIFResourceFile
{
	be<uint32_t> fUDIFSignature;
	be<uint32_t> fUDIFVersion;
	be<uint32_t> fUDIFHeaderSize;
	be<uint32_t> fUDIFFlags;
	
	be<uint64_t> fUDIFRunningDataForkOffset;
	be<uint64_t> fUDIFDataForkOffset;
	be<uint64_t> fUDIFDataForkLength;
	be<uint64_t> fUDIFRsrcForkOffset;
	be<uint64_t> fUDIFRsrcForkLength;
	
	be<uint32_t> fUDIFSegmentNumber;
	be<uint32_t> fUDIFSegmentCount;
	UDIFID fUDIFSegmentID;  /* a 128-bit number like a GUID, but does not seem to be a OSF GUID, since it doesn't have the proper versioning byte */
	
	UDIFChecksum fUDIFDataForkChecksum;
	
	be<uint64_t> fUDIFXMLOffset;
	be<uint64_t> fUDIFXMLLength;
	
	uint8_t reserved1[0x78]; /* this is actually the perfect amount of space to store every thing in this struct until the checksum */
	
	UDIFChecksum fUDIFMasterChecksum;
	
	be<uint32_t> fUDIFImageVariant;
	be<uint64_t> fUDIFSectorCount;
	
	be<uint32_t> reserved2;
	be<uint32_t> reserved3;
	be<uint32_t> reserved4;
	
};

struct BLKXRun
{
	be<uint32_t> type;
	be<uint32_t> reserved;
	be<uint64_t> sectorStart;
	be<uint64_t> sectorCount;
	be<uint64_t> compOffset;
	be<uint64_t> compLength;
};

enum class RunType : uint32_t
{
	ZeroFill = 0,
	Raw = 1,
	Unknown = 2,
	ADC = 0x80000004,
	Zlib = 0x80000005,
	Bzip2 = 0x80000006,
	LZFSE = 0x80000007,
	Comment = 0x7ffffffe,
	Terminator = 0xffffffff
};

struct SizeResource
{
	uint16_t version; /* set to 5 */
	be<uint32_t> isHFS; /* first dword of v53(ImageInfoRec): Set to 1 if it's a HFS or HFS+ partition -- duh. */
	be<uint32_t> unknown1; /* second dword of v53: seems to be garbage if it's HFS+, stuff related to HFS embedded if it's that*/
	uint8_t dataLen; /* length of data that proceeds, comes right before the data in ImageInfoRec. Always set to 0 for HFS, HFS+ */
	uint8_t data[255]; /* other data from v53, dataLen + 1 bytes, the rest NULL filled... a string? Not set for HFS, HFS+ */
	be<uint32_t> unknown2; /* 8 bytes before volumeModified in v53, seems to be always set to 0 for HFS, HFS+  */
	be<uint32_t> unknown3; /* 4 bytes before volumeModified in v53, seems to be always set to 0 for HFS, HFS+ */
	be<uint32_t> volumeModified; /* offset 272 in v53 */
	be<uint32_t> unknown4; /* always seems to be 0 for UDIF */
	uint16_t volumeSignature; /* HX in our case */
	uint16_t sizePresent; /* always set to 1 */
};

struct CSumResource
{
	uint16_t version; /* set to 1 */
	be<uint32_t> type; /* set to 0x2 for MKBlockChecksum */
	be<uint32_t> checksum;
};

#define DDM_DESCRIPTOR 0xFFFFFFFF
#define ENTIRE_DEVICE_DESCRIPTOR 0xFFFFFFFE

struct BLKXTable
{
	be<uint32_t> fUDIFBlocksSignature;
	be<uint32_t> infoVersion;
	be<uint64_t> firstSectorNumber;
	be<uint64_t> sectorCount;
	
	be<uint64_t> dataStart;
	be<uint32_t> decompressBufferRequested;
	be<uint32_t> blocksDescriptor;
	
	be<uint32_t> reserved1;
	be<uint32_t> reserved2;
	be<uint32_t> reserved3;
	be<uint32_t> reserved4;
	be<uint32_t> reserved5;
	be<uint32_t> reserved6;
	
	UDIFChecksum checksum;
	
	be<uint32_t> blocksRunCount;
	BLKXRun runs[0];
};

#pragma pack()

#endif

