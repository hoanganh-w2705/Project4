#pragma once

#include "ntdef.h"
#include "fltKernel.h"
#include "ntddstor.h"
#include "storduid.h"
#include "ntstrsafe.h"

#define EXTERNAL_DEVICE_MONITOR_DEVICE_NAME		L"\\Device\\ExternalDeviceMonitor"
#define EXTERNAL_DEVICE_MONITOR_DEVICE_SYMLINK	L"\\??\\ExternalDeviceMonitor"

#define IOCTL_SET_PATH_STRING CTL_CODE(FILE_DEVICE_UNKNOWN, 0x900, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_GET_GLOBAL_LOG CTL_CODE(FILE_DEVICE_UNKNOWN, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STOP_GET_GLOBAL_LOG CTL_CODE(FILE_DEVICE_UNKNOWN, 0x903, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_BLOCK_WRITE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x904, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_BLOCK_DELETE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x905, METHOD_BUFFERED, FILE_ANY_ACCESS)


#define GLOBAL_BUFFER_SIZE 256
extern char* globalOutputBuffer;
#define BOOT_VOLUME_NT_PATH L"\\Device\\HardDiskVolume3"
extern WCHAR* globalLogBuffer;
extern UNICODE_STRING BootVolumePrefix;

extern BOOLEAN startSaveLog;
extern BOOLEAN blockWrite; 
extern BOOLEAN blockDelete;

typedef struct {
	PFLT_FILTER Filter;
	PDRIVER_OBJECT DriverObject;

} MINIFILTER_DATA, * PMINIFILTER_DATA;