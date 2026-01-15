#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <utility>
#include "binding.hpp"

struct WinHandles
{
  HANDLE hFile = INVALID_HANDLE_VALUE;
  HANDLE hMapping = nullptr;
  int64_t length = 0;

  WinHandles() = default;
  WinHandles(const WinHandles &) = delete;
  WinHandles &operator=(const WinHandles &) = delete;

  WinHandles(WinHandles &&other) noexcept
      : hFile(std::exchange(other.hFile, INVALID_HANDLE_VALUE)),
        hMapping(std::exchange(other.hMapping, nullptr)),
        length(std::exchange(other.length, 0))
  {
  }

  WinHandles &operator=(WinHandles &&other) noexcept
  {
    if (this != &other)
    {
      reset();
      hFile = std::exchange(other.hFile, INVALID_HANDLE_VALUE);
      hMapping = std::exchange(other.hMapping, nullptr);
      length = std::exchange(other.length, 0);
    }
    return *this;
  }

  ~WinHandles()
  {
    reset();
  }

private:
  void reset() noexcept
  {
    if (hMapping != nullptr)
    {
      CloseHandle(hMapping);
      hMapping = nullptr;
    }
    if (hFile != INVALID_HANDLE_VALUE)
    {
      CloseHandle(hFile);
      hFile = INVALID_HANDLE_VALUE;
    }
    length = 0;
  }
};

static std::string to_utf8(const std::wstring &wstr)
{
  int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
  if (utf8Size <= 0)
  {
    return std::string();
  }
  std::string str(utf8Size, 0);
  WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), &str[0], utf8Size, nullptr, nullptr);
  return str;
}

static std::wstring to_wide(const std::string &str)
{
  int wideSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
  if (wideSize <= 0)
  {
    return std::wstring();
  }
  std::wstring wstr(wideSize, 0);
  MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), &wstr[0], wideSize);
  return wstr;
}

static std::string last_error()
{
  DWORD errorMessageID = GetLastError();
  if (errorMessageID == 0)
  {
    return std::string();
  }
  LPWSTR messageBuffer = nullptr;
  size_t size = FormatMessageW(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr,
      errorMessageID,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPWSTR)&messageBuffer,
      0,
      nullptr);
  std::string message = to_utf8(std::wstring(messageBuffer, size));
  LocalFree(messageBuffer);
  return message;
}

static bool open_shm(const Napi::Env &env, WinHandles &handles, const std::wstring &shm_name)
{
  // For shared memory without specified length, try to open existing mapping
  handles.hMapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, shm_name.c_str());
  if (handles.hMapping == nullptr)
  {
    Napi::Error::New(env, "OpenFileMapping failed for shm without length, error=" + last_error()).ThrowAsJavaScriptException();
    return false;
  }

  // Map the entire view to determine size using VirtualQuery
  void *ptr = MapViewOfFile(handles.hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  if (ptr == nullptr)
  {
    Napi::Error::New(env, "MapViewOfFile failed, error=" + last_error()).ThrowAsJavaScriptException();
    return false;
  }

  MEMORY_BASIC_INFORMATION mbi;
  if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0)
  {
    UnmapViewOfFile(ptr);
    Napi::Error::New(env, "VirtualQuery failed, error=" + last_error()).ThrowAsJavaScriptException();
    return false;
  }

  handles.length = static_cast<int64_t>(mbi.RegionSize);
  if (handles.length <= 0)
  {
    UnmapViewOfFile(ptr);
    return false;
  }

  return true;
}

static bool create_shm(const Napi::Env &env, WinHandles &handles, const std::wstring &shm_name)
{
  const auto length = handles.length;
  DWORD sizeHigh = static_cast<DWORD>((length >> 32) & 0xFFFFFFFF);
  DWORD sizeLow = static_cast<DWORD>(length & 0xFFFFFFFF);

  handles.hMapping = CreateFileMappingW(
      INVALID_HANDLE_VALUE,
      nullptr,
      PAGE_READWRITE,
      sizeHigh,
      sizeLow,
      L"XX");

  if (handles.hMapping == nullptr)
  {
    Napi::Error::New(env, "CreateFileMapping failed for shm, error=" + last_error()).ThrowAsJavaScriptException();
    return false;
  }

  return true;
}

static bool create_file(const Napi::Env &env, WinHandles &handles, const std::wstring &path)
{
  // Regular file mapping
  handles.hFile = CreateFileW(
      path.c_str(),
      GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      nullptr,
      OPEN_ALWAYS,
      FILE_ATTRIBUTE_NORMAL,
      nullptr);

  if (handles.hFile == INVALID_HANDLE_VALUE)
  {
    Napi::Error::New(env, "CreateFile failed, error=" + last_error()).ThrowAsJavaScriptException();
    return false;
  }

  // Get file size
  LARGE_INTEGER fileSize;
  if (!GetFileSizeEx(handles.hFile, &fileSize))
  {
    Napi::Error::New(env, "GetFileSizeEx failed, error=" + last_error()).ThrowAsJavaScriptException();
    return false;
  }

  if (handles.length <= 0)
  {
    handles.length = fileSize.QuadPart;
  }
  else if (handles.length > fileSize.QuadPart)
  {
    // Extend file to requested length
    LARGE_INTEGER newSize;
    newSize.QuadPart = handles.length;
    if (!SetFilePointerEx(handles.hFile, newSize, nullptr, FILE_BEGIN) ||
        !SetEndOfFile(handles.hFile))
    {
      // If extension fails, use original size
      handles.length = fileSize.QuadPart;
    }
  }

  if (handles.length <= 0)
  {
    return false;
  }

  // Create file mapping
  DWORD sizeHigh = static_cast<DWORD>((handles.length >> 32) & 0xFFFFFFFF);
  DWORD sizeLow = static_cast<DWORD>(handles.length & 0xFFFFFFFF);

  handles.hMapping = CreateFileMappingW(
      handles.hFile,
      nullptr,
      PAGE_READWRITE,
      sizeHigh,
      sizeLow,
      nullptr);

  if (handles.hMapping == nullptr)
  {
    Napi::Error::New(env, "CreateFileMapping failed, error=" + last_error()).ThrowAsJavaScriptException();
    return false;
  }

  return true;
}

Napi::Buffer<uint8_t> node_mmap_impl(const Napi::Env &env, std::string path, int64_t length)
{
  WinHandles handles;
  handles.length = length;

  const auto shm_name = to_wide(get_shm_name(path));

  bool success = false;
  if (!shm_name.empty())
  {
    // Shared memory: use INVALID_HANDLE_VALUE to create mapping backed by system paging file
    if (length <= 0)
    {
      success = open_shm(env, handles, shm_name);
    }
    else
    {
      success = create_shm(env, handles, shm_name);
    }
  }
  else
  {
    // Regular file
    success = create_file(env, handles, to_wide(path));
  }

  if (!success)
  {
    return Napi::Buffer<uint8_t>::New(env, 0);
  }

  // Map view of file
  void *ptr = MapViewOfFile(
      handles.hMapping,
      FILE_MAP_ALL_ACCESS,
      0,
      0,
      static_cast<SIZE_T>(handles.length));
  if (ptr == nullptr)
  {
    Napi::Error::New(env, "MapViewOfFile failed, error=" + last_error()).ThrowAsJavaScriptException();
    return Napi::Buffer<uint8_t>::New(env, 0);
  }

  auto buf = static_cast<uint8_t *>(ptr);
  auto size = static_cast<size_t>(handles.length);

  return Napi::Buffer<uint8_t>::New(
      env,
      buf,
      size,
      [p = std::make_unique<WinHandles>(std::move(handles))](Napi::Env env, void *data) mutable
      {
        // p goes out of scope and is destroyed here
        UnmapViewOfFile(data);
      });
}