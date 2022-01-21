/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.tv.input@1.0-service"
#include <android-base/logging.h>

#include "TvInput.h"

#ifndef container_of
#define container_of(ptr, type, member) \
    (type *)((char*)(ptr) - offsetof(type, member))
#endif

namespace android {
namespace hardware {
namespace tv {
namespace input {
namespace V1_0 {
namespace implementation {

static_assert(TV_INPUT_TYPE_OTHER_HARDWARE == static_cast<int>(TvInputType::OTHER),
        "TvInputType::OTHER must match legacy value.");
static_assert(TV_INPUT_TYPE_TUNER == static_cast<int>(TvInputType::TUNER),
        "TvInputType::TUNER must match legacy value.");
static_assert(TV_INPUT_TYPE_COMPOSITE == static_cast<int>(TvInputType::COMPOSITE),
        "TvInputType::COMPOSITE must match legacy value.");
static_assert(TV_INPUT_TYPE_SVIDEO == static_cast<int>(TvInputType::SVIDEO),
        "TvInputType::SVIDEO must match legacy value.");
static_assert(TV_INPUT_TYPE_SCART == static_cast<int>(TvInputType::SCART),
        "TvInputType::SCART must match legacy value.");
static_assert(TV_INPUT_TYPE_COMPONENT == static_cast<int>(TvInputType::COMPONENT),
        "TvInputType::COMPONENT must match legacy value.");
static_assert(TV_INPUT_TYPE_VGA == static_cast<int>(TvInputType::VGA),
        "TvInputType::VGA must match legacy value.");
static_assert(TV_INPUT_TYPE_DVI == static_cast<int>(TvInputType::DVI),
        "TvInputType::DVI must match legacy value.");
static_assert(TV_INPUT_TYPE_HDMI == static_cast<int>(TvInputType::HDMI),
        "TvInputType::HDMI must match legacy value.");
static_assert(TV_INPUT_TYPE_DISPLAY_PORT == static_cast<int>(TvInputType::DISPLAY_PORT),
        "TvInputType::DISPLAY_PORT must match legacy value.");

static_assert(TV_INPUT_EVENT_DEVICE_AVAILABLE == static_cast<int>(
        TvInputEventType::DEVICE_AVAILABLE),
        "TvInputEventType::DEVICE_AVAILABLE must match legacy value.");
static_assert(TV_INPUT_EVENT_DEVICE_UNAVAILABLE == static_cast<int>(
        TvInputEventType::DEVICE_UNAVAILABLE),
        "TvInputEventType::DEVICE_UNAVAILABLE must match legacy value.");
static_assert(TV_INPUT_EVENT_STREAM_CONFIGURATIONS_CHANGED == static_cast<int>(
        TvInputEventType::STREAM_CONFIGURATIONS_CHANGED),
        "TvInputEventType::STREAM_CONFIGURATIONS_CHANGED must match legacy value.");
static_assert(TV_INPUT_EVENT_CAPTURE_SUCCEEDED == static_cast<int>(
        TvInputEventType::STREAM_CAPTURE_SUCCEEDED),
        "TvInputEventType::STREAM_CAPTURE_SUCCEEDED must match legacy value.");
static_assert(TV_INPUT_EVENT_CAPTURE_FAILED == static_cast<int>(
        TvInputEventType::STREAM_CAPTURE_FAILED),
        "TvInputEventType::STREAM_CAPTURE_FAILED must match legacy value.");

sp<ITvInputCallback> TvInput::mCallback = nullptr;

TvInput::TvInput(tv_input_device_t* device) : mDevice(device) {
    mCallbackOps.notify = &TvInput::notify;
}

TvInput::~TvInput() {
    if (mDevice != nullptr) {
        free(mDevice);
    }
}

Return<Result> TvInput::requestCapture(int32_t deviceId, int32_t streamId, uint64_t buffId, const hidl_handle& buffer, int32_t seq) {
    mDevice->request_capture(mDevice, deviceId, streamId, buffId, buffer, seq);
    return Result::OK;
}

Return<void> TvInput::cancelCapture(int32_t deviceId, int32_t streamId, int32_t seq) {
    mDevice->cancel_capture(mDevice, deviceId, streamId, seq);
    return Void();
}

Return<Result> TvInput::setPreviewInfo(int32_t deviceId, int32_t streamId, int32_t top, int32_t left, int32_t width, int32_t height, int32_t extInfo) {
    mDevice->set_preview_info(deviceId, streamId, top, left, width, height, extInfo);
    int ret = mDevice->set_preview_info(deviceId, streamId, top, left, width, height,extInfo);
    Result res = Result::UNKNOWN;
    if (ret == 0) {
        res = Result::OK;
    } else if (ret == -ENOENT) {
        res = Result::INVALID_STATE;
    } else if (ret == -EINVAL) {
        res = Result::INVALID_ARGUMENTS;
    }
    return res;
}

Return<void> TvInput::setSinglePreviewBuffer(const PreviewBuffer& buff) {
    mDevice->set_preview_buffer(buff.buffer, buff.bufferId);
    return Void();
}

// Methods from ::android::hardware::tv_input::V1_0::ITvInput follow.
Return<void> TvInput::setCallback(const sp<ITvInputCallback>& callback)  {
    mCallback = callback;
    if (mCallback != nullptr) {
        mDevice->initialize(mDevice, &mCallbackOps, nullptr);
    }
    return Void();
}

Return<void> TvInput::getStreamConfigurations(int32_t deviceId, getStreamConfigurations_cb cb)  {
    int32_t configCount = 0;
    const tv_stream_config_t* configs = nullptr;
    int ret = mDevice->get_stream_configurations(mDevice, deviceId, &configCount, &configs);
    Result res = Result::UNKNOWN;
    hidl_vec<TvStreamConfig> tvStreamConfigs;
    if (ret == 0) {
        res = Result::OK;
        tvStreamConfigs.resize(configCount);
        int32_t pos = 0;
        for (int32_t i = 0; i < configCount; i++) {
            tvStreamConfigs[pos].streamId = configs[i].stream_id;
            tvStreamConfigs[pos].maxVideoWidth = configs[i].max_video_width;
            tvStreamConfigs[pos].maxVideoHeight = configs[i].max_video_height;
            if (configs[i].type == TV_STREAM_TYPE_BUFFER_PRODUCER) {
                tvStreamConfigs[pos].format = configs[i].format;
                tvStreamConfigs[pos].usage = configs[i].usage;
                tvStreamConfigs[pos].width = configs[i].width;
                tvStreamConfigs[pos].height = configs[i].height;
                tvStreamConfigs[pos].buffCount = configs[i].buffCount;
            }
            pos++;
        }
    } else if (ret == -EINVAL) {
        res = Result::INVALID_ARGUMENTS;
    }
    cb(res, tvStreamConfigs);
    return Void();
}

Return<void> TvInput::openStream(int32_t deviceId, int32_t streamId, int32_t streamType, openStream_cb cb)  {
    tv_stream_t stream;
    stream.stream_id = streamId;
    stream.type = streamType;
    int ret = mDevice->open_stream(mDevice, deviceId, &stream);
    Result res = Result::UNKNOWN;
    native_handle_t* sidebandStream = nullptr;
    if (ret == 0) {
        // if (isSupportedStreamType(stream.type)) {
        if (stream.type != TV_STREAM_TYPE_BUFFER_PRODUCER) {
            res = Result::OK;
            sidebandStream = stream.sideband_stream_source_handle;
        } else {
            res = Result::OK;
        }
    } else {
        if (ret == -EBUSY) {
            res = Result::NO_RESOURCE;
        } else if (ret == -EEXIST) {
            res = Result::INVALID_STATE;
        } else if (ret == -EINVAL) {
            res = Result::INVALID_ARGUMENTS;
        }
    }
    cb(res, sidebandStream);
    return Void();
}

Return<Result> TvInput::closeStream(int32_t deviceId, int32_t streamId)  {
    int ret = mDevice->close_stream(mDevice, deviceId, streamId);
    Result res = Result::UNKNOWN;
    if (ret == 0) {
        res = Result::OK;
    } else if (ret == -ENOENT) {
        res = Result::INVALID_STATE;
    } else if (ret == -EINVAL) {
        res = Result::INVALID_ARGUMENTS;
    }
    return res;
}

// static
void TvInput::notify(struct tv_input_device* __unused, tv_input_event_t* event,
        void* __unused) {
    if (mCallback != nullptr && event != nullptr) {
        TvInputEvent tvInputEvent;
        tvInputEvent.type = static_cast<TvInputEventType>(event->type);
        if (event->type >= TV_INPUT_EVENT_CAPTURE_SUCCEEDED) {
            tvInputEvent.deviceInfo.deviceId = event->capture_result.device_id;
            tvInputEvent.deviceInfo.streamId = event->capture_result.stream_id;
            tvInputEvent.capture_result.buffId = event->capture_result.buff_id;
            tvInputEvent.capture_result.buffSeq = event->capture_result.seq;
            // tvInputEvent.capture_result.buffer = event->capture_result.buffer;
        } else {
        tvInputEvent.deviceInfo.deviceId = event->device_info.device_id;
            tvInputEvent.deviceInfo.type = static_cast<TvInputType>(
                    event->device_info.type);
            tvInputEvent.deviceInfo.portId = event->device_info.hdmi.port_id;
            tvInputEvent.deviceInfo.cableConnectionStatus = CableConnectionStatus::UNKNOWN;
            // TODO: Ensure the legacy audio type code is the same once audio HAL default
            // implementation is ready.
            tvInputEvent.deviceInfo.audioType = static_cast<AudioDevice>(
                    event->device_info.audio_type);
            memset(tvInputEvent.deviceInfo.audioAddress.data(), 0,
                    tvInputEvent.deviceInfo.audioAddress.size());
            const char* address = event->device_info.audio_address;
            if (address != nullptr) {
                size_t size = strlen(address);
                if (size > tvInputEvent.deviceInfo.audioAddress.size()) {
                    LOG(ERROR) << "Audio address is too long. Address:" << address << "";
                    return;
                }
                for (size_t i = 0; i < size; ++i) {
                    tvInputEvent.deviceInfo.audioAddress[i] =
                        static_cast<uint8_t>(event->device_info.audio_address[i]);
                }
            }
        }
        mCallback->notify(tvInputEvent);
    }
}

// static
uint32_t TvInput::getSupportedConfigCount(uint32_t configCount,
        const tv_stream_config_t* configs) {
    uint32_t supportedConfigCount = 0;
    for (uint32_t i = 0; i < configCount; ++i) {
        if (isSupportedStreamType(configs[i].type)) {
            supportedConfigCount++;
        }
    }
    return supportedConfigCount;
}

// static
bool TvInput::isSupportedStreamType(int type) {
    // Buffer producer type is no longer supported.
    return type != TV_STREAM_TYPE_BUFFER_PRODUCER;
}

ITvInput* HIDL_FETCH_ITvInput(const char* /* name */) {
    int ret = 0;
    const hw_module_t* hw_module = nullptr;
    tv_input_device_t* input_device;
    ret = hw_get_module(TV_INPUT_HARDWARE_MODULE_ID, &hw_module);
    if (ret == 0 && hw_module->methods->open != nullptr) {
        ret = hw_module->methods->open(hw_module, TV_INPUT_DEFAULT_DEVICE,
                reinterpret_cast<hw_device_t**>(&input_device));
        if (ret == 0) {
            return new TvInput(input_device);
        }
        else {
            LOG(ERROR) << "Passthrough failed to load legacy HAL.";
            return nullptr;
        }
    }
    else {
        LOG(ERROR) << "hw_get_module " << TV_INPUT_HARDWARE_MODULE_ID
                   << " failed: " << ret;
        return nullptr;
    }
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace input
}  // namespace tv
}  // namespace hardware
}  // namespace android
