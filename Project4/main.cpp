//﻿#include "common.h"
#include "mnf.h"

char* globalOutputBuffer;
BOOLEAN startSaveLog = FALSE;
BOOLEAN blockWrite = FALSE;
BOOLEAN blockDelete = FALSE;

void SaveToGlobalBuffer(const char* inputBuffer, ULONG inputBufferLength) {
	if (inputBuffer != NULL) {
		// Đảm bảo không vượt quá kích thước của buffer global

		if (inputBufferLength < GLOBAL_BUFFER_SIZE) {
			RtlCopyMemory(globalOutputBuffer, inputBuffer, inputBufferLength);
		}
		else {
			// Xử lý trường hợp chuỗi quá dài
			RtlCopyMemory(globalOutputBuffer, inputBuffer, GLOBAL_BUFFER_SIZE - 1);
			globalOutputBuffer[GLOBAL_BUFFER_SIZE - 1] = '\0'; // Đảm bảo null-terminated
		}
	}
}

NTSTATUS CompleteRequest(PIRP pIrp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR info = 0) {
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = info;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS DriverRead(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp
) {
	UNREFERENCED_PARAMETER(pDeviceObject);
	return CompleteRequest(pIrp, STATUS_SUCCESS, 0);
}

VOID DriverUnload(
	PDRIVER_OBJECT pDriverObject
) {


	FltUnregisterFilter(MinifilterData.Filter);
	/*ExFreePoolWithTag(globalLogQueueHead, 'LnLg');
	ExFreePoolWithTag(globalLogBuffer, 'LogB');
	ExFreePoolWithTag(globalOutputBuffer, 'tuOg');*/
	UNICODE_STRING symlink = RTL_CONSTANT_STRING(EXTERNAL_DEVICE_MONITOR_DEVICE_SYMLINK);
	IoDeleteSymbolicLink(&symlink);
	IoDeleteDevice(pDriverObject->DeviceObject);
}

NTSTATUS DriverCreateClose(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp
) {
	UNREFERENCED_PARAMETER(pDeviceObject);
	return CompleteRequest(pIrp, STATUS_SUCCESS, 0);
}

NTSTATUS DriverControl(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp
) {
	UNREFERENCED_PARAMETER(pDeviceObject);

	PIO_STACK_LOCATION pIoStack = IoGetCurrentIrpStackLocation(pIrp);
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
	SIZE_T information = 0;
	// Xử lý IOCTL_SET_PATH_STRING
	if (pIoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SET_PATH_STRING) {
		PVOID inputBuffer = pIrp->AssociatedIrp.SystemBuffer;
		ULONG inputBufferLength = pIoStack->Parameters.DeviceIoControl.InputBufferLength;
		startSaveLog = TRUE;


		if (inputBuffer != NULL && inputBufferLength > 0 && inputBufferLength <= 100) {
			globalOutputBuffer = (char*)ExAllocatePool2(POOL_FLAG_NON_PAGED, inputBufferLength, 'tuOg');
			SaveToGlobalBuffer(static_cast<const char*>(inputBuffer), inputBufferLength);
			DbgPrint("Received Path: %s\n", static_cast<const char*>(inputBuffer));
			status = STATUS_SUCCESS;
		}
	}
	// Xử lý IOCTL_GET_GLOBAL_LOG
	else if (pIoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_GET_GLOBAL_LOG) {
		//PVOID inputBuffer = pIrp->AssociatedIrp.SystemBuffer;
		PVOID outputBuffer = pIrp->AssociatedIrp.SystemBuffer;
		ULONG outputBufferLength = pIoStack->Parameters.DeviceIoControl.OutputBufferLength;

		if (outputBuffer != NULL && outputBufferLength >= sizeof(WCHAR)) {
			WCHAR* logBuffer = DequeueLogBuffer();

			if (logBuffer != NULL) {
				RtlCopyMemory(outputBuffer, logBuffer, wcslen(logBuffer) * sizeof(WCHAR));
				information = wcslen(logBuffer) * sizeof(WCHAR);
				DbgPrint("Global Log Buffer: %ws\n", logBuffer);
				//ExFreePoolWithTag(logBuffer, 'GlLg');

				status = STATUS_SUCCESS;
			}
			else {
				status = STATUS_NO_MORE_ENTRIES;
			}
		}
		else {
			status = STATUS_BUFFER_TOO_SMALL;
		}
	}
	//check lại
	else if (pIoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_STOP_GET_GLOBAL_LOG) {
		startSaveLog = FALSE;
		blockWrite = FALSE;
		blockDelete = FALSE;


	}

	else if (pIoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_BLOCK_WRITE) {
		PVOID outputBuffer = pIrp->AssociatedIrp.SystemBuffer;
		ULONG outputBufferLength = pIoStack->Parameters.DeviceIoControl.OutputBufferLength;

		blockWrite = TRUE;

		if (outputBuffer != NULL && outputBufferLength >= sizeof(WCHAR*)) {
			WCHAR* logBuffer = DequeueLogBuffer();

			if (logBuffer != NULL) {
				RtlCopyMemory(outputBuffer, logBuffer, wcslen(logBuffer) * sizeof(WCHAR));
				information = wcslen(logBuffer) * sizeof(WCHAR);
				DbgPrint("Global Log Buffer: %ws\n", logBuffer);

				status = STATUS_SUCCESS;
			}
			else {
				status = STATUS_NO_MORE_ENTRIES;
			}
		}
		else {
			status = STATUS_BUFFER_TOO_SMALL;
		}
	}
	else if (pIoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_BLOCK_DELETE) {
		PVOID outputBuffer = pIrp->AssociatedIrp.SystemBuffer;
		ULONG outputBufferLength = pIoStack->Parameters.DeviceIoControl.OutputBufferLength;

		blockDelete = TRUE;

		if (outputBuffer != NULL && outputBufferLength >= sizeof(WCHAR*)) {
			WCHAR* logBuffer = DequeueLogBuffer();

			if (logBuffer != NULL) {
				RtlCopyMemory(outputBuffer, logBuffer, wcslen(logBuffer) * sizeof(WCHAR));
				information = wcslen(logBuffer) * sizeof(WCHAR);
				DbgPrint("Global Log Buffer: %ws\n", logBuffer);

				status = STATUS_SUCCESS;
			}
			else {
				status = STATUS_NO_MORE_ENTRIES;
			}
		}
		else {
			status = STATUS_BUFFER_TOO_SMALL;
		}
	}




	//ExFreePoolWithTag(globalLogQueueHead, 'LnLg');
	//ExFreePoolWithTag(globalLogBuffer, 'LogB');
	return CompleteRequest(pIrp, status, information); //information: size  mà user đọc được (ban đầu để 0 -> lỗi)
}

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath) {
	UNREFERENCED_PARAMETER(pRegistryPath);

	InitializeLogQueue();
	MinifilterData.DriverObject = pDriverObject;
	NTSTATUS status = FltRegisterFilter(
		pDriverObject,
		&FilterRegistration,
		&MinifilterData.Filter
	);

	if (NT_SUCCESS(status)) {
		status = FltStartFiltering(MinifilterData.Filter);
		if (!NT_SUCCESS(status)) {
			FltUnregisterFilter(MinifilterData.Filter);
		}
	}

	PDEVICE_OBJECT pDeviceObject;
	UNICODE_STRING deviceName;
	UNICODE_STRING symLink;

	RtlInitUnicodeString(&deviceName, EXTERNAL_DEVICE_MONITOR_DEVICE_NAME);
	RtlInitUnicodeString(&symLink, EXTERNAL_DEVICE_MONITOR_DEVICE_SYMLINK);

	status = IoCreateDevice(pDriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, TRUE, &pDeviceObject);
	if (!NT_SUCCESS(status)) {
		DbgPrint("Failed to create device (0x%08X)\n", status);
	}
	BOOLEAN symlinkCreated = FALSE;

	status = IoCreateSymbolicLink(
		&symLink,
		&deviceName
	);

	if (!NT_SUCCESS(status)) {
		DbgPrint("====================== FAILED TO CREATE SYMLINK (0x%08X)\n", status);
	}
	else {
		symlinkCreated = TRUE;
	}

	if (!NT_SUCCESS(status)) {
		if (symlinkCreated)
			IoDeleteSymbolicLink(&symLink);
		if (pDeviceObject)
			IoDeleteDevice(pDeviceObject);
		return status;
	}

	//pDeviceObject->Flags |= DO_DIRECT_IO;
	pDriverObject->Flags |= DO_DIRECT_IO;
	pDriverObject->DriverUnload = DriverUnload;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = pDriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCreateClose;
	pDriverObject->MajorFunction[IRP_MJ_READ] = DriverRead;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverControl;
	MinifilterData.DriverObject = pDriverObject;
	return status;
}