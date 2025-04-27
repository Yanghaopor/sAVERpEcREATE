#include <windows.h>
#include <winioctl.h>
#include <ntddstor.h>
#include <string>
#include <shellapi.h>
#include <fstream>
#include<algorithm>
#include <system_error>
#include <vector>
#include <cstdio>

bool Us = false;
bool MBR = true;

// 需要创建清单文件或通过编译器选项请求管理员权限
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
//U盘检查
bool IsUSBDrive(const std::wstring& driveLetter) {
    // 检查输入格式（例如"D:"）
    if (driveLetter.size() != 2 || driveLetter[1] != L':') {
        return false;
    }

    // 检查驱动器类型是否为可移动设备
    const std::wstring rootPath = driveLetter + L"\\";
    if (GetDriveTypeW(rootPath.c_str()) != DRIVE_REMOVABLE) {
        return false;
    }

    // 构造物理设备路径（"\\.\D:"）
    const std::wstring devicePath = L"\\\\.\\" + driveLetter.substr(0, 2);

    // 打开存储设备
    HANDLE hDevice = CreateFileW(
        devicePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        return false;
    }

    // 准备存储设备属性查询
    STORAGE_PROPERTY_QUERY storageQuery{};
    storageQuery.PropertyId = StorageDeviceProperty;
    storageQuery.QueryType = PropertyStandardQuery;

    BYTE outputBuffer[4096]{}; // 足够大的缓冲区
    DWORD bytesReturned = 0;

    // 查询设备属性
    BOOL success = DeviceIoControl(
        hDevice,
        IOCTL_STORAGE_QUERY_PROPERTY,
        &storageQuery,
        sizeof(storageQuery),
        outputBuffer,
        sizeof(outputBuffer),
        &bytesReturned,
        nullptr
    );

    CloseHandle(hDevice);

    if (!success || bytesReturned == 0) {
        return false;
    }

    // 解析设备描述符
    const auto* deviceDescriptor =
        reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(outputBuffer);

    return (deviceDescriptor->BusType == BusTypeUsb);
}

//格式化
bool FormatDriveWithCMD(const std::wstring& driveLetter) {
    // 参数检查（格式如"D:"）
    if (driveLetter.size() != 2 || driveLetter[1] != L':') {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    // 构造格式化命令（/Q快速格式化 /FS指定文件系统 /Y自动确认）
    std::wstring command = L"cmd.exe /c format " + driveLetter +
        L" /FS:NTFS /Q /Y";

    // 初始化进程结构体
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;  // 隐藏CMD窗口

    // 创建可写命令行缓冲区
    wchar_t cmdLine[MAX_PATH] = { 0 };
    wcscpy_s(cmdLine, command.c_str());

    // 创建进程
    if (!CreateProcessW(
        nullptr,          // 不使用模块名
        cmdLine,          // 命令行
        nullptr,          // 进程安全属性
        nullptr,          // 线程安全属性
        FALSE,            // 不继承句柄
        CREATE_NO_WINDOW, // 创建标志
        nullptr,          // 环境变量
        nullptr,          // 当前目录
        &si,              // 启动信息
        &pi               // 进程信息
    )) {
        return false;
    }

    // 等待格式化完成（最多5分钟）
    WaitForSingleObject(pi.hProcess, 300000);

    // 获取退出代码
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    // 清理资源
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // 检查执行结果（format命令成功返回0）
    return (exitCode == 0);
}

bool FormatToFAT32(const std::wstring& driveLetter) {
    // 构造format命令（示例：format D: /FS:FAT32 /Q /X /A:16K /V:PE_SYSTEM /Y）
    std::wstring command = L"cmd.exe /c format " + driveLetter.substr(0, 2) +
        L" /FS:FAT32 /Q /X /A:16K /V:PE_SYSTEM /Y";

    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    wchar_t cmdLine[MAX_PATH] = { 0 };
    wcscpy_s(cmdLine, command.c_str());

    if (!CreateProcessW(
        nullptr,
        cmdLine,
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW | CREATE_NEW_CONSOLE,
        nullptr,
        nullptr,
        &si,
        &pi
    )) {
        return false;
    }

    // 等待格式化完成（建议较长时间）
    WaitForSingleObject(pi.hProcess, 600000); // 10分钟超时

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    /*
    format.exe 返回代码：
    0 - 成功
    其他 - 失败
    */
    return (exitCode == 0);
}

bool FormatToNTFS(const std::wstring& driveLetter) {
    // 构造format命令（示例：format D: /FS:FAT32 /Q /X /A:16K /V:PE_SYSTEM /Y）
    std::wstring command = L"cmd.exe /c format " + driveLetter.substr(0, 2) +
        L" /FS:NTFS /Q /X /A:16K /V:PE_SYSTEM /Y";

    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    wchar_t cmdLine[MAX_PATH] = { 0 };
    wcscpy_s(cmdLine, command.c_str());

    if (!CreateProcessW(
        nullptr,
        cmdLine,
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW | CREATE_NEW_CONSOLE,
        nullptr,
        nullptr,
        &si,
        &pi
    )) {
        return false;
    }

    // 等待格式化完成（建议较长时间）
    WaitForSingleObject(pi.hProcess, 600000); // 10分钟超时

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    /*
    format.exe 返回代码：
    0 - 成功
    其他 - 失败
    */
    return (exitCode == 0);
}

//系统盘制作
// 物理驱动器路径转换函数
//DispER 脚本
int GetDiskNumberFromDriveLetter(LPCWSTR driveLetter) {
    HANDLE hVolume = INVALID_HANDLE_VALUE;
    VOLUME_DISK_EXTENTS volumeExtents = { 0 };
    DWORD bytesReturned = 0;
    int diskNumber = -1;

    // 构建卷设备路径（格式：\\.\E:）
    std::wstring volumePath = L"\\\\.\\" + std::wstring(driveLetter);

    // 打开卷句柄
    hVolume = CreateFileW(
        volumePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hVolume == INVALID_HANDLE_VALUE) {
        std::wcerr << L"无法打开卷 " << driveLetter << L"，错误码: " << GetLastError() << std::endl;
        return -1;
    }

    // 获取卷的磁盘扩展信息
    if (!DeviceIoControl(
        hVolume,
        IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
        NULL,
        0,
        &volumeExtents,
        sizeof(volumeExtents),
        &bytesReturned,
        NULL
    )) {
        std::wcerr << L"获取磁盘扩展信息失败，错误码: " << GetLastError() << std::endl;
        CloseHandle(hVolume);
        return -1;
    }

    // 解析返回的磁盘编号（仅取第一个扩展）
    if (volumeExtents.NumberOfDiskExtents >= 1) {
        diskNumber = volumeExtents.Extents[0].DiskNumber;
    }

    CloseHandle(hVolume);
    return diskNumber;
}

//脚本生成
// 根据盘符获取磁盘编号的函数（需提前定义，参考之前的代码）
int GetDiskNumberFromDriveLetter(LPCWSTR driveLetter);

// 安全的系统命令执行函数（同步等待完成）
bool ExecuteCommandSync(const std::wstring& command) {
    // 将命令转换为多字节字符串（适配 system()）
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, command.c_str(), -1, NULL, 0, NULL, NULL);
    std::string narrowCommand(bufferSize, 0);
    WideCharToMultiByte(CP_UTF8, 0, command.c_str(), -1, &narrowCommand[0], bufferSize, NULL, NULL);

    // 执行并等待完成
    int result = system(narrowCommand.c_str());
    return (result == 0);
}

// 转MBR
bool ConvertToMBR(const std::wstring& driveLetter) {
    // 获取物理磁盘号
    const int diskNumber = GetDiskNumberFromDriveLetter(driveLetter.c_str());
    if (diskNumber < 0) {
        // 无效盘符或获取失败
        return false;
    }

    // 创建临时脚本文件
    wchar_t tempPath[MAX_PATH] = { 0 };
    wchar_t tempFile[MAX_PATH] = { 0 };

    // 获取系统临时目录
    if (!GetTempPathW(MAX_PATH, tempPath)) {
        return false;
    }

    // 生成唯一临时文件名
    if (!GetTempFileNameW(tempPath, L"mbr", 0, tempFile)) {
        return false;
    }

    // 构造diskpart脚本内容
    const std::wstring scriptContent =
        L"select disk " + std::to_wstring(diskNumber) + L"\r\n"
        L"clean\r\n"
        L"convert mbr\r\n"
        L"exit\r\n";

    // 写入脚本文件
    HANDLE hFile = CreateFileW(
        tempFile,
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        DeleteFileW(tempFile);
        return false;
    }

    DWORD bytesWritten = 0;
    const BOOL writeResult = WriteFile(
        hFile,
        scriptContent.c_str(),
        static_cast<DWORD>(scriptContent.size() * sizeof(wchar_t)),
        &bytesWritten,
        nullptr
    );

    CloseHandle(hFile);

    if (!writeResult) {
        DeleteFileW(tempFile);
        return false;
    }

    // 构造diskpart命令
    const std::wstring command =
        L"diskpart.exe /s \"" + std::wstring(tempFile) + L"\"";

    // 执行diskpart进程
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    wchar_t cmdLine[MAX_PATH] = { 0 };
    wcscpy_s(cmdLine, command.c_str());

    BOOL processStarted = CreateProcessW(
        nullptr,
        cmdLine,
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!processStarted) {
        DeleteFileW(tempFile);
        return false;
    }

    // 等待操作完成（建议较长时间）
    WaitForSingleObject(pi.hProcess, 300000); // 5分钟超时

    // 获取退出代码
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    // 清理资源
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    DeleteFileW(tempFile);

    /*
    diskpart返回代码说明：
    0 - 成功
    非零 - 失败
    */
    return (exitCode == 0);
}

// 转GPT
bool ConvertToGPT(const std::wstring& driveLetter) {
    // 获取物理磁盘号
    const int diskNumber = GetDiskNumberFromDriveLetter(driveLetter.c_str());
    if (diskNumber < 0) {
        // 无效盘符或获取失败
        return false;
    }

    // 创建临时脚本文件
    wchar_t tempPath[MAX_PATH] = { 0 };
    wchar_t tempFile[MAX_PATH] = { 0 };

    // 获取系统临时目录
    if (!GetTempPathW(MAX_PATH, tempPath)) {
        return false;
    }

    // 生成唯一临时文件名
    if (!GetTempFileNameW(tempPath, L"mbr", 0, tempFile)) {
        return false;
    }

    // 构造diskpart脚本内容
    const std::wstring scriptContent =
        L"select disk " + std::to_wstring(diskNumber) + L"\r\n"
        L"clean\r\n"
        L"convert gpt\r\n"
        L"exit\r\n";

    // 写入脚本文件
    HANDLE hFile = CreateFileW(
        tempFile,
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        DeleteFileW(tempFile);
        return false;
    }

    DWORD bytesWritten = 0;
    const BOOL writeResult = WriteFile(
        hFile,
        scriptContent.c_str(),
        static_cast<DWORD>(scriptContent.size() * sizeof(wchar_t)),
        &bytesWritten,
        nullptr
    );

    CloseHandle(hFile);

    if (!writeResult) {
        DeleteFileW(tempFile);
        return false;
    }

    // 构造diskpart命令
    const std::wstring command =
        L"diskpart.exe /s \"" + std::wstring(tempFile) + L"\"";

    // 执行diskpart进程
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    wchar_t cmdLine[MAX_PATH] = { 0 };
    wcscpy_s(cmdLine, command.c_str());

    BOOL processStarted = CreateProcessW(
        nullptr,
        cmdLine,
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!processStarted) {
        DeleteFileW(tempFile);
        return false;
    }

    // 等待操作完成（建议较长时间）
    WaitForSingleObject(pi.hProcess, 300000); // 5分钟超时

    // 获取退出代码
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    // 清理资源
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    DeleteFileW(tempFile);

    /*
    diskpart返回代码说明：
    0 - 成功
    非零 - 失败
    */
    return (exitCode == 0);
}

std::pair<char, char> GetTwoAvailableDriveLetters() {
    std::vector<char> availableDrives;

    // 获取所有已使用的盘符
    DWORD driveMask = GetLogicalDrives();

    // 检查从C到Z的所有盘符
    for (char drive = 'C'; drive <= 'Z'; ++drive) {
        if (!(driveMask & (1 << (drive - 'A')))) {
            availableDrives.push_back(drive);
        }
    }

    // 确保至少有两个可用盘符
    if (availableDrives.size() >= 2) {
        return { availableDrives[0], availableDrives[1] };
    }

    // 如果没有足够的盘符，返回默认值(可能需要处理这种情况)
    return { 'X', 'Y' };
}

std::wstring charToWstring(char c) {
    // 将 char 转换为 wstring (1个字符长度)
    return std::wstring(1, static_cast<wchar_t>(c));
}

//挂载硬盘
bool RuanPand(std::wstring FileISO, std::wstring DL) {
    
    auto Dive = GetTwoAvailableDriveLetters();
    std::wstring Len;//脚本
    Len = L"select disk " + std::to_wstring(GetDiskNumberFromDriveLetter(DL.c_str()))+ L"\n";
    Len += L"clean\n";
    Len += L"convert mbr\n";
    Len += L"create partition primary size=1024\n";
    Len += L"format fs=fat32 quick label=\"sPE\"\n";
    Len += L"assign letter=" + charToWstring(Dive.first) + L"\n";
    Len += L"active\n";
    Len += L"create partition primary\n";
    Len += L"format fs=ntfs quick label=\"Data\"\n";
    Len += L"assign letter="+ charToWstring(Dive.second)+L"\n";
    Len += L"list volume\n";
    Len += L"exit\n";
    
    std::wofstream OrRun(L"OrRun.txt");
    OrRun << Len;
    OrRun.close();
    //默认的FaT32
    _wsystem(L"diskpart /s OrRun.txt");
     Len = L"cd 7z & 7z x " + FileISO + L" -o"+ charToWstring(Dive.first)+L":\\ -y";
    _wsystem(Len.c_str());
    return true;
}

