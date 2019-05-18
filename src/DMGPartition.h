#ifndef DMGPARTITION_H
#define DMGPARTITION_H
#include "Reader.h"
#include "dmg.h"
#include <memory>
#include <map>
#include <vector>

class DMGPartitionRun
{
  public:
#ifdef DEBUG
    uint64_t index;
#endif
    RunType type;
    uint64_t blockStart;
    uint64_t blockEnd; // it's supposed to be the same as block start of the next run, but who knows...
    uint64_t compOffset;
    uint64_t compLength;
    DMGPartitionRun(RunType _type, uint64_t _blockStart, uint64_t _blockEnd, uint64_t _compOffset, uint64_t _compLength) : type(_type), blockStart(_blockStart), blockEnd(_blockEnd), compOffset(_compOffset), compLength(_compLength) {};
    bool operator < (const DMGPartitionRun &other) const { return blockEnd < other.blockEnd; }
};

class DMGPartition : public Reader
{
public:
	DMGPartition(std::shared_ptr<Reader> disk, BLKXTable* table);
    ~DMGPartition();
	
	virtual int32_t read(void* buf, int32_t count, uint64_t offset) override;
	virtual uint64_t length() override;
	virtual void adviseOptimalBlock(uint64_t offset, uint64_t& blockStart, uint64_t& blockEnd) override;
private:
    int32_t readRun(void* buf, const DMGPartitionRun& run, uint64_t offsetInSector, int32_t count);
private:
	std::shared_ptr<Reader> m_disk;

    uint64_t m_tableDataStart;
    uint64_t m_tableSectorCount;
    std::vector<DMGPartitionRun> m_runs;
};

#endif
