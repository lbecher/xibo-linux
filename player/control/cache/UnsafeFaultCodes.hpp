#pragma once

enum class UnsafeFaultCode
{
    NotLicensed = 1000,
    MemoryRunningLow = 1001,
    MemoryCritical = 1002,
    PowerPointNotAvailable = 1003,
    VideoSource = 2001,
    VideoUnexpected = 2099,
    ImageUnknown = 3000,
    ImageDecode = 3001,
    ImageOutOfMemory = 3002,
    RemoteResourceFailed = 4404,
    XlfNoContent = 5000,
    XlfNoWidgetData = 5001
};
