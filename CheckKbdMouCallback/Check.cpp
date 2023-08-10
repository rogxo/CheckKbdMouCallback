#include "Check.h"
#include "KbdMou.hpp"


namespace Check
{
	KEVENT SyncEvent = {};
	HANDLE CheckThreadHandle = NULL;
	PETHREAD CheckThread = NULL;


	VOID CheckThreadProc(PVOID StartContext)
	{
		UNREFERENCED_PARAMETER(StartContext);

		NTSTATUS status;
		PVOID MouseClassServiceCallback = NULL;
		PVOID KeyboardClassServiceCallback = NULL;

		UCHAR MouseClassServiceCallbackCode[0x4E4] = { 0 };
		UCHAR KeyboardClassServiceCallbackCode[0x4E4] = { 0 };

		status = KbdMou::InitializeKeyBoard();
		if (!NT_SUCCESS(status)) {
			DbgPrint("[Checker] InitializeKeyBoard failed status:%X\n", status);
		}
		status = KbdMou::InitializeMouse();
		if (!NT_SUCCESS(status)) {
			DbgPrint("[Checker] InitializeMouse failed status:%X\n", status);
		}

		KeyboardClassServiceCallback = KbdMou::KeyboardClassServiceCallback;
		MouseClassServiceCallback = KbdMou::MouseClassServiceCallback;

		DbgPrint("[Checker] KeyboardClassServiceCallback init value:%p\n", KeyboardClassServiceCallback);
		DbgPrint("[Checker] MouseClassServiceCallback init value:%p\n", MouseClassServiceCallback);

		if (KeyboardClassServiceCallback) {
			memcpy(KeyboardClassServiceCallbackCode, KeyboardClassServiceCallback, sizeof(KeyboardClassServiceCallbackCode));
		}

		DbgPrint(("[Checker] KeyboardClassServiceCallback first 16 bytes:\n"));
		for (size_t j = 0; j < 0x10; j++)
		{
			DbgPrint("%02X ", KeyboardClassServiceCallbackCode[j]);
		}
		DbgPrint("\n");

		DbgPrint(("[Checker] KeyboardClassServiceCallback bytes:\n"));
		for (size_t i = 0; i < 0x10; i++)
		{
			DbgPrint("[Checker] %p ", (PUCHAR)KeyboardClassServiceCallback + i * 0x10);
			for (size_t j = 0; j < 0x10; j++)
			{
				DbgPrint("%02X ", KeyboardClassServiceCallbackCode[i * 0x10 + j]);
			}
			DbgPrint("\n");
		}

		if (MouseClassServiceCallback) {
			memcpy(MouseClassServiceCallbackCode, MouseClassServiceCallback, sizeof(MouseClassServiceCallbackCode));
		}

		DbgPrint(("[Checker] MouseClassServiceCallback first 16 bytes:\n"));
		for (size_t j = 0; j < 0x10; j++)
		{
			DbgPrint("%02X ", MouseClassServiceCallbackCode[j]);
		}
		DbgPrint("\n");

		DbgPrint(("[Checker] MouseClassServiceCallback bytes:\n"));
		for (size_t i = 0; i < 0x10; i++)
		{
			DbgPrint("[Checker] %p ", (PUCHAR)MouseClassServiceCallback + i * 0x10);
			for (size_t j = 0; j < 0x10; j++)
			{
				DbgPrint("%02X ", MouseClassServiceCallbackCode[i * 0x10 + j]);
			}
			DbgPrint("\n");
		}


		while (true)
		{
			DbgPrint("[Checker] Checking...\n");

			KbdMou::InitializeKeyBoard();
			KbdMou::InitializeMouse();

			if (KeyboardClassServiceCallback != KbdMou::KeyboardClassServiceCallback) {
				DbgPrint("[Checker] Detect KeyboardClassServiceCallback Ptr Hook\n");
			}
			if (MouseClassServiceCallback != KbdMou::MouseClassServiceCallback) {
				DbgPrint("[Checker] Detect MouseClassServiceCallback Ptr Hook\n");
			}

			if (KeyboardClassServiceCallback) {
				if (memcmp(KeyboardClassServiceCallbackCode, KeyboardClassServiceCallback, sizeof(KeyboardClassServiceCallbackCode))) {
					DbgPrint("[Checker] Detect KeyboardClassServiceCallback Inline Hook\n");
				}
			}
			if (MouseClassServiceCallback) {
				if (memcmp(MouseClassServiceCallbackCode, MouseClassServiceCallback, sizeof(MouseClassServiceCallbackCode))) {
					DbgPrint("[Checker] Detect MouseClassServiceCallback Inline Hook\n");
				}
			}

			LARGE_INTEGER Interval = {};
			Interval.QuadPart = -10000LL * 1000;
			status = KeWaitForSingleObject(
				&SyncEvent,						//同步对象的指针，
				Executive,						//等待的原因，一般为Executive
				KernelMode,						//等待模式，一般为KernelMode
				FALSE,							//指明等待是否为“警惕”的，一般为FALSE
				&Interval);						//等待时间，如果为NULL，就表示无限期等待，直到同步对象变为激发态
			if (status == STATUS_SUCCESS) {
				PsTerminateSystemThread(STATUS_SUCCESS);
			}
		}
	}

	NTSTATUS StartCheck()
	{
		NTSTATUS status;
		if (CheckThread) {
			return STATUS_ALREADY_COMPLETE;
		}
		KeInitializeEvent(&SyncEvent, NotificationEvent, FALSE);
		status = PsCreateSystemThread(&CheckThreadHandle, THREAD_ALL_ACCESS, NULL, NtCurrentProcess(), NULL, CheckThreadProc, NULL);
		if (!NT_SUCCESS(status)) {
			return status;
		}
		DbgPrint("[Checker] Check Started\n");
		status = ObReferenceObjectByHandle(
			CheckThreadHandle,		
			THREAD_ALL_ACCESS,
			NULL,
			KernelMode,
			(PVOID*)&CheckThread,
			NULL);
		if (!NT_SUCCESS(status)) {
			return status;
		}
		status = ZwClose(CheckThreadHandle);
		if (!NT_SUCCESS(status)) {
			return status;
		}
		CheckThreadHandle = NULL;
		return STATUS_SUCCESS;
	}

	NTSTATUS StopCheck()
	{
		NTSTATUS status;
		if (CheckThread)
		{
			KeSetEvent(&SyncEvent, 0, FALSE);
			status = KeWaitForSingleObject(CheckThread, Executive, KernelMode, FALSE, NULL);
			ObDereferenceObject(CheckThread);
			CheckThread = NULL;
			DbgPrint("[Checker] Check Stoped\n");
			return status;
		}
		return STATUS_SUCCESS;
	}
};

