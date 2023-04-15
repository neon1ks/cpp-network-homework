#if !defined(_GNU_SOURCE)
#   define _GNU_SOURCE
#endif

//
// System calls interceptor for the networking spoiling...
//

extern "C"
{
#include <dlfcn.h>
#include <unistd.h>
#include <sys/socket.h>
}

#include <cstdio>
#include <cstdlib>
#include <ctime>


static void init (void) __attribute__ ((constructor));

typedef ssize_t (*write_t)(int fd, const void *buf, size_t count);
typedef int (*socket_t)(int domain, int type, int protocol);
typedef int (*close_t)(int fd);

static close_t old_close;
static socket_t old_socket;
static write_t old_write;

static int socket_fd = -1;

using send_t = ssize_t (*)(int sockfd, const void *buf, size_t len, int flags);
using  sendto_t = ssize_t(*)(int sockfd, const void *buf, size_t len, int flags,
                             const struct sockaddr *dest_addr, socklen_t addrlen);
using  sendmsg_t = ssize_t(*)(int sockfd, const struct msghdr *msg, int flags);

static send_t old_send;
static sendto_t old_sendto;
static sendmsg_t old_sendmsg;

static FILE *log_file = nullptr;


void init(void)
{
    srand(time(nullptr));
    printf("Interceptor library loaded.\n");

    old_close = reinterpret_cast<close_t>(dlsym(RTLD_NEXT, "close"));
    old_write = reinterpret_cast<write_t>(dlsym(RTLD_NEXT, "write"));
    old_socket = reinterpret_cast<socket_t>(dlsym(RTLD_NEXT, "socket"));

    old_send = reinterpret_cast<send_t>(dlsym(RTLD_NEXT, "send"));
    old_sendto = reinterpret_cast<sendto_t>(dlsym(RTLD_NEXT, "sendto"));
    old_sendmsg = reinterpret_cast<sendmsg_t>(dlsym(RTLD_NEXT, "sendmsg"));
}


extern "C"
{

int close(int fd)
{
    if (fd == socket_fd)
    {
        printf("> close() on the socket was called!\n");
        socket_fd = -1;

        if (nullptr != log_file)
        {
            puts("Log file was closed");
            fclose(log_file);
        }
    }

    return old_close(fd);
}


ssize_t write(int fd, const void *buf, size_t count)
{
/*
    auto char_buf = reinterpret_cast<const char*>(buf);

    if (char_buf && (count > 1) && (fd == socket_fd))
    {
        printf("> write() on the socket was called with a string!\n");
        printf("New buffer = [");

        for (size_t i = 0; i < count - 1; ++i)
        {
            int r = rand();
            char *c = const_cast<char *>(char_buf) + i;

            // ASCII symbol.
            if (1 == r % count) *c = r % (0x7f - 0x20) + 0x20;

            putchar(*c);
        }
        printf("]\n");
    }
*/

    const auto *char_buf = reinterpret_cast<const char*>(buf);

    if ((nullptr != char_buf) && (count > 1) && (nullptr != log_file))
    {
        printf("> write() on the socket was called with a string!\n");

        for (size_t i = 0; i < count - 1; ++i)
        {
            const char c = char_buf[i];
            fprintf(log_file, "%c", c);
            putchar(c);
        }
    }

    return old_write(fd, buf, count);
}


int socket(int domain, int type, int protocol)
{
    int cur_socket_fd = old_socket(domain, type, protocol);

    if (-1 == socket_fd)
    {
        printf("> socket() was called, fd = %d!\n", cur_socket_fd);

        socket_fd = cur_socket_fd;

        const time_t now = time(nullptr);
        char filename[256];
        sprintf(filename, "/tmp/libcall-intercepter_log_file_%lu.txt", now);

        log_file = fopen(filename, "w");
        if (nullptr == log_file)
        {
            puts("Log file could not be opened");
        }
        else
        {
            puts("Log file was successfully opened");
        }
    }
    else
    {
        printf("> socket() was called, but socket was opened already...\n");
    }

    return cur_socket_fd;
}


ssize_t send(int sockfd, const void* buf, size_t len, int flags)
{
    printf("====replace send()\n");
    if (log_file == nullptr)
    {
        printf(" file is closed()\n");
    }

    const auto* char_buf = reinterpret_cast<const char*>(buf);

    if (char_buf && (len > 0) && log_file != nullptr)
    {
        for (size_t i = 0; i < len - 1; ++i)
        {
            const char c = char_buf[i];
            fprintf(log_file, "%c", c);
            putchar(c);
        }
    }

    return old_send(sockfd, buf, len, flags);
}


ssize_t sendto(int sockfd, const void* buf, size_t len, int flags, const struct sockaddr* dest_addr, socklen_t addrlen)
{
    printf("====replace old_sendto()\n");

    const auto* char_buf = reinterpret_cast<const char*>(buf);

    if (char_buf && (len > 1) && log_file != nullptr)
    {
        for (size_t i = 0; i < len - 1; ++i)
        {
            const char c = char_buf[i];
            fprintf(log_file, "%c", c);
            putchar(c);
        }
    }

    return old_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}


ssize_t sendmsg(int sockfd, const struct msghdr* msg, int flags)
{
    printf("====replace sendmsg()\n");

    for (size_t i = 0; i < msg->msg_iovlen; ++i)
    {

        const auto* char_buf = reinterpret_cast<const char*>(msg->msg_iov[i].iov_base);

        if (char_buf != nullptr && msg->msg_iov[i].iov_len > 1 && log_file != nullptr)
        {
            for (size_t i = 0; i < msg->msg_iov[i].iov_len - 1; ++i)
            {
                const char c = char_buf[i];
                fprintf(log_file, "%c", c);
                putchar(c);
            }
        }
    }

    return old_sendmsg(sockfd, msg, flags);
}

} // extern "C"
