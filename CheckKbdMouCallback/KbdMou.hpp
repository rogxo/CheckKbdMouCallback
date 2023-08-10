#include <ntifs.h>
#include "Imports.h"


#pragma warning(disable:4201)

typedef struct _KEYBOARD_INPUT_DATA {
	USHORT UnitId;
	USHORT MakeCode;
	USHORT Flags;
	USHORT Reserved;
	ULONG ExtraInformation;
} KEYBOARD_INPUT_DATA, * PKEYBOARD_INPUT_DATA;

typedef struct _MOUSE_INPUT_DATA {
	USHORT UnitId;
	USHORT Flags;
	union {
		ULONG Buttons;
		struct {
			USHORT ButtonFlags;
			USHORT ButtonData;
		};
	};
	ULONG  RawButtons;
	LONG   LastX;
	LONG   LastY;
	ULONG  ExtraInformation;
} MOUSE_INPUT_DATA, * PMOUSE_INPUT_DATA;


typedef VOID(*MouseClassServiceCallback_T)(
	IN PDEVICE_OBJECT DeviceObject,
	IN PMOUSE_INPUT_DATA InputDataStart,
	IN PMOUSE_INPUT_DATA InputDataEnd,
	IN OUT PULONG InputDataConsumed);

typedef VOID(*KeyboardClassServiceCallback_T)(
	IN PDEVICE_OBJECT DeviceObject,
	IN PKEYBOARD_INPUT_DATA InputDataStart,
	IN PKEYBOARD_INPUT_DATA InputDataEnd,
	IN OUT PULONG InputDataConsumed);


namespace KbdMou
{
	// ¼üÊó³éÏóÀà
	UNICODE_STRING KbdClassDriverName = RTL_CONSTANT_STRING(L"\\Driver\\kbdclass");
	UNICODE_STRING MouClassDriverName = RTL_CONSTANT_STRING(L"\\Driver\\mouclass");
	// PS/2-8042
	UNICODE_STRING i8042prtDriverName = RTL_CONSTANT_STRING(L"\\Driver\\i8042prt");
	// USB-Hid
	UNICODE_STRING MouHIDDriverName = RTL_CONSTANT_STRING(L"\\Driver\\mouhid");
	UNICODE_STRING KbdHIDDriverName = RTL_CONSTANT_STRING(L"\\Driver\\kbdhid");

	PDEVICE_OBJECT MouDeviceObject = NULL;
	EXTERN_C MouseClassServiceCallback_T MouseClassServiceCallback = NULL;
	PDEVICE_OBJECT KbdDeviceObject = NULL;
	EXTERN_C KeyboardClassServiceCallback_T KeyboardClassServiceCallback = NULL;


	NTSTATUS InitializeMouse()
	{
		NTSTATUS status;
		PDRIVER_OBJECT MouClassDriverObject;
		PDRIVER_OBJECT MouHIDDriverObject;

		status = ObReferenceObjectByName(&MouClassDriverName, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, (PVOID*)&MouClassDriverObject);
		if (!NT_SUCCESS(status)) {
			return status;
		}
		status = ObReferenceObjectByName(&MouHIDDriverName, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, (PVOID*)&MouHIDDriverObject);
		if (!NT_SUCCESS(status))
		{
			if (MouHIDDriverObject) {
				ObfDereferenceObject(MouClassDriverObject);
			}
			return status;
		}

		PDEVICE_OBJECT MouHIDDeviceObject = MouHIDDriverObject->DeviceObject;

		while (MouHIDDeviceObject && !MouseClassServiceCallback)
		{
			PDEVICE_OBJECT MouClassDeviceObject = MouClassDriverObject->DeviceObject;
			while (MouClassDeviceObject && !MouseClassServiceCallback)
			{
				if (!MouClassDeviceObject->NextDevice && !MouDeviceObject) {
					MouDeviceObject = MouClassDeviceObject;
				}
				PULONG64 MouHIDDeviceExtensionStart = (PULONG64)MouHIDDeviceObject->DeviceExtension;
				ULONG64 MouHIDDeviceExtensionSize = ((ULONG64)MouHIDDeviceObject->DeviceObjectExtension - (ULONG64)MouHIDDeviceObject->DeviceExtension) / 4;

				for (ULONG64 i = 0; i < MouHIDDeviceExtensionSize; i++)
				{
					if (MouHIDDeviceExtensionStart[i] == (ULONG64)MouClassDeviceObject && MouHIDDeviceExtensionStart[i + 1] > (ULONG64)MouClassDriverObject->DriverStart)
					{
						MouseClassServiceCallback = (MouseClassServiceCallback_T)(MouHIDDeviceExtensionStart[i + 1]);
						break;
					}
				}
				MouClassDeviceObject = MouClassDeviceObject->NextDevice;
			}
			MouHIDDeviceObject = MouHIDDeviceObject->AttachedDevice;
		}
		if (!MouDeviceObject)
		{
			PDEVICE_OBJECT TmpDeviceObject = MouClassDriverObject->DeviceObject;
			while (TmpDeviceObject)
			{
				if (!TmpDeviceObject->NextDevice)
				{
					MouDeviceObject = TmpDeviceObject;
					break;
				}
				TmpDeviceObject = TmpDeviceObject->NextDevice;
			}
		}

		//KdPrint(("[SimpleDriver] MouDeviceObject:%p\n", MouDeviceObject));
		//KdPrint(("[SimpleDriver] MouseClassServiceCallback:%p\n", MouseClassServiceCallback));

		ObfDereferenceObject(MouClassDriverObject);
		ObfDereferenceObject(MouHIDDriverObject);
		return STATUS_SUCCESS;
	}

	NTSTATUS InitializeKeyBoard()
	{
		NTSTATUS status;
		PDRIVER_OBJECT KbdClassDriverObject;
		PDRIVER_OBJECT KbdHIDDriverObject;

		status = ObReferenceObjectByName(&KbdClassDriverName, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, (PVOID*)&KbdClassDriverObject);
		if (!NT_SUCCESS(status)) {
			return status;
		}
		status = ObReferenceObjectByName(&KbdHIDDriverName, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, (PVOID*)&KbdHIDDriverObject);
		if (!NT_SUCCESS(status))
		{
			if (KbdHIDDriverObject) {
				ObfDereferenceObject(KbdClassDriverObject);
			}
			return status;
		}

		PDEVICE_OBJECT KbdHIDDeviceObject = KbdHIDDriverObject->DeviceObject;

		while (KbdHIDDeviceObject && !KeyboardClassServiceCallback)
		{
			PDEVICE_OBJECT KbdClassDeviceObject = KbdClassDriverObject->DeviceObject;
			while (KbdClassDeviceObject && !KeyboardClassServiceCallback)
			{
				if (!KbdClassDeviceObject->NextDevice && !KbdDeviceObject) {
					KbdDeviceObject = KbdClassDeviceObject;
				}
				PULONG64 KbdHIDDeviceExtensionStart = (PULONG64)KbdHIDDeviceObject->DeviceExtension;
				ULONG64 KbdHIDDeviceExtensionSize = ((ULONG64)KbdHIDDeviceObject->DeviceObjectExtension - (ULONG64)KbdHIDDeviceObject->DeviceExtension) / 4;

				for (ULONG64 i = 0; i < KbdHIDDeviceExtensionSize; i++)
				{
					if (KbdHIDDeviceExtensionStart[i] == (ULONG64)KbdClassDeviceObject && KbdHIDDeviceExtensionStart[i + 1] > (ULONG64)KbdClassDriverObject->DriverStart)
					{
						KeyboardClassServiceCallback = (KeyboardClassServiceCallback_T)(KbdHIDDeviceExtensionStart[i + 1]);
						break;
					}
				}
				KbdClassDeviceObject = KbdClassDeviceObject->NextDevice;
			}
			KbdHIDDeviceObject = KbdHIDDeviceObject->AttachedDevice;
		}
		if (!KbdDeviceObject)
		{
			PDEVICE_OBJECT TmpDeviceObject = KbdClassDriverObject->DeviceObject;
			while (TmpDeviceObject)
			{
				if (!TmpDeviceObject->NextDevice)
				{
					KbdDeviceObject = TmpDeviceObject;
					break;
				}
				TmpDeviceObject = TmpDeviceObject->NextDevice;
			}
		}

		//KdPrint(("[SimpleDriver] KbdDeviceObject:%p\n", KbdDeviceObject));
		//KdPrint(("[SimpleDriver] KeyboardClassServiceCallback:%p\n", KeyboardClassServiceCallback));

		ObfDereferenceObject(KbdClassDriverObject);
		ObfDereferenceObject(KbdHIDDriverObject);
		return STATUS_SUCCESS;
	}



}