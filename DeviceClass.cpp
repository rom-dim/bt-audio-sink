#include "DeviceClass.h"
using namespace std;

DeviceClass::DeviceClass(GDBusConnection *con, string path):con(con),path(path){
}

int DeviceClass::Helper(std::string method){
    GVariant *result;
    GError *error = nullptr;

    result = g_dbus_connection_call_sync(con,
                                         "org.bluez",
                                         path.c_str(),
                                         "org.bluez.Device1",
                                         method.c_str(),
                                         nullptr,
                                         nullptr,
                                         G_DBUS_CALL_FLAGS_NONE,
                                         -1,
                                         nullptr,
                                         &error);
    if(error != nullptr){
        g_print("Error %s\n", error->message);
        return 1;
    }

    g_variant_unref(result);
    return 0;
}

int DeviceClass::HelperSetProperty(string prop, GVariant *value){
    GVariant *result;
    GError *error = nullptr;

    result = g_dbus_connection_call_sync(con,
                                         "org.bluez",
                                         path.c_str(),
                                         "org.freedesktop.DBus.Properties",
                                         "Set",
                                         g_variant_new("(ssv)", "org.bluez.Device1", prop.c_str(), value),
                                         nullptr,
                                         G_DBUS_CALL_FLAGS_NONE,
                                         -1,
                                         nullptr,
                                         &error);
    if(error != nullptr){
        g_print("Error %s\n", error->message);
        return 1;
    }

    g_variant_unref(result);
    return 0;
}


void DeviceClass::Connect(){
    Helper("Connect");
}

void DeviceClass::Disconnect(){
    Helper("Disconnect");
}

void DeviceClass::Pair(){
    Helper("Pair");
}

void DeviceClass::CancelPairing(){
    Helper("CancelPairing");
}

void DeviceClass::SetTrusted(bool state){
    g_print("SetTrusted for %s\n", path.c_str());
    HelperSetProperty("Trusted",g_variant_new("b", state));
}
