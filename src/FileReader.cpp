#include "FileReader.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <algorithm> // for std::replace
#include "exceptions.h"
#include "../../Utils.h"

#ifdef _MSC_VER
#include "Windows.h"
#endif

#ifdef _MSC_VER

std::string GetLastErrorAsString(DWORD errorMessageID)
{
	if (errorMessageID == 0)
		return std::string(); //No error message has been recorded

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	std::string message(messageBuffer, size);
	std::replace(message.begin(), message.end(), '\r', ' ');
	std::replace(message.begin(), message.end(), '\n', ' ');

	//Free the buffer.
	LocalFree(messageBuffer);

	return message;
}

#endif


FileReader::FileReader(const std::string& path)
: m_fd(-1)
{
#ifdef DEBUG
    m_path = path;
#endif

#ifdef _MSC_VER

//I got this code here : https://stackoverflow.com/questions/15004984/how-to-write-an-image-containing-multiple-partitions-to-a-usb-flash-drive-on-win/15013624#15013624

	m_fdw = CreateFile
	(
		//"\\\\.\\PhysicalDrive1",
		path.c_str(),
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING,
		NULL
	);
	if (m_fdw == INVALID_HANDLE_VALUE)
	{
		DWORD err = GetLastError();
		throw file_not_found_error(stringPrintf("Cannot open '%s'. Error %s (%d)\n", path.c_str(), GetLastErrorAsString(err).c_str(), err));
	}

	DWORD bytes_to_transfer, byte_count;
	DISK_GEOMETRY source_diskgeometry;

// Locking seems no needed
	//if (!DeviceIoControl
	//(
	//	m_fdw,
	//	FSCTL_LOCK_VOLUME,
	//	NULL,
	//	0,
	//	NULL,
	//	0,
	//	&byte_count,
	//	NULL
	//))
	//{
	//	DWORD err = GetLastError();
	//	fprintf(stderr, "Error %u locking input volume. Ok if itùs not a phys volume.\n", err);
	//}

	if (!DeviceIoControl
	(
		m_fdw,
		IOCTL_DISK_GET_DRIVE_GEOMETRY,
		NULL,
		0,
		&source_diskgeometry,
		sizeof(source_diskgeometry),
		&byte_count,
		NULL
	))
	{
		DWORD err = GetLastError();
		std::string errAsString = GetLastErrorAsString(err);
		//fprintf(stderr, "Error %u getting device geometry.\n", err);

		LARGE_INTEGER newoffset;
		if ( GetFileSizeEx(m_fdw, &newoffset) == 0 ) {
			throw io_error(stringPrintf("Cannot get file size of '%s'. Error %s (%d). Ok it's not a physical disk\n", path.c_str(), GetLastErrorAsString(err).c_str(), err));
		}
		len = newoffset.QuadPart;

		// Todo : instead of this FileReader, we should have different class (like WindowsFileReader, WindowsPhysicalFileReader, UnixFileReader) and use a factory to get the reader instance.
		// That would remove the need for m_isPhys...
		m_isPhys = false;

		return;
	}

	m_isPhys = true;

	GET_LENGTH_INFORMATION source_disklength;

	if (!DeviceIoControl
	(
		m_fdw,
		IOCTL_DISK_GET_LENGTH_INFO,
		NULL,
		0,
		&source_disklength,
		sizeof(source_disklength),
		&byte_count,
		NULL
	))
	{
		DWORD err = GetLastError();
		throw io_error(stringPrintf("Cannot get disk size for '%s'. Error %s (%d). Ok it's not a physical disk\n", path.c_str(), GetLastErrorAsString(err).c_str(), err));
	}

#ifdef DEBUG
	printf("\nInput disk has %I64i bytes.\n\n", source_disklength.Length.QuadPart);
#endif
	len = source_disklength.Length.QuadPart;

	printf("");


#else
	m_fd = open(path.c_str(), O_RDONLY + O_BINARY);
	if (m_fd == -1)
	{
		int errnoSaved = errno;
#ifdef DEBUG
		std::cerr << "Cannot open " << path << ": " << strerror(errnoSaved) << std::endl;
#endif
		char* errnoAsString = strerror(errnoSaved);
		std::string extractedExpr = stringPrintf("Cannot open '%s'. Error : %s (%d)", path.c_str(), errnoAsString, errnoSaved);
		throw file_not_found_error(extractedExpr);
	}
	uint64_t rv = lseek(m_fd, 0, SEEK_END);
	if ( rv == -1 ) throw file_not_found_error(stringPrintf("Cannot lseek to end in file '%s'", path.c_str()));
	len = rv;

#endif
//	m_fd = open("\\\\.\\PhysicalDrive1", O_RDONLY + O_BINARY);

}
FileReader::~FileReader()
{
	if (m_fd != -1)
		close(m_fd);
}

int32_t FileReader::read(void* buf, int32_t count, uint64_t offset)
{
	assert(count < INT32_MAX);

	int32_t nbreadTotal = 0;
	int32_t nbread = 0;
	while (nbreadTotal < count) {
		nbread = read_(buf, count - nbreadTotal, offset + nbreadTotal);
		if (nbread < 0) return -1;
		nbreadTotal += nbread;
		if (nbread == 0) return nbreadTotal;
	}
	return nbreadTotal;
}



int32_t FileReader::read_(void* buf, int32_t count, uint64_t offset)
{

#ifdef _MSC_VER

	if (m_isPhys) {

		uint64_t newOffset = (offset & ~511);

		LARGE_INTEGER li;
		li.QuadPart = newOffset;
		if (!SetFilePointerEx(m_fdw, li, NULL, FILE_BEGIN)) {
			return -1;
		}

		if (newOffset < offset) {
			// we're not starting aligned
			char buf512[512];
			DWORD nbread;
			if (!ReadFile(m_fdw, buf512, 512, &nbread, NULL)) {
				return -1;
			}
			if (nbread < (offset - newOffset)) {
				return 0;
			}
			nbread -= (offset - newOffset); // nb bytes read from offset position (not from newOffset)
			uint64_t rv = MIN(nbread, count);
			memcpy(buf, buf512 + offset - newOffset, rv);
			return rv;
		}

		uint64_t newCount = (count & ~511);
		if ( newCount == 0  &&  count > 0 )
		{
			// count is less than 512 (but > 0)
			char buf512[512];
			DWORD nbread;
			if (!ReadFile(m_fdw, buf512, 512, &nbread, NULL)) {
				return -1;
			}
			uint64_t rv = MIN(nbread, count);
			memcpy(buf, buf512 + offset - newOffset, rv);
			return rv;
		}
		else
		{
			DWORD nbread;
			if (!ReadFile(m_fdw, buf, newCount, &nbread, NULL)) {
				return -1;
			}
			return nbread;
		}
	}
	else {
		LARGE_INTEGER li;
		li.QuadPart = offset;
		if (!SetFilePointerEx(m_fdw, li, NULL, FILE_BEGIN)) {
			return -1;
		}
		DWORD nbread;
		if (!ReadFile(m_fdw, buf, count, &nbread, NULL)) {
			return -1;
		}
		return nbread;

	}
#else
	return pread(m_fd, (char*)buf, count, offset);
#endif
}
