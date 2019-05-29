#ifndef HFSPLUS_H
#define HFSPLUS_H
#include <stdint.h>

#pragma pack(1)

#include "be.h"

typedef uint16_t unichar;
typedef uint32_t HFSCatalogNodeID;

#define HFSP_SIGNATURE 0x482b
#define HFSX_SIGNATURE 0x4858

#define HFSPLUS_S_IFMT   0170000	/* type of file mask */
#define HFSPLUS_S_IFIFO  0010000	/* named pipe (fifo) */
#define HFSPLUS_S_IFCHR  0020000	/* character special */
#define HFSPLUS_S_IFDIR  0040000	/* directory */
#define HFSPLUS_S_IFBLK  0060000	/* block special */
#define HFSPLUS_S_IFREG  0100000	/* regular */
#define HFSPLUS_S_IFLNK  0120000	/* symbolic link */
#define HFSPLUS_S_IFSOCK 0140000	/* socket */
#define HFSPLUS_S_IFWHT  0160000	/* whiteout */

#define HFS_PERM_OFLAG_COMPRESSED 0x20

struct HFSString
{
	be<uint16_t> length;
	unichar string[255];
};

struct HFSPlusBSDInfo
{
	be<uint32_t> ownerID;
	be<uint32_t> groupID;
	be<uint8_t> adminFlags;
	be<uint8_t> ownerFlags;
	be<uint16_t> fileMode;
	union
	{
		be<uint32_t> iNodeNum;
		be<uint32_t> linkCount;
		be<uint32_t> rawDevice;
	} special;
};

struct HFSPlusExtentDescriptor
{
	be<uint32_t> startBlock;
	be<uint32_t> blockCount;
};

struct HFSPlusForkData
{
	be<uint64_t> logicalSize;
	be<uint32_t> clumpSize;
	be<uint32_t> totalBlocks;
	HFSPlusExtentDescriptor extents[8];
};

struct HFSPlusVolumeHeader
{
	be<uint16_t> signature;
	be<uint16_t> version;
	be<uint32_t> attributes;
	be<uint32_t> lastMountedVersion;
	be<uint32_t> journalInfoBlock;
 
	be<uint32_t> createDate;
	be<uint32_t> modifyDate;
	be<uint32_t> backupDate;
	be<uint32_t> checkedDate;
 
	be<uint32_t> fileCount;
	be<uint32_t> folderCount;
 
	be<uint32_t> blockSize;
	be<uint32_t> totalBlocks;
	be<uint32_t> freeBlocks;
 
	be<uint32_t> nextAllocation;
	be<uint32_t> rsrcClumpSize;
	be<uint32_t> dataClumpSize;
	be<uint32_t> nextCatalogID;
 
	be<uint32_t> writeCount;
	be<uint64_t> encodingsBitmap;
 
	be<uint32_t> finderInfo[8];
 
	HFSPlusForkData allocationFile;
	HFSPlusForkData extentsFile;
	HFSPlusForkData catalogFile;
	HFSPlusForkData attributesFile;
	HFSPlusForkData startupFile;
};

enum class NodeKind : int8_t
{
	kBTLeafNode       = -1,
	kBTIndexNode      =  0,
	kBTHeaderNode     =  1,
	kBTMapNode        =  2
};

struct BTNodeDescriptor
{
	be<uint32_t> fLink;
	be<uint32_t> bLink;
	NodeKind kind;
	be<uint8_t> height;
	be<uint16_t> numRecords;
	be<uint16_t> reserved;
};

enum class KeyCompareType : uint8_t
{
	kHFSCaseFolding = 0xCF,
	kHFSBinaryCompare = 0xBC
};

struct BTHeaderRec
{
	be<uint16_t> treeDepth;
	be<uint32_t> rootNode;
	be<uint32_t> leafRecords;
	be<uint32_t> firstLeafNode;
	be<uint32_t> lastLeafNode;
	be<uint16_t> nodeSize;
	be<uint16_t> maxKeyLength;
	be<uint32_t> totalNodes;
	be<uint32_t> freeNodes;
	be<uint16_t> reserved1;
	be<uint32_t> clumpSize;	  // misaligned
	be<uint8_t> btreeType;
	KeyCompareType keyCompareType;
	be<uint32_t> attributes;	 // long aligned again
	be<uint32_t> reserved3[16];
};

enum : uint32_t
{
	kHFSNullID = 0,
	kHFSRootParentID = 1,
	kHFSRootFolderID = 2,
	kHFSExtentsFileID = 3,
	kHFSCatalogFileID = 4,
	kHFSBadBlockFileID = 5,
	kHFSAllocationFileID = 6,
	kHFSStartupFileID = 7,
	kHFSAttributesFileID = 8,
	kHFSRepairCatalogFileID = 14,
	kHFSBogusExtentFileID = 15,
	kHFSFirstUserCatalogNodeID = 16
};

struct HFSPlusCatalogKey
{
	be<uint16_t> keyLength;
	be<HFSCatalogNodeID> parentID;
	HFSString nodeName;
};

enum class RecordType : uint16_t
{
	kHFSPlusFolderRecord        = 0x0001,
	kHFSPlusFileRecord          = 0x0002,
	kHFSPlusFolderThreadRecord  = 0x0003,
	kHFSPlusFileThreadRecord    = 0x0004
};

struct Point
{
	be<int16_t> v, h;
};

struct Rect
{
	be<int16_t> top, left, bottom, right;
};

struct FileInfo
{
	be<uint32_t> fileType;
	be<uint32_t> fileCreator;
	be<uint16_t> finderFlags;
	Point location;
	be<uint16_t> reservedField;
};

//struct ExtendedFileInfo
//{
//    be<int16_t> reserved1[4];
//    be<uint16_t> extendedFinderFlags;
//    be<int16_t> reserved2;
//    be<int32_t> putAwayFolderID;
//};
// looking in Apple Source, this is the modern definition of ExtendedFileInfo
struct ExtendedFileInfo
{
	be<uint32_t> document_id;
	be<uint32_t> date_added;
	be<uint16_t> extended_flags;
	be<uint16_t> reserved2;
	be<uint32_t> write_gen_counter;
} __attribute__((aligned(2), packed));
static_assert(sizeof(ExtendedFileInfo) == 16, "sizeof(ExtendedFileInfo) != 16");

struct FolderInfo
{
	Rect windowBounds;
	be<uint16_t> finderFlags;
	Point location;
	be<uint16_t> reservedField;
};

//struct ExtendedFolderInfo
//{
//    Point scrollPosition;
//    be<int32_t> reserved1;
//    be<uint16_t> extendedFinderFlags;
//    be<int16_t> reserved2;
//    be<int32_t> putAwayFolderID;
//};
// looking in Apple Source, this is the modern definition of ExtendedFolderInfo
struct ExtendedFolderInfo
{
	be<uint32_t> document_id;
	be<uint32_t> date_added;
	be<uint16_t> extended_flags;
	be<uint16_t> reserved3;
	be<uint32_t> write_gen_counter;
} __attribute__((aligned(2), packed));

inline RecordType bswap_be_to_h(const RecordType &x) { return RecordType(be16toh((uint16_t)x)); }

struct HFSPlusCatalogFolder
{
	be<RecordType> recordType;
	be<uint16_t> flags;
	be<uint32_t> valence;
	be<HFSCatalogNodeID> folderID;
	be<uint32_t> createDate;
	be<uint32_t> contentModDate;
	be<uint32_t> attributeModDate;
	be<uint32_t> accessDate;
	be<uint32_t> backupDate;
	HFSPlusBSDInfo permissions;
	FolderInfo userInfo;
	ExtendedFolderInfo finderInfo;
	be<uint32_t> textEncoding;
	be<uint32_t> reserved;
};

struct HFSPlusCatalogFile
{
	be<RecordType> recordType;
	be<uint16_t> flags;
	be<uint32_t> reserved1;
	be<HFSCatalogNodeID> fileID;
	be<uint32_t> createDate;
	be<uint32_t> contentModDate;
	be<uint32_t> attributeModDate;
	be<uint32_t> accessDate;
	be<uint32_t> backupDate;
	HFSPlusBSDInfo permissions;
	FileInfo userInfo;
	ExtendedFileInfo finderInfo;
	be<uint32_t> textEncoding;
	be<uint32_t> reserved2;
 
	HFSPlusForkData dataFork;
	HFSPlusForkData resourceFork;
};

struct HFSPlusCatalogFileOrFolder
{
	union
	{
		HFSPlusCatalogFile file;
		HFSPlusCatalogFolder folder;
	};
};

struct HFSPlusCatalogThread
{
	be<RecordType> recordType;
	be<int16_t> reserved;
	be<HFSCatalogNodeID> parentID;
	HFSString nodeName;
};

struct HFSPlusExtentKey
{
	be<uint16_t> keyLength;
	be<uint8_t> forkType;
	be<uint8_t> pad;
	be<HFSCatalogNodeID> fileID;
	be<uint32_t> startBlock;
};

struct HFSPlusAttributeKey
{
	be<uint16_t> keyLength;
	be<uint16_t> padding;
	be<HFSCatalogNodeID> fileID;
	be<uint32_t> startBlock; // first allocation block number for extents
	be<uint16_t> attrNameLength;
	uint16_t attrName[127]; // TODO ?
};

enum
{
	kHFSPlusAttrInlineData  = 0x10,
	kHFSPlusAttrForkData    = 0x20,
	kHFSPlusAttrExtents     = 0x30
};

struct HFSPlusAttributeDataInline
{
	be<uint32_t> recordType; // kHFSPlusAttrInlineData
	be<uint64_t> reserved;
	be<uint32_t> attrSize;
	be<uint8_t> attrData[];
};

// File type and creator for symlink
enum {
	kSymLinkFileType  = 0x736C6E6B, /* 'slnk' */
	kSymLinkCreator   = 0x72686170  /* 'rhap' */
};

#pragma pack()

// File type and creator for hard link
enum {
	kHardLinkFileType = 0x686C6E6B,  /* 'hlnk' */
	kHFSPlusCreator   = 0x6866732B   /* 'hfs+' */
};

#endif

