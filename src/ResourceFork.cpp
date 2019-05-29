#include "ResourceFork.h"
#include <stdexcept>
#include "be.h"
#include <stddef.h>
#include <memory>
#include "SubReader.h"

ResourceFork::ResourceFork(std::shared_ptr<Reader> reader)
	: m_reader(reader)
{
	loadResources();
}

inline bool operator<(const ResourceFork::Resource& t, const ResourceFork::Resource& that)
{
	if (t.type < that.type)
		return true;
	else if (t.type > that.type)
		return false;
	else
		return (t.id < that.id) ? true : false;
}

void ResourceFork::loadResources()
{
	HFSResourceForkHeader header;
	HFSResourceMapHeader mapHeader;
	HFSResourceList listHeader;
	
	if (m_reader->read(&header, sizeof(header), 0) != sizeof(header))
		throw std::runtime_error("Short read of resource fork header");
	
	header.dataOffset = header.dataOffset;
	header.mapOffset = header.mapOffset;
	header.dataLength = header.dataLength;
	header.mapLength = header.mapLength;
	
	if (m_reader->read(&mapHeader, sizeof(mapHeader), header.mapOffset) != sizeof(mapHeader))
		throw std::runtime_error("Short read of resource fork map header");
	
	mapHeader.listOffset = mapHeader.listOffset;
	
	if (m_reader->read(&listHeader, sizeof(listHeader), header.mapOffset + mapHeader.listOffset) != sizeof(listHeader))
		throw std::runtime_error("Short read of resource fork map list");
	
	listHeader.count = listHeader.count;
	
	int pos = header.mapOffset + mapHeader.listOffset + offsetof(HFSResourceList, items);
	for (int i = 0; i < listHeader.count+1; i++)
	{
		HFSResourceListItem item;
		std::unique_ptr<HFSResourcePointer[]> ptrs;
		const int offset = pos + sizeof(item)*i;
		
		if (m_reader->read(&item, sizeof(item), offset) != sizeof(item))
			throw std::runtime_error("Short read of an HFSResourceListItem");
		
		item.type = item.type;
		item.count = item.count;
		item.offset = item.offset;
		
		ptrs.reset(new HFSResourcePointer[item.count+1]);
		
		if (m_reader->read(ptrs.get(), sizeof(HFSResourcePointer) * (item.count+1), offset + item.offset) != sizeof(HFSResourcePointer) * (item.count+1))
			throw std::runtime_error("Short read of HFSResourcePointers");
		
		for (int j = 0; j < item.count+1; j++)
		{
			HFSResourceHeader hdr;
			Resource res = { item.type, ptrs[j].resourceId };
			ResourceLocation loc;
			
			loc.offset = header.dataOffset + ptrs[j].dataOffset;
			
			if (m_reader->read(&hdr, sizeof(hdr), loc.offset) != sizeof(hdr))
				throw std::runtime_error("Short read of HFSResourceHeader");
			
			loc.offset += offsetof(HFSResourceHeader, data);
			loc.length = hdr.length;
			
			m_resources.insert({ res, loc });
		}
	}
}

std::shared_ptr<Reader> ResourceFork::getResource(uint32_t resourceType, uint16_t id)
{
	Resource res = { resourceType, id };
	auto it = m_resources.find(res);
	
	if (it == m_resources.end())
		return nullptr;
	else
		return std::shared_ptr<Reader>(new SubReader(m_reader, it->second.offset, it->second.length));
}
