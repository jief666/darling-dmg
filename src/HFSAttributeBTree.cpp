#include "HFSAttributeBTree.h"
#include <cstring>
#include <stdexcept>
#include <unicode/unistr.h>
#include "unichar.h"
using icu::UnicodeString;
HFSAttributeBTree::HFSAttributeBTree(std::shared_ptr<HFSFork> fork, CacheZone* zone)
: HFSBTree(fork, zone, "Attribute")
{
}

std::map<std::string, std::vector<uint8_t>> HFSAttributeBTree::getattr(HFSCatalogNodeID cnid)
{
	HFSPlusAttributeKey key;
	std::vector<std::shared_ptr<HFSBTreeNode>> leaves;
	std::map<std::string, std::vector<uint8_t>> rv;

	memset(&key, 0, sizeof(key));
	key.fileID = cnid;

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

			if (recordKey->fileID != cnid)
				continue;

			data = leaf.getRecordData<HFSPlusAttributeDataInline>(i);

			// process data
			if (data->recordType != kHFSPlusAttrInlineData)
				continue;
			
			vecData = std::vector<uint8_t>(data->attrData, &data->attrData[data->attrSize]);
			name = UnicharToString(recordKey->attrNameLength, recordKey->attrName);
			
			rv[name] = vecData;
		}
	}
	
	return rv;
}

bool HFSAttributeBTree::getattr(HFSCatalogNodeID cnid, const std::string& attrName, std::vector<uint8_t>& dataOut)
{
	HFSPlusAttributeKey key;
	std::shared_ptr<HFSBTreeNode> leafNodePtr;
	UnicodeString ucAttrName = UnicodeString::fromUTF8(attrName);
	
	memset(&key, 0, sizeof(key));
	key.fileID = cnid;
	
	key.attrNameLength = StringToUnichar(attrName, key.attrName, sizeof(key.attrName));
	key.attrNameLength = key.attrNameLength;
	
	leafNodePtr = findLeafNode((Key*) &key, cnidAttrComparator);
	if (!leafNodePtr)
		return false;
	
	HFSBTreeNode& leafNode = *leafNodePtr; // convenience
	for (int i = 0; i < leafNode.recordCount(); i++)
	{
		HFSPlusAttributeKey* recordKey = leafNode.getRecordKey<HFSPlusAttributeKey>(i);
		HFSPlusAttributeDataInline* data;
		
		UnicodeString recAttrName((char*)recordKey->attrName, recordKey->attrNameLength*2, "UTF-16BE");

		if (recordKey->fileID == cnid && recAttrName == ucAttrName)
		{
			data = leafNode.getRecordData<HFSPlusAttributeDataInline>(i);

			// process data
			if (data->recordType != kHFSPlusAttrInlineData)
				continue;
		
			dataOut = std::vector<uint8_t>(data->attrData, &data->attrData[data->attrSize]);
			return true;
		}
	}
	
	return false;
}

int HFSAttributeBTree::cnidAttrComparator(const Key* indexKey, const Key* desiredKey)
{
	const HFSPlusAttributeKey* indexAttributeKey = reinterpret_cast<const HFSPlusAttributeKey*>(indexKey);
	const HFSPlusAttributeKey* desiredAttributeKey = reinterpret_cast<const HFSPlusAttributeKey*>(desiredKey);
	
	//std::cout << "Attr search: index cnid: " << indexAttributeKey->fileID << " desired cnid: " << desiredAttributeKey->fileID << std::endl;

	if (indexAttributeKey->fileID > desiredAttributeKey->fileID)
		return 1;
	else if (indexAttributeKey->fileID < desiredAttributeKey->fileID)
		return -1;
	else
	{
		UnicodeString desiredName, indexName;
		
		desiredName = UnicodeString((char*)desiredAttributeKey->attrName, desiredAttributeKey->attrNameLength*2, "UTF-16BE");
		indexName = UnicodeString((char*)indexAttributeKey->attrName, indexAttributeKey->attrNameLength*2, "UTF-16BE");

		return indexName.compare(desiredName);
	}
}

int HFSAttributeBTree::cnidComparator(const Key* indexKey, const Key* desiredKey)
{
	const HFSPlusAttributeKey* indexAttributeKey = reinterpret_cast<const HFSPlusAttributeKey*>(indexKey);
	const HFSPlusAttributeKey* desiredAttributeKey = reinterpret_cast<const HFSPlusAttributeKey*>(desiredKey);

	if (indexAttributeKey->fileID > desiredAttributeKey->fileID)
		return 1;
	else if (indexAttributeKey->fileID < desiredAttributeKey->fileID)
		return -1;
	else
		return 0;
}
