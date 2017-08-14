// tocs-audio-toggle.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <string>
#include <fstream>
#include <regex>

#include "../../libs/minhook/MinHook.h"

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "Shlwapi.lib")

HANDLE(WINAPI* pCreateFile)
(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
   DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) = CreateFileA;

HANDLE WINAPI createFileDetour(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
   DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);

BOOL CALLBACK enumWindows(__in  HWND hWnd, __in  LPARAM lParam);
void replaceStr(std::string& in, const std::string& find, const std::string& replace);
bool saveSDSlot = false;
std::regex re("(?:.*|)english(?:\=|.*)([01])");
HWND tocsWindow = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
   switch (dwReason)
   {
   case DLL_PROCESS_ATTACH:
      if (MH_Initialize() != MH_OK)
         return false;
      MH_CreateHookApiEx(L"kernel32", "CreateFileA", &createFileDetour, reinterpret_cast<LPVOID*>(&pCreateFile), nullptr);
      MH_EnableHook(MH_ALL_HOOKS);
      break;
   case DLL_THREAD_ATTACH:
   case DLL_THREAD_DETACH:
      break;
   case DLL_PROCESS_DETACH:
      MH_DisableHook(MH_ALL_HOOKS);
      MH_Uninitialize();
      break;
   }
   return true;
}

HANDLE WINAPI createFileDetour(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
   std::string fileName(lpFileName);

   // confirm dialog for saving
   bool isSDSlot = false;
   if (dwDesiredAccess & GENERIC_WRITE && fileName.find("save") != std::string::npos || (isSDSlot = (fileName.find("sdslot") != std::string::npos)))
   {
      if (!isSDSlot)
      {
         auto pos = fileName.find_last_of("/");
         EnumWindows(enumWindows, NULL);
         saveSDSlot = MessageBoxA(tocsWindow, std::string("are you sure you wish to overwrite " + fileName.substr(pos, fileName.size() - pos)).c_str(), "Trails of Cold Steel Save Prompt", MB_YESNO) == IDYES;

         if (!saveSDSlot)
            return nullptr;
      }
      else if (saveSDSlot)
      {
         saveSDSlot = false;
      }
   }
   else if (fileName.find("voice/wav") != std::string::npos)
   {
      std::ifstream conf("audioHookConf.ini");
      bool enabled = false;
      if (conf.good())
      {
         std::string line;
         std::smatch match;

         while (std::getline(conf, line))
         {
            if (line[0] == '#')
               continue;

            if (std::regex_search(line, match, re))
            {
               enabled = std::atoi(match[1].str().c_str());
               
               if (enabled)
                  replaceStr(fileName, "/wav_jp/", "/wav/");

               break;
            }
         }
      }
      else
      {
         std::ofstream("audioHookConf.ini").write("english = 0", sizeof("english = 0"));
      }
      conf.close();
   }
   return pCreateFile(fileName.c_str(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

void replaceStr(std::string& in, const std::string& find, const std::string& replace)
{
   auto pos = 0;
   while ((pos = in.find(find, pos)) != std::string::npos)
   {
      in.replace(pos, find.length(), replace);
      pos += replace.length();
   }
}

BOOL CALLBACK enumWindows(
   __in  HWND hWnd,
   __in  LPARAM lParam
) {
   if (!::IsIconic(hWnd)) {
      return TRUE;
   }

   int length = ::GetWindowTextLengthA(hWnd);
   if (0 == length) return TRUE;

   std::string title;
   title.reserve(length);

   GetWindowTextA(hWnd, &title[0], title.size());

   if (_strnicmp(title.c_str(), "The Legend of Heroes - Trails of Cold Steel", sizeof("The Legend of Heroes - Trails of Cold Steel") == 0))
   {
      tocsWindow = hWnd;
      return FALSE;
   }
   return true;
}