#ifndef GPT_H
#define GPT_H
#include <stdint.h>
#include "be.h"

#pragma pack(1)

#define MPT_GPT_FAKE_TYPE 0xEE

struct MBRPartition
{
	uint8_t status;
	uint8_t chsFirst[3];
	uint8_t type;
	uint8_t chsLast[3];
	le_jief<uint32_t> lbaFirst;
	le_jief<uint32_t> numSectors;
};

#define MBR_SIGNATURE 0x55AA

struct ProtectiveMBR
{
	uint8_t unused[446];
	MBRPartition partitions[4];
	be<uint16_t> signature;
};

#define GPT_SIGNATURE "EFI PART"

struct GPTHeader
{
	char signature[8];
	// TODO
};

struct GPT_GUID
{
	le_jief<uint32_t> data1;
	le_jief<uint16_t> data2, data3;
	le_jief<uint8_t> data4[8];
};

struct GPTPartition
{
	GPT_GUID typeGUID;
	GPT_GUID partitionGUID;
	le_jief<uint64_t> firstLBA, lastLBA;
	le_jief<uint64_t> flags;
	uint16_t name[36];
};

#define GUID_EMPTY "00000000-0000-0000-0000-000000000000"
#define GUID_HFSPLUS "48465300-0000-11AA-AA11-00306543ECAC"

#pragma pack()

#endif
