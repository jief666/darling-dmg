#include "HFSCatalogBTree.h"
#include "be.h"
#include "exceptions.h"
#include <sstream>
#include <cstring>

#include "../../conversion/fast_unicode_compare_apple.h"
#include "../../conversion/utf816Conversion.h"
#include "../../utf8proc/utf8proc.h"

static const int MAX_SYMLINKS = 50;

HFSCatalogBTree::HFSCatalogBTree(std::shared_ptr<HFSFork> fork, HFSVolume* volume, std::shared_ptr<CacheZone> zone)
	: HFSBTree(fork, zone, "Catalog"), m_volume(volume), m_hardLinkDirID(0)
{
	HFSPlusCatalogFileOrFolder ff;
	int rv = stat(std::string("\0\0\0\0HFS+ Private Data", 21), &ff);
	if (rv == 0)
		m_hardLinkDirID = be(ff.folder.folderID);
}

bool HFSCatalogBTree::isCaseSensitive() const
{
	return m_volume->isHFSX() && m_header.keyCompareType == KeyCompareType::kHFSBinaryCompare;
}
//
//int HFSCatalogBTree::caseInsensitiveComparator(const Key* indexKey, const Key* desiredKey)
//{
//    const HFSPlusCatalogKey* catIndexKey = reinterpret_cast<const HFSPlusCatalogKey*>(indexKey);
//    const HFSPlusCatalogKey* catDesiredKey = reinterpret_cast<const HFSPlusCatalogKey*>(desiredKey);
//    UnicodeString desiredName, indexName;
//    UErrorCode error = U_ZERO_ERROR;
//
//    //std::cout << "desired: " << be(catDesiredKey->parentID) << ", index: " << be(catIndexKey->parentID) << "\n";
//    if (be(catDesiredKey->parentID) < be(catIndexKey->parentID))
//    {
//        //std::cout << "\t -> bigger\n";
//        return 1;
//    }
//    else if (be(catDesiredKey->parentID) > be(catIndexKey->parentID))
//    {
//        //std::cout << "\t -> smaller\n";
//        return -1;
//    }
//
//    desiredName = UnicodeString((char*)catDesiredKey->nodeName.string, be(catDesiredKey->nodeName.length)*2, g_utf16be, error);
//    indexName = UnicodeString((char*)catIndexKey->nodeName.string, be(catIndexKey->nodeName.length)*2, g_utf16be, error);
//    
//    // Hack for "\0\0\0\0HFS+ Private Data" which should come as last in ordering (issue #11)
//    if (indexName.charAt(0) == 0)
//        return 1;
//    else if (desiredName.charAt(0) == 0)
//        return -1;
//    
//    {
//        //std::string des, idx;
//        //desiredName.toUTF8String(des);
//        //indexName.toUTF8String(idx);
//        
//        int r = indexName.caseCompare(desiredName, 0);
//        
//        //std::cout << "desired: " << des << " - index: " << idx << " -> r=" << r << std::endl;
//
//        return r;
//    }
//    
//    return 0;
//}

int HFSCatalogBTree::caseInsensitiveComparator(const Key* indexKey, const Key* desiredKey)
{
    const HFSPlusCatalogKey* catIndexKey = reinterpret_cast<const HFSPlusCatalogKey*>(indexKey);
    const HFSPlusCatalogKey* catDesiredKey = reinterpret_cast<const HFSPlusCatalogKey*>(desiredKey);

    //std::cout << "desired: " << be(catDesiredKey->parentID) << ", index: " << be(catIndexKey->parentID) << "\n";
    if (be(catDesiredKey->parentID) < be(catIndexKey->parentID))
    {
        //std::cout << "\t -> bigger\n";
        return 1;
    }
    else if (be(catDesiredKey->parentID) > be(catIndexKey->parentID))
    {
        //std::cout << "\t -> smaller\n";
        return -1;
    }

//    desiredName = UnicodeString((char*)catDesiredKey->nodeName.string, be(catDesiredKey->nodeName.length)*2, g_utf16be, error);
//    indexName = UnicodeString((char*)catIndexKey->nodeName.string, be(catIndexKey->nodeName.length)*2, g_utf16be, error);
    
    // Hack for "\0\0\0\0HFS+ Private Data" which should come as last in ordering (issue #11)
    
    if (catIndexKey->nodeName.string[0] == 0)
        return 1;
    else if (catDesiredKey->nodeName.string[0] == 0)
        return -1;
    
    {
        //std::string des, idx;
        //desiredName.toUTF8String(des);
        //indexName.toUTF8String(idx);
        
        int r = FastUnicodeCompare(catIndexKey->nodeName.string, catIndexKey->nodeName.length, catDesiredKey->nodeName.string, catDesiredKey->nodeName.length);
        
        //std::cout << "desired: " << des << " - index: " << idx << " -> r=" << r << std::endl;

        return r;
    }
    
    return 0;
}

int HFSCatalogBTree::caseSensitiveComparator(const Key* indexKey, const Key* desiredKey)
{
	const HFSPlusCatalogKey* catIndexKey = reinterpret_cast<const HFSPlusCatalogKey*>(indexKey);
	const HFSPlusCatalogKey* catDesiredKey = reinterpret_cast<const HFSPlusCatalogKey*>(desiredKey);
//    UnicodeString desiredName, indexName;
//    UErrorCode error = U_ZERO_ERROR;

	if (be(catDesiredKey->parentID) < be(catIndexKey->parentID))
		return 1;
	else if (be(catDesiredKey->parentID) > be(catIndexKey->parentID))
		return -1;

//    desiredName = UnicodeString((char*)catDesiredKey->nodeName.string, be(catDesiredKey->nodeName.length)*2, g_utf16be, error);
//    indexName = UnicodeString((char*)catIndexKey->nodeName.string, be(catIndexKey->nodeName.length)*2, g_utf16be, error);
	
	// Hack for "\0\0\0\0HFS+ Private Data" which should come as last in ordering (issue #11)
	if (catIndexKey->nodeName.string[0] == 0)
		return 1;
	else if (catDesiredKey->nodeName.string[0] == 0)
		return 1;

	if (catDesiredKey->nodeName.length > 0)
	{
		//std::string des, idx;
		//desiredName.toUTF8String(des);
		//indexName.toUTF8String(idx);
		
        int r = memcmp(catDesiredKey->nodeName.string, catIndexKey->nodeName.string, std::min(be(catDesiredKey->nodeName.length)*2, be(catIndexKey->nodeName.length)*2));
//        int r = indexName.caseCompare(desiredName, 0);
		
		// std::cout << "desired: " << des << " - index: " << idx << " -> r=" << r << std::endl;

		return r;
	}

	return 0;
}

int HFSCatalogBTree::idOnlyComparator(const Key* indexKey, const Key* desiredKey)
{
	const HFSPlusCatalogKey* catIndexKey = reinterpret_cast<const HFSPlusCatalogKey*>(indexKey);
	const HFSPlusCatalogKey* catDesiredKey = reinterpret_cast<const HFSPlusCatalogKey*>(desiredKey);
	
	//std::cerr << "idOnly: desired: " << be(catDesiredKey->parentID) << ", index: " << be(catIndexKey->parentID) << std::endl;

	if (be(catDesiredKey->parentID) > be(catIndexKey->parentID))
		return -1;
	else if (be(catIndexKey->parentID) > be(catDesiredKey->parentID))
		return 1;
	else
		return 0;
}

int HFSCatalogBTree::listDirectory(const std::string& path, std::map<std::string, std::shared_ptr<HFSPlusCatalogFileOrFolder>>& contents)
{
	HFSPlusCatalogFileOrFolder dir;
	int rv;
	std::vector<std::shared_ptr<HFSBTreeNode>> leaves;
	HFSPlusCatalogKey key;
	std::map<std::string, std::shared_ptr<HFSPlusCatalogFileOrFolder>> beContents;

	contents.clear();

	// determine the CNID of the directory
	rv = stat(path, &dir);
	if (rv != 0)
		return rv;

	if (be(dir.folder.recordType) != RecordType::kHFSPlusFolderRecord)
		return -ENOTDIR;

	// find leaves that may contain directory elements
	key.parentID = dir.folder.folderID;
	leaves = findLeafNodes((Key*) &key, idOnlyComparator);

	for (std::shared_ptr<HFSBTreeNode> leafPtr : leaves)
	{
		//std::cerr << "**** Looking for elems with CNID " << be(key.parentID) << std::endl;
		 appendNameAndHFSPlusCatalogFileOrFolderFromLeafForParentId(leafPtr, be(key.parentID), beContents);
	}

	for (auto it = beContents.begin(); it != beContents.end(); it++)
	{
		std::string filename = it->first;

		/* Filter out :
		 * - "\0\0\0\0HFS+ Private Data" (truth is, every filename whose first char is \0 will be filtered out)
		 * - ".HFS+ Private Directory Data\r"
		 * - ".journal"
		 * - ".journal_info_block"
		 * from root directory
		 */
		if (be(dir.folder.folderID) != kHFSRootFolderID  ||  (filename[0]!=0  &&  filename.compare(".HFS+ Private Directory Data\r")!=0  &&  filename.compare(".journal")!=0  &&  filename.compare(".journal_info_block")!=0))
		{
			replaceChars(filename, '/', ':'); // Issue #36: / and : have swapped meaning in HFS+
			contents[filename] = it->second;
		}
	}

	return 0;
}

void HFSCatalogBTree::incrementCountLeafForParentId(std::shared_ptr<HFSBTreeNode> leafNodePtr, HFSCatalogNodeID cnid, uint32_t* fileAndFolderCount)
{
    for (int i = 0; i < leafNodePtr->recordCount(); i++)
    {
        HFSPlusCatalogKey* recordKey;
        HFSPlusCatalogFileOrFolder* ff;
        RecordType recType;

        recordKey = leafNodePtr->getRecordKey<HFSPlusCatalogKey>(i);
        ff = leafNodePtr->getRecordData<HFSPlusCatalogFileOrFolder>(i);

        recType = be(ff->folder.recordType);
        //{
            //std::string name = UnicharToString(recordKey->nodeName);
            //std::cerr << "RecType " << int(recType) << ", ParentID: " << be(recordKey->parentID) << ", nodeName " << name << std::endl;
        //}

        switch (recType)
        {
            case RecordType::kHFSPlusFolderRecord:
            case RecordType::kHFSPlusFileRecord:
            {
                
                // filter "\0\0\0\0HFS+ Private Data"
                if ( /*recordKey->nodeName.string[0] != 0 && */ be(recordKey->parentID) == cnid)
                {
                    *fileAndFolderCount += 1;
                }
                //else
                //    std::cerr << "CNID not matched - " << cnid << " required\n";
                break;
            }
            case RecordType::kHFSPlusFolderThreadRecord:
            case RecordType::kHFSPlusFileThreadRecord:
                break;
        }
    }
}

int HFSCatalogBTree::countDirectory(HFSCatalogNodeID id, uint32_t* fileAndFolderCount)
{
    std::vector<std::shared_ptr<HFSBTreeNode>> leaves;
    HFSPlusCatalogKey key;
    std::map<std::string, std::shared_ptr<HFSPlusCatalogFileOrFolder>> beContents;
    *fileAndFolderCount = 0;

    // find leaves that may contain directory elements
    key.parentID = id;
    leaves = findLeafNodes((Key*) &key, idOnlyComparator);

    for (std::shared_ptr<HFSBTreeNode> leafPtr : leaves)
    {
        //std::cerr << "**** Looking for elems with CNID " << be(key.parentID) << std::endl;
         incrementCountLeafForParentId(leafPtr, be(key.parentID), fileAndFolderCount);
    }

    return 0;
}

static void split(const std::string &s, char delim, std::vector<std::string>& elems)
{
	std::stringstream ss(s);
	std::string item;

	while (std::getline(ss, item, delim))
		elems.push_back(item);
}

std::shared_ptr<HFSPlusCatalogFileOrFolder> HFSCatalogBTree::findHFSPlusCatalogFileOrFolderForParentIdAndName(HFSCatalogNodeID parentID, const std::string &elem)
{
	HFSPlusCatalogKey key;
	key.parentID = htobe32(parentID);
	std::vector<std::shared_ptr<HFSBTreeNode>> leaves;
	leaves = findLeafNodes((Key*) &key, idOnlyComparator);
	std::map<std::string, std::shared_ptr<HFSPlusCatalogFileOrFolder>> beContents;
	for (std::shared_ptr<HFSBTreeNode> leafPtr : leaves)
	{
		//std::cerr << "**** Looking for elems with CNID " << be(key.parentID) << std::endl;
		appendNameAndHFSPlusCatalogFileOrFolderFromLeafForParentIdAndName(leafPtr, be(key.parentID), elem, beContents);
	}
	if (beContents.size() == 0)
		return nullptr;
	if (beContents.size() > 1)
		throw io_error("Multiple records with same name");
	
	return beContents.begin()->second;
}

int HFSCatalogBTree::stat(std::string path, HFSPlusCatalogFileOrFolder* s)
{
	std::vector<std::string> elems;
	std::shared_ptr<HFSBTreeNode> leafNodePtr;
	std::shared_ptr<HFSPlusCatalogFileOrFolder> last = nullptr;

	memset(s, 0, sizeof(*s));

	if (path.compare(0, 1, "/") == 0)
		path = path.substr(1);
	if (!path.empty() && path.compare(path.length()-1, 1, "/") == 0)
		path = path.substr(0, path.length()-1);

	elems.push_back(std::string());
	split(path, '/', elems);
#ifdef DEBUG
if (path== "/utf8_names/pomme.txt") {
        printf("");
}
#endif
	for (size_t i = 0; i < elems.size(); i++)
	{
		std::string elem = elems[i];
		replaceChars(elem, ':', '/'); // Issue #36: / and : have swapped meaning in HFS+

		HFSCatalogNodeID parentID = last ? be(last->folder.folderID) : kHFSRootParentID;

		//if (ustr.length() > 255) // FIXME: there is a UCS-2 vs UTF-16 issue here!
		//	return -ENAMETOOLONG;

#ifdef DEBUG
if (elem== "pomme.txt") {
        printf("");
}
#endif
		last = findHFSPlusCatalogFileOrFolderForParentIdAndName(parentID, elem);
		if (last==nullptr)
			return -ENOENT;

		// resolve symlinks, check if directory...
		// FUSE takes care of this
		/*
		{
			RecordType recType = RecordType(be(uint16_t(last->folder.recordType)));
			const bool isLastElem = i+1 == elems.size();

			if (recType == RecordType::kHFSPlusFileRecord)
			{
				if (last->file.permissions.fileMode & HFSPLUS_S_IFLNK && (!lstat || !isLastElem))
				{
					if (currentDepth >= MAX_SYMLINKS)
						return -ELOOP;
					// TODO: deal with symlinks
					// recurse with increased depth
				}
				else if (!isLastElem)
					return -ENOTDIR;
			}
		}
		*/

		//parent = last->folder.folderID;
	}
	if (be(last->file.userInfo.fileType) == kHardLinkFileType  &&  m_hardLinkDirID != 0) {
		std::string iNodePath;
		iNodePath += "iNode";
		iNodePath += std::to_string(be(last->file.permissions.special.iNodeNum));
		std::shared_ptr<HFSPlusCatalogFileOrFolder> leafNodeHl = findHFSPlusCatalogFileOrFolderForParentIdAndName(m_hardLinkDirID, iNodePath);
		if (leafNodeHl!=nullptr)
			last = leafNodeHl;
	}
	if (be((*last).folder.recordType) == RecordType::kHFSPlusFolderRecord)
    {
		// HFSPlusCatalogFolder is smaller than HFSPlusCatalogFile. If we do *s = *last, it breaks if run with guard memory
		memcpy(s, last.get(), sizeof(HFSPlusCatalogFolder));
		uint32_t nlink;
		countDirectory(s->folder.folderID, &nlink);
		if ( path.length() > 0) {
			s->file.permissions.special.linkCount = be(nlink+2);
		}else{
			s->file.permissions.special.linkCount = be(nlink);
		}
    }else{
		memcpy(s, last.get(), sizeof(HFSPlusCatalogFile));
	}
	//std::cout << "File/folder flags: 0x" << std::hex << s->file.flags << std::endl;

	return 0;
}

void HFSCatalogBTree::appendNameAndHFSPlusCatalogFileOrFolderFromLeafForParentId(std::shared_ptr<HFSBTreeNode> leafNodePtr, HFSCatalogNodeID cnid, std::map<std::string, std::shared_ptr<HFSPlusCatalogFileOrFolder>>& map)
{
	appendNameAndHFSPlusCatalogFileOrFolderFromLeafForParentIdAndName(leafNodePtr, cnid, "", map);
}

void HFSCatalogBTree::appendNameAndHFSPlusCatalogFileOrFolderFromLeafForParentIdAndName(std::shared_ptr<HFSBTreeNode> leafNodePtr, HFSCatalogNodeID cnid, const std::string& name, std::map<std::string, std::shared_ptr<HFSPlusCatalogFileOrFolder>>& map)
{
    HFSString utf16Name;
#ifdef DEBUG
if (name== "pomme.txt") {
        printf("");
}
#endif
    uint8_t* res = utf8proc_NFD((const uint8_t*)name.c_str());

    utf16Name.length = be((uint16_t)utf8_to_utf16BE(utf16Name.string, sizeof(utf16Name.string)/2, (const char*)res, strlen((char*)res), 0, NULL));
	
	uint16_t recordCount = leafNodePtr->recordCount();
	for (int i = 0; i < recordCount; i++)
	{
		HFSPlusCatalogKey* recordKey;
		HFSPlusCatalogFileOrFolder* ff;
		RecordType recType;

		recordKey = leafNodePtr->getRecordKey<HFSPlusCatalogKey>(i);
		ff = leafNodePtr->getRecordData<HFSPlusCatalogFileOrFolder>(i);

		recType = be(ff->folder.recordType);
#ifdef DEBUG
//if (recordKey->keyLength == 0x402 ) {
//printf("");
//}
//	std::string nameDebug;
//	utf16BE_to_utf8(recordKey->nodeName, &nameDebug);
//	std::cerr << "RecType " << int(recType) << ", ParentID: " << be(recordKey->parentID) << ", nodeName " << nameDebug << std::endl;
//if (name == "utf8_names" && nameDebug== "utf8_names") {
//	printf("");
//		HFSBTreeNode* lp = leafNodePtr.get();
//		char buf[4096];
//		memcpy(buf, lp->m_descriptorData, 4096);
//		ff = leafNodePtr->getRecordData<HFSPlusCatalogFileOrFolder>(i);
//HFSPlusCatalogFileOrFolder s = *ff;
//}
#endif


		switch (recType)
		{
			case RecordType::kHFSPlusFolderRecord:
			case RecordType::kHFSPlusFileRecord:
			{
				
				// do NOT skip "\0\0\0\0HFS+ Private Data", we need it to get is folderID in constructor
				if ( /* recordKey->nodeName.string[0] != 0 &&*/ be(recordKey->parentID) == cnid)
				{
					bool equal = name.empty();
					if (!equal)
					{
						if (isCaseSensitive())
                            equal = utf16Name.length == be(recordKey->nodeName.length) && memcmp(utf16Name.string, recordKey->nodeName.string, utf16Name.length) == 0;
//                            equal = EqualCase(recordKey->nodeName, name);
						else
                            equal = FastUnicodeCompare(recordKey->nodeName, utf16Name) == 0;
//                            equal = EqualNoCase(recordKey->nodeName, name);
					}

					if (equal)
					{
//                        std::string name2 = UnicharToString(recordKey->nodeName);
                        std::string name2;
                        utf16BE_to_utf8(recordKey->nodeName, &name2);
						map[name2] = std::shared_ptr<HFSPlusCatalogFileOrFolder>(leafNodePtr, ff); // retain leafPtr, act as a HFSPlusCatalogFileOrFolder
					}
				}
				//else
				//	std::cerr << "CNID not matched - " << cnid << " required\n";
				break;
			}
			case RecordType::kHFSPlusFolderThreadRecord:
			case RecordType::kHFSPlusFileThreadRecord:
				break;
		}
	}
    free(res);
}

time_t HFSCatalogBTree::appleToUnixTime(uint32_t apple)
{
	const time_t offset = 2082844800L; // Nb of seconds between 1st January 1904 12:00:00 and unix epoch
	if (apple == 0)
		return 0; // 0 stays 0, even if it change the date from 1904 to 1970.
	// Force time to wrap around and stay positive number. That's how Mac does it.
	// File from before 70 will have date in the future.
	// Example : Time value 2082844799, that should be Dec 31st 12:59:59 PM will become February 7th 2106 6:28:15 AM.
	return uint32_t(apple - offset);
}

int HFSCatalogBTree::openFile(const std::string& path, std::shared_ptr<Reader>& forkOut, bool resourceFork)
{
	HFSPlusCatalogFileOrFolder ff;
	int rv;

	forkOut.reset();

	rv = stat(path, &ff);
	if (rv < 0)
		return rv;

	if (be(ff.folder.recordType) != RecordType::kHFSPlusFileRecord)
		return -EISDIR;

	forkOut.reset(new HFSFork(m_volume, resourceFork ? ff.file.resourceFork : ff.file.dataFork,
		be(ff.file.fileID), resourceFork));

	return 0;
}


#ifdef DEBUG

void HFSCatalogBTree::dumpTree() const
{
	dumpTree(be(m_header.rootNode), 0);
}

void HFSCatalogBTree::dumpTree(int nodeIndex, int depth) const
{
	HFSBTreeNode node(m_reader, nodeIndex, be(m_header.nodeSize));

	switch (node.kind())
	{
		case NodeKind::kBTIndexNode:
		{
			for (size_t i = 0; i < node.recordCount(); i++)
			{
//                UErrorCode error = U_ZERO_ERROR;
                HFSPlusCatalogKey* key = node.getRecordKey<HFSPlusCatalogKey>(i);
//                UnicodeString keyName((char*)key->nodeName.string, be(key->nodeName.length)*2, g_utf16be, error);
				std::string str;
				
//                keyName.toUTF8String(str);
                utf16BE_to_utf8(key->nodeName, &str);

				// recurse down
				uint32_t* childIndex = node.getRecordData<uint32_t>(i);
#ifdef DEBUG
				printf("Index Node(%4d,%4zd) %s %s(%d) ->child %d\n", nodeIndex, i, std::string(depth, ' ').c_str(), str.c_str(), be(key->parentID), be(*childIndex));
//				std::cout << "Index node(" << nodeIndex << "): " << std::string(depth, ' ') << str << "(" << be(key->parentID) << ")\n";
#endif
				dumpTree(be(*childIndex), depth+2);
			}
			
			break;
		}
		case NodeKind::kBTLeafNode:
		{
			for (size_t i = 0; i < node.recordCount(); i++)
			{
				HFSPlusCatalogKey* recordKey;
//                UErrorCode error = U_ZERO_ERROR;
//                UnicodeString keyName;
				std::string str;
				
				recordKey = node.getRecordKey<HFSPlusCatalogKey>(i);
//                keyName = UnicodeString((char*)recordKey->nodeName.string, be(recordKey->nodeName.length)*2, g_utf16be, error);
//                keyName.toUTF8String(str);
                utf16BE_to_utf8(recordKey->nodeName, &str);
				
#ifdef DEBUG
				printf("Leaf Node(%4d,%4zd)  %s %s(%d)\n", nodeIndex, i, std::string(depth, ' ').c_str(), str.c_str(), be(recordKey->parentID));
//				std::cout << "dumpTree(l): " << std::string(depth, ' ') << str << "(" << be(recordKey->parentID) << ")\n";
#endif
			}
			
			break;
		}
		case NodeKind::kBTHeaderNode:
		case NodeKind::kBTMapNode:
			break;
		default:
			std::cerr << "Invalid node kind! Kind: " << int(node.kind()) << std::endl;
			
	}
}
#endif

void HFSCatalogBTree::replaceChars(std::string& str, char oldChar, char newChar)
{
	size_t pos = 0;

	while ((pos = str.find(oldChar, pos)) != std::string::npos)
	{
		str[pos] = newChar;
		pos++;
	}
}

