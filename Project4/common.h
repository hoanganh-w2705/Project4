#pragma once
#include "ntdef.h"
#include "fltKernel.h"
#include "ntddstor.h"
#include "storduid.h"

#define EXTERNAL_DEVICE_MONITOR_DEVICE_NAME		L"\\Device\\ExternalDeviceMonitor"
#define EXTERNAL_DEVICE_MONITOR_DEVICE_SYMLINK	L"\\??\\ExternalDeviceMonitor"
//#define MAX_ITEMS_COUNT 1024


class FastMutex {
public:
	void Init() {
		//KeInitializeMutex(&_mtx, 0);
		ExInitializeFastMutex(&_mtx);
	}

	void Lock() {
		//KeWaitForSingleObject(&_mtx, Executive, KernelMode, FALSE, NULL);
		ExAcquireFastMutex(&_mtx);
	}

	void Unlock() {
		//KeReleaseMutex(&_mtx, FALSE);
		ExReleaseFastMutex(&_mtx);
	}
private:
	FAST_MUTEX _mtx;
};

//template<typename T>
//struct TRACKED_ITEM {
//	LIST_ENTRY Entry;
//	T Data;
//};

//typedef struct {
//	PDEVICE_OBJECT DeviceObject;
//	PDEVICE_OBJECT VolumeDeviceObject;
//	PFILE_OBJECT FileObject;
//	PSTORAGE_DEVICE_UNIQUE_IDENTIFIER StorageDeviceUniqueId;
//	PSTORAGE_DEVICE_DESCRIPTOR StorageDeviceDescriptor;
//	PSTORAGE_HOTPLUG_INFO StorageHotplugInfo;
//	UNICODE_STRING DeviceId;
//	ULONG DeviceNumber;
//	BOOLEAN IsMountBlocked;
//	BOOLEAN IsReadBlocked;
//	BOOLEAN IsWriteBlocked;
//	BOOLEAN IsLogged;
//} TRACKED_STORAGE, * PTRACKED_STORAGE;
//
//typedef struct {
//	UNICODE_STRING DriveLetterDosPath;
//	UNICODE_STRING VolumeGuid;
//	UNICODE_STRING VolumeNtPath;
//	ULONG DeviceNumber;
//	ULONG DeviceType;
//	ULONG PartitionNumber;
//	PTRACKED_STORAGE AssociatedTrackedStorage;
//} TRACKED_VOLUME, PTRACKED_VOLUME;
//
//typedef struct {
//	LIST_ENTRY ItemsHead;
//	UINT32 ItemsCount;
//	FastMutex mtx;
//} LIST_TRACKED_ITEM, * PLIST_TRACKED_ITEM;

typedef struct {
	PFLT_FILTER Filter;
	PDRIVER_OBJECT DriverObject;
	//PLIST_TRACKED_ITEM TrackedStorageItems;
	//PLIST_TRACKED_ITEM LoggingItems;
	//PLIST_TRACKED_ITEM TrackedVolumeItems;
} MINIFILTER_DATA, * PMINIFILTER_DATA;