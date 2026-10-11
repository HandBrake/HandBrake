/* nr_transport.h — the frame transport, in one place.
 *
 * Upstream this project talks to the daemon over a Unix socket, which is what the
 * Linux build does (NR_TRANSPORT_UNIX below, the default there). On Windows the same
 * conversation has to happen over a named pipe: Winsock's AF_UNIX exists but cannot
 * bind here (measured: bind() fails WSAENETDOWN 10050), and CPython on Windows does
 * not expose socket.AF_UNIX at all, so the daemon side could not answer even if the
 * layer could ask. A named pipe is the transport both ends can reach.
 *
 * Only the transport is different. The bytes on the wire — 16-byte header, then the
 * colour, then the interface mask for a masked frame — are unchanged, so a daemon on
 * either platform speaks the same protocol.
 *
 * The pipe is opened in byte mode and read with PeekNamedPipe + a deadline, because a
 * named pipe has no SO_RCVTIMEO: a blocking ReadFile waits forever when the daemon has
 * nothing to say, which is what the Unix build's 60-second timeout exists to prevent.
 */
#ifndef NR_TRANSPORT_H
#define NR_TRANSPORT_H

#ifdef _WIN32

#include <windows.h>
#include <stdio.h>   /* snprintf, used by both arms */

/* One connected endpoint. `handle` is a pipe handle; `deadline` is the same
 * whole-exchange budget the Unix build gives SO_SNDTIMEO/SO_RCVTIMEO. */
typedef struct {
	HANDLE handle;
	DWORD deadline_ms;
} nr_link;

static int nr_link_connect(nr_link *link, const char *path)
{
	link->handle = INVALID_HANDLE_VALUE;
	link->deadline_ms = 60000;

	/* The daemon creates the pipe before it listens, so the first attempt can
	 * catch ERROR_FILE_NOT_FOUND while it is still starting. WaitNamedPipe does
	 * not distinguish "not yet" from "never", so this polls instead. */
	for (int attempt = 0; attempt < 100; attempt++) {
		HANDLE h = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
				       OPEN_EXISTING, 0, NULL);
		if (h != INVALID_HANDLE_VALUE) {
			DWORD mode = PIPE_READMODE_BYTE;
			SetNamedPipeHandleState(h, &mode, NULL, NULL);
			link->handle = h;
			return 0;
		}
		DWORD e = GetLastError();
		if (e != ERROR_PIPE_BUSY && e != ERROR_FILE_NOT_FOUND) return -1;
		Sleep(50);
	}
	return -1;
}

static void nr_link_close(nr_link *link)
{
	if (link->handle != INVALID_HANDLE_VALUE) CloseHandle(link->handle);
	link->handle = INVALID_HANDLE_VALUE;
}

/* A connection carried as an int, so callers that keep one across calls do not have to
 * name the platform type. Only ever round-tripped through the pair below. */
static inline int nr_link_handle(nr_link link)
{
	return (int)(intptr_t)link.handle;
}

static inline nr_link nr_link_from_handle(int fd)
{
	nr_link link;
	link.handle = (HANDLE)(intptr_t)fd;
	link.deadline_ms = 60000;
	return link;
}

/* Send exactly `count` bytes, or fail. The layer's three writes are small
 * (16-byte header, then the colour, then nothing) but the colour is a whole
 * frame, so this loops the way the Unix build's send() loop does. */
static int nr_link_write(nr_link *link, const void *data, size_t count)
{
	const unsigned char *p = data;
	DWORD budget = link->deadline_ms;
	while (count > 0) {
		DWORD chunk = (DWORD)(count > (1u << 20) ? (1u << 20) : count);
		DWORD written = 0;
		if (!WriteFile(link->handle, p, chunk, &written, NULL)) return -1;
		if (written == 0) return -1;
		p += written;
		count -= written;
		(void)budget;
	}
	return 0;
}

/* Read exactly `count` bytes within the deadline, or fail.
 *
 * PeekNamedPipe first: ReadFile on a byte-mode pipe returns whatever is there,
 * including zero bytes, so a single blocking ReadFile cannot express "wait until
 * count bytes exist, but not past the deadline" the way read() on a socket can. */
static int nr_link_read(nr_link *link, void *data, size_t count)
{
	unsigned char *p = data;
	DWORD waited = 0;
	while (count > 0) {
		DWORD available = 0;
		if (!PeekNamedPipe(link->handle, NULL, 0, NULL, &available, NULL))
			return -1;
		if (available == 0) {
			if (waited >= link->deadline_ms) return -1;
			Sleep(10);
			waited += 10;
			continue;
		}
		DWORD want = (DWORD)(count < available ? count : available);
		DWORD got = 0;
		if (!ReadFile(link->handle, p, want, &got, NULL)) return -1;
		if (got == 0) return -1;
		p += got;
		count -= got;
	}
	return 0;
}

#else  /* the Linux build, unchanged: a Unix socket */

#include <sys/socket.h>
#include <sys/types.h>   /* ssize_t, which <sys/socket.h> does not promise */
#include <sys/un.h>
#include <sys/time.h>
/* read(), write() and close() come from <unistd.h>; errno and EINTR from <errno.h>.
 * <stdio.h> is for snprintf below — this header is included before the includer's own
 * stdio in one of its two callers, so it cannot rely on theirs. */
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

typedef struct {
	int fd;
} nr_link;

static int nr_link_connect(nr_link *link, const char *path)
{
	link->fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (link->fd < 0) return -1;
	struct timeval timeout = { .tv_sec = 60 };
	if (setsockopt(link->fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout) ||
	    setsockopt(link->fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout)) {
		close(link->fd);
		link->fd = -1;
		return -1;
	}
	struct sockaddr_un address = { .sun_family = AF_UNIX };
	snprintf(address.sun_path, sizeof address.sun_path, "%s", path);
	if (connect(link->fd, (struct sockaddr *)&address, sizeof address) < 0) {
		close(link->fd);
		link->fd = -1;
		return -1;
	}
	return 0;
}

static void nr_link_close(nr_link *link)
{
	if (link->fd >= 0) close(link->fd);
	link->fd = -1;
}

/* A connection carried as an int, so callers that keep one across calls do not have to
 * name the platform type. Only ever round-tripped through the pair below. */
static inline int nr_link_handle(nr_link link)
{
	return link.fd;
}

static inline nr_link nr_link_from_handle(int fd)
{
	nr_link link;
	link.fd = fd;
	return link;
}

static int nr_link_write(nr_link *link, const void *data, size_t count)
{
	const unsigned char *p = data;
	while (count > 0) {
		ssize_t n = send(link->fd, p, count, MSG_NOSIGNAL);
		if (n < 0 && errno == EINTR) continue;
		if (n <= 0) return -1;
		p += n;
		count -= (size_t)n;
	}
	return 0;
}

static int nr_link_read(nr_link *link, void *data, size_t count)
{
	unsigned char *p = data;
	while (count > 0) {
		ssize_t n = read(link->fd, p, count);
		if (n < 0 && errno == EINTR) continue;
		if (n <= 0) return -1;
		p += n;
		count -= (size_t)n;
	}
	return 0;
}

#endif

#endif
