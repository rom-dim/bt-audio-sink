#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/stat.h>

#include "gpiolib.h"

int gpio_direction(int gpio, int dir)
{
    int ret = 0;
    char buf[50];
    sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio);
    int gpiofd = open(buf, O_WRONLY | O_SYNC);
    if(gpiofd < 0) {
        perror("Couldn't open direction file");
        ret = -1;
    }

    if(dir == 2 && gpiofd){
        if (4 != write(gpiofd, "high", 4)) {
            fprintf(stderr, "Couldn't set GPIO %d direction to out/high: %s\n",
                    gpio,
                    strerror(errno));
            ret = -2;
        }
    } else if(dir == 1 && gpiofd){
        if (3 != write(gpiofd, "out", 3)) {
            fprintf(stderr, "Couldn't set GPIO %d direction to out/low: %s\n",
                    gpio,
                    strerror(errno));
            ret = -3;
        }
    } else if(gpiofd) {
        if(2 != write(gpiofd, "in", 2)) {
            fprintf(stderr, "Couldn't set GPIO %d direction to in: %s\n",
                    gpio,
                    strerror(errno));
            ret = -4;
        }
    }

    close(gpiofd);
    return ret;
}

int gpio_setedge(int gpio, int rising, int falling)
{
    int ret = 0;
    char buf[50];
    sprintf(buf, "/sys/class/gpio/gpio%d/edge", gpio);
    int gpiofd = open(buf, O_WRONLY);
    if(gpiofd < 0) {
        perror("Couldn't open IRQ file");
        ret = -1;
    }

    if(gpiofd && rising && falling) {
        if(4 != write(gpiofd, "both", 4)) {
            perror("Failed to set IRQ to both falling & rising");
            ret = -2;
        }
    } else {
        if(rising && gpiofd) {
            if(6 != write(gpiofd, "rising", 6)) {
                perror("Failed to set IRQ to rising");
                ret = -2;
            }
        } else if(falling && gpiofd) {
            if(7 != write(gpiofd, "falling", 7)) {
                perror("Failed to set IRQ to falling");
                ret = -3;
            }
        }
    }

    return close(gpiofd);
}

int gpio_export(int gpio)
{
    int efd, ret;
    char buf[50];

    /* Quick test if it has already been exported */
    sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
    efd = open(buf, O_WRONLY);
    if(efd != -1) {
        close(efd);
        return 0;
    }

    efd = open("/sys/class/gpio/export", O_WRONLY | O_SYNC);

    if(efd != -1) {
        sprintf(buf, "%d", gpio);
        ret = write(efd, buf, strlen(buf));
        if(ret < 0) {
            perror("Export failed");
            return -2;
        }
        close(efd);
    } else {
        // If we can't open the export file, we probably
        // dont have any gpio permissions
        return -1;
    }
    return 0;
}

void gpio_unexport(int gpio)
{
    int gpiofd, ret;
    char buf[50];
    gpiofd = open("/sys/class/gpio/unexport", O_WRONLY | O_SYNC);
    sprintf(buf, "%d", gpio);
    ret = write(gpiofd, buf, strlen(buf));
    close(gpiofd);
}

int gpio_getfd(int gpio)
{
    char buf[50];
    int gpiofd;
    sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
    gpiofd = open(buf, O_RDWR | O_SYNC);
    if(gpiofd < 0) {
        fprintf(stderr, "Failed to open gpio %d value\n", gpio);
        perror("gpio failed");
    }

    return gpiofd;
}

int gpio_read(int gpio)
{
    char in[3] = {0, 0, 0};
    char buf[50];
    int nread, gpiofd;
    sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
    gpiofd = open(buf, O_RDWR | O_SYNC);
    if(gpiofd < 0) {
        fprintf(stderr, "Failed to open gpio %d value\n", gpio);
        perror("gpio failed");
    }

    do {
        nread = read(gpiofd, in, 1);
    } while (nread == 0);
    if(nread == -1){
        perror("GPIO Read failed");
        return -1;
    }

    close(gpiofd);
    return atoi(in);
}

int gpio_write(int gpio, int val)
{	
    char buf[50];
    int ret, gpiofd;
    sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
    gpiofd = open(buf, O_RDWR);
    if(gpiofd > 0) {
        snprintf(buf, 2, "%d", val ? 1 : 0);
        ret = write(gpiofd, buf, 2);
        if(ret < 0) {
            perror("failed to set gpio");
            return 1;
        }

        close(gpiofd);
        if(ret == 2) return 0;
    }
    return 1;
}

int gpio_select(int gpio)
{
    char gpio_irq[64];
    int ret = 0, buf, irqfd;
    fd_set fds;
    FD_ZERO(&fds);

    snprintf(gpio_irq, sizeof(gpio_irq), "/sys/class/gpio/gpio%d/value", gpio);
    irqfd = open(gpio_irq, O_RDONLY, S_IREAD);
    if(irqfd < 1) {
        perror("Couldn't open the value file");
        return -1;
    }

    // Read first since there is always an initial status
    ret = read(irqfd, &buf, sizeof(buf));

    while(1) {
        FD_SET(irqfd, &fds);
        ret = select(irqfd + 1, NULL, NULL, &fds, NULL);
        if(FD_ISSET(irqfd, &fds))
        {
            FD_CLR(irqfd, &fds);  //Remove the filedes from set
            // Clear the junk data in the IRQ file
            ret = read(irqfd, &buf, sizeof(buf));
            return 1;
        }
    }
}


int gpio_multi_select(int* gpios, size_t nr)
{
    char gpio_irq[64];
    int ret = 0, buf, irqfd[128], max=0;
    fd_set fds;
    FD_ZERO(&fds);

    for(size_t i = 0 ; i < nr ; i ++){
        snprintf(gpio_irq, sizeof(gpio_irq), "/sys/class/gpio/gpio%d/value", gpios[i]);
        irqfd[i] = open(gpio_irq, O_RDONLY, S_IREAD);
        if(irqfd[i] < 1) {
            perror("Couldn't open the value file");
            return -1;
        }
        ret = read(irqfd[i], &buf, sizeof(buf));
    }

    while(1) {
        for(size_t i = 0 ; i < nr ; i ++){
            FD_SET(irqfd[i], &fds);
            if(irqfd[i] > max)
                max = irqfd[i];
        }
        ret = select(max + 1, NULL, NULL, &fds, NULL);
        for(size_t i = 0 ; i < nr ; i ++){
            if(FD_ISSET(irqfd[i], &fds))
            {
                FD_CLR(irqfd[i], &fds);
                // Clear the junk data in the IRQ file
                ret = read(irqfd[i], &buf, sizeof(buf));
                return i;
            }
        }
    }
    return -1;
}
