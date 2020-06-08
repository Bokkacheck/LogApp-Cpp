#include <Lmcons.h>
#include <windows.h>
#include<vfw.h>
#include <stdio.h>
#include<iostream>
#include<thread>
#include <fstream>
#include<future>
#include<csignal>

#pragma comment(lib,"Vfw32.lib")
#pragma comment(lib,"Winmm.lib")

using namespace std;

string DateTime();
void OnExit();
void SignalHandler(int signum);
void CloseVideoSourceDialog();
void CheckForCamera();
void CheckForApplication();

static char username[UNLEN + 1];
static ofstream logAppFile;
static ofstream logOpenFile;
static ofstream logCameraFile;
static volatile bool camera = true;
static volatile bool work = true;
static HWND m_hCapWnd;

LRESULT CALLBACK WndProc(HWND   hWnd, UINT   message, WPARAM wParam, LPARAM lParam){
    switch (message)
    {
        case WM_CLOSE || WM_DESTROY || WM_QUIT: 
            OnExit();
            DefWindowProc(hWnd, message, wParam, lParam);
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
            break;
    }
    return 0;
}
HINSTANCE hInst;

INT __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
    CreateDirectory("C:\\logapp", NULL);
    logAppFile.open("C:\\logapp\\log-app.txt", std::ios_base::app);
    logOpenFile.open("C:\\logapp\\log-open-log-file.txt", std::ios_base::app);
    logCameraFile.open("C:\\logapp\\log-camera.txt", std::ios_base::app);

    atexit(&OnExit);
    signal(SIGTERM, SignalHandler);
    signal(SIGABRT, SignalHandler);

    DWORD username_len = UNLEN + 1;
    GetUserName(username, &username_len);
    logAppFile << "USER_LOGIN :-: " << username << " " << DateTime() << endl;

    UINT micDevices = waveInGetNumDevs();
    logCameraFile << username << ":-:NUMBER_OF_AUDIO_INPUTS:-:" << micDevices << endl;

    future<void> cameraCheck = async(launch::async, CheckForCamera);
    future<void> applicationCheck = async(launch::async, CheckForApplication);

    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "logapp";
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    RegisterClassEx(&wcex);
    hInst = hInstance;
    HWND hiddenWindow = CreateWindow("logapp", "", WS_VISIBLE, 0, 0, 100, 100, HWND_MESSAGE, 0, hInstance, 0);
    ShowWindow(hiddenWindow, nCmdShow);
    UpdateWindow(hiddenWindow);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    applicationCheck.get();
    cameraCheck.get();
    return 0;
}

void CheckForApplication() {
    string lastApplicationUsed = "";
    while (true) {
        HWND focusedWindow = GetForegroundWindow();
        if (focusedWindow)
        {
            char window_title[256];
            GetWindowText(focusedWindow, window_title, 256);
            string currentApplication = string(window_title);
            if (currentApplication != lastApplicationUsed)
            {
                if (currentApplication != "")
                {
                    if (currentApplication.rfind("log-app", 0) == 0) {
                        PostMessage(focusedWindow, WM_CLOSE, 0, 0);            //Close log-app.txt viewer, user is not allowed to look this :)
                        logOpenFile << username << " -> try to open log-app.txt ->" << DateTime() << endl;
                    }
                    logAppFile << username << " :-: " << currentApplication << " :-: " << DateTime() << endl;
                }
                lastApplicationUsed = currentApplication;
            }
        }
        this_thread::sleep_for(chrono::milliseconds(200));      //Eevery 0.2sec check what user do
    }
}

void CheckForCamera() {
    m_hCapWnd = capCreateCaptureWindow(NULL, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 400, NULL, 0);
    while (work) {
        (new thread(CloseVideoSourceDialog))->detach();
        BOOL result = capDriverConnect(m_hCapWnd, 0);
        if (result == TRUE)
        {
            capCaptureStop(m_hCapWnd);
            capDriverDisconnect(m_hCapWnd);
            if (camera == true) {                       //It was enabled but now it is disabled
                logCameraFile << username << ":-:CAMERA_DISABLED:-:" << DateTime() << endl;
            }
            camera = false;
        }
        else
        {
            if (camera == false) {                       //It was disabled but now it is enabled
                logCameraFile << username << ":-:CAMERA_ENABLED:-:" << DateTime();
            }
            camera = true;
        }
        this_thread::sleep_for(std::chrono::milliseconds(5000));
    }
}

static void CloseVideoSourceDialog() {
    while (camera) {
        HWND dialog = GetForegroundWindow();
        char window_title[256];
        GetWindowText(dialog, window_title, 256);
        string name = string(window_title);
        if (name == "Video Source") {
            PostMessage(dialog, WM_CLOSE, 0, 0);
            break;
        }
    }
}

void OnExit() {
    logAppFile << "USER_LOGOUT :-: " << username << " " << DateTime() << endl;
    logAppFile.close();
    logOpenFile.close();
    logCameraFile.close();
    work = false;
}

void SignalHandler(int signum) {
    OnExit();
}

string DateTime() {
    chrono::system_clock::time_point p = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(p);
    char str[26];
    ctime_s(str, sizeof str, &t);
    return str;
}