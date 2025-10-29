#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <napi.h>
#include <memory>

std::unique_ptr<int, void (*)(int *)> open_file(const Napi::Env &env, const std::string &path)
{
  int fd = 0;
  // If the path starts with /dev/shm/, and contains no other slashes, open it as a shared memory object
  if (path.rfind("/dev/shm/", 0) == 0 && path.find('/', 10) == std::string::npos)
  {
    auto shm_name = path.substr(8); // remove /dev/shm
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

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  exports.Set(Napi::String::New(env, "mmap"),
              Napi::Function::New(env, node_mmap));
  return exports;
}

NODE_API_MODULE(mmap, Init)
