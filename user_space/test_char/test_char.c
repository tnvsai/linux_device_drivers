#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE_PATH "/dev/sai_char"
#define BUFFER_SIZE 256

int main(int argc, char *argv[])
{
    int fd;
    char buffer[BUFFER_SIZE];

    if (argc < 2) {
        printf("Usage:\n");
        printf("  %s read\n", argv[0]);
        printf("  %s write <message>\n", argv[0]);
        return -1;
    }

    /* Open device */
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }

    /* READ operation */
    if (strcmp(argv[1], "read") == 0) {
        int ret = read(fd, buffer, BUFFER_SIZE);
        if (ret < 0) {
            perror("Read failed");
        } else {
            buffer[ret] = '\0';  // Null terminate for printing
            printf("Data from driver: %s\n", buffer);
        }
    }

    /* WRITE operation */
    else if (strcmp(argv[1], "write") == 0) {
        if (argc < 3) {
            printf("Error: No message provided for write\n");
        } else {
            int ret = write(fd, argv[2], strlen(argv[2]));
            if (ret < 0) {
                perror("Write failed");
            } else {
                printf("Wrote %d bytes to driver\n", ret);
            }
        }
    }

    else {
        printf("Invalid command. Use read or write.\n");
    }

    close(fd);
    return 0;
}
