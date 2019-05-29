#ifndef RSRC_H
#define RSRC_H
#include <stdint.h>
#include "be.h"

// Resource fork structure declarations
// Big Endian

#pragma pack(1)


struct HFSResourceForkHeader
{
	be<uint32_t> dataOffset;
	be<uint32_t> mapOffset; // offset to HFSResourceMapHeader
	be<uint32_t> dataLength;
	be<uint32_t> mapLength;
};

struct HFSResourceHeader
{
	be<uint32_t> length;
	uint8_t data[];
};

struct HFSResourceMapHeader
{
	be<uint32_t> dataOffset;
	be<uint32_t> mapOffset;
	be<uint32_t> dataLength;
	be<uint32_t> mapLength;
	be<uint32_t> reserved2;
	be<uint16_t> reserved3;
	be<uint16_t> attributes;
	be<uint16_t> listOffset; // offset to HFSResourceList from the start of HFSResourceMapHeader
};

struct HFSResourceListItem
{
	be<uint32_t> type; // fourcc
	be<uint16_t> count; // contains count - 1
	be<uint16_t> offset; // offset to HFSResourcePointer from this list item
};

struct HFSResourceList
{
	be<uint16_t> count; // contains count - 1
	HFSResourceListItem items[];
};

struct HFSResourcePointer
{
	be<uint16_t> resourceId; // 0xffff for cmpfs
	be<uint16_t> offsetName;
	be<uint32_t> dataOffset; // offset to HFSResourceHeader from added to HFSResourceForkHeader::dataOffset
	be<uint16_t> reserved;
};

#pragma pack()

#endif
