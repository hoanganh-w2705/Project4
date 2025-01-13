#include "common.h"
#include "mnf.h"


UNICODE_STRING BootVolumePrefix = RTL_CONSTANT_STRING(BOOT_VOLUME_NT_PATH);
WCHAR* globalLogBuffer;
LOG_BUFFER_ENTRY* globalLogQueueHead;
KMUTEX queueMutex;

void InitializeLogQueue() {
	KeInitializeMutex(&queueMutex, 0); // Initialize the mutex
	globalLogQueueHead = (LOG_BUFFER_ENTRY*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(LOG_BUFFER_ENTRY), 'LnLg');
}

void EnqueueLogBuffer(WCHAR* logBuffer) {
	LOG_BUFFER_ENTRY* newEntry = (LOG_BUFFER_ENTRY*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(LOG_BUFFER_ENTRY), 'LnLg');
	if (newEntry != NULL) {
		newEntry->logBuffer = logBuffer;
		newEntry->next = NULL;

		// Acquire the mutex to ensure thread safety
		KeWaitForSingleObject(&queueMutex, Executive, KernelMode, FALSE, NULL);

		LOG_BUFFER_ENTRY* p = globalLogQueueHead;
		while (p->next != NULL)
			p = p->next;
		p->next = newEntry;

		// Release the mutex
		KeReleaseMutex(&queueMutex, FALSE);
	}
}

WCHAR* DequeueLogBuffer() {
	WCHAR* logBuffer = NULL;

	// Acquire the mutex to ensure thread safety
	KeWaitForSingleObject(&queueMutex, Executive, KernelMode, FALSE, NULL);

	if (globalLogQueueHead != NULL) {
		LOG_BUFFER_ENTRY* entryToRemove = globalLogQueueHead;
		logBuffer = entryToRemove->logBuffer;
		globalLogQueueHead = globalLogQueueHead->next;

		ExFreePoolWithTag(entryToRemove, 'LnLg'); // Free the removed entry
	}

	// Release the mutex
	KeReleaseMutex(&queueMutex, FALSE);

	return logBuffer;
}

void LogToGlobalBuffer(
	PUNICODE_STRING parentDir,
	PUNICODE_STRING extension,
	SIZE_T sizeBuffer,
	PCWSTR logPrefix // Thêm tham số logPrefix
) {
	WCHAR* logBuffer = (WCHAR*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeBuffer, 'GlLg');

	if (logBuffer != NULL) {
		NTSTATUS status = RtlStringCbPrintfW(
			logBuffer,
			sizeBuffer,
			L"%wS %wZ%wZ\n",
			logPrefix,
			parentDir,
			extension
		);

		if (NT_SUCCESS(status)) {
			EnqueueLogBuffer(logBuffer); // Thêm logBuffer vào hàng đợi
			
		}
		else {
			DbgPrint("RtlStringCbPrintfW failed with status: 0x%X\n", status);

		}
	}

}

FLT_PREOP_CALLBACK_STATUS CreatePreoperationCallback(
	PFLT_CALLBACK_DATA data,
	PCFLT_RELATED_OBJECTS fltObjects,
	PVOID* completionContext
) {
	UNREFERENCED_PARAMETER(completionContext);
	UNREFERENCED_PARAMETER(fltObjects);
	
		PFLT_FILE_NAME_INFORMATION fileNameInfo = NULL;
		NTSTATUS status;
		status = FltGetFileNameInformation(data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &fileNameInfo);
		if (NT_SUCCESS(status)) {
			if (fileNameInfo != NULL) {
				FltParseFileNameInformation(fileNameInfo);
				UNICODE_STRING string = { 0 };
				if (globalOutputBuffer != NULL && globalOutputBuffer[0] != L'\0') {
					RtlInitUnicodeString(&string, (PCWSTR)globalOutputBuffer);// Khởi tạo UNICODE_STRING từ buffer
				}

				if (string.Buffer != NULL && string.Length > 0) {
					LONG check = RtlCompareUnicodeString(&string, &fileNameInfo->ParentDir, TRUE); //check path 
					if (check == 0) {

						if (startSaveLog) {
							DbgPrint("Callback =======[LOG][Create] A file is being created in a directory containing ...: %wZ%wZ\n", &fileNameInfo->ParentDir, &fileNameInfo->Extension);
							SIZE_T bufferSize = fileNameInfo->ParentDir.Length + fileNameInfo->Extension.Length + sizeof(L"=======[LOG][Create] A file is being created in directory: %wZ%wZ\n");

							//lưu vào global
							LogToGlobalBuffer(&fileNameInfo->ParentDir, &fileNameInfo->Extension, bufferSize, L"=======[LOG][Create] A file is being created in directory:");
						}

						if (blockWrite) {
							DbgPrint("Callback =======[BLOCK][WRITE] Block a write action in directory containing ...: %wZ%wZ\n", &fileNameInfo->ParentDir, &fileNameInfo->Extension);
							SIZE_T bufferSize = fileNameInfo->ParentDir.Length + fileNameInfo->Extension.Length + sizeof(L"=======[BLOCK][WRITE] Block a write action in directory: %wZ%wZ\n");

							//lưu vào global
							LogToGlobalBuffer(&fileNameInfo->ParentDir, &fileNameInfo->Extension, bufferSize, L"=======[BLOCK][WRITE] Block a write action in directory:");
							data->IoStatus.Status = STATUS_ACCESS_DENIED; // Gán trạng thái cho IRP
							data->IoStatus.Information = 0;  // Không có thông tin trả lại

						}
					}
				}
			}
			FltReleaseFileNameInformation(fileNameInfo);
		}
		
	else {
		UNREFERENCED_PARAMETER(data);
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	return FLT_PREOP_SUCCESS_NO_CALLBACK;
	
}

FLT_PREOP_CALLBACK_STATUS ReadPreoperationCallback(
	PFLT_CALLBACK_DATA data,
	PCFLT_RELATED_OBJECTS fltObjects,
	PVOID* completionContext
) {
	UNREFERENCED_PARAMETER(completionContext);
	UNREFERENCED_PARAMETER(fltObjects);

	PFLT_FILE_NAME_INFORMATION fileNameInfo;
	NTSTATUS status;
	status = FltGetFileNameInformation(data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP, &fileNameInfo);
	if (NT_SUCCESS(status)) {
		if (fileNameInfo != NULL) {
			FltParseFileNameInformation(fileNameInfo);
			UNICODE_STRING string = { 0 };
			if (globalOutputBuffer != NULL && globalOutputBuffer[0] != L'\0') {
				RtlInitUnicodeString(&string, (PCWSTR)globalOutputBuffer);// Khởi tạo UNICODE_STRING từ buffer
				//DbgPrint("RtlInitUnicodeString success!\n");
			}

			// Kiểm tra nếu Buffer không phải NULL và độ dài lớn hơn 0
			if (string.Buffer != NULL && string.Length > 0) {
				//DbgPrint("String content: %wZ\n", &string.Buffer);
				LONG check = RtlCompareUnicodeString(&string, &fileNameInfo->ParentDir, TRUE); //check path 
				if (check == 0) {
					if (startSaveLog) {
						DbgPrint("Callback =======[LOG][Read] A file is being read in a directory containing ...: %wZ%wZ\n", &fileNameInfo->ParentDir, &fileNameInfo->Extension);
						SIZE_T bufferSize = fileNameInfo->ParentDir.Length + fileNameInfo->Extension.Length + sizeof(L"=======[LOG][Read] A file is being read in directory: %wZ%wZ\n");

						//lưu vào global
						LogToGlobalBuffer(&fileNameInfo->ParentDir, &fileNameInfo->Extension, bufferSize, L"=======[LOG][Read] A file is being read in directory:");
					}
					
				}
			}
			FltReleaseFileNameInformation(fileNameInfo);
		}
	}
	else {
		UNREFERENCED_PARAMETER(data);
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	return FLT_PREOP_SUCCESS_NO_CALLBACK;

}

FLT_PREOP_CALLBACK_STATUS WritePreoperationCallback(
	PFLT_CALLBACK_DATA data,
	PCFLT_RELATED_OBJECTS fltObjects,
	PVOID* completionContext
) {
	UNREFERENCED_PARAMETER(completionContext);
	UNREFERENCED_PARAMETER(fltObjects);

	PFLT_FILE_NAME_INFORMATION fileNameInfo;
	NTSTATUS status;
	status = FltGetFileNameInformation(data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP, &fileNameInfo);
	if (NT_SUCCESS(status)) {
		if (fileNameInfo != NULL) {
			FltParseFileNameInformation(fileNameInfo);
			UNICODE_STRING string = { 0 };
			if (globalOutputBuffer != NULL && globalOutputBuffer[0] != L'\0') {
				RtlInitUnicodeString(&string, (PCWSTR)globalOutputBuffer);// Khởi tạo UNICODE_STRING từ buffer
				//DbgPrint("RtlInitUnicodeString success!\n");
			}

			// Kiểm tra nếu Buffer không phải NULL và độ dài lớn hơn 0
			if (string.Buffer != NULL && string.Length > 0) {
				//DbgPrint("String content: %wZ\n", &string.Buffer);
				LONG check = RtlCompareUnicodeString(&string, &fileNameInfo->ParentDir, TRUE); //check path 
				if (check == 0) {
					if (startSaveLog) {
						DbgPrint("Callback =======[LOG][Write] A file is being writen in a directory containing ...: %wZ%wZ\n", &fileNameInfo->ParentDir, &fileNameInfo->Extension);
						SIZE_T bufferSize = fileNameInfo->ParentDir.Length + fileNameInfo->Extension.Length + sizeof(L"=======[LOG][Write] A file is being writen in directory: %wZ%wZ\n");

						//lưu vào global
						LogToGlobalBuffer(&fileNameInfo->ParentDir, &fileNameInfo->Extension, bufferSize, L"=======[LOG][Write] A file is being writen in directory:");
					}
					if (blockWrite) {
						DbgPrint("Callback =======[BLOCK][WRITE] Block a write action in directory containing ...: %wZ%wZ\n", &fileNameInfo->ParentDir, &fileNameInfo->Extension);
						SIZE_T bufferSize = fileNameInfo->ParentDir.Length + fileNameInfo->Extension.Length + sizeof(L"=======[BLOCK][WRITE] Block a write action in directory: %wZ%wZ\n");

						//lưu vào global
						LogToGlobalBuffer(&fileNameInfo->ParentDir, &fileNameInfo->Extension, bufferSize, L"=======[BLOCK][WRITE] Block a write action in directory:");
						data->IoStatus.Status = STATUS_ACCESS_DENIED; // Gán trạng thái cho IRP
						data->IoStatus.Information = 0;  // Không có thông tin trả lại
						return FLT_PREOP_COMPLETE;
					}
					
				}
			}
			FltReleaseFileNameInformation(fileNameInfo);
		}
	}
	else {
		UNREFERENCED_PARAMETER(data);
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	return FLT_PREOP_SUCCESS_NO_CALLBACK;

}

FLT_PREOP_CALLBACK_STATUS SetInformationPreoperationCallback(
	PFLT_CALLBACK_DATA data,
	PCFLT_RELATED_OBJECTS fltObjects,
	PVOID* completionContext
) {
	UNREFERENCED_PARAMETER(completionContext);
	UNREFERENCED_PARAMETER(fltObjects);
	NTSTATUS status;
	PFLT_FILE_NAME_INFORMATION fileNameInfo;

	if (data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileDispositionInformation) {
		PFILE_DISPOSITION_INFORMATION fileInfo = (PFILE_DISPOSITION_INFORMATION)data->Iopb->Parameters.SetFileInformation.InfoBuffer;

		if (fileInfo->DeleteFile) {
			status = FltGetFileNameInformation(
				data,
				FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
				&fileNameInfo
			);

			if (NT_SUCCESS(status)) {
				FltParseFileNameInformation(fileNameInfo);
				UNICODE_STRING string = { 0 };
				if (globalOutputBuffer != NULL && globalOutputBuffer[0] != L'\0') {
					RtlInitUnicodeString(&string, (PCWSTR)globalOutputBuffer);// Khởi tạo UNICODE_STRING từ buffer
					//DbgPrint("RtlInitUnicodeString success!\n");
				}

				// Kiểm tra nếu Buffer không phải NULL và độ dài lớn hơn 0
				if (string.Buffer != NULL && string.Length > 0) {
					//DbgPrint("String content: %wZ\n", &string.Buffer);
					LONG check = RtlCompareUnicodeString(&string, &fileNameInfo->ParentDir, TRUE); //check path 
					if (check == 0) {
						if (startSaveLog) {
							DbgPrint("Callback =======[LOG][Delete] A file is being deleted in a directory containing ...: %wZ%wZ\n", &fileNameInfo->ParentDir, &fileNameInfo->Extension);
							SIZE_T bufferSize = fileNameInfo->ParentDir.Length + fileNameInfo->Extension.Length + sizeof(L"=======[LOG][Delete] A file is being deleted in directory: %wZ%wZ\n");

							//lưu vào global
							LogToGlobalBuffer(&fileNameInfo->ParentDir, &fileNameInfo->Extension, bufferSize, L"=======[LOG][Delete] A file is being deleted in directory:");
						}
						if (blockDelete) {
							DbgPrint("Callback =======[BLOCK][WRITE] Block a write action in directory containing ...: %wZ%wZ\n", &fileNameInfo->ParentDir, &fileNameInfo->Extension);
							SIZE_T bufferSize = fileNameInfo->ParentDir.Length + fileNameInfo->Extension.Length + sizeof(L"=======[BLOCK][WRITE] Block a write action in directory: %wZ%wZ\n");

							//lưu vào global
							LogToGlobalBuffer(&fileNameInfo->ParentDir, &fileNameInfo->Extension, bufferSize, L"=======[BLOCK][WRITE] Block a write action in directory:");
							data->IoStatus.Status = STATUS_ACCESS_DENIED; // Gán trạng thái cho IRP
							data->IoStatus.Information = 0;  // Không có thông tin trả lại
							return FLT_PREOP_COMPLETE;
						}
						
					}
				}
				FltReleaseFileNameInformation(fileNameInfo);
			}
		}
	}
	else {
		UNREFERENCED_PARAMETER(data);
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


NTSTATUS InstanceSetupCallback(
	PCFLT_RELATED_OBJECTS fltObjects,
	FLT_INSTANCE_SETUP_FLAGS flags,
	DEVICE_TYPE volumeDeviceType,
	FLT_FILESYSTEM_TYPE fileSystemType
) {
	UNREFERENCED_PARAMETER(fltObjects);
	UNREFERENCED_PARAMETER(flags);
	UNREFERENCED_PARAMETER(volumeDeviceType);
	UNREFERENCED_PARAMETER(fileSystemType);

	UNICODE_STRING volumeName;
	RtlInitUnicodeString(&volumeName, NULL);
	ULONG returnedLength;
	NTSTATUS status = FltGetVolumeName(fltObjects->Volume, &volumeName, &returnedLength);

	if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_BUFFER_OVERFLOW || status == STATUS_INFO_LENGTH_MISMATCH) {
		volumeName.Length = volumeName.MaximumLength = (USHORT)returnedLength;
		do {
			volumeName.Buffer = (PWCH)ExAllocatePool2(POOL_FLAG_NON_PAGED, returnedLength, 'mNlV');
		} while (volumeName.Buffer == NULL);
		status = FltGetVolumeName(fltObjects->Volume, &volumeName, &returnedLength);
		if (!NT_SUCCESS(status)) {
			return STATUS_FLT_DO_NOT_ATTACH;
		}
		if (RtlCompareUnicodeString(&volumeName, &BootVolumePrefix, TRUE) == 0) {
			// todo: in the future, there will be excluded cases (e.g don't monitor C:\ but C:\ProgramData)
			// handle them here
			ExFreePoolWithTag(volumeName.Buffer, 'mNlV');
			return STATUS_SUCCESS;
		}

		else {
			return STATUS_FLT_DO_NOT_ATTACH;
		}
		

	}

	return STATUS_FLT_DO_NOT_ATTACH;
}


NTSTATUS FilterUnload(
	FLT_FILTER_UNLOAD_FLAGS flags
) {
	UNREFERENCED_PARAMETER(flags);

	return STATUS_SUCCESS;
}

MINIFILTER_DATA MinifilterData;

const FLT_OPERATION_REGISTRATION Callbacks[] = {			//khởi tạo callback 
	{
		IRP_MJ_CREATE,
		0,
		CreatePreoperationCallback,
		NULL
	},
	{
		IRP_MJ_READ,
		0,
		ReadPreoperationCallback,
		NULL
	},
	{
		IRP_MJ_WRITE,
		0,
		WritePreoperationCallback,
		NULL
	},
	{
		IRP_MJ_SET_INFORMATION,				//chính là DELETE, ngoài delete ra thì còn nhiều thứ khác
		0,
		SetInformationPreoperationCallback,
		NULL
	},
	{IRP_MJ_OPERATION_END }
};

const FLT_REGISTRATION FilterRegistration = {
	sizeof(FLT_REGISTRATION),
	FLT_REGISTRATION_VERSION,
	0,
	NULL,
	Callbacks,					// Mảng callback cho hoạt động file system
	FilterUnload,						// Callback gỡ bỏ filter
	InstanceSetupCallback,		// Callback khi gắn filter vào volume
	NULL,
	NULL,
	NULL
};