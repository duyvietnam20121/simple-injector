#include "roblox_process.h"

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <TlHelp32.h>

#include <cwchar>
#include <optional>
#endif

namespace SimpleInjector {

#ifdef Q_OS_WIN
namespace {

constexpr const wchar_t *kProcessName = L"RobloxPlayerBeta.exe";

struct FoundProcess {
    DWORD pid = 0;
    quintptr imageBase = 0;
};

std::optional<DWORD> findProcessId()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return std::nullopt;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    std::optional<DWORD> pid;

    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, kProcessName) == 0) {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return pid;
}

quintptr findMainModuleBase(DWORD pid)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    MODULEENTRY32W module{};
    module.dwSize = sizeof(module);
    quintptr base = 0;
    if (Module32FirstW(snapshot, &module)) {
        do {
            if (_wcsicmp(module.szModule, kProcessName) == 0) {
                base = reinterpret_cast<quintptr>(module.modBaseAddr);
                break;
            }
        } while (Module32NextW(snapshot, &module));
    }
    CloseHandle(snapshot);
    return base;
}

} // namespace
#endif

RobloxProcess::~RobloxProcess()
{
    detach();
}

void RobloxProcess::detach()
{
#ifdef Q_OS_WIN
    if (handle_)
        CloseHandle(static_cast<HANDLE>(handle_));
#endif
    handle_ = nullptr;
    pid_ = 0;
    imageBase_ = 0;
}

RobloxProcess::Status RobloxProcess::refresh()
{
#ifdef Q_OS_WIN
    // Đang gắn: handle báo "chưa tín hiệu" nghĩa là tiến trình còn sống,
    // không cần duyệt lại toàn bộ danh sách tiến trình mỗi lần.
    if (handle_) {
        if (WaitForSingleObject(static_cast<HANDLE>(handle_), 0) == WAIT_TIMEOUT)
            return {State::Connected, pid_, imageBase_, 0};
        detach();
    }

    const std::optional<DWORD> pid = findProcessId();
    if (!pid)
        return {State::NotRunning, 0, 0, 0};

    HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_WRITE
                                    | PROCESS_VM_OPERATION | SYNCHRONIZE,
                                FALSE, *pid);
    if (!handle)
        return {State::AccessDenied, *pid, 0, static_cast<quint32>(GetLastError())};

    const quintptr base = findMainModuleBase(*pid);
    if (base == 0) {
        CloseHandle(handle);
        return {State::NoModuleBase, *pid, 0, 0};
    }

    handle_ = handle;
    pid_ = *pid;
    imageBase_ = base;
    return {State::Connected, pid_, imageBase_, 0};
#else
    return {State::Unsupported, 0, 0, 0};
#endif
}

bool RobloxProcess::writeInt32(quintptr address, qint32 value)
{
#ifdef Q_OS_WIN
    if (!handle_)
        return false;
    SIZE_T bytesWritten = 0;
    const BOOL ok = WriteProcessMemory(static_cast<HANDLE>(handle_),
                                       reinterpret_cast<LPVOID>(address),
                                       &value, sizeof(value), &bytesWritten);
    return ok && bytesWritten == sizeof(value);
#else
    Q_UNUSED(address)
    Q_UNUSED(value)
    return false;
#endif
}

} // namespace SimpleInjector
