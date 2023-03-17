
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

int main (int argc, char *argv[]) {

    int fd = 0;
    size_t num_to_read = 256;
    if (argc < 2){
        printf("not enough arguments\n");
        goto exit;
    }

    if (argc > 2) {
        num_to_read = strtoull(argv[2], NULL, 10);
    }

    char *buf = malloc(num_to_read);
    if (buf == NULL) {
        printf("failed to malloc\n");
        goto exit;
    }
    memset(buf, 0, num_to_read);

    //size_t addr = (size_t)atoll(argv[1]);
    char* intarg = argv[1];
    size_t addr = strtoull(intarg, NULL, 16);
    //printf("arg %p\n", (void*)addr);

    fd = open("/dev/kmaps", O_RDWR);
    if (fd < 0){
        printf("Failed to open device %d\n", fd);
        goto exit;
    }
    off_t lseek_res = lseek(fd, (off_t)addr, SEEK_SET);
    if (lseek_res == -1) {
        printf("lseek failed %d\n", errno);
        goto exit;
    }

    ssize_t nread = read(fd, buf, num_to_read);
    if (nread < 0) {
        printf("read failed %d\n", errno);
        // goto exit;
    }
    //printf("nread %ld\n", nread);

    write(1, buf, nread);

exit:
    if (fd > 0) {
        close(fd);
    }
    return 0;
}
