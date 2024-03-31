#include "SharedResource.h"
#include "IntStrConv.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <utils.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>

#define SR_ROOT_DIR "sr"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#ifdef __MINGW32__

typedef unsigned int uid_t;
uid_t geteuid() { return 1; }

// from /usr/include/asm-generic/fcntl.h

/* operations for bsd flock(), also used by the kernel implementation */
#define LOCK_SH		1	/* shared lock */
#define LOCK_EX		2	/* exclusive lock */
#define LOCK_NB		4	/* or'd with one of the above to prevent
				   blocking */
#define LOCK_UN		8	/* remove lock */

#define LOCK_MAND	32	/* This is a mandatory flock ... */
#define LOCK_READ	64	/* which allows concurrent read operations */
#define LOCK_WRITE	128	/* which allows concurrent write operations */
#define LOCK_RW		192	/* which allows concurrent read & write ops */


ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset) {
    off_t original_offset = lseek(fd, 0, SEEK_CUR);
    off_t new_offset = lseek(fd, offset, SEEK_SET);
    if (new_offset == -1) {
        return -1;
    }

    ssize_t bytes_written = write(fd, buf, count);

    off_t restore_offset = lseek(fd, original_offset, SEEK_SET);
    if (restore_offset == -1) {
        return -1;
    }

    return bytes_written;
}

ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
    off_t current_offset = lseek(fd, 0, SEEK_CUR);
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("lseek");
        return -1;
    }
    ssize_t bytes_read = read(fd, buf, count);
    lseek(fd, current_offset, SEEK_SET);
    return bytes_read;
}

int flock(int fd, int operation) {
    fprintf(stderr, "flock() function is not available on this system.\n");
    errno = ENOSYS;
    return -1;
}

#endif

bool SharedResource::sEnum(const char *group, std::vector<uint64_t> &ids) noexcept
{
	char buf[128];
	snprintf(buf, sizeof(buf) - 1, SR_ROOT_DIR"/%s", group);
	DIR *dir = nullptr;
	bool out = true;
	try {
		const std::string &path = InMyCache(buf);
		dir = opendir(path.c_str());
		if (!dir) {
			fprintf(stderr, "SharedResource: error %u enuming '%s'\n", errno, path.c_str());
			out = false;

		} else for (;;)  {
			struct dirent *de = readdir(dir);
			if (!de) {
				break;
			}
			if (IsHexaDecimalNumberStr(de->d_name)) {
				ids.emplace_back(strtoull(de->d_name, nullptr, 16));
			}
		}

	} catch (std::exception &e) {
		fprintf(stderr, "SharedResource: excpt '%s' enuming %s\n", e.what(), group);
		out = false;
	}

	if (dir) {
		closedir(dir);
	}

	return out;
}

bool SharedResource::sCleanup(const char *group, uint64_t id) noexcept
{
	char buf[128];
	snprintf(buf, sizeof(buf) - 1, SR_ROOT_DIR"/%s/%llx", group, (unsigned long long)id);
	try {
		const std::string &path = InMyCache(buf);
		if (unlink(path.c_str()) == 0) {
			return true;
		}
		fprintf(stderr, "SharedResource: error %u unlinking '%s'\n", errno, path.c_str());

	} catch (std::exception &e) {
		fprintf(stderr, "SharedResource: excpt '%s' unlinking %s/%llx\n", e.what(), group, (unsigned long long)id);
	}
	return false;
}


SharedResource::SharedResource(const char *group, uint64_t id) noexcept :
	_modify_id(0),
	_modify_counter(0),
	_fd(-1)
{
	char buf[128];
	snprintf(buf, sizeof(buf) - 1, SR_ROOT_DIR"/%s/%llx", group, (unsigned long long)id);
	try {
		_fd = open(InMyCache(buf).c_str(), O_CREAT | O_RDWR, 0640);
		if (_fd == -1) {
			perror("SharedResource: open");

		} else if (pread(_fd, &_modify_id, sizeof(_modify_id), 0) != sizeof(_modify_id)) {
			_modify_id = 0;
		}
	} catch (std::exception &e) {
		fprintf(stderr, "SharedResource: excpt %s opening %s/%llx\n", e.what(), group, (unsigned long long)id);
	}
}

SharedResource::~SharedResource()
{
	if (_fd != -1)
		close(_fd);
}

void SharedResource::GenerateModifyId() noexcept
{
	++_modify_counter;
	_modify_id = (uint64_t)(((uintptr_t)this) & 0xffff000) << 12;
	_modify_id^= getpid();
	_modify_id<<= 32;
	_modify_id^= (uint64_t)time(NULL);
	_modify_id+= _modify_counter;
}

bool SharedResource::Lock(int op, int timeout) noexcept
{
	if (_fd == -1)
		return false;

	bool out = false;
	if (timeout != -1) {
		for (const time_t ts = time(NULL);;) {
			out = flock(_fd, op | LOCK_NB) != -1;
			if (out)
				break;

			const time_t tn = time(NULL);
			if (tn - ts >= timeout) {
				fprintf(stderr,
					"SharedResource::Lock(%d, %d): timeout\n", op, timeout);
				break;
			}
			if (tn - ts > 1) {
				sleep(1);
			} else if (tn - ts == 1) {
				usleep(100000);
			} else
				usleep(10000);
		}
		
	} else {
		out = flock(_fd, op) != -1;
	}
	if (!out) {
		fprintf(stderr, "SharedResource::Lock(%d, %d): err=%d\n", op, timeout, errno);
	}
	return out;
}

bool SharedResource::LockRead(int timeout) noexcept
{
	return Lock(LOCK_SH, timeout);
}

bool SharedResource::LockWrite(int timeout) noexcept
{
	return Lock(LOCK_EX, timeout);
}

void SharedResource::UnlockRead() noexcept
{
	if (_fd != -1) {
		if (pread(_fd, &_modify_id, sizeof(_modify_id), 0) != sizeof(_modify_id)) {
			perror("SharedResource::UnlockRead: pread");
			_modify_id = 0;
		}
		flock(_fd, LOCK_UN);
	}
}

void SharedResource::UnlockWrite() noexcept
{
	if (_fd != -1) {
		GenerateModifyId();
		if (pwrite(_fd, &_modify_id, sizeof(_modify_id), 0) != sizeof(_modify_id)) {
			perror("SharedResource::UnlockWrite: pwrite");
			_modify_id = 0;
		}
		flock(_fd, LOCK_UN);
	}
}

bool SharedResource::IsModified() noexcept
{
	if (_fd == -1)
		return false;

	uint64_t modify_id = 0;
	if (pread(_fd, &modify_id, sizeof(modify_id), 0) != sizeof(modify_id)) {
//		perror("SharedResource::IsModified: pread");
		return false;
	}

	return modify_id != _modify_id;
}
