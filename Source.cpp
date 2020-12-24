#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#include<fileapi.h>
#include<timezoneapi.h>
#include<iostream>
#include<fstream>
#include<string>
#include<iomanip>
#include<wchar.h>
#include<sstream>
#include<map>
#include<algorithm>
#include<vector>
using namespace std;
// To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS
// and compile with -DPSAPI_VERSION=1
vector<DWORD>allEnum, Thisenum;
HANDLE OFile;
bool flag = false;
map<int, string>processname;
map<int, bool>status;
struct Time
{
    int hour;
    int minute;
    int second;
};
Time convertGMTtoIST(int hour, int min, int sec)
{
    Time data;
    int inseconds;
    inseconds = hour * 3600 + min * 60 + sec;
    inseconds += 19800; //adding +5:30hr in seconds
    data.hour = inseconds / 3600;
    inseconds %= 3600;
    data.minute = inseconds / 60;
    inseconds %= 60;
    data.second = inseconds;
    return data;
}
void PrintProcessNameAndID(DWORD processID)
{

    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
    // Get a handle to the process.
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
  
    if (status[processID])
    {
        Thisenum.push_back(processID);
        return;
    }
    Thisenum.push_back(processID);
    allEnum.push_back(processID);
    status[processID] = true;
    SYSTEMTIME sysTime;
    GetSystemTime(&sysTime);
    if (NULL != hProcess)
    {
        HMODULE hMod;
        DWORD cbNeeded;

        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod),
            &cbNeeded))
        {
            GetModuleBaseName(hProcess, hMod, szProcessName,
                sizeof(szProcessName) / sizeof(TCHAR));
        }
    }
    wstring wname(szProcessName);
    string name(wname.begin(), wname.end());
    //swprintf()
    processname[processID] = name;

    char buff[1000];
    // cout << name << "\n";
    Time Start = convertGMTtoIST(sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
    sprintf_s(buff, "%-30s %-8d       %02d:%02d:%02d     START\n", name.c_str(), processID, Start.hour, Start.minute, Start.second);
    DWORD        nNumberOfBytesToWrite = strlen(buff) * sizeof(char);
    DWORD      lpNumberOfBytesWritten = 0;

    WriteFile(
        OFile,
        buff,
        nNumberOfBytesToWrite,
        &lpNumberOfBytesWritten,
        NULL
    );

    // Release the handle to the process;
    CloseHandle(hProcess);
}
void checkforEndprocess()
{
    int enumsize = allEnum.size(), i; //the bug is here this is the reason for crash in code.
    for (i = 0; i < allEnum.size(); i++)
    {
        if (find(Thisenum.begin(), Thisenum.end(), allEnum[i]) == Thisenum.end())
        {
            DWORD processID = allEnum[i];
            string name = processname[allEnum[i]];
            SYSTEMTIME EndTime;
            GetSystemTime(&EndTime);
            char buff[1000];
            Time End = convertGMTtoIST(EndTime.wHour, EndTime.wMinute, EndTime.wSecond);
            sprintf_s(buff, "%-30s %-8d       %02d:%02d:%02d     END\n", name.c_str(), processID, End.hour, End.minute, End.second);
            DWORD        nNumberOfBytesToWrite = strlen(buff) * sizeof(char);
            DWORD      lpNumberOfBytesWritten = 0;
            WriteFile(
                OFile,
                buff,
                nNumberOfBytesToWrite,
                &lpNumberOfBytesWritten,
                NULL
            );
            allEnum.erase(allEnum.begin() + i);
            status[processID] = false;
        }
    }
}
int main(void)
{
    DeleteFileA("output.txt"); //delete if already exist

    OFile = CreateFileA(
        "output.txt",
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (OFile == INVALID_HANDLE_VALUE)
    {
        cout << "error in creation";
        return 0;
    }

    bool flag = true;
    while (1)
    {
        // Sleep(1000);
        DWORD aProcesses[1024], cbNeeded, cProcesses;
        unsigned int i;

        if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
        {
            return 1;
        }
        // Calculate how many process identifiers were returned.
        cProcesses = cbNeeded / sizeof(DWORD);
        for (i = 0; i < cProcesses; i++)
        {
            if (aProcesses[i] != 0)
            {
                PrintProcessNameAndID(aProcesses[i]);
            }
        }
        if (flag)
        {
            CloseHandle(OFile);
            OFile = CreateFileA(
                "output.txt",
                GENERIC_WRITE,
                0,
                NULL,
                TRUNCATE_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );

            //  out << setfill(' ')<<setw(20) << Hprocess << " " << setw(8) << Hid << setw(8) << " " << HTime << "     " << "STATUS\n";
            char buff[500];
            sprintf_s(buff, "%-30s %-8s %12s   %10s\n", "PROCESS NAME", "PID", "TIME", "STATUS");
            DWORD        nNumberOfBytesToWrite = strlen(buff) * sizeof(char);
            DWORD      lpNumberOfBytesWritten = 0;
            //cout << strlen(buff);
            WriteFile(
                OFile,
                buff,
                nNumberOfBytesToWrite,
                &lpNumberOfBytesWritten,
                NULL
            );
            flag = false;
        }
        checkforEndprocess();
        Thisenum.clear();
    }
    CloseHandle(OFile);
    return 0;
}