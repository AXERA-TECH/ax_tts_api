#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include "api/ax_tts_api.h"

namespace py = pybind11;

PYBIND11_MODULE(_ax_tts_core, m) {
    m.doc() = "Axera TTS C++ API bindings";

    // AX_TTS_TYPE_E enum
    py::enum_<AX_TTS_TYPE_E>(m, "TTSType", py::arithmetic())
        .value("KOKORO", AX_KOKORO)
        .value("MELOTTS", AX_MELOTTS)
        .export_values();

    // AX_TTS_INIT_CONFIG
    py::class_<AX_TTS_INIT_CONFIG>(m, "InitConfig")
        .def(py::init<>())
        .def_readwrite("max_seq_len", &AX_TTS_INIT_CONFIG::max_seq_len)
        .def_readwrite("model_path", &AX_TTS_INIT_CONFIG::model_path)
        .def_readwrite("espeak_data_path", &AX_TTS_INIT_CONFIG::espeak_data_path)
        .def_readwrite("jieba_dict_path", &AX_TTS_INIT_CONFIG::jieba_dict_path);

    // AX_TTS_RUN_CONFIG
    py::class_<AX_TTS_RUN_CONFIG>(m, "RunConfig")
        .def(py::init<>())
        .def_readwrite("speed", &AX_TTS_RUN_CONFIG::speed)
        .def_readwrite("fade_out", &AX_TTS_RUN_CONFIG::fade_out)
        .def_readwrite("sample_rate", &AX_TTS_RUN_CONFIG::sample_rate)
        .def_readwrite("voice", &AX_TTS_RUN_CONFIG::voice)
        .def_readwrite("language", &AX_TTS_RUN_CONFIG::language);

    // AX_TTS_AUDIO (returned as numpy array through synthesize wrapper)
    py::class_<AX_TTS_AUDIO>(m, "Audio", py::buffer_protocol())
        .def_property_readonly("sample_rate", [](const AX_TTS_AUDIO& a) { return a.sample_rate; })
        .def_property_readonly("num_samples", [](const AX_TTS_AUDIO& a) { return a.num_samples; })
        .def_property_readonly("channels", [](const AX_TTS_AUDIO& a) { return a.channels; });


    // AX_TTS_ERROR_E enum
    py::enum_<AX_TTS_ERROR_E>(m, "ErrorCode", py::arithmetic())
        .value("OK", AX_TTS_OK)
        .value("ERR_NULL_HANDLE", AX_TTS_ERR_NULL_HANDLE)
        .value("ERR_NULL_CONFIG", AX_TTS_ERR_NULL_CONFIG)
        .value("ERR_NULL_INPUT", AX_TTS_ERR_NULL_INPUT)
        .value("ERR_INVALID_MODEL_PATH", AX_TTS_ERR_INVALID_MODEL_PATH)
        .value("ERR_MODEL_LOAD_FAILED", AX_TTS_ERR_MODEL_LOAD_FAILED)
        .value("ERR_INFERENCE_FAILED", AX_TTS_ERR_INFERENCE_FAILED)
        .value("ERR_INVALID_LANGUAGE", AX_TTS_ERR_INVALID_LANGUAGE)
        .value("ERR_OUT_OF_MEMORY", AX_TTS_ERR_OUT_OF_MEMORY)
        .value("ERR_UNKNOWN_MODEL", AX_TTS_ERR_UNKNOWN_MODEL)
        .value("ERR_FRONTEND_FAILED", AX_TTS_ERR_FRONTEND_FAILED)
        .value("ERR_INTERNAL", AX_TTS_ERR_INTERNAL)
        .export_values();
    // Core C API functions
    m.def("init", [](AX_TTS_TYPE_E tts_type, const AX_TTS_INIT_CONFIG& config) -> py::tuple {
        AX_TTS_HANDLE handle = NULL;
        AX_TTS_ERROR_E err = AX_TTS_Init(tts_type, &config, &handle);
        return py::make_tuple(static_cast<int>(err), handle);
    }, "Initialize TTS engine", py::arg("tts_type"), py::arg("init_config"));

    m.def("get_version", &AX_TTS_GetVersion, "Get API version string");

    m.def("uninit", &AX_TTS_Uninit, "Release TTS resources",
          py::arg("handle"));

    m.def("run", [](AX_TTS_HANDLE handle,
                     const std::string& text,
                     const AX_TTS_RUN_CONFIG& run_config) -> py::tuple {
        AX_TTS_AUDIO* audio = nullptr;
        AX_TTS_ERROR_E err = AX_TTS_Run(handle, text.c_str(), &run_config, &audio);
        if (err != AX_TTS_OK || !audio) {
            throw std::runtime_error("AX_TTS_Run failed with code " + std::to_string(static_cast<int>(err)));
        }

        // Return as numpy array and free C memory
        py::capsule free_when_done(audio, [](void* p) { free(p); });

        auto arr = py::array_t<float>(
            {static_cast<ssize_t>(audio->num_samples)},
            {sizeof(float)},
            audio->data,
            free_when_done
        );

        return py::make_tuple(audio->sample_rate, arr);
    }, py::arg("handle"), py::arg("text"), py::arg("run_config"));
}
