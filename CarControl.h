#ifndef CARCONTROLLER_H
#define CARCONTROLLER_H

#include<string>
#include<functional>
#include<vector>
#include <linux/can.h>

using namespace std;

enum CarEventType { Button, RPM, Speed, WaterTemp, Gear, None };

struct __attribute__((__packed__)) SteeringWheelButtons{
    uint16_t Mobile: 1;
    uint16_t : 1;
    uint16_t Quieter: 1;
    uint16_t Louder: 1;
    uint16_t Down: 1;
    uint16_t Up: 1;
    uint16_t : 2;
    uint16_t Voice : 1;
    uint16_t : 1;
    uint16_t : 1;
    uint16_t : 1;
    uint16_t Star: 1;
    uint16_t : 1;
    uint16_t Diamond: 1;
    uint16_t : 1;
};

class CarController
{
public:
    typedef std::function<void(CarController*s, void * userdata)> Callback;
    typedef std::function<void(struct SteeringWheelButtons, void * userdata)> ButtonCallback;

    typedef std::function<string(char *)> ToStringCallback;
    struct ValueMapping{
        CarEventType type;
        canid_t can_id;
        char data[CAN_MAX_DLEN];
        ToStringCallback convertCallback;
    };

    CarController();
    void open(string port);
    void close();
    void run(void * userdata, bool* ready);
    void addHandler(Callback callback);
    void addButtonHandler(ButtonCallback callback);
    vector<struct ValueMapping> values;

private:
    int soc;
    string port;
    Callback fCallback;
    ButtonCallback buttonCallback;
};

#endif // CARCONTROLLER_H
