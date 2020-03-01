#include "CarControl.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctime>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

CarController::CarController():soc(-1),port(""){
    ValueMapping tmp;
    tmp.can_id = 0x00AA;
    tmp.convertCallback = [](char* data){return to_string(*((uint16_t*)&data[4]) / 4);};
    tmp.type = CarEventType::RPM;
    values.push_back(tmp);

    tmp.can_id = 0x01D0;
    tmp.convertCallback = [](char* data){return to_string(data[0]-48);};
    tmp.type = CarEventType::WaterTemp;
    values.push_back(tmp);

    tmp.can_id = 0x01D2;
    tmp.convertCallback = [](char* data){
        string ret("");
        switch(data[0])
        {
        case 0xD2: ret += "R"; break;
        case 0xB4: ret += "N"; break;
        case 0x78: ret += "D"; break;
        case 0xE1: ret += "P"; break;
        }
        switch(data[1])
        {
        case 0x5C: ret += "1"; break;
        case 0x6C: ret += "2"; break;
        case 0x7C: ret += "3"; break;
        case 0x8C: ret += "4"; break;
        case 0x9C: ret += "5"; break;
        case 0xAC: ret += "6"; break;
        }
        return ret;
    };
    tmp.type = CarEventType::Gear;
    values.push_back(tmp);

    tmp.can_id = 0x01D6;
    tmp.convertCallback = [](char* data){return to_string(*((uint16_t*)data));};
    *((uint16_t*)tmp.data) = 0x00C0;
    tmp.type = CarEventType::Button;
    values.push_back(tmp);

    tmp.can_id = 0x01A6;
    tmp.convertCallback = [](char* data){return to_string(*((uint16_t*)data));};
    tmp.type = CarEventType::Speed;
    values.push_back(tmp);
}

void CarController::open(string port){
    struct ifreq ifr;
    struct sockaddr_can addr;

    //TODO check if port exist

    soc = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(soc < 0)
        throw "can open can socket";

    addr.can_family = AF_CAN;
    strcpy(ifr.ifr_name, port.c_str());

    if (ioctl(soc, SIOCGIFINDEX, &ifr) < 0)
        throw "can ioctl interface name is failed";

    addr.can_ifindex = ifr.ifr_ifindex;

    fcntl(soc, F_SETFL, O_NONBLOCK);

    if (bind(soc, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        throw "can bind socketadd is failed";
}

void CarController::close(){
    ::close(soc);
}

void CarController::addHandler(Callback c){
    fCallback=c;
}

void CarController::addButtonHandler(ButtonCallback c){
    buttonCallback=c;
}


void CarController::run(void * userdata, bool* shouldExit){
    struct can_frame frame_rd;
    int recvbytes = 0;
    clock_t begin = clock();

    while(!*shouldExit)
    {
        struct timeval timeout = {0, 100000};
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(soc, &readSet);

        while (select((soc + 1), &readSet, NULL, NULL, &timeout) < 0);

        if (FD_ISSET(soc, &readSet))
        {
            recvbytes = read(soc, &frame_rd, sizeof(struct can_frame));
            if(recvbytes)
            {
                for (ValueMapping& e : values){
                    if(frame_rd.can_id == e.can_id)
                    {
                        if (e.type == CarEventType::Button && *((uint16_t*)e.data) != *((uint16_t*)frame_rd.data)){
                            buttonCallback(*((struct SteeringWheelButtons*)frame_rd.data), userdata);
                        }
                        memcpy(e.data,frame_rd.data,CAN_MAX_DLEN);
                        break;
                    }
                }
            }
            if(fCallback != nullptr)
            {
                clock_t end = clock() ;
                double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC ;
                if(elapsed_secs > 0.1)
                {
                    begin = clock();
                    fCallback(this, userdata);
                }
            }
        }else{/*time out*/}
    }
}
