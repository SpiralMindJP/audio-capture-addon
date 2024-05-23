#include <node_api.h>
#include <inttypes.h> 
#include "captureclient.h"

namespace CaptureClientAddon {

void finalizeCaptureClient(napi_env env, void* finalize_data, void* finalize_hint) {
  //std::cerr << "C++ in finalizeCaptureClient" << std::endl;
  delete (AudioCaptureClient*) finalize_data;
  //std::cerr << "C++ in finalizeCaptureClient FINISHED destroy" << std::endl;
}

napi_value CreateCaptureClient(napi_env env, napi_callback_info info) {
  napi_value pointer_value;
  napi_status status;

  // create C++ object. deletion is handled in the finalizeCaptureClient callback,
  // called when JS does GC on the JS-side object. we must not allow delete to be
  // called directly (and must not provide a deleteCaptureClient API) because this
  // could cause the C++ object to be deleted while possibly JS still has some
  // references to it. so we use napi_create_external and the finalize callback
  // to only delete when JS does GC.
  // see https://nodejs.org/api/n-api.html#napi_create_external
  void* clientPointer = createCaptureClient();

  status = napi_create_external(env, clientPointer, finalizeCaptureClient, NULL, &pointer_value);
  if (status != napi_ok) {
    std::cerr << "C++ err in CreateCaptureClient: could not create pointer external" << std::endl; 
    return NULL; 
  }

  //std::cerr << "C++ success in CreateCaptureClient" << std::endl;
  return pointer_value;
}

// NOTE: do not call InitializeCom from electron app because electron already
// initializes COM by itself. only call InitializeCom when testing in a
// standalone node.js app.
napi_value InitializeCom(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_status status;

  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (status != napi_ok) {
    std::cerr << "C++ error in InitializeCom: could not get args" << std::endl;
    return nullptr;
  }

  napi_valuetype value_type;
  status = napi_typeof(env, args[0], &value_type);
  if (status != napi_ok || value_type != napi_external) {
    std::cerr << "C++ error in InitializeCom: could not get args[0]" << std::endl;
    return nullptr;
  }

  void* clientPointer;
  status = napi_get_value_external(env, args[0], &clientPointer);
  if (status != napi_ok) {
    std::cerr << "C++ error in InitializeCom: could not get pointer value" << std::endl;
    return nullptr;
  }

  initializeCom(clientPointer);
  //std::cerr << "C++ success in InitializeCom" << std::endl;
}


napi_value StartCapture(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_status status;

  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (status != napi_ok) {
    std::cerr << "C++ error in StartCapture: could not get args" << std::endl;
    return nullptr;
  }

  napi_valuetype value_type;
  status = napi_typeof(env, args[0], &value_type);
  if (status != napi_ok || value_type != napi_external) {
    std::cerr << "C++ error in StartCapture: could not get args[0]" << std::endl;
    return nullptr;
  }

  void* clientPointer;
  status = napi_get_value_external(env, args[0], &clientPointer);
  if (status != napi_ok) {
    std::cerr << "C++ error in StartCapture: could not get pointer value" << std::endl;
    return nullptr;
  }

  startCapture(clientPointer);
  //std::cerr << "C++ success in StartCapture" << std::endl;
}

napi_value GetAudioFormat(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_status status;

  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetAudioFormat: could not get args" << std::endl;
    return nullptr;
  }

  napi_valuetype value_type;
  status = napi_typeof(env, args[0], &value_type);
  if (status != napi_ok || value_type != napi_external) {
    std::cerr << "C++ error in GetAudioFormat: could not get args[0]" << std::endl;
    return nullptr;
  }

  void* clientPointer;
  status = napi_get_value_external(env, args[0], &clientPointer);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetAudioFormat: could not get client pointer value" << std::endl;
    return nullptr;
  }

  AudioCaptureFormat captureFormat = getAudioFormat(clientPointer);
  // std::cerr << "framesize " << captureFormat.frameSize << std::endl;
  // std::cerr << "C++ in GetAudioFormat: got captureFormat";

  napi_value js_array;
  status = napi_create_array(env, &js_array);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetAudioFormat: cannot create array" << std::endl;
    return nullptr;
  }

  int iEntry = 0;
  napi_value entry;
  status = napi_create_int32(env, captureFormat.formatisValid ? 1:0, &entry);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetAudioFormat: cannot create entry for formatisValid" << std::endl;
    return nullptr;
  }
  status = napi_set_element(env, js_array, iEntry, entry);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetAudioFormat: cannot set entry for formatisValid" << std::endl;
    return nullptr;
  }

  iEntry++;
  status = napi_create_int32(env, captureFormat.frameSize, &entry);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetAudioFormat: cannot create entry for frameSize" << std::endl;
    return nullptr;
  }
  status = napi_set_element(env, js_array, iEntry, entry);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetAudioFormat: cannot set entry for frameSize" << std::endl;
    return nullptr;
  }

  iEntry++;
  status = napi_create_int32(env, captureFormat.numChannels, &entry);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetAudioFormat: cannot create entry for numChannels" << std::endl;
    return nullptr;
  }
  status = napi_set_element(env, js_array, iEntry, entry);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetAudioFormat: cannot set entry for numChannels" << std::endl;
    return nullptr;
  }

  iEntry++;
  status = napi_create_int32(env, captureFormat.bitsPerSample, &entry);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetAudioFormat: cannot create entry for bitsPerSample" << std::endl;
    return nullptr;
  }
  status = napi_set_element(env, js_array, iEntry, entry);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetAudioFormat: cannot set entry for bitsPerSample" << std::endl;
    return nullptr;
  }

  iEntry++;
  status = napi_create_int32(env, captureFormat.samplesPerSec, &entry);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetAudioFormat: cannot create entry for samplesPerSec" << std::endl;
    return nullptr;
  }
  status = napi_set_element(env, js_array, iEntry, entry);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetAudioFormat: cannot set entry for samplesPerSec" << std::endl;
    return nullptr;
  }

  //std::cerr << "C++ success in GetAudioFormat" << std::endl;
  return js_array;
}

napi_value GetNextPacketSize(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_status status;

  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetNextPacketSize: could not get args" << std::endl;
    return nullptr;
  }

  napi_valuetype value_type;
  status = napi_typeof(env, args[0], &value_type);
  if (status != napi_ok || value_type != napi_external) {
    std::cerr << "C++ error in GetNextPacketSize: could not get args[0]" << std::endl;
    return nullptr;
  }

  void* clientPointer;
  status = napi_get_value_external(env, args[0], &clientPointer);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetNextPacketSize: could not get client pointer value" << std::endl;
    return nullptr;
  }

  napi_value result;
  status = napi_create_int32(env, getNextPacketSize(clientPointer), &result);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetNextPacketSize: could not create result" << std::endl;
    return nullptr;
  }
  return result;
}

napi_value GetBuffer(napi_env env, napi_callback_info info) {
  size_t argc = 4;
  napi_value args[4];
  napi_status status;

  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetBuffer: could not get args" << std::endl;
    return nullptr;
  }

  napi_valuetype value_type;
  status = napi_typeof(env, args[0], &value_type);
  if (status != napi_ok || value_type != napi_external) {
    std::cerr << "C++ error in GetBuffer: could not get args[0]" << std::endl;
    return nullptr;
  }

  void* clientPointer;
  status = napi_get_value_external(env, args[0], &clientPointer);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetBuffer: could not get client pointer value" << std::endl;
    return nullptr;
  }

    status = napi_typeof(env, args[1], &value_type);
  if (status != napi_ok || value_type != napi_number) {
    std::cerr << "C++ error in GetBuffer: could not get args[1]" << std::endl;
    return nullptr;
  }

  int32_t expectedFrameCount;
  status = napi_get_value_int32(env, args[1], &expectedFrameCount);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetBuffer: could not get value of args[1]" << std::endl;
    return nullptr;
  }

  status = napi_typeof(env, args[2], &value_type);
  if (status != napi_ok || value_type != napi_number) {
      std::cerr << "C++ error in GetBuffer: could not get args[2]" << std::endl;
      return nullptr;
  }

  int32_t maximumFrameCount;
  status = napi_get_value_int32(env, args[2], &maximumFrameCount);
  if (status != napi_ok) {
      std::cerr << "C++ error in GetBuffer: could not get value of args[2]" << std::endl;
      return nullptr;
  }

  status = napi_typeof(env, args[3], &value_type);
  if (status != napi_ok || value_type != napi_object) {
    std::cerr << "C++ error in GetBuffer: could not get args[3]" << std::endl;
    return nullptr;
  }

  bool isArrayBuffer;
  status = napi_is_arraybuffer(env, args[3], &isArrayBuffer);
  if (status != napi_ok || isArrayBuffer != true) {
    std::cerr << "C++ error in GetBuffer: args[3] is not arraybuffer" << std::endl;
    return nullptr;
  }  

  size_t abLength;
  float* abData;
  status = napi_get_arraybuffer_info(env, args[3],
    (void **)&abData,
    &abLength);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetBuffer: could not get arraybuffer info of args[3]" << std::endl;
    return nullptr;
  }

  uint32_t nFrames = getBuffer(clientPointer, expectedFrameCount, maximumFrameCount, (char *)abData);

  napi_value result;
  status = napi_create_int32(env, nFrames, &result);
  if (status != napi_ok) {
    std::cerr << "C++ error in GetBuffer: could not create result" << std::endl;
    return nullptr;
  }
  return result;
}


napi_value StopCapture(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_status status;

  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (status != napi_ok) {
    std::cerr << "C++ error in StopCapture: could not get args" << std::endl;
    return nullptr;
  }

  napi_valuetype value_type;
  status = napi_typeof(env, args[0], &value_type);
  if (status != napi_ok || value_type != napi_external) {
    std::cerr << "C++ error in StopCapture: could not get args[0]" << std::endl;
    return nullptr;
  }

  void* clientPointer;
  status = napi_get_value_external(env, args[0], &clientPointer);
  if (status != napi_ok) {
    std::cerr << "C++ error in StopCapture: could not get client pointer value" << std::endl;
    return nullptr;
  }

  stopCapture(clientPointer);
  return nullptr;
}

napi_value init(napi_env env, napi_value exports) {
  napi_status status;
  napi_value fn;

  status = napi_create_function(env, nullptr, 0, CreateCaptureClient, nullptr, &fn);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, exports, "CreateCaptureClient", fn);
  if (status != napi_ok) return nullptr;

  status = napi_create_function(env, nullptr, 0, InitializeCom, nullptr, &fn);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, exports, "InitializeCom", fn);
  if (status != napi_ok) return nullptr;

  status = napi_create_function(env, nullptr, 0, StartCapture, nullptr, &fn);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, exports, "StartCapture", fn);
  if (status != napi_ok) return nullptr;

  status = napi_create_function(env, nullptr, 0, GetAudioFormat, nullptr, &fn);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, exports, "GetAudioFormat", fn);
  if (status != napi_ok) return nullptr;

  status = napi_create_function(env, nullptr, 0, GetNextPacketSize, nullptr, &fn);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, exports, "GetNextPacketSize", fn);
  if (status != napi_ok) return nullptr;

  status = napi_create_function(env, nullptr, 0, GetBuffer, nullptr, &fn);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, exports, "GetBuffer", fn);
  if (status != napi_ok) return nullptr;

  status = napi_create_function(env, nullptr, 0, StopCapture, nullptr, &fn);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, exports, "StopCapture", fn);
  if (status != napi_ok) return nullptr;

  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)

}  // namespace demo 