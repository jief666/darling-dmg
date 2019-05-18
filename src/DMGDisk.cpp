#include "DMGDisk.h"
#include <stdexcept>
#include "be.h"
#include <iostream>
#include <cstring>
#include <memory>
#include <sstream>
#include "DMGPartition.h"
#include "AppleDisk.h"
#include "GPTDisk.h"
#include "CachedReader.h"
#include "SubReader.h"
#include "exceptions.h"
#include "DarlingDMGCrypto.h" // for base64decode
#include "../../tinyxml2/tinyxml2.h"
#include "../../Utils.h"

using namespace tinyxml2;

static uint64_t findKolyBlock(std::shared_ptr<Reader> reader)
{
	if (reader->length() < 1024)
//		throw io_error("File too small");
		return 0;

	char buf[1024];
	reader->read(buf, sizeof(buf), reader->length() - 1024);
	for (uint32_t i = 0 ; i < 1024 - sizeof(UDIFResourceFile) + 1 ; i++) {
		if (strcmp("koly", buf+i) == 0)
			return reader->length() - 1024 + i;
	}
	return 0;
}

DMGDisk::DMGDisk(std::shared_ptr<Reader> reader)
	: m_reader(reader), m_zone(std::make_shared<CacheZone>(40000))
{
	uint64_t offset = findKolyBlock(m_reader);

	if (offset == 0)
		throw io_error("Cannot find koly block at the end");

	if (m_reader->read(&m_udif, sizeof(m_udif), offset) != sizeof(m_udif))
		throw io_error("Cannot read the KOLY block");

	if (be(m_udif.fUDIFSignature) != UDIF_SIGNATURE)
		throw io_error("Invalid KOLY block signature");
	
	loadKoly(m_udif);
}

DMGDisk::~DMGDisk()
{
}

bool DMGDisk::isDMG(std::shared_ptr<Reader> reader)
{
	uint64_t offset = findKolyBlock(reader);
	return offset != 0;
}

static XMLElement* getElementForKey(XMLNode* xmlElement, std::string key)
{
    XMLNode * xmlElementChild = xmlElement->FirstChild();
    while (xmlElementChild) {
        if ( strcmp(xmlElementChild->Value(), "key") == 0 )
        {
            if ( xmlElementChild->ToElement() ) {
                if ( xmlElementChild->ToElement()->GetText() == key ) {
                    XMLNode * xmlNodeChildNextSibling = xmlElementChild->NextSibling();
                    if ( xmlNodeChildNextSibling->ToElement() ) {
                        return xmlNodeChildNextSibling->ToElement();
                    }
                }
            }
        }
        xmlElementChild = xmlElementChild->NextSibling();
    }
    return NULL;
}

void DMGDisk::loadKoly(const UDIFResourceFile& koly)
{
	uint64_t offset, length;

	offset = be(koly.fUDIFXMLOffset);
	length = be(koly.fUDIFXMLLength);

    char* xmlData = (char*)alloca(length+1);
	m_reader->read(xmlData, length, offset);
    xmlData[length] = 0; // to be sure.

    try
    {
		XMLDocument tixmlDoc;
		XMLError tixmlError;
		tixmlError = tixmlDoc.Parse(xmlData);
		XMLNode * pRoot = tixmlDoc.FirstChild();
		if ( !pRoot ) return; // TODO error message ?
		XMLElement * pElementPList = tixmlDoc.FirstChildElement("plist");
		if ( !pElementPList ) return;
		XMLElement * pElementRootDict = pElementPList->FirstChildElement("dict");
		if ( !pElementRootDict ) return;
		XMLNode * pElementRootDictChildNote = pElementRootDict->FirstChild();
		while (pElementRootDictChildNote) {
			if ( pElementRootDictChildNote->Value() == std::string("key") ) {
				if ( !pElementRootDictChildNote->ToElement() ) return;
				if ( pElementRootDictChildNote->ToElement()->GetText() == std::string("resource-fork") ) {
					XMLElement * pElementRsrcDict = pElementRootDictChildNote->NextSiblingElement();
					if ( !pElementRsrcDict ) return;
					if ( pElementRsrcDict->Value() == std::string("dict") ) {
						// Iterate the rsrc dict. Should <key>blkx</key> + <array>...</array>
						XMLNode * pElementRsrcDictChild = pElementRsrcDict->FirstChild();
						while (pElementRsrcDictChild) {
							if ( pElementRsrcDictChild->Value() == std::string("key") ) {
								// We got a <key>, let's check it's a blkx
								if ( !pElementRsrcDictChild->ToElement() ) return;
								if ( pElementRsrcDictChild->ToElement()->GetText() == std::string("blkx") ) {
									// We found a blkx. Next sibling must be a <array>
									XMLElement * pElementBlkxArrayElement = pElementRsrcDictChild->NextSiblingElement();
									if ( !pElementBlkxArrayElement ) return;
									if ( pElementBlkxArrayElement->Value() == std::string("array") ) {
										// We got the <array>. It's an array of dict, so let's iterate.
										XMLNode * pElementBlkxArrayDict = pElementBlkxArrayElement->FirstChild();
										while (pElementBlkxArrayDict) {
											if ( pElementBlkxArrayDict->Value() == std::string("dict") ) {

												XMLElement* pElementBlkxName = getElementForKey(pElementBlkxArrayDict, "CFName");
												if ( !pElementBlkxName  ||  strlen(pElementBlkxName->GetText()) == 0 ) {
													pElementBlkxName = getElementForKey(pElementBlkxArrayDict, "Name");
												}
												if ( !pElementBlkxName  ||  strlen(pElementBlkxName->GetText()) == 0 ) {
													throw io_error("Invalid XML data, partition Name key not found");
												}

												XMLElement* pElementBlkxID = getElementForKey(pElementBlkxArrayDict, "ID");
												if ( !pElementBlkxID ) {
													throw io_error("Invalid XML data, partition ID key not found");
												}
												int partID = std::atoi(pElementBlkxID->GetText());
												Partition part;
												if ( parseNameAndType(pElementBlkxName->GetText(), part.name, part.type) )
												{
													if ( pElementBlkxName  &&  strlen(pElementBlkxName->GetText()) > 0 ) {
														XMLElement* pElementBlkxData = getElementForKey(pElementBlkxArrayDict, "Data");

														if ( strlen(pElementBlkxData->GetText()) > 0 ) {
															std::vector<uint8_t> data;
															base64Decode(std::string(pElementBlkxData->GetText()), data);
															BLKXTable* blkxTable = (BLKXTable*)data.data();
															part.offset = be(blkxTable->firstSectorNumber) * 512;
															part.size = be(blkxTable->sectorCount) * 512;
															m_partitions.push_back(part);
															m_partitionsBlkxTables.push_back(data);
															if ( m_partitionIDs.find(partID) != m_partitionIDs.end() ) {
																throw io_error("Invalid XML data, partition Name key not found");
															}
															m_partitionIDs.insert(std::make_pair(partID, m_partitions.size()-1));
														}
													}
												}else{
	#ifdef DEBUG
													fprintf(stderr, "Parsing name and type failed for : '%s'\n", pElementBlkxName->GetText());
	#endif
												}
											}
											pElementBlkxArrayDict = pElementBlkxArrayDict->NextSiblingElement();
										}
									}
								}
							}
							pElementRsrcDictChild = pElementRsrcDictChild->NextSiblingElement();
						}
					}
				}
			}
			pElementRootDictChildNote = pElementRootDictChildNote->NextSiblingElement();
		}
	}catch(...){
		throw io_error(stringPrintf("Unknow error while parsing xml for koly block"));
	}
    if ( m_partitions.empty() ) {
        throw io_error(stringPrintf("loadKoly : No partition found"));
    }
// Asian copies of OS X put crap UTF characters into XML data making type/name parsing unreliable.
// Jief : TODO I should install one and test.

}

bool DMGDisk::parseNameAndType(const std::string& nameAndType, std::string& name, std::string& type)
{
	// Format: "Apple (Apple_partition_map : 1)"
	size_t paren = nameAndType.find('(');
	size_t colon, space;

	if (paren == std::string::npos)
		return false;

	name = nameAndType.substr(0, paren-1);
	colon = nameAndType.find(':', paren);

	if (colon == std::string::npos)
		return false;

	type = nameAndType.substr(paren+1, (colon - paren) - 1);
	space = type.rfind(' ');
	
	if (space != std::string::npos && space == type.length()-1)
		type.resize(type.length() - 1); // remove space at the end
	
	return true;
}

std::shared_ptr<Reader> DMGDisk::readerForPartition(int index)
{
    if ( index >= m_partitions.size() )
        throw io_error(stringPrintf("readerForPartition : requested partition %d doesn't exist. Only %zd partition is this DMG", index, m_partitions.size()));


    auto table_ = m_partitionsBlkxTables[index];
    BLKXTable* table = (BLKXTable*)(table_.data());

//    if (be(table->firstSectorNumber)*512 == m_partitions[index].offset)
//    {
        std::stringstream partName;
        uint64_t l = m_reader->length();
        uint32_t data_offset = be(m_udif.fUDIFDataForkOffset);

        partName << "part-" << index;

        if (data_offset) {
            std::shared_ptr<Reader> r(new SubReader(m_reader,
                data_offset,
                m_reader->length() - data_offset));

            return std::shared_ptr<Reader>(
                    new CachedReader(std::shared_ptr<Reader>(new DMGPartition(r, table)), m_zone, partName.str())
                    );
        } else {
            return std::shared_ptr<Reader>(
                    new CachedReader(std::shared_ptr<Reader>(new DMGPartition(m_reader, table)), m_zone, partName.str())
                    );
        }
//    }
}

std::shared_ptr<Reader> DMGDisk::readerForKolyBlock(int partID)
{
    if ( m_partitionIDs.find(partID) == m_partitionIDs.end() )
        throw io_error(stringPrintf("readerForKolyBlock, partition ID %d not found", partID));

    BLKXTable* table = (BLKXTable*)m_partitionsBlkxTables[m_partitionIDs[partID]].data();
	if (!table)
		return nullptr;
	return std::shared_ptr<Reader>(new DMGPartition(m_reader, table));
}

