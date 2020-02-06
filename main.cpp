#include "MediaControlClass.h"
#include "AdapterControlClass.h"

GDBusConnection *con;
std::string player;

void *player_thread(gpointer data) {
    MediaPlayerClass p(con,player);
    while(true)
    {
        g_print("Enter character: ");
        switch(getchar())
        {
        case 'q':  g_main_loop_quit((GMainLoop *)data); return 0;
        case 'p': p.Play(); break;
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
    if(strcmp(type,"audio") == 0)
    {
        g_print("StartMediaPlayer : %s\n",path);
        player =std::string(media);
        GThread *th_console;
        th_console = g_thread_new(path,player_thread,main_loop);
    }
}

void StopMediaPlayer(void* ,const char* path, gpointer main_loop){
    g_print("StopMediaPlayer : %s\n",path);
    g_print("TODO : Stop player thread for device %s\n",path);
}

int main(void)
{
    GMainLoop *loop;
    con = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
    if(con == NULL) {
        g_print("Not able to get connection to system bus\n");
        return 1;
    }

    loop = g_main_loop_new(NULL, FALSE);

    g_signal_connect(con, "signal_media_player_add", G_CALLBACK(StartMediaPlayer), NULL);
    g_signal_connect(con, "signal_media_player_remove", G_CALLBACK(StopMediaPlayer), NULL);

    AdapterControlClass adapter(con,"/org/bluez/hci0");

    adapter.RegisterMainLoop(loop);
    adapter.SetPairable(false);
    adapter.SetDiscoverable(false);
    adapter.SetPowered(false);

    adapter.SetPowered(true);
    adapter.SetDiscoverable(true);
    adapter.SetPairable(true);

    g_main_loop_run(loop);

    g_usleep(100);
    adapter.SetPairable(false);
    adapter.SetDiscoverable(false);
    adapter.SetPowered(false);

    return 0;
}
