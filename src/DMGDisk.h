#ifndef DMGDISK_H
#define DMGDISK_H

#include <vector>
#include <map>

#include "PartitionedDisk.h"
#include "Reader.h"
#include "dmg.h"
#include "CacheZone.h"

class DMGDisk : public PartitionedDisk
{
public:
	DMGDisk(std::shared_ptr<Reader>reader);
	~DMGDisk();

	virtual const std::vector<Partition>& partitions() const override { return m_partitions; }
	virtual std::shared_ptr<Reader> readerForPartition(int index) override;

	static bool isDMG(std::shared_ptr<Reader> reader);
private:
	void loadKoly(const UDIFResourceFile& koly);
	static bool parseNameAndType(const std::string& nameAndType, std::string& name, std::string& type);
//    BLKXTable* loadBLKXTableForPartition(int index);
	std::shared_ptr<Reader> readerForKolyBlock(int index);
private:
	std::shared_ptr<Reader> m_reader;
	std::vector<Partition> m_partitions;
    std::map<int, int> m_partitionIDs;
    std::vector<std::vector<uint8_t>> m_partitionsBlkxTables;
	UDIFResourceFile m_udif;
	std::shared_ptr<CacheZone> m_zone;
};

#endif

