#ifndef APM_H
#define APM_H
#include <stdint.h>
#include "be.h"

#pragma pack(1)

static const uint16_t BLOCK0_SIGNATURE = 0x4552;
static const uint16_t DPME_SIGNATURE = 0x504D;

struct DDMap
{
    be<uint32_t>  ddBlock;
    be<uint16_t>  ddSize;
    be<uint16_t>  ddType;
};

struct DPME
{
    be<uint16_t>  dpme_signature;
    be<uint16_t>  dpme_reserved_1;
    be<uint32_t>  dpme_map_entries;
    be<uint32_t>  dpme_pblock_start;
    be<uint32_t>  dpme_pblocks;
    char      dpme_name[32];
    char      dpme_type[32];
    be<uint32_t>  dpme_lblock_start;
    be<uint32_t>  dpme_lblocks;
    be<uint32_t>  dpme_flags;
    be<uint32_t>  dpme_boot_block;
    be<uint32_t>  dpme_boot_bytes;
    be<uint32_t>  dpme_load_addr;
    be<uint32_t>  dpme_load_addr_2;
    be<uint32_t>  dpme_goto_addr;
    be<uint32_t>  dpme_goto_addr_2;
    be<uint32_t>  dpme_checksum;
    be<uint8_t>   dpme_process_id[16];
    be<uint32_t>  dpme_reserved_2[32];
    be<uint32_t>  dpme_reserved_3[62];
};

struct Block0
{
    be<uint16_t>  sbSig;
    be<uint16_t>  sbBlkSize;
    be<uint32_t>  sbBlkCount;
    be<uint16_t>  sbDevType;
    be<uint16_t>  sbDevId;
    be<uint32_t>  sbDrvrData;
    be<uint16_t>  sbDrvrCount;
    DDMap     sbDrvrMap[8];
    uint8_t   sbReserved[430];
};

#pragma pack()

#endif
