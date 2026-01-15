#include <napi.h>
#include <memory>

// Extract shm name from path
std::string get_shm_name(const std::string &path)
{
  if (path.rfind("/dev/shm/", 0) == 0 && path.find('/', 10) == std::string::npos)
  {
    return path.substr(9); // remove /dev/shm/ to get just the name
  }
  return "";
}

// Platform-specific implementations
Napi::Buffer<uint8_t> node_mmap_impl(const Napi::Env &env, std::string path, int64_t length);

#ifdef _WIN32
// Windows implementation
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

struct WinHandles
{
  HANDLE hFile;
  HANDLE hMapping;
  WinHandles() : hFile(INVALID_HANDLE_VALUE), hMapping(NULL) {}
  ~WinHandles()
  {
    if (hMapping != NULL)
      CloseHandle(hMapping);
    if (hFile != INVALID_HANDLE_VALUE)
      CloseHandle(hFile);
  }
};

Napi::Buffer<uint8_t> node_mmap_impl(const Napi::Env &env, std::string path, int64_t length)
{
  auto handles = std::make_unique<WinHandles>();

  const auto shm_name = get_shm_name(path);

  if (!shm_name.empty())
  {
    // Shared memory: use INVALID_HANDLE_VALUE to create mapping backed by system paging file
    if (length <= 0)
    {
      // For shared memory without specified length, try to open existing mapping
      handles->hMapping = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, shm_name.c_str());
      if (handles->hMapping == NULL)
      {
        Napi::TypeError::New(env, "OpenFileMapping failed for shm without length, error=" + std::to_string(GetLastError())).ThrowAsJavaScriptException();
        return Napi::Buffer<uint8_t>::New(env, 0);
      }

      // Map the entire view to determine size using VirtualQuery
      void *ptr = MapViewOfFile(handles->hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
      if (ptr == NULL)
      {
        Napi::TypeError::New(env, "MapViewOfFile failed, error=" + std::to_string(GetLastError())).ThrowAsJavaScriptException();
        return Napi::Buffer<uint8_t>::New(env, 0);
      }

      MEMORY_BASIC_INFORMATION mbi;
      if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0)
      {
        UnmapViewOfFile(ptr);
        Napi::TypeError::New(env, "VirtualQuery failed, error=" + std::to_string(GetLastError())).ThrowAsJavaScriptException();
        return Napi::Buffer<uint8_t>::New(env, 0);
      }

      length = static_cast<int64_t>(mbi.RegionSize);
      if (length <= 0)
      {
        UnmapViewOfFile(ptr);
        return Napi::Buffer<uint8_t>::New(env, 0);
      }

      auto handlesPtr = std::make_shared<std::unique_ptr<WinHandles>>(std::move(handles));
      return Napi::Buffer<uint8_t>::New(
          env,
          static_cast<uint8_t *>(ptr),
          static_cast<size_t>(length),
          [handlesPtr](Napi::Env env, void *data)
          {
            UnmapViewOfFile(data);
          });
    }

    DWORD sizeHigh = static_cast<DWORD>((length >> 32) & 0xFFFFFFFF);
    DWORD sizeLow = static_cast<DWORD>(length & 0xFFFFFFFF);

    handles->hMapping = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        sizeHigh,
        sizeLow,
        shm_name.c_str());

    if (handles->hMapping == NULL)
    {
      Napi::TypeError::New(env, "CreateFileMapping failed for shm, error=" + std::to_string(GetLastError())).ThrowAsJavaScriptException();
      return Napi::Buffer<uint8_t>::New(env, 0);
    }
  }
  else
  {
    // Regular file mapping
    handles->hFile = CreateFileA(
        path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (handles->hFile == INVALID_HANDLE_VALUE)
    {
      Napi::TypeError::New(env, "CreateFile failed, error=" + std::to_string(GetLastError())).ThrowAsJavaScriptException();
      return Napi::Buffer<uint8_t>::New(env, 0);
    }

    // Get file size
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(handles->hFile, &fileSize))
    {
      Napi::TypeError::New(env, "GetFileSizeEx failed, error=" + std::to_string(GetLastError())).ThrowAsJavaScriptException();
      return Napi::Buffer<uint8_t>::New(env, 0);
    }

    if (length <= 0)
    {
      length = fileSize.QuadPart;
    }
    else if (length > fileSize.QuadPart)
    {
      // Extend file to requested length
      LARGE_INTEGER newSize;
      newSize.QuadPart = length;
      if (!SetFilePointerEx(handles->hFile, newSize, NULL, FILE_BEGIN) ||
          !SetEndOfFile(handles->hFile))
      {
        // If extension fails, use original size
        length = fileSize.QuadPart;
      }
    }

    if (length <= 0)
    {
      return Napi::Buffer<uint8_t>::New(env, 0);
    }

    // Create file mapping
    DWORD sizeHigh = static_cast<DWORD>((length >> 32) & 0xFFFFFFFF);
    DWORD sizeLow = static_cast<DWORD>(length & 0xFFFFFFFF);

    handles->hMapping = CreateFileMappingA(
        handles->hFile,
        NULL,
        PAGE_READWRITE,
        sizeHigh,
        sizeLow,
        NULL);

    if (handles->hMapping == NULL)
    {
      Napi::TypeError::New(env, "CreateFileMapping failed, error=" + std::to_string(GetLastError())).ThrowAsJavaScriptException();
      return Napi::Buffer<uint8_t>::New(env, 0);
    }
  }

  // Map view of file
  void *ptr = MapViewOfFile(
      handles->hMapping,
      FILE_MAP_ALL_ACCESS,
      0,
      0,
      static_cast<SIZE_T>(length));

  if (ptr == NULL)
  {
    Napi::TypeError::New(env, "MapViewOfFile failed, error=" + std::to_string(GetLastError())).ThrowAsJavaScriptException();
    return Napi::Buffer<uint8_t>::New(env, 0);
  }

  // Transfer ownership of handles to the release shared_ptr
  auto handlesPtr = std::make_shared<std::unique_ptr<WinHandles>>(std::move(handles));
  return Napi::Buffer<uint8_t>::New(
      env,
      static_cast<uint8_t *>(ptr),
      static_cast<size_t>(length),
      [handlesPtr](Napi::Env env, void *data)
      {
        UnmapViewOfFile(data);
        // handles are automatically closed when handlesPtr is destroyed
      });
}

#else
// POSIX implementation
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>

std::unique_ptr<int, void (*)(int *)> open_file(const Napi::Env &env, const std::string &path)
{
  int fd = 0;
  const auto shm_name = get_shm_name(path);
  if (!shm_name.empty())
  {
    fd = ::shm_open(shm_name.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  }
  else
  {
    fd = ::open(path.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  }
  if (fd == -1)
  {
    Napi::TypeError::New(env, "open failed, errno=" + std::to_string(errno)).ThrowAsJavaScriptException();
    return std::unique_ptr<int, void (*)(int *)>(nullptr, nullptr);
  }
  return std::unique_ptr<int, void (*)(int *)>(new int(fd), [](int *fd)
                                               { ::close(*fd); delete fd; });
}

Napi::Buffer<uint8_t> node_mmap_impl(const Napi::Env &env, std::string path, int64_t length)
{
  const auto fd = open_file(env, path);
  if (!fd)
  {
    return Napi::Buffer<uint8_t>::New(env, 0);
  }

  struct stat sb;
  if (fstat(*fd, &sb) == -1)
  {
    Napi::TypeError::New(env, "fstat failed, errno=" + std::to_string(errno)).ThrowAsJavaScriptException();
    return Napi::Buffer<uint8_t>::New(env, 0);
  }
  if (length <= 0)
  {
    length = sb.st_size;
  }
  else if (length > sb.st_size)
  {
    if (ftruncate(*fd, length) == -1)
    {
      length = sb.st_size;
    }
  }
  if (length <= 0)
  {
    return Napi::Buffer<uint8_t>::New(env, 0);
  }
  void *ptr = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
  if (ptr == MAP_FAILED)
  {
    Napi::TypeError::New(env, "mmap failed, errno=" + std::to_string(errno)).ThrowAsJavaScriptException();
    return Napi::Buffer<uint8_t>::New(env, 0);
  }
  return Napi::Buffer<uint8_t>::New(env, static_cast<uint8_t *>(ptr), length, [=](Napi::Env env, void *data)
                                    { munmap(data, length); });
}

#endif
Napi::Buffer<uint8_t> node_mmap(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  if (info.Length() != 2)
  {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return Napi::Buffer<uint8_t>::New(env, 0);
  }
  if (!info[0].IsString())
  {
    Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
    return Napi::Buffer<uint8_t>::New(env, 0);
  }
  if (!info[1].IsNumber())
  {
    Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
    return Napi::Buffer<uint8_t>::New(env, 0);
  }
  auto length = info[1].As<Napi::Number>().Int64Value();
  const auto path = info[0].As<Napi::String>().Utf8Value();
  return node_mmap_impl(env, path, length);
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  exports.Set(Napi::String::New(env, "mmap"),
              Napi::Function::New(env, node_mmap));
  return exports;
}

NODE_API_MODULE(mmap, Init)
