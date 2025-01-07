#include "common.h"
#include "mnf.h"

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
	// TODO: implement direct io for reading logs here
	return CompleteRequest(pIrp, STATUS_SUCCESS, 0);
}

VOID DriverUnload	(
	PDRIVER_OBJECT pDriverObject
) {
	// should be Remount all
	//RemountAll();
	// just make a troller here
//	DismountAll();
	FltUnregisterFilter(MinifilterData.Filter);
	// TODO: release devices linked list

	// TODO: release log linked list

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

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath) {
	UNREFERENCED_PARAMETER(pRegistryPath);								//hệ thống sẽ tự pass Path vào

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

	status = IoCreateDevice(pDriverObject, 0, NULL, FILE_DEVICE_UNKNOWN, 0, TRUE, &pDeviceObject);
	if (!NT_SUCCESS(status)) {
		DbgPrint("Failed to create device (0x%08X)\n", status);
	}
	BOOLEAN symlinkCreated = false;

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

	pDeviceObject->Flags |= DO_DIRECT_IO;
	pDriverObject->DriverUnload = DriverUnload;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = pDriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCreateClose;
	pDriverObject->MajorFunction[IRP_MJ_READ] = DriverRead;
	//pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverControl;
	MinifilterData.DriverObject = pDriverObject;
	return status;
}