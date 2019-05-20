#include "FileReader.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include "exceptions.h"

FileReader::FileReader(const std::string& path)
: m_fd(-1)
{
#ifdef DEBUG
    m_path = path;
#endif

	m_fd = open(path.c_str(), O_RDONLY + O_BINARY);

	if (m_fd == -1)
	{
#ifdef DEBUG
		std::cerr << "Cannot open " << path << ": " << strerror(errno) << std::endl;
#endif
		throw file_not_found_error(path);
	}
}

FileReader::~FileReader()
{
	if (m_fd != -1)
		close(m_fd);
}

int32_t FileReader::read(void* buf, int32_t count, uint64_t offset)
{
	if (m_fd == -1)
		return -1;
	
	return pread(m_fd, (char*)buf, count, offset);
}

uint64_t FileReader::length()
{
	return lseek(m_fd, 0, SEEK_END);
}
