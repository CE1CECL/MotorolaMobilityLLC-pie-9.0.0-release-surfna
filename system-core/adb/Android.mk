# Copyright 2005 The Android Open Source Project
#
# Android.mk for adb
#

LOCAL_PATH:= $(call my-dir)

include $(LOCAL_PATH)/../platform_tools_tool_version.mk

adb_host_sanitize :=
adb_target_sanitize :=

ADB_COMMON_CFLAGS := \
    -frtti \
    -Wall -Wextra  \
    -Wno-unused-parameter \
    -Wno-missing-field-initializers \
    -Wvla \
    -DADB_VERSION="\"$(tool_version)\"" \

ADB_COMMON_posix_CFLAGS := \
    -Wexit-time-destructors \
    -Wthread-safety \

ADB_COMMON_linux_CFLAGS := \
    $(ADB_COMMON_posix_CFLAGS) \

ADB_COMMON_darwin_CFLAGS := \
    $(ADB_COMMON_posix_CFLAGS) \

# Define windows.h and tchar.h Unicode preprocessor symbols so that
# CreateFile(), _tfopen(), etc. map to versions that take wchar_t*, breaking the
# build if you accidentally pass char*. Fix by calling like:
#   std::wstring path_wide;
#   if (!android::base::UTF8ToWide(path_utf8, &path_wide)) { /* error handling */ }
#   CreateFileW(path_wide.c_str());
ADB_COMMON_windows_CFLAGS := \
    -DUNICODE=1 -D_UNICODE=1 \
    -D_POSIX_SOURCE

# libadb
# =========================================================

# Much of adb is duplicated in bootable/recovery/minadb and fastboot. Changes
# made to adb rarely get ported to the other two, so the trees have diverged a
# bit. We'd like to stop this because it is a maintenance nightmare, but the
# divergence makes this difficult to do all at once. For now, we will start
# small by moving common files into a static library. Hopefully some day we can
# get enough of adb in here that we no longer need minadb. https://b/17626262
LIBADB_SRC_FILES := \
    adb.cpp \
    adb_io.cpp \
    adb_listeners.cpp \
    adb_trace.cpp \
    adb_utils.cpp \
    fdevent.cpp \
    sockets.cpp \
    socket_spec.cpp \
    sysdeps/errno.cpp \
    transport.cpp \
    transport_local.cpp \
    transport_usb.cpp \

LIBADB_TEST_SRCS := \
    adb_io_test.cpp \
    adb_listeners_test.cpp \
    adb_utils_test.cpp \
    fdevent_test.cpp \
    socket_spec_test.cpp \
    socket_test.cpp \
    sysdeps_test.cpp \
    sysdeps/stat_test.cpp \
    transport_test.cpp \

LIBADB_CFLAGS := \
    $(ADB_COMMON_CFLAGS) \
    -fvisibility=hidden \

LIBADB_linux_CFLAGS := \
    $(ADB_COMMON_linux_CFLAGS) \

LIBADB_darwin_CFLAGS := \
    $(ADB_COMMON_darwin_CFLAGS) \

LIBADB_windows_CFLAGS := \
    $(ADB_COMMON_windows_CFLAGS) \

LIBADB_darwin_SRC_FILES := \
    sysdeps_unix.cpp \
    sysdeps/posix/network.cpp \
    client/usb_dispatch.cpp \
    client/usb_libusb.cpp \
    client/usb_osx.cpp \

LIBADB_linux_SRC_FILES := \
    sysdeps_unix.cpp \
    sysdeps/posix/network.cpp \
    client/usb_dispatch.cpp \
    client/usb_libusb.cpp \
    client/usb_linux.cpp \

LIBADB_windows_SRC_FILES := \
    sysdeps_win32.cpp \
    sysdeps/win32/errno.cpp \
    sysdeps/win32/stat.cpp \
    client/usb_dispatch.cpp \
    client/usb_libusb.cpp \
    client/usb_windows.cpp \

LIBADB_TEST_windows_SRCS := \
    sysdeps/win32/errno_test.cpp \
    sysdeps_win32_test.cpp \

include $(CLEAR_VARS)
LOCAL_MODULE := libadbd_usb
LOCAL_CFLAGS := $(LIBADB_CFLAGS) -DADB_HOST=0
LOCAL_SRC_FILES := daemon/usb.cpp

LOCAL_SANITIZE := $(adb_target_sanitize)

# Even though we're building a static library (and thus there's no link step for
# this to take effect), this adds the includes to our path.
LOCAL_STATIC_LIBRARIES := libcrypto_utils libcrypto libbase libasyncio

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libadbd
LOCAL_CFLAGS := $(LIBADB_CFLAGS) -DADB_HOST=0
ifeq ($(TARGET_DEVICE),payton)
LOCAL_CFLAGS += -DPATCH_RESET_ADBD=1
endif
LOCAL_SRC_FILES := \
    $(LIBADB_SRC_FILES) \
    adbd_auth.cpp \
    jdwp_service.cpp \
    sysdeps/posix/network.cpp \

LOCAL_SANITIZE := $(adb_target_sanitize)

# Even though we're building a static library (and thus there's no link step for
# this to take effect), this adds the includes to our path.
LOCAL_STATIC_LIBRARIES := libcrypto_utils libcrypto libqemu_pipe libbase

LOCAL_WHOLE_STATIC_LIBRARIES := libadbd_usb

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libadb
LOCAL_MODULE_HOST_OS := darwin linux windows
LOCAL_CFLAGS := $(LIBADB_CFLAGS) -DADB_HOST=1
LOCAL_CFLAGS_windows := $(LIBADB_windows_CFLAGS)
LOCAL_CFLAGS_linux := $(LIBADB_linux_CFLAGS)
LOCAL_CFLAGS_darwin := $(LIBADB_darwin_CFLAGS)
LOCAL_SRC_FILES := \
    $(LIBADB_SRC_FILES) \
    adb_auth_host.cpp \
    transport_mdns.cpp \

LOCAL_SRC_FILES_darwin := $(LIBADB_darwin_SRC_FILES)
LOCAL_SRC_FILES_linux := $(LIBADB_linux_SRC_FILES)
LOCAL_SRC_FILES_windows := $(LIBADB_windows_SRC_FILES)

LOCAL_SANITIZE := $(adb_host_sanitize)

# Even though we're building a static library (and thus there's no link step for
# this to take effect), this adds the includes to our path.
LOCAL_STATIC_LIBRARIES := libcrypto_utils libcrypto libbase libmdnssd libusb

LOCAL_C_INCLUDES_windows := development/host/windows/usb/api/
LOCAL_MULTILIB := first

include $(BUILD_HOST_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := adbd_test
LOCAL_CFLAGS := -DADB_HOST=0 $(LIBADB_CFLAGS)
LOCAL_SRC_FILES := \
    $(LIBADB_TEST_SRCS) \
    $(LIBADB_TEST_linux_SRCS) \
    shell_service.cpp \
    shell_service_protocol.cpp \
    shell_service_protocol_test.cpp \
    shell_service_test.cpp \

LOCAL_SANITIZE := $(adb_target_sanitize)
LOCAL_STATIC_LIBRARIES := libadbd libcrypto_utils libcrypto libusb libmdnssd
LOCAL_SHARED_LIBRARIES := liblog libbase libcutils
include $(BUILD_NATIVE_TEST)

# libdiagnose_usb
# =========================================================

include $(CLEAR_VARS)
LOCAL_MODULE := libdiagnose_usb
LOCAL_MODULE_HOST_OS := darwin linux windows
LOCAL_CFLAGS := $(LIBADB_CFLAGS)
LOCAL_SRC_FILES := diagnose_usb.cpp
# Even though we're building a static library (and thus there's no link step for
# this to take effect), this adds the includes to our path.
LOCAL_STATIC_LIBRARIES := libbase
include $(BUILD_HOST_STATIC_LIBRARY)

# adb_test
# =========================================================

include $(CLEAR_VARS)
LOCAL_MODULE := adb_test
LOCAL_MODULE_HOST_OS := darwin linux windows
LOCAL_CFLAGS := -DADB_HOST=1 $(LIBADB_CFLAGS)
LOCAL_CFLAGS_windows := $(LIBADB_windows_CFLAGS)
LOCAL_CFLAGS_linux := $(LIBADB_linux_CFLAGS)
LOCAL_CFLAGS_darwin := $(LIBADB_darwin_CFLAGS)
LOCAL_SRC_FILES := \
    $(LIBADB_TEST_SRCS) \
    adb_client.cpp \
    bugreport.cpp \
    bugreport_test.cpp \
    line_printer.cpp \
    services.cpp \
    shell_service_protocol.cpp \
    shell_service_protocol_test.cpp \

LOCAL_SRC_FILES_linux := $(LIBADB_TEST_linux_SRCS)
LOCAL_SRC_FILES_darwin := $(LIBADB_TEST_darwin_SRCS)
LOCAL_SRC_FILES_windows := $(LIBADB_TEST_windows_SRCS)
LOCAL_SANITIZE := $(adb_host_sanitize)
LOCAL_STATIC_LIBRARIES := \
    libadb \
    libbase \
    libcrypto_utils \
    libcrypto \
    libcutils \
    libdiagnose_usb \
    libmdnssd \
    libgmock_host \
    libusb \

# Set entrypoint to wmain from sysdeps_win32.cpp instead of main
LOCAL_LDFLAGS_windows := -municode
LOCAL_LDLIBS_linux := -lrt -ldl -lpthread
LOCAL_LDLIBS_darwin := -framework CoreFoundation -framework IOKit -lobjc
LOCAL_LDLIBS_windows := -lws2_32 -luserenv
LOCAL_SHARED_LIBRARIES_windows := AdbWinApi

LOCAL_MULTILIB := first

include $(BUILD_HOST_NATIVE_TEST)

# adb host tool
# =========================================================
include $(CLEAR_VARS)

LOCAL_LDLIBS_linux := -lrt -ldl -lpthread

LOCAL_LDLIBS_darwin := -lpthread -framework CoreFoundation -framework IOKit -framework Carbon -lobjc

# Use wmain instead of main
LOCAL_LDFLAGS_windows := -municode
LOCAL_LDLIBS_windows := -lws2_32 -lgdi32
LOCAL_SHARED_LIBRARIES_windows := AdbWinApi
LOCAL_REQUIRED_MODULES_windows := AdbWinUsbApi

LOCAL_SRC_FILES := \
    adb_client.cpp \
    bugreport.cpp \
    client/main.cpp \
    console.cpp \
    commandline.cpp \
    file_sync_client.cpp \
    line_printer.cpp \
    services.cpp \
    shell_service_protocol.cpp \

LOCAL_CFLAGS += \
    $(ADB_COMMON_CFLAGS) \
    -D_GNU_SOURCE \
    -DADB_HOST=1 \

LOCAL_CFLAGS_windows := \
    $(ADB_COMMON_windows_CFLAGS)

LOCAL_CFLAGS_linux := \
    $(ADB_COMMON_linux_CFLAGS) \

LOCAL_CFLAGS_darwin := \
    $(ADB_COMMON_darwin_CFLAGS) \
    -Wno-sizeof-pointer-memaccess -Wno-unused-parameter \

LOCAL_MODULE := adb
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_HOST_OS := darwin linux windows

LOCAL_SANITIZE := $(adb_host_sanitize)
LOCAL_STATIC_LIBRARIES := \
    libadb \
    libbase \
    libcrypto_utils \
    libcrypto \
    libdiagnose_usb \
    liblog \
    libmdnssd \
    libusb \

# Don't use libcutils on Windows.
LOCAL_STATIC_LIBRARIES_darwin := libcutils
LOCAL_STATIC_LIBRARIES_linux := libcutils

LOCAL_CXX_STL := libc++_static

# Don't add anything here, we don't want additional shared dependencies
# on the host adb tool, and shared libraries that link against libc++
# will violate ODR
LOCAL_SHARED_LIBRARIES :=

include $(BUILD_HOST_EXECUTABLE)

$(call dist-for-goals,dist_files sdk win_sdk,$(LOCAL_BUILT_MODULE))
ifdef HOST_CROSS_OS
# Archive adb.exe for win_sdk build.
$(call dist-for-goals,win_sdk,$(ALL_MODULES.host_cross_adb.BUILT))
endif


# adbd device daemon
# =========================================================

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    daemon/main.cpp \
    daemon/mdns.cpp \
    services.cpp \
    file_sync_service.cpp \
    framebuffer_service.cpp \
    remount_service.cpp \
    set_verity_enable_state_service.cpp \
    shell_service.cpp \
    shell_service_protocol.cpp \

LOCAL_CFLAGS := \
    $(ADB_COMMON_CFLAGS) \
    $(ADB_COMMON_linux_CFLAGS) \
    -DADB_HOST=0 \
    -D_GNU_SOURCE \
    -Wno-deprecated-declarations \

ifeq ($(MOT_ALLOW_ADBD_NO_AUTH),true)
LOCAL_CFLAGS += -DALLOW_ADBD_NO_AUTH=1
else
LOCAL_CFLAGS += -DALLOW_ADBD_NO_AUTH=$(if $(filter userdebug eng,$(TARGET_BUILD_VARIANT)),1,0)
endif

ifneq (,$(filter userdebug eng,$(TARGET_BUILD_VARIANT)))
LOCAL_CFLAGS += -DALLOW_ADBD_DISABLE_VERITY=1
LOCAL_CFLAGS += -DALLOW_ADBD_ROOT=1
endif

ifeq ($(MOT_TARGET_BUILD_ADDITIONAL_CONFIG),bldccfg)
LOCAL_CFLAGS += -DALLOW_ADBD_DISABLE_VERITY=1
LOCAL_CFLAGS += -DMOTO_BLD_C
endif

LOCAL_MODULE := adbd

LOCAL_FORCE_STATIC_EXECUTABLE := true

LOCAL_SANITIZE := $(adb_target_sanitize)
LOCAL_STRIP_MODULE := keep_symbols
LOCAL_STATIC_LIBRARIES := \
    libadbd \
    libasyncio \
    libavb_user \
    libbase \
    libqemu_pipe \
    libbootloader_message \
    libfs_mgr \
    libfec \
    libfec_rs \
    libselinux \
    liblog \
    libext4_utils \
    libsquashfs_utils \
    libcutils \
    libbase \
    libcrypto_utils \
    libcrypto \
    libminijail \
    libmdnssd \
    libdebuggerd_handler \

include $(BUILD_EXECUTABLE)

# adb integration test
# =========================================================
$(call dist-for-goals,sdk,$(ALL_MODULES.adb_integration_test_adb.BUILT))
$(call dist-for-goals,sdk,$(ALL_MODULES.adb_integration_test_device.BUILT))

include $(call first-makefiles-under,$(LOCAL_PATH))
