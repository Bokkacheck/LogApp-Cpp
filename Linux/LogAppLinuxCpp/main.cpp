#include <iostream>
#include <string>
#include <unistd.h>
#include <fstream>
#include <stdio.h>
#include <string>
#include <sstream>
#include <set>
#include <time.h>
#include <pthread.h>
#include <future>
#include <csignal>

#define SIGTERM 15
#define SIGABRT 6
#define SIGKILL 9

using namespace std;

static ofstream logAppFile;
static ofstream logCameraFile;
static set<string> allApps;
static string user;
static volatile bool work = true;

void* CheckForApps(void *arg);
void* CheckForDevices(void *arg);
string ExecuteCommand(char *cmd);
string DateTime();

void OnExit(){
    logAppFile << "USER_LOGOUT:-:" << user << endl;
    logAppFile.close();
    logCameraFile.close();
    work = false;
}

void SignalHandler( int signum ) {
    OnExit();
}

int main()
{
    atexit(&OnExit);
    signal(SIGTERM,SignalHandler);
    signal(SIGTERM,SignalHandler);
    signal(SIGABRT,SignalHandler);
    signal(SIGKILL,SignalHandler);

    char username[256];
    int size = getlogin_r(username, 256);
    user = username;
    string pathAppFile = "/home/" + user + "/log-app";
    string pathCameraFile = "/home/" + user + "/log-camera";
    logAppFile.open(pathAppFile.c_str(), std::ios_base::app);
    logCameraFile.open(pathCameraFile, std::ios_base::app);
    logAppFile << "USER_LOGIN:-:" << user << endl;

    pthread_t checkForDevices;
    pthread_create(&checkForDevices, NULL, CheckForDevices, NULL);
    pthread_t checkForApps;
    pthread_create(&checkForApps, NULL, CheckForApps, NULL);

    pthread_join(checkForDevices, NULL);
    pthread_join(checkForApps, NULL);
    logAppFile.close();
    logCameraFile.close();
    return 0;
}

void *CheckForApps(void *arg)
{
    char *cmd = "wmctrl -l|awk '{$3=\"\"; $2=\"\"; $1=\"\"; print $0}'"; //get all active apps, first install wmctrl with sudo apt install wmctrl
    while (work)
    {
        string result = ExecuteCommand(cmd);
        if (result == "")
        {
            continue;
        }
        std::stringstream ssRes(result);
        set<string> activeApps;
        string tmp;
        while (std::getline(ssRes, tmp, '\n')) //make set of active apps
        {
            if (allApps.find(tmp) == allApps.end())
            { //if it wasn't active, log it and save in allApps set
                allApps.insert(tmp);
                string txt = user + ":-:START:-:" + tmp + ":-:" + DateTime() + "\n";
                logAppFile << txt;
                logAppFile.flush();
            }
            activeApps.insert(tmp); //add to active apps
        }
        for (auto app : allApps)
        { //If some app from allApps is no more active, remove it from allApps
            if (activeApps.find(app) == activeApps.end())
            {
                allApps.erase(app);
                string txt = user + ":-:START:-:" + tmp + ":-:" + DateTime() + "\n";
                logAppFile << txt;
                logAppFile.flush();
            }
        }
        this_thread::sleep_for(chrono::milliseconds(200)); //Every 200ms check for action
    }
    return NULL;
}

void* CheckForDevices(void *)
{
    bool camera = false;
    bool mic = false;
    while (work)
    {
        string result = ExecuteCommand("lsof /dev/video0");
        if (result == "")       //No process which use camera
        {
            if (camera)
            {
                logCameraFile << user << ":-:CAMERA_DISABLED:-:" << DateTime() << endl;
            }
            camera = false;
        }
        else        //Some process use camera
        {
            if (!camera)
            {
                logCameraFile << user << ":-:CAMERA_ACTIVATED:-:" << DateTime() << endl;
            }
            camera = true;
        }

        result = ExecuteCommand("lsof /dev/snd/pcmC0D0p");
        cout<<result<<endl;
        if (result == "")       //No process which use microphone
        {

            if (mic)
            {
                logCameraFile << user << ":-:MIC_DISABLED:-:" << DateTime() << endl;
            }
            mic = false;
        }
        else        //Some process use microphone
        {
            if (!mic)
            {
                logCameraFile << user << ":-:MIC_ACTIVATED:-:" << DateTime() << endl;
            }
            mic = true;
        }
        this_thread::sleep_for(chrono::milliseconds(3000));
    }
    return NULL;
}

string ExecuteCommand(char *cmd)
{
    char buf[256];
    FILE *fp;
    string result = "";
    if ((fp = popen(cmd, "r")) == NULL)
    {
        return "";
    }
    while (fgets(buf, 256, fp) != NULL)
    {
        result += buf;
    }
    pclose(fp);
    return result;
}

string DateTime()
{
    chrono::system_clock::time_point p = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(p);
    char str[26];
    ctime_r(&t, str);
    return str;
}