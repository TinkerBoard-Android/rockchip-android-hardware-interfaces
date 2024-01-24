/*
 * Copyright (C) 2022 The Android Open Source Project
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

#define LOG_TAG "android.hardware.tv.hdmi.connection"
#include <android-base/logging.h>
#include <fcntl.h>
#include <utils/Log.h>
#include <hardware/hardware.h>
#include <hardware/hdmi_cec.h>

#include "HdmiConnectionMock.h"

using ndk::ScopedAStatus;

namespace android {
namespace hardware {
namespace tv {
namespace hdmi {
namespace connection {
namespace implementation {

std::shared_ptr<IHdmiConnectionCallback> HdmiConnectionMock::mCallback = nullptr;

void HdmiConnectionMock::serviceDied(void* cookie) {
    ALOGE("[RK_HDMI_CEC_imp_aidl connection] HdmiConnectionMock died");
    ALOGD("[RK_HDMI_CEC_imp_aidl connection] %s.", __FUNCTION__);

    auto hdmi = static_cast<HdmiConnectionMock*>(cookie);
	mCallback = nullptr;
	rk_hdmi_connection_destroy(&(hdmi->rkdev));

}

ScopedAStatus HdmiConnectionMock::getPortInfo(std::vector<HdmiPortInfo>* _aidl_return) {
    struct hdmi_port_info* legacyPorts;
    int numPorts;

    ALOGD("[RK_HDMI_CEC_imp_aidl connection] %s.", __FUNCTION__);

    //hidl_vec<HdmiPortInfo> portInfos;
    hdmi_connection_get_port_info(&rkdev, &legacyPorts, &numPorts);
    //portInfos.resize(numPorts);
	mPortInfos.resize(numPorts);
    for (int i = 0; i < numPorts; ++i) {
        mPortInfos[i] = {
            .type = static_cast<HdmiPortType>(legacyPorts[i].type),
            .portId = static_cast<int32_t>(legacyPorts[i].port_id),
            .cecSupported = legacyPorts[i].cec_supported != 0,
            .arcSupported = legacyPorts[i].arc_supported != 0,
            .eArcSupported = false,
            .physicalAddress = legacyPorts[i].physical_address
        };
    }

    mTotalPorts = numPorts;

    *_aidl_return = mPortInfos;
    return ScopedAStatus::ok();
}

ScopedAStatus HdmiConnectionMock::isConnected(int32_t portId, bool* _aidl_return) {
    ALOGD("[RK_HDMI_CEC_imp_aidl connection] %s.", __FUNCTION__);

	*_aidl_return = hdmi_connection_is_connected(&rkdev, portId) > 0;
    // Maintain port connection status and update on hotplug event

    return ScopedAStatus::ok();
}

ScopedAStatus HdmiConnectionMock::setCallback(
        const std::shared_ptr<IHdmiConnectionCallback>& callback) {
    ALOGD("[RK_HDMI_CEC_imp_aidl connection] %s.", __FUNCTION__);

    if (mCallback != nullptr) {
        mCallback = nullptr;
    }

    if (callback != nullptr) {
        mCallback = callback;
        AIBinder_linkToDeath(this->asBinder().get(), mDeathRecipient.get(), 0 /* cookie */);

        hdmi_connection_register_event_callback(&rkdev, eventCallback, nullptr);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus HdmiConnectionMock::setHpdSignal(HpdSignal signal, int32_t portId) {
    ALOGD("[RK_HDMI_CEC_imp_aidl connection] %s.", __FUNCTION__);

    if (portId > mTotalPorts || portId < 1) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

	hdmi_connection_set_phd_signal(&rkdev, portId, static_cast<int>(signal));
	
    return ScopedAStatus::ok();
}

ScopedAStatus HdmiConnectionMock::getHpdSignal(int32_t portId, HpdSignal* _aidl_return) {
    ALOGD("[RK_HDMI_CEC_imp_aidl connection] %s.", __FUNCTION__);
    if (portId > mTotalPorts || portId < 1) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    *_aidl_return = static_cast<HpdSignal>(hdmi_connection_get_phd_signal(&rkdev, portId));
    return ScopedAStatus::ok();
}


void HdmiConnectionMock::printEventBuf(const char* msg_buf, int len) {
    int i, size = 0;
    const int bufSize = MESSAGE_BODY_MAX_LENGTH * 3;
    // Use 2 characters for each byte in the message plus 1 space
    char buf[bufSize] = {0};

    // Messages longer than max length will be truncated.
    for (i = 0; i < len && size < bufSize; i++) {
        size += sprintf(buf + size, " %02x", msg_buf[i]);
    }
    ALOGD("[halimp_aidl] %s, msg:%.*s", __FUNCTION__, size, buf);
}


HdmiConnectionMock::HdmiConnectionMock() {
    ALOGE("[RK_HDMI_CEC_imp_aidl connection] Opening a RK HDMI Connection HAL AIDL Implementation.");
    mCallback = nullptr;
    rk_hdmi_connection_init(&rkdev);

    mPortInfos.resize(mTotalPorts);
    //mPortConnectionStatus.resize(mTotalPorts);
    //mHpdSignal.resize(mTotalPorts);
    mPortInfos[0] = {.type = HdmiPortType::OUTPUT,
                     .portId = static_cast<uint32_t>(1),
                     .cecSupported = true,
                     .arcSupported = false,
                     .eArcSupported = false,
                     .physicalAddress = mPhysicalAddress};
    //mPortConnectionStatus[0] = false;
    //mHpdSignal[0] = HpdSignal::HDMI_HPD_PHYSICAL;

    mDeathRecipient = ndk::ScopedAIBinder_DeathRecipient(AIBinder_DeathRecipient_new(serviceDied));
}

}  // namespace implementation
}  // namespace connection
}  // namespace hdmi
}  // namespace tv
}  // namespace hardware
}  // namespace android
