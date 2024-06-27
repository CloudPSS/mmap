#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <napi.h>
#include <memory>

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
  const auto fd = std::unique_ptr<int, void (*)(int *)>(new int(::open(path.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)), [](int *fd)
                                                        { ::close(*fd); delete fd; });
  if (*fd == -1)
  {
    Napi::TypeError::New(env, "open failed, errno=" + std::to_string(errno)).ThrowAsJavaScriptException();
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
