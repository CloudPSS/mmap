#include "binding.hpp"

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
