#include <ntifs.h>
#include "Check.h"


EXTERN_C NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	NTSTATUS status = STATUS_SUCCESS;
	KdPrint(("[Checker] DriverEntry\n"));

	if (DriverObject)
	{
		Check::StartCheck();

		DriverObject->DriverUnload = [](PDRIVER_OBJECT DriverObject)->VOID {
			UNREFERENCED_PARAMETER(DriverObject);
			KdPrint(("[Checker] DriverUnload\n"));

			Check::StopCheck();
		};
	}
	return status;
}
 