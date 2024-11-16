#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>

#define DEVICE_FILE "/dev/btnintp"

int main()
{
    int fd, ret;
    struct pollfd pfd;

    fd = open(DEVICE_FILE, O_RDONLY);
    if (fd < 0) {
        printf("Failed to open device file %s\n", DEVICE_FILE);
        return 1;
    }

    pfd.fd = fd;
    pfd.events = POLLIN;

    while (1) {
        printf("Waiting for button interrupt...\n");
        ret = poll(&pfd, 1, -1);
        if (ret > 0) {
            if (pfd.revents & POLLIN) {
                printf("Button interrupt detected!\n");
            }
        }
    }

    close(fd);
    return 0;
}