#ifndef DEVICECLASS_H
#define DEVICECLASS_H

#include <glib.h>
#include <gio/gio.h>
#include <string>

class DeviceClass
{
public:
    DeviceClass(GDBusConnection *con, std::string path);
    void Connect();
    void Disconnect();
    void Pair();
    void CancelPairing();
    void SetTrusted(bool state);

private:
    GDBusConnection *con;
    std::string path;
    int Helper(std::string method);
    int HelperSetProperty(std::string prop, GVariant *value);
};

#endif // DEVICECLASS_H
