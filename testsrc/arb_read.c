
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include "memory_map.h"

void print_usage() {
    printf("Usage:\n"
           "  arb_read -a <address> [-p] [-n <length-of-read>]\n"
           "     '-p' will switch to physical memory read mode\n"
           "     length of read will be 256 by default\n"
           "\n"
          );
    return;
}

#define READ_BUFFER_SIZE 1024


int main (int argc, char *argv[]) {

    int fd = 0;
    size_t num_to_read = 256;
    int opt = 0;
    int positional_argument_ind = 0;
    char* read_addr_arg = NULL;
    bool do_physical_read = false;
    size_t addr = 0;
    char buf[READ_BUFFER_SIZE];
    memset(buf, 0, sizeof(buf));

    if (argc < 2){
        fprintf(stderr, "not enough arguments\n");
        print_usage();
        goto exit;
    }

    while ((opt = getopt(argc, argv, "a:pn:")) != -1) {
        switch (opt) {
            case 'p': {
                do_physical_read = true;
                fprintf(stderr, "doing physical read\n");
                break;
            }
            case 'n': {
                num_to_read = strtoull(optarg, NULL, 10);
                fprintf(stderr, "reading %ld\n", num_to_read);
                break;
            }
            case 'a': {
                addr = strtoull(optarg, NULL, 16);
                fprintf(stderr, "reading from 0x%lx\n", addr);
                break;
            }
            default:
                 break;
        }
    }

    /*
    if (addr == 0) {
        fprintf(stderr, "addr must be supplied\n");
        goto exit;
    }
    */

    /*
    if (argc > 2) {
        num_to_read = strtoull(argv[2], NULL, 10);
    }
    */

    /*
    char *buf = malloc(num_to_read);
    if (buf == NULL) {
        fprintf(stderr, "failed to malloc %d\n", errno);
        goto exit;
    }
    memset(buf, 0, num_to_read);
    */

    //size_t addr = (size_t)atoll(argv[1]);
    //char* read_addr_arg = argv[1];
    //size_t addr = strtoull(read_addr_arg, NULL, 16);
    //printf("arg %p\n", (void*)addr);

    fd = open("/dev/kmaps", O_RDWR);
    if (fd < 0){
        fprintf(stderr, "Failed to open device %d\n", errno);
        goto exit;
    }

    if (do_physical_read == true) {
        int ioctl_res = ioctl(fd, MEMORY_MAP_IOCTL_ACCESS_MODE_SET,
                ACCESS_MODE_PHYS);
        if (ioctl_res != 0) {
            fprintf(stderr, "ioctl failed %d\n", errno);
        }
    }

    off_t lseek_res = lseek(fd, (off_t)addr, SEEK_SET);
    if (lseek_res == -1) {
        fprintf(stderr, "lseek failed %d\n", errno);
        goto exit;
    }

    ssize_t nread = 0;
    ssize_t total_read = 0;
    while (total_read < num_to_read) {
        nread = read(fd, buf, sizeof(buf));
        if (nread < 0) {
            fprintf(stderr, "read failed %d\n", errno);
            goto exit;
        }
        write(1, buf, nread);
        total_read += nread;
    }
    //printf("nread %ld\n", nread);


exit:
    if (fd > 0) {
        close(fd);
    }
    return 0;
}
