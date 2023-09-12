#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <napi.h>

Napi::Buffer<uint8_t> node_mmap(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  if (info.Length() != 1)
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
  const auto path = info[0].As<Napi::String>().Utf8Value();
  const auto fd = open(path.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (fd == -1)
  {
    Napi::TypeError::New(env, "open failed").ThrowAsJavaScriptException();
    return Napi::Buffer<uint8_t>::New(env, 0);
  }
  const auto size = lseek(fd, 0, SEEK_END);
  void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);
  if (ptr == MAP_FAILED)
  {
    Napi::TypeError::New(env, "mmap failed").ThrowAsJavaScriptException();
    return Napi::Buffer<uint8_t>::New(env, 0);
  }
  return Napi::Buffer<uint8_t>::New(env, static_cast<uint8_t *>(ptr), size, [](Napi::Env env, void *data)
                                    { munmap(data, 0); });
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  exports.Set(Napi::String::New(env, "mmap"),
              Napi::Function::New(env, node_mmap));
  return exports;
}

NODE_API_MODULE(mmap, Init)
