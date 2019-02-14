#include "jni.h"
#include <assert.h>
#include "cubeb-gonk-instances.h"

#define AUDIO_STREAM_TYPE_MUSIC 3

struct cubeb_jni {
  jobject s_audio_manager_obj = nullptr;
  jclass s_audio_manager_class = nullptr;
  jmethodID s_get_output_latency_id = nullptr;
};

extern "C"
cubeb_jni *
cubeb_jni_init()
{
  cubeb_jni * cubeb_jni_ptr = new cubeb_jni;
  assert(cubeb_jni_ptr);

  return cubeb_jni_ptr;
}

extern "C"
int cubeb_get_output_latency_from_jni(cubeb_jni * cubeb_jni_ptr)
{
  assert(cubeb_jni_ptr);
  return 0;
}

extern "C"
void cubeb_jni_destroy(cubeb_jni * cubeb_jni_ptr)
{
  delete cubeb_jni_ptr;
}
