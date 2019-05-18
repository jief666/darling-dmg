#include "HFSAttributeBTree.h"
#include <cstring>
#include <stdexcept>

#include "../../conversion/fast_unicode_compare_apple.h"
#include "../../conversion/utf816Conversion.h"

HFSAttributeBTree::HFSAttributeBTree(std::shared_ptr<HFSFork> fork, std::shared_ptr<CacheZone> zone)
: HFSBTree(fork, zone, "Attribute")
{
}

std::map<std::string, std::vector<uint8_t>> HFSAttributeBTree::getattr(HFSCatalogNodeID cnid)
{
	HFSPlusAttributeKey key;
	std::vector<std::shared_ptr<HFSBTreeNode>> leaves;
	std::map<std::string, std::vector<uint8_t>> rv;

	memset(&key, 0, sizeof(key));
	key.fileID = htobe32(cnid);

	leaves = findLeafNodes((Key*) &key, cnidComparator);
	
	for (std::shared_ptr<HFSBTreeNode> leafPtr : leaves)
	{
		HFSBTreeNode& leaf = *leafPtr;
		for (int i = 0; i < leaf.recordCount(); i++)
		{
			HFSPlusAttributeKey* recordKey = leaf.getRecordKey<HFSPlusAttributeKey>(i);
			HFSPlusAttributeDataInline* data;
			std::vector<uint8_t> vecData;
			std::string name;

			if (be(recordKey->fileID) != cnid)
				continue;

			data = leaf.getRecordData<HFSPlusAttributeDataInline>(i);

			// process data
			if (be(data->recordType) != kHFSPlusAttrInlineData)
				continue;
			
			vecData = std::vector<uint8_t>((uint8_t*)data + sizeof(HFSPlusAttributeDataInline), (uint8_t*)data + sizeof(HFSPlusAttributeDataInline) + be(data->attrSize));
			size_t len = utf16BE_to_utf8(recordKey->attrName, &name);

			rv[name] = vecData;
		}
	}
	
	return rv;
}

bool HFSAttributeBTree::getattr(HFSCatalogNodeID cnid, const std::string& attrName, std::vector<uint8_t>& dataOut)
{
//if ( cnid == 47 && attrName == "xattr_0" ) {
//printf("");
//}
	HFSPlusAttributeKey key;
	std::shared_ptr<HFSBTreeNode> leafNodePtr;
//    UnicodeString ucAttrName = UnicodeString::fromUTF8(attrName);
//    UnicodeString ucAttrName = Utf8ToUnicodeString(attrName);
//    int ucAttrNameAllocatedSize = attrName.size()*2;
//    uint16_t* ucAttrName = (uint16_t*)alloca(ucAttrNameAllocatedSize);
    utf8_to_utf16BE(attrName, &key.attrName);

//    memset(&key, 0, sizeof(key));
	key.fileID = htobe32(cnid);
	
//    key.attrName.length = StringToUnichar(attrName, key.attrName, sizeof(key.attrName));
//    key.attrName.length = htobe16(ucAttrNameSize);
	
	leafNodePtr = findLeafNode((Key*) &key, cnidAttrComparator);
	if (!leafNodePtr)
		return false;
	
	HFSBTreeNode& leafNode = *leafNodePtr; // convenience
	for (int i = 0; i < leafNode.recordCount(); i++)
	{
		HFSPlusAttributeKey* recordKey = leafNode.getRecordKey<HFSPlusAttributeKey>(i);
		HFSPlusAttributeDataInline* data;
		
//        UnicodeString recAttrName((char*)recordKey->attrName, be(recordKey->attrName.length)*2, "UTF-16BE");

        if (be(recordKey->fileID) == cnid && FastUnicodeCompare(recordKey->attrName, key.attrName) == 0 )
		{
			data = leafNode.getRecordData<HFSPlusAttributeDataInline>(i);

			// process data
			if (be(data->recordType) != kHFSPlusAttrInlineData)
				continue;
#ifdef DEBUG
  uint64_t b = be(data->attrSize);
  uint8_t* a  = (uint8_t*)data + be(data->attrSize);
#endif
			dataOut = std::vector<uint8_t>((uint8_t*)data + sizeof(HFSPlusAttributeDataInline), (uint8_t*)data + sizeof(HFSPlusAttributeDataInline) + be(data->attrSize));
			return true;
		}
	}
	
	return false;
}

int HFSAttributeBTree::cnidAttrComparator(const Key* indexKey, const Key* desiredKey)
{
	const HFSPlusAttributeKey* indexAttributeKey = reinterpret_cast<const HFSPlusAttributeKey*>(indexKey);
	const HFSPlusAttributeKey* desiredAttributeKey = reinterpret_cast<const HFSPlusAttributeKey*>(desiredKey);
	
	//std::cout << "Attr search: index cnid: " << be(indexAttributeKey->fileID) << " desired cnid: " << be(desiredAttributeKey->fileID) << std::endl;

	if (be(indexAttributeKey->fileID) > be(desiredAttributeKey->fileID))
		return 1;
	else if (be(indexAttributeKey->fileID) < be(desiredAttributeKey->fileID))
		return -1;
	else
	{
//        UnicodeString desiredName, indexName;
//        
//        desiredName = UnicodeString((char*)desiredAttributeKey->attrName, be(desiredAttributeKey->attrName.length)*2, "UTF-16BE");
//        indexName = UnicodeString((char*)indexAttributeKey->attrName, be(indexAttributeKey->attrName.length)*2, "UTF-16BE");
#ifdef DEBUG
  std::string desiredName_utf8 = toUtf8(*desiredAttributeKey);
  std::string indexName_utf8 = toUtf8(*indexAttributeKey);
#endif

        int rv = FastUnicodeCompare(indexAttributeKey->attrName, desiredAttributeKey->attrName);
        return rv;
//        return indexName.compare(desiredName);
	}
}

int HFSAttributeBTree::cnidComparator(const Key* indexKey, const Key* desiredKey)
{
	const HFSPlusAttributeKey* indexAttributeKey = reinterpret_cast<const HFSPlusAttributeKey*>(indexKey);
	const HFSPlusAttributeKey* desiredAttributeKey = reinterpret_cast<const HFSPlusAttributeKey*>(desiredKey);

	if (be(indexAttributeKey->fileID) > be(desiredAttributeKey->fileID))
		return 1;
	else if (be(indexAttributeKey->fileID) < be(desiredAttributeKey->fileID))
		return -1;
	else
		return 0;
}
