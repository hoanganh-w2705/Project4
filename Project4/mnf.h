#pragma once
#include "common.h"

extern MINIFILTER_DATA MinifilterData;

typedef struct _LOG_BUFFER_ENTRY {
	WCHAR* logBuffer;
	struct _LOG_BUFFER_ENTRY* next;
} LOG_BUFFER_ENTRY;
extern LOG_BUFFER_ENTRY* globalLogQueueHead;
extern KSPIN_LOCK queueLock;


void InitializeLogQueue();
void EnqueueLogBuffer(WCHAR* logBuffer);
WCHAR* DequeueLogBuffer();

void LogToGlobalBuffer(
	PUNICODE_STRING parentDir,
	PUNICODE_STRING extension,
	SIZE_T sizeBuffer,
	PCWSTR logPrefix // Thêm tham s? logPrefix
);

FLT_PREOP_CALLBACK_STATUS CreatePreoperationCallback(
	PFLT_CALLBACK_DATA data,
	PCFLT_RELATED_OBJECTS fltObjects,
	PVOID* completionContext
);

//FLT_PREOP_CALLBACK_STATUS ReadPreoperationCallback(
//	PFLT_CALLBACK_DATA data,
//	PCFLT_RELATED_OBJECTS fltObjects,
//	PVOID* completionContext
//);
//
//FLT_PREOP_CALLBACK_STATUS WritePreoperationCallback(
//	PFLT_CALLBACK_DATA data,
//	PCFLT_RELATED_OBJECTS fltObjects,
//	PVOID* completionContext
//);
//
//FLT_PREOP_CALLBACK_STATUS SetInformationPreoperationCallback(
//	PFLT_CALLBACK_DATA data,
//	PCFLT_RELATED_OBJECTS fltObjects,
//	PVOID* completionContext
//);

NTSTATUS InstanceSetupCallback(
	PCFLT_RELATED_OBJECTS fltObjects,
	FLT_INSTANCE_SETUP_FLAGS flags,
	DEVICE_TYPE volumeDeviceType,
	FLT_FILESYSTEM_TYPE fileSystemType
);

NTSTATUS FilterUnload(
	FLT_FILTER_UNLOAD_FLAGS flags
);

extern const FLT_OPERATION_REGISTRATION Callbacks[];

extern const FLT_REGISTRATION FilterRegistration;