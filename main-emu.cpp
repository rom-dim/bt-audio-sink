#include <string>
#include "gpiolib.h"
#include "CarControl.h"

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

void init_can(int *soc){
    struct ifreq ifr;
    struct sockaddr_can addr;
    *soc = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(*soc < 0)
        throw "can open can socket";

    addr.can_family = AF_CAN;
    strcpy(ifr.ifr_name, "vcan0");

    if (ioctl(*soc, SIOCGIFINDEX, &ifr) < 0)
        throw "can ioctl interface name is failed";

    addr.can_ifindex = ifr.ifr_ifindex;

    fcntl(*soc, F_SETFL, O_NONBLOCK);

    if (bind(*soc, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        throw "can bind socketadd is failed";
}

void init_gpios(int *gpios, size_t nr){
    for(size_t i=0 ;i < nr ;i++)
    {
        gpio_export(gpios[i]);
        gpio_direction(gpios[i], 0);
        gpio_setedge(gpios[i],1,1);
        printf("Wait for pin %d\n", gpios[i]);
    }
}

int main(void){
    int soc;
    struct can_frame frame_wr;
    init_can(&soc);
    frame_wr.can_id = 0x1D6;
    frame_wr.can_dlc = 2;

    int gpio_pin[] = {110, 112};
    size_t nr_of_pins = sizeof (gpio_pin)/sizeof(int);
    init_gpios(gpio_pin, nr_of_pins);

    struct SteeringWheelButtons button = {};
    uint16_t  b = 0x00C0;
    while(true)
    {
        int pin_index = gpio_multi_select(gpio_pin, nr_of_pins);
        if(pin_index >= 0)
        {
            switch(gpio_pin[pin_index])
            {
            case 110:   button.Up   = !button.Up;   break;
            case 112:   button.Down = !button.Down; break;
            default: break;
            }
            b = 0x00C0 | *((uint16_t*)&button);
            *((uint16_t*)frame_wr.data) = b;
            if(write(soc, &frame_wr, sizeof(struct can_frame)) !=  sizeof(struct can_frame))
                printf("send is failed\n");
        }
        else
            printf("pins are not ok\n");
    }
    return 0;
}
