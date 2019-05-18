#include "DMGPartition.h"
#include "be.h"
#include <stdexcept>
#include <cstring>
#include "DMGDecompressor.h"
#include <memory>
#include <algorithm>
//#include <cstdio>
#include <iostream>
#include "SubReader.h"
#include "exceptions.h"
#include "../../Utils.h"

static const int SECTOR_SIZE = 512;

DMGPartition::DMGPartition(std::shared_ptr<Reader> disk, BLKXTable* table)
: m_disk(disk)/*, m_table(table)*/
{
    m_tableDataStart = table->dataStart;
    m_tableSectorCount = table->sectorCount;
    
    BLKXRun* runs = (BLKXRun*) (  (uint8_t*)table+sizeof(*table)  );
	for (uint32_t i = 0; i < be(table->blocksRunCount); i++)
	{
        RunType type = RunType(be(runs[i].type));
		if (type == RunType::Comment || type == RunType::Terminator)
			continue;
		
        m_runs.push_back(DMGPartitionRun(type, be(runs[i].sectorStart), be(runs[i].sectorStart)+be(runs[i].sectorCount), be(runs[i].compOffset), be(runs[i].compLength)));
#ifdef DEBUG
        m_runs[m_runs.size()-1].index = i;
#endif


#ifdef DEBUG
//        printf("Run %d (type %x) start at sector %llu, end at sector %llu. Comp len %llu offset %llu. Uncomp size %llu\n", i, uint32_t(type),
//               be(runs[i].sectorStart), be(runs[i].sectorStart)+be(runs[i].sectorCount),
//               be(runs[i].compLength), be(runs[i].compOffset) + be(table->dataStart), be(runs[i].sectorCount)*512);
#endif
	}
}

DMGPartition::~DMGPartition()
{
//    delete m_table;
}

void DMGPartition::adviseOptimalBlock(uint64_t offset, uint64_t& blockStart, uint64_t& blockEnd)
{
    std::vector<DMGPartitionRun>::iterator itRun2 = std::upper_bound(m_runs.begin(), m_runs.end(), DMGPartitionRun(RunType::Comment, 0, offset / SECTOR_SIZE, 0, 0));

    blockStart = itRun2->blockStart * SECTOR_SIZE;
    blockEnd = itRun2->blockEnd * SECTOR_SIZE;
    // Issue #22: empty areas may be larger than 2**31 (causing bugs in callers).
    // Moreover, there is no such thing as "optimal block" in zero-filled areas.
    if (itRun2->type == RunType::ZeroFill || itRun2->type == RunType::Unknown || itRun2->type == RunType::Raw)
        Reader::adviseOptimalBlock(offset, itRun2->blockStart, itRun2->blockEnd);
}

int32_t DMGPartition::read(void* buf, int32_t count, uint64_t offset)
{
    std::vector<DMGPartitionRun>::iterator itRun2 = std::upper_bound(m_runs.begin(), m_runs.end(), DMGPartitionRun(RunType::Comment, 0, offset / SECTOR_SIZE, 0, 0));
    uint64_t offsetInSector = offset - itRun2->blockStart*SECTOR_SIZE;
    int32_t done = 0;

	while (done < count)
	{
        int32_t thistime = readRun(((char*)buf) + done, *itRun2, offsetInSector, count-done);
        if (!thistime)
            throw io_error("Unexpected EOF from readRun");
        
        done += thistime;
        itRun2 = std::upper_bound(m_runs.begin(), m_runs.end(), DMGPartitionRun(RunType::Comment, 0, (offset + done) / SECTOR_SIZE, 0, 0));
        if ( itRun2 == m_runs.end() ) {
            throw io_error(stringPrintf("DMGPartition::read : cannot find next run"));

        }
	}
	
	return done;
}

int32_t DMGPartition::readRun(void* buf, const DMGPartitionRun& run, uint64_t offsetInSector, int32_t count)
{

	count = std::min<uint64_t>(count, (run.blockEnd-run.blockStart)*512 - offsetInSector);
	
#ifdef DEBUG
//    std::cout << "readRun(): runIndex = " << runIndex << ", offsetInSector = " << offsetInSector << ", count = " << count << std::endl;
#endif
	
	switch (run.type)
	{
		case RunType::Unknown: // My guess is that this value indicates a hole in the file (sparse file)
		case RunType::ZeroFill:
			//std::cout << "ZeroFill\n";
			memset(buf, 0, count);
			return count;
		case RunType::Raw:
			//std::cout << "Raw\n";
			return m_disk->read(buf, count, run.compOffset + m_tableDataStart + offsetInSector);
		case RunType::LZFSE:
#ifndef COMPILE_WITH_LZFSE
			throw function_not_implemented_error("LZFSE is not yet supported");
#endif
		case RunType::Zlib:
		case RunType::Bzip2:
		case RunType::ADC:
		{
			std::unique_ptr<DMGDecompressor> decompressor;
			std::shared_ptr<Reader> subReader;
			
			subReader.reset(new SubReader(m_disk, run.compOffset + m_tableDataStart, run.compLength));
			decompressor.reset(DMGDecompressor::create(run.type, subReader));
			
			if (!decompressor)
				throw std::logic_error("DMGDecompressor::create() returned nullptr!");

			unsigned long long int compLength = (run.blockStart-run.blockEnd)*512;
			if ( offsetInSector > compLength )
				return 0;
			if ( offsetInSector + count > compLength )
				count = compLength - offsetInSector;

			int32_t dec = decompressor->decompress((uint8_t*)buf, count, offsetInSector);
			if (dec < count)
				throw io_error("Error decompressing stream");
			return count;
		}
		default:
			return 0;
	}
}

uint64_t DMGPartition::length()
{
	return m_tableSectorCount * SECTOR_SIZE;
}
