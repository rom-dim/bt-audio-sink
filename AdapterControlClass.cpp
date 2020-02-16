#include <cstring>
#include "AdapterControlClass.h"

using namespace std;

guint mediaPlayerAddId = g_signal_new("signal_media_player_add",
                                      G_TYPE_OBJECT, G_SIGNAL_RUN_FIRST,
                                      0, nullptr, nullptr,
                                      g_cclosure_marshal_generic,
                                      G_TYPE_NONE, 4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,G_TYPE_POINTER);

guint mediaPlayerRemoveId = g_signal_new("signal_media_player_remove",
                                         G_TYPE_OBJECT, G_SIGNAL_RUN_FIRST,
                                         0, nullptr, nullptr,
                                         g_cclosure_marshal_generic,
                                         G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_POINTER);

AdapterControlClass::AdapterControlClass(GDBusConnection *con, string path):con(con),path(path){
}

void AdapterControlClass::SetPowered(bool state){
    HelperSetProperty("Powered",g_variant_new("b", state));
}

void AdapterControlClass::SetDiscoverable(bool state){
    HelperSetProperty("Discoverable",g_variant_new("b", state));
}

void AdapterControlClass::SetPairable(bool state){
    HelperSetProperty("Pairable",g_variant_new("b", state));
}

void AdapterControlClass::SetAlias(string alias){
    HelperSetProperty("Alias",g_variant_new("s", alias.c_str()));
}

static AdapterControlClass *adapter;

void AdapterControlClass::bluez_device_appeared(GDBusConnection *sig,
                                                const gchar *sender_name,
                                                const gchar *object_path,
                                                const gchar *interface,
                                                const gchar *signal_name,
                                                GVariant *parameters,
                                                gpointer user_data)
{
    (void)sig;
    (void)sender_name;
    (void)object_path;
    (void)interface;
    (void)signal_name;
    (void)user_data;

    if(adapter == nullptr)
    {
        g_print("Error AdapterControlClass : adapter == nullptr\n");
        return;
    }

    GVariantIter *interfaces;
    const char *object;
    const gchar *interface_name;
    GVariant *properties;

    std::string path;
    std::string player;
    std::string player_type;
    bool found=false;

    g_variant_get(parameters, "(&oa{sa{sv}})", &object, &interfaces);
    while(g_variant_iter_next(interfaces, "{&s@a{sv}}", &interface_name, &properties)) {
        //g_print("IFace [ %s ]\n", interface_name);
        if(g_strstr_len(g_ascii_strdown(interface_name, -1), -1, "org.bluez.mediaitem1") ||
                g_strstr_len(g_ascii_strdown(interface_name, -1), -1, "org.bluez.mediaplayer1" )) {
            const gchar *property_name;
            GVariantIter i;
            GVariant *prop_val;
            g_variant_iter_init(&i, properties);

            while(g_variant_iter_next(&i, "{&sv}", &property_name, &prop_val))
            {
                const gchar *type = g_variant_get_type_string(prop_val);
                if(*type== 's'  || *type == 'o'){
                    if(strcmp(property_name,"Player") == 0)
                    {
                        player = std::string(g_variant_get_string(prop_val,nullptr));
                        path= std::string(object);
                    }
                    else if(strcmp(property_name,"Type") == 0)
                    {
                        player_type = std::string(g_variant_get_string(prop_val,nullptr));
                        found=true;
                    }
                    else if(strcmp(property_name,"Device") == 0)
                    {
                        player = std::string(g_variant_get_string(prop_val,nullptr));
                        path= std::string(object);
                        player_type= std::string("fallback");
                        found=true;
                    }
                }
            }
            g_variant_unref(prop_val);
        }
        g_variant_unref(properties);
    }
    if(found)
    {
        if(adapter != nullptr)
        {
            g_signal_emit(adapter->con, mediaPlayerAddId, 0,path.c_str(), player.c_str(),player_type.c_str(),user_data);
        }
    }

    return;
}

void AdapterControlClass::bluez_device_disappeared(GDBusConnection *sig,
                                                   const gchar *sender_name,
                                                   const gchar *object_path,
                                                   const gchar *interface,
                                                   const gchar *signal_name,
                                                   GVariant *parameters,
                                                   gpointer user_data)
{
    (void)sig;
    (void)sender_name;
    (void)object_path;
    (void)interface;
    (void)signal_name;

    GVariantIter *interfaces;
    const char *object;
    const gchar *interface_name;
    std::string player;
    bool found=false;

    g_variant_get(parameters, "(&oas)", &object, &interfaces);
    while(g_variant_iter_next(interfaces, "s", &interface_name)) {
        if(g_strstr_len(g_ascii_strdown(interface_name, -1), -1, "org.bluez.mediaitem1") ||
                g_strstr_len(g_ascii_strdown(interface_name, -1), -1, "org.bluez.mediaplayer1")) {

            player=string(object);
            found = true;
        }
    }

    if(adapter != nullptr)
    {
        g_signal_emit(adapter->con, mediaPlayerRemoveId, 0,player.c_str(),user_data);
    }
    return;
}


void AdapterControlClass::RegisterMainLoop(GMainLoop *loop){
    g_dbus_connection_signal_subscribe(con,
                                       "org.bluez",
                                       "org.freedesktop.DBus.ObjectManager",
                                       "InterfacesAdded",
                                       nullptr,
                                       nullptr,
                                       G_DBUS_SIGNAL_FLAGS_NONE,
                                       bluez_device_appeared,
                                       loop,
                                       nullptr);

    g_dbus_connection_signal_subscribe(con,
                                       "org.bluez",
                                       "org.freedesktop.DBus.ObjectManager",
                                       "InterfacesRemoved",
                                       nullptr,
                                       nullptr,
                                       G_DBUS_SIGNAL_FLAGS_NONE,
                                       bluez_device_disappeared,
                                       loop,
                                       nullptr);
    adapter = this;
}

int AdapterControlClass::Helper(std::string method){
    GVariant *result;
    GError *error = nullptr;

    result = g_dbus_connection_call_sync(con,
                                         "org.bluez",
                                         path.c_str(),
                                         "org.bluez.Adapter1",
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

int AdapterControlClass::HelperSetProperty(string prop, GVariant *value){
    GVariant *result;
    GError *error = nullptr;

    result = g_dbus_connection_call_sync(con,
                                         "org.bluez",
                                         path.c_str(),
                                         "org.freedesktop.DBus.Properties",
                                         "Set",
                                         g_variant_new("(ssv)", "org.bluez.Adapter1", prop.c_str(), value),
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
