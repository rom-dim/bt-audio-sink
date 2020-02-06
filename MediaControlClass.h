#ifndef MEDIACONTROLCLASS_H
#define MEDIACONTROLCLASS_H

#include <glib.h>
#include <gio/gio.h>
#include <string>

class MediaPlayerClass
{
public:
    MediaPlayerClass(GDBusConnection *con, std::string path);
    void Previous();
    void Play();
    void Next();
    void Stop();

private:
    GDBusConnection *con;
    std::string path;
    int Helper(std::string method);
};

#endif // MEDIACONTROLCLASS_H
