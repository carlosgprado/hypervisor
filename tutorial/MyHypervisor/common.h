#pragma once

#ifndef _H_HYPER_COMMON
#define _H_HYPER_COMMON

#define METHOD_BUFFERED 0
#define FILE_ANY_ACCESS 0
#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

// IOCTL definitions
#define HYPER_IOCTL_TYPE 40000

#define HYPER_IOCTL_METHOD_BUFFERED \
        CTL_CODE(HYPER_IOCTL_TYPE, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif
