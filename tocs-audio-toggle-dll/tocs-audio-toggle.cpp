// tocs-audio-toggle.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <string>

#include "../minhook/MinHook.h"

#pragma comment(lib, "kernel32.lib")

HANDLE(WINAPI* pCreateFile)
(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
   DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) = CreateFileA;

HANDLE WINAPI createFileDetour(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
   DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);

void replaceStr(std::string& in, const std::string& find, const std::string& replace);

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
   replaceStr(fileName, "/wav_jp/", "/wav/");
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