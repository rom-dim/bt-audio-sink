#include "MediaControlClass.h"
using namespace std;

MediaPlayerClass::MediaPlayerClass(GDBusConnection *con, string path):con(con),path(path){
}

int MediaPlayerClass::Helper(std::string method){
    GVariant *result;
    GError *error = nullptr;

    result = g_dbus_connection_call_sync(con,
                                         "org.bluez",
                                         path.c_str(),
                                         "org.bluez.MediaPlayer1",
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

void MediaPlayerClass::Previous(){
    Helper("Previous");
}

void MediaPlayerClass::Play(){
    Helper("Play");
}

void MediaPlayerClass::Next(){
    Helper("Next");
}

void MediaPlayerClass::Stop(){
    Helper("Stop");
}

void MediaPlayerClass::Pause(){
    Helper("Pause");
}
