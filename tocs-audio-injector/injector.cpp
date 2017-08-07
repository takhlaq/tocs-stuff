#include <Windows.h>
#include <TlHelp32.h>
#include <tchar.h>
#include <stdio.h>
#include <psapi.h>
#include <Shlwapi.h>

#include <iostream>
#include <thread>
#include <vector>
#include <regex>
#include <fstream>
#include <string>

bool enabled = false;
BOOL bProcess = false;
HANDLE hProcess = nullptr;
LPVOID LLParam = nullptr;
PROCESSENTRY32 pe32;
std::string dllPath;

void cleanup();

int main()
{
   std::cout << "[info] starting..." << std::endl;

   // unhook on cleanup
   std::atexit(cleanup);
   std::string enableStr("english = 0");
   {
      std::ifstream conf("audioHookConf.ini");
      if (conf.good())
      {
         std::string line;
         std::regex re("(?:.*|)english(?:\=|.*)([01])");
         std::smatch match;

         while (std::getline(conf, line))
         {
            if (line[0] == '#')
               continue;

            if (std::regex_search(line, match, re))
            {
               enabled = std::atoi(match[1].str().c_str());
            }
         }
      }
      conf.close();
   }

   while (true)
   {
      if (enabled)
      {
         pe32.dwSize = sizeof(PROCESSENTRY32);

         find:
         HANDLE hTool32 = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
         bProcess = Process32First(hTool32, &pe32);
         while (bProcess && Process32Next(hTool32, &pe32))
         {
            if (pe32.szExeFile == std::wstring(L"ed8.exe") || pe32.szExeFile == std::wstring(L"ed8_jp.exe"))
            {
               std::string currentDir;
               currentDir.reserve(MAX_PATH);

               GetCurrentDirectoryA(MAX_PATH, &currentDir[0]);
               dllPath = (&currentDir[0] + std::string("\\\\tocs-audio-toggle.dll"));

               hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
                  PROCESS_VM_WRITE, FALSE, pe32.th32ProcessID);

               LPVOID LoadLibraryAddr = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");

               LLParam = (LPVOID)VirtualAllocEx(hProcess, NULL, dllPath.size(),
                  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

               WriteProcessMemory(hProcess, LLParam, dllPath.c_str(), dllPath.size(), NULL);
               CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibraryAddr,
                  LLParam, NULL, NULL);
               std::cout << "[success] found Trails of Cold Steel process" << std::endl;
               std::cout << "[info] defaulting to EN audio - change areas to clear voice cache" << std::endl;

               std::ofstream("audioHookConf.ini").write("english = 1", enableStr.size());
               break;
            }
         }
         if (!hProcess)
         {
            std::cout << "[error] unable to find Trails of Cold Steel process, checking again in 15 seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(15));
            goto find;
         }
      }
      else
      {
         std::cout << "[info] defaulting to JP audio - change areas to clear voice cache" << std::endl;
         bProcess = false;
         if (hProcess)
         {
            cleanup();
            std::ofstream("audioHookConf.ini").write("english = 0", enableStr.size());
         }
      }
      
      // wait for next crap
      std::string line;
      if (std::getline(std::cin, line))
      {
         if (line == "qqq")
            exit(0);
         else if (line == "help")
            std::cout << "[info] unknown command, try <wat> instead" << std::endl;
         else if (line == "wat")
            std::cout << "[info] place jp voice files in Trails of Cold Steel/data/voice/wav_jp/ > hit enter in this window to toggle english/jp voices > change areas to clear voice cache"
            << std::endl << "[info] qqq to unhook and exit"
            << std::endl << "[info] i dont care to add gui atm so toggle here instead"
            << std::endl;
         else if (line.size() == 0 || line == "t")
            enabled = !enabled;
      }
   }
   return 0;
}

void cleanup()
{
   if (hProcess)
   {
      hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
      HMODULE hMod;
      LPVOID FreeLibAddress;
      HMODULE hMods[1024];
      DWORD cbNeeded;

      EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded);

      for (unsigned int i = 0; i < (cbNeeded / sizeof(DWORD)); i++)
      {
         char szPath[MAX_PATH] = "";
         GetModuleFileNameExA(hProcess, hMods[i], szPath, MAX_PATH);
         PathStripPathA(szPath);

         if (!_stricmp(szPath, "tocs-audio-toggle.dll"))
         {
            hMod = hMods[i];
         }
      }

      FreeLibAddress = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "FreeLibrary");

      HANDLE hThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)FreeLibAddress, (LPVOID)hMod, NULL, NULL);

      WaitForSingleObjectEx(hThread, INFINITE, FALSE);
      CloseHandle(hProcess);

      hProcess = nullptr;
      pe32 = PROCESSENTRY32();
   }
}