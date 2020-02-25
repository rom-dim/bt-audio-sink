#include <cstring>
#include "MediaControlClass.h"
#include "AdapterControlClass.h"
#include "DeviceClass.h"

static GDBusConnection *con;
static std::string player;

GThread *th_console;

void *player_thread(gpointer data) {
    MediaPlayerClass p(con,player);
    while(true)
    {
        g_print("Enter character: ");
        switch(getchar())
        {
        case 'q':  g_main_loop_quit((GMainLoop *)data); return nullptr;
        case 'p': p.Play(); break;
        case 'a': p.Pause(); break;
        case 'n': p.Next(); break;
        case 'b': p.Previous(); break;
        case 's': p.Stop(); break;
        default: break;
        }
        g_print("\n");
    }
}

void StartMediaPlayer(void* ,const char* path,const char* media, const char* type, gpointer main_loop)
{
    g_print("registred path : %s\n",path);
    g_print("registred media : %s\n",media);
    g_print("registred type : %s\n",type);
    if(th_console != nullptr)
    {
        g_print("Only one player can be started\n");
        return;
    }

    if(strcmp(type,"audio") == 0)
    {
        g_print("StartMediaPlayer : %s\n",path);
        player =std::string(media);
        th_console = g_thread_new(path,player_thread,main_loop);
    }

    if(strcmp(type,"fallback") == 0)
    {
        g_print("StartMediaPlayer : %s\n",path);
        player =std::string(path);
        th_console = g_thread_new(path,player_thread,main_loop);
    }
}

void StopMediaPlayer(void* ,const char* path, gpointer /*main_loop*/){
    g_print("StopMediaPlayer : %s\n",path);
    g_print("TODO : Stop player thread for device %s\n",path);
}

#define AGENT_PATH  "/org/bluez/AutoPinAgent"

static int bluez_agent_call_method(const gchar *method, GVariant *param)
{
    GVariant *result;
    GError *error = nullptr;

    result = g_dbus_connection_call_sync(con,
                                         "org.bluez",
                                         "/org/bluez",
                                         "org.bluez.AgentManager1",
                                         method,
                                         param,
                                         nullptr,
                                         G_DBUS_CALL_FLAGS_NONE,
                                         -1,
                                         nullptr,
                                         &error);
    if(error != nullptr) {
        g_print("Register %s: %s\n", AGENT_PATH, error->message);
        return 1;
    }

    g_variant_unref(result);
    return 0;
}

static int bluez_register_autopair_agent(const char *cap)
{
    int rc;

    rc = bluez_agent_call_method("RegisterAgent", g_variant_new("(os)", AGENT_PATH, cap));
    if(rc)
        return 1;

    rc = bluez_agent_call_method("RequestDefaultAgent", g_variant_new("(o)", AGENT_PATH));
    if(rc) {
        bluez_agent_call_method("UnregisterAgent", g_variant_new("(o)", AGENT_PATH));
        return 1;
    }

    return 0;
}

static void bluez_signal_device_changed(GDBusConnection *conn,
                                        const gchar */*sender*/,
                                        const gchar *path,
                                        const gchar */*interface*/,
                                        const gchar *signal,
                                        GVariant *params,
                                        void */*userdata*/)
{
    GVariantIter *properties = nullptr;
    GVariantIter *unknown = nullptr;
    const char *iface;
    const char *key;
    GVariant *value = nullptr;
    const gchar *signature = g_variant_get_type_string(params);

    g_print("org.bluez.Device1 : %s\n", path);

    if(strcmp(signature, "(sa{sv}as)") != 0) {
        g_print("Invalid signature for %s: %s != %s", signal, signature, "(sa{sv}as)");
        goto done;
    }

    g_variant_get(params, "(&sa{sv}as)", &iface, &properties, &unknown);
    while(g_variant_iter_next(properties, "{&sv}", &key, &value)) {
        g_print("\tProperty: %s\n",key);
        if(g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN)) {
            g_print("\tValue :\"%s\"\n", g_variant_get_boolean(value) ? "true" : "false");
        }

        if(!g_strcmp0(key, "Connected")) {
            if(!g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN)) {
                g_print("Invalid argument type for %s: %s != %s", key,
                        g_variant_get_type_string(value), "b");
                goto done;
            }
            if(g_variant_get_boolean(value)){
                DeviceClass d(conn,path);
                d.SetTrusted(true);
            }


        }
    }
done:
    if(properties != nullptr)
        g_variant_iter_free(properties);
    if(value != nullptr)
        g_variant_unref(value);
}

static void bluez_adapter_changed(GDBusConnection */*conn*/,
                                  const gchar */*sender*/,
                                  const gchar *path,
                                  const gchar */*interface*/,
                                  const gchar *signal,
                                  GVariant *params,
                                  void */*userdata*/)
{
    GVariantIter *properties = nullptr;
    GVariantIter *unknown = nullptr;
    const char *iface;
    const char *key;
    GVariant *value = nullptr;
    const gchar *signature = g_variant_get_type_string(params);

    g_print("org.bluez.Adapter1 : %s\n", path);

    if(strcmp(signature, "(sa{sv}as)") != 0) {
        g_print("Invalid signature for %s: %s != %s", signal, signature, "(sa{sv}as)");
        goto done;
    }

    g_variant_get(params, "(&sa{sv}as)", &iface, &properties, &unknown);
    while(g_variant_iter_next(properties, "{&sv}", &key, &value)) {
        g_print("\tProperty: %s\n",key);
        if(g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN)) {
            g_print("\tValue :\"%s\"\n", g_variant_get_boolean(value) ? "true" : "false");
        }
    }
done:
    if(properties != nullptr)
        g_variant_iter_free(properties);
    if(value != nullptr)
        g_variant_unref(value);
}

int main(void)
{
    GMainLoop *loop;
    int rc;

    con = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, nullptr);
    if(con == nullptr) {
        g_print("Not able to get connection to system bus\n");
        return 1;
    }

    loop = g_main_loop_new(nullptr, FALSE);

    g_signal_connect(con, "signal_media_player_add", G_CALLBACK(StartMediaPlayer), nullptr);
    g_signal_connect(con, "signal_media_player_remove", G_CALLBACK(StopMediaPlayer), nullptr);

    g_print("Create adapter\n");
    AdapterControlClass adapter(con,"/org/bluez/hci0");

    adapter.RegisterMainLoop(loop);
    adapter.SetPairable(false);
    adapter.SetDiscoverable(false);
    adapter.SetPowered(false);
    g_print("Power off\n");
    g_usleep(1000000);
    g_print("Power on\n");
    adapter.SetPowered(true);
    adapter.SetAlias("bt-audio-sink");
    adapter.SetDiscoverable(true);
    adapter.SetPairable(true);


    g_dbus_connection_signal_subscribe(con,
                                       "org.bluez",
                                       "org.freedesktop.DBus.Properties",
                                       "PropertiesChanged",
                                       nullptr,
                                       "org.bluez.Device1",
                                       G_DBUS_SIGNAL_FLAGS_NONE,
                                       bluez_signal_device_changed,
                                       nullptr,
                                       nullptr);

    g_dbus_connection_signal_subscribe(con,
                                       "org.bluez",
                                       "org.freedesktop.DBus.Properties",
                                       "PropertiesChanged",
                                       nullptr,
                                       "org.bluez.Adapter1",
                                       G_DBUS_SIGNAL_FLAGS_NONE,
                                       bluez_adapter_changed,
                                       nullptr,
                                       nullptr);

    rc = bluez_register_autopair_agent("NoInputNoOutput");
    if(rc) {
        g_print("Not able to register default autopair agent\n");
        return 1;
    }

    g_print("Start main loop\n");
    g_main_loop_run(loop);

    g_usleep(100);
    adapter.SetPairable(false);
    adapter.SetDiscoverable(false);
    adapter.SetPowered(false);

    return 0;
}
