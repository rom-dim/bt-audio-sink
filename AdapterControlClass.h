#ifndef ADAPTERCONTROLCLASS_H
#define ADAPTERCONTROLCLASS_H

#include <glib.h>
#include <gio/gio.h>
#include <string>

class AdapterControlClass
{
public:
    AdapterControlClass(GDBusConnection *con, std::string path);
    void SetPowered(bool state);
    void SetDiscoverable(bool state);
    void SetPairable(bool state);
    void SetAlias(std::string alias);

    void RegisterMainLoop(GMainLoop *loop);

   private:
    GDBusConnection *con;
    std::string path;

    int Helper(std::string method);
    int HelperSetProperty(std::string prop, GVariant *value);
    static void bluez_device_appeared(GDBusConnection *sig,
                                      const gchar *sender_name,
                                      const gchar *object_path,
                                      const gchar *interface,
                                      const gchar *signal_name,
                                      GVariant *parameters,
                                      gpointer user_data);
    static void bluez_device_disappeared(GDBusConnection *sig,
                                         const gchar *sender_name,
                                         const gchar *object_path,
                                         const gchar *interface,
                                         const gchar *signal_name,
                                         GVariant *parameters,
                                         gpointer user_data);
};

#endif // ADAPTERCONTROLCLASS_H
