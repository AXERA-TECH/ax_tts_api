#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include "api/ax_tts_api.h"

namespace py = pybind11;

PYBIND11_MODULE(_ax_tts_core, m) {
    m.doc() = "Axera TTS C++ API bindings";

    // AX_TTS_TYPE_E enum
    py::enum_<AX_TTS_TYPE_E>(m, "TTSType")
        .value("KOKORO", AX_KOKORO)
        .value("MELOTTS", AX_MELOTTS)
        .export_values();

    // AX_TTS_INIT_CONFIG
    py::class_<AX_TTS_INIT_CONFIG>(m, "InitConfig")
        .def(py::init<>())
        .def_readwrite("max_seq_len", &AX_TTS_INIT_CONFIG::max_seq_len)
        .def_property("model_path",
            [](const AX_TTS_INIT_CONFIG& c) { return std::string(c.model_path); },
            [](AX_TTS_INIT_CONFIG& c, const std::string& v) { snprintf(c.model_path, AX_TTS_MAX_STR_LEN, "%s", v.c_str()); })
        .def_property("espeak_data_path",
            [](const AX_TTS_INIT_CONFIG& c) { return std::string(c.espeak_data_path); },
            [](AX_TTS_INIT_CONFIG& c, const std::string& v) { snprintf(c.espeak_data_path, AX_TTS_MAX_STR_LEN, "%s", v.c_str()); })
        .def_property("language",
            [](const AX_TTS_INIT_CONFIG& c) { return std::string(c.language); },
            [](AX_TTS_INIT_CONFIG& c, const std::string& v) { snprintf(c.language, AX_TTS_MAX_STR_LEN, "%s", v.c_str()); })
        .def_property("jieba_dict_path",
            [](const AX_TTS_INIT_CONFIG& c) { return std::string(c.jieba_dict_path); },
            [](AX_TTS_INIT_CONFIG& c, const std::string& v) { snprintf(c.jieba_dict_path, AX_TTS_MAX_STR_LEN, "%s", v.c_str()); });

    // AX_TTS_RUN_CONFIG
    py::class_<AX_TTS_RUN_CONFIG>(m, "RunConfig")
        .def(py::init<>())
        .def_readwrite("speed", &AX_TTS_RUN_CONFIG::speed)
        .def_readwrite("fade_out", &AX_TTS_RUN_CONFIG::fade_out)
        .def_readwrite("sample_rate", &AX_TTS_RUN_CONFIG::sample_rate)
        .def_property("voice",
            [](const AX_TTS_RUN_CONFIG& c) { return std::string(c.voice); },
            [](AX_TTS_RUN_CONFIG& c, const std::string& v) { snprintf(c.voice, AX_TTS_MAX_STR_LEN, "%s", v.c_str()); })
        .def_property("language",
            [](const AX_TTS_RUN_CONFIG& c) { return std::string(c.language); },
            [](AX_TTS_RUN_CONFIG& c, const std::string& v) { snprintf(c.language, AX_TTS_MAX_STR_LEN, "%s", v.c_str()); });

    // AX_TTS_AUDIO (returned as numpy array through synthesize wrapper)
    py::class_<AX_TTS_AUDIO>(m, "Audio", py::buffer_protocol())
        .def_property_readonly("sample_rate", [](const AX_TTS_AUDIO& a) { return a.sample_rate; })
        .def_property_readonly("num_samples", [](const AX_TTS_AUDIO& a) { return a.num_samples; })
        .def_property_readonly("channels", [](const AX_TTS_AUDIO& a) { return a.channels; });

    // Core C API functions
    m.def("init", &AX_TTS_Init, "Initialize TTS engine",
          py::arg("tts_type"), py::arg("init_config"));

    m.def("uninit", &AX_TTS_Uninit, "Release TTS resources",
          py::arg("handle"));

    m.def("run", [](AX_TTS_HANDLE handle,
                     const std::string& text,
                     AX_TTS_RUN_CONFIG* run_config) -> py::tuple {
        AX_TTS_AUDIO* audio = nullptr;
        int ret = AX_TTS_Run(handle, text.c_str(), run_config, &audio);
        if (ret != 0 || !audio) {
            throw std::runtime_error("AX_TTS_Run failed with code " + std::to_string(ret));
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
