#ifndef HFSHIGHLEVELVOLUME_H
#define HFSHIGHLEVELVOLUME_H
#include <memory>
#if defined(_MSC_VER)
#  include <fuse.h> // for FUSE_STAT
#else
#  include <sys/stat.h>
#endif
#include <vector>
#include <string>
#include "HFSVolume.h"
#include "HFSCatalogBTree.h"

struct decmpfs_disk_header;

// HFSHighLevelVolume wraps HFSVolume to provide a more high-level approach to the filesystem.
// It transparently decompresses compressed files (decmpfs) and translates HFS+ file information
// to well-known Unix-like types.
class HFSHighLevelVolume
{
public:
	HFSHighLevelVolume(std::shared_ptr<HFSVolume> volume);

	inline bool isHFSX() const { return m_volume->isHFSX(); }
	inline uint64_t volumeSize() const { return m_volume->volumeSize(); }

	// See exceptions.h for the list of possible exceptions
	std::map<std::string, struct FUSE_STAT> listDirectory(const std::string& path);
	std::shared_ptr<Reader> openFile(const std::string& path);
	struct FUSE_STAT stat(const std::string& path);
	std::vector<std::string> listXattr(const std::string& path);
	std::vector<uint8_t> getXattr(const std::string& path, const std::string& xattrName);
private:
	void hfs_nativeToStat(const HFSPlusCatalogFileOrFolder& ff, struct FUSE_STAT* stat, bool resourceFork = false);
	void hfs_nativeToStat_decmpfs(const HFSPlusCatalogFileOrFolder& ff, struct FUSE_STAT* stat, bool resourceFork = false);
	decmpfs_disk_header* get_decmpfs(HFSCatalogNodeID cnid, std::vector<uint8_t>& holder);
private:
	std::shared_ptr<HFSVolume> m_volume;
	std::unique_ptr<HFSCatalogBTree> m_tree;
};

#endif
