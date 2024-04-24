
#include "project.h"


#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <iostream>
#include <stdint.h>




#define PROT_NONE       0
#define PROT_READ       1
#define PROT_WRITE      2
#define PROT_EXEC       4

#define MAP_FILE        0
#define MAP_SHARED      1
#define MAP_PRIVATE     2
#define MAP_TYPE        0xf
#define MAP_FIXED       0x10
#define MAP_ANONYMOUS   0x20
#define MAP_ANON        MAP_ANONYMOUS

#define MAP_FAILED      ((void *)-1)

/* Flags for msync. */
#define MS_ASYNC        1
#define MS_SYNC         2
#define MS_INVALIDATE   4

#if defined(_WIN32)
typedef int64_t OffsetType;
#else
#include <unistd.h>
typedef uint32_t OffsetType;
#endif

#if defined(_WIN32)
#include <windows.h>
#include <io.h>
#include <process.h>





static int __map_mman_error(const DWORD err, const int deferr)
{
	if (err == 0)
		return 0;
	//TODO: implement
	return err;
}

static DWORD __map_mmap_prot_page(const int prot)
{
	DWORD protect = 0;

	if (prot == PROT_NONE)
		return protect;

	if ((prot & PROT_EXEC) != 0)
	{
		protect = ((prot & PROT_WRITE) != 0) ?
			PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
	}
	else
	{
		protect = ((prot & PROT_WRITE) != 0) ?
			PAGE_READWRITE : PAGE_READONLY;
	}

	return protect;
}

static DWORD __map_mmap_prot_file(const int prot)
{
	DWORD desiredAccess = 0;

	if (prot == PROT_NONE)
		return desiredAccess;

	if ((prot & PROT_READ) != 0)
		desiredAccess |= FILE_MAP_READ;
	if ((prot & PROT_WRITE) != 0)
		desiredAccess |= FILE_MAP_WRITE;
	if ((prot & PROT_EXEC) != 0)
		desiredAccess |= FILE_MAP_EXECUTE;

	return desiredAccess;
}

void* mmap(void* addr, size_t len, int prot, int flags, int fildes, OffsetType off)
{
	HANDLE fm, h;

	void* map = MAP_FAILED;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4293)
#endif

	const DWORD dwFileOffsetLow = (sizeof(OffsetType) <= sizeof(DWORD)) ?
		(DWORD)off : (DWORD)(off & 0xFFFFFFFFL);
	const DWORD dwFileOffsetHigh = (sizeof(OffsetType) <= sizeof(DWORD)) ?
		(DWORD)0 : (DWORD)((off >> 32) & 0xFFFFFFFFL);
	const DWORD protect = __map_mmap_prot_page(prot);
	const DWORD desiredAccess = __map_mmap_prot_file(prot);

	const OffsetType maxSize = off + (OffsetType)len;

	const DWORD dwMaxSizeLow = (sizeof(OffsetType) <= sizeof(DWORD)) ?
		(DWORD)maxSize : (DWORD)(maxSize & 0xFFFFFFFFL);
	const DWORD dwMaxSizeHigh = (sizeof(OffsetType) <= sizeof(DWORD)) ?
		(DWORD)0 : (DWORD)((maxSize >> 32) & 0xFFFFFFFFL);

#ifdef _MSC_VER
#pragma warning(pop)
#endif

	errno = 0;

	if (len == 0
		/* Usupported protection combinations */
		|| prot == PROT_EXEC)
	{
		errno = EINVAL;
		return MAP_FAILED;
	}

	h = ((flags & MAP_ANONYMOUS) == 0) ?
		(HANDLE)_get_osfhandle(fildes) : INVALID_HANDLE_VALUE;

	if ((flags & MAP_ANONYMOUS) == 0 && h == INVALID_HANDLE_VALUE)
	{
		errno = EBADF;
		return MAP_FAILED;
	}

	fm = CreateFileMapping(h, nullptr, protect, dwMaxSizeHigh, dwMaxSizeLow, nullptr);

	if (fm == nullptr)
	{
		errno = __map_mman_error(GetLastError(), EPERM);
		return MAP_FAILED;
	}

	if ((flags & MAP_FIXED) == 0)
	{
		map = MapViewOfFile(fm, desiredAccess, dwFileOffsetHigh, dwFileOffsetLow, len);
	}
	else
	{
		map = MapViewOfFileEx(fm, desiredAccess, dwFileOffsetHigh, dwFileOffsetLow, len, addr);
	}

	CloseHandle(fm);

	if (map == nullptr)
	{
		errno = __map_mman_error(GetLastError(), EPERM);
		return MAP_FAILED;
	}

	return map;
}



int munmap(void* addr, size_t len)
{
	if (UnmapViewOfFile(addr))
		return 0;

	errno = __map_mman_error(GetLastError(), EPERM);

	return -1;
}

int _mprotect(void* addr, size_t len, int prot)
{
	DWORD newProtect = __map_mmap_prot_page(prot);
	DWORD oldProtect = 0;

	if (VirtualProtect(addr, len, newProtect, &oldProtect))
		return 0;

	errno = __map_mman_error(GetLastError(), EPERM);

	return -1;
}
#else
#include <sys/mman.h>
#include <sys/stat.h>
#endif


bool silly_mmap::open_m(const std::string& file, const int mode)
{

	int fd_mode = O_RDONLY;
	int mmap_mode = PROT_READ;
	/*switch (mode)
	{
	case open_mode::WRITE:
		mmap_mode = PROT_WRITE;
		fd_mode = O_WRONLY;
		break;
	case open_mode::RW:
		mmap_mode = PROT_READ | PROT_WRITE;
		fd_mode = O_RDWR;
		break;
	case open_mode::READ:
	default:
		break;
	}*/

#if defined(_WIN32)
	int fd = _open(file.c_str(), fd_mode);
	struct _stat64 stat;
	int ret = _fstati64(fd, &stat);
	m_mmap = (char*)mmap(NULL, stat.st_size, mmap_mode, MAP_SHARED, fd, 0);
#else
	int fd = open(file.c_str(), fd_mode);
	struct stat  stat;
	int ret = fstat(fd, &stat);
	m_mmap = (char*)mmap(NULL, stat.st_size, mmap_mode, MAP_SHARED, fd, 0);
#endif
	m_size = stat.st_size;
	close(fd);
	return (nullptr != m_mmap);
}

mmap_cur* silly_mmap::at(const size_t& offset)
{
	if (m_mmap && offset < m_size)
	{
		return m_mmap + offset;
	}
	return nullptr;
}

bool silly_mmap::read(mmap_cur* dst, const size_t& size, const size_t& offset)
{
	if (m_mmap && dst && size + offset < m_size)
	{
		memcpy(dst, m_mmap + offset, size);
		return true;
	}
	return false;
}

bool silly_mmap::write(mmap_cur* src, const size_t& size, const size_t& offset)
{
	// msync(m_mmap, m_size, MS_SYNC);
	return false;
}

void silly_mmap::close_m()
{
	if (m_mmap)
	{
		munmap(m_mmap, m_size);
		m_mmap = nullptr;
	}
}

silly_mmap::silly_mmap(const std::string)
{
}

silly_mmap::~silly_mmap()
{
	close_m();
}
