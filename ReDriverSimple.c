#include <ntifs.h>

#define W_DOWN 17
#define A_DOWN 30
#define S_DOWN 31
#define D_DOWN 32

//�豸��չ
typedef struct _DEVICE_EXTENSION
{
	PDEVICE_OBJECT TargetDevice;
	PDEVICE_OBJECT LowerDevice;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//������������
typedef struct _KEYBOARD_INPUT_DATA {
	USHORT UnitId;
	USHORT MakeCode;
	USHORT Flags;
	USHORT Reserved;
	ULONG  ExtraInformation;
} KEYBOARD_INPUT_DATA, *PKEYBOARD_INPUT_DATA;

extern POBJECT_TYPE *IoDriverObjectType;

NTSTATUS ObReferenceObjectByName(
	PUNICODE_STRING ObjectName,
	ULONG Attributes,
	PACCESS_STATE AccessState,
	ACCESS_MASK DesiredAccess,
	POBJECT_TYPE ObjectType,
	KPROCESSOR_MODE AccessMode,
	PVOID ParseContext,
	PVOID *Object
);

VOID DriverUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS DispatchGeneral(PDEVICE_OBJECT DeviceObject, PIRP irp);
NTSTATUS DispatchPower(PDEVICE_OBJECT DeviceObject, PIRP irp);
NTSTATUS DispatchPNP(PDEVICE_OBJECT DeviceObject, PIRP irp);
NTSTATUS DispatchRead(PDEVICE_OBJECT DeviceObject, PIRP irp);
NTSTATUS ReadComplete(PDEVICE_OBJECT DeviceObject, PIRP irp, PVOID Context);
NTSTATUS AttachDevices(PDRIVER_OBJECT DriverObject);

//��ȷ���� DDDDDDDDDDDDDDSSAASASASASASASASASAS
CHAR g_Maze[] =
"****************"
"o..............*"
"**************.*"
"************...*"
"***********..***"
"**********..****"
"*********..*****"
"********..******"
"*******..*******"
"******..********"
"*****..*********"
"****..**********"
"****7***********"
"****************";


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;

	//DbgPrint("Enter DriverEntry\n");

	//ж�ػ����� ��ж����
	//DriverObject->DriverUnload = DriverUnload;

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		DriverObject->MajorFunction[i] = DispatchGeneral;
	}
	DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPNP;
	DriverObject->MajorFunction[IRP_MJ_POWER] = DispatchPower;
	DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;

	status = AttachDevices(DriverObject);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	return status;
}

NTSTATUS DispatchGeneral(PDEVICE_OBJECT DeviceObject, PIRP irp)
{
	PDEVICE_EXTENSION DevExt;
	DevExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
	IoSkipCurrentIrpStackLocation(irp);
	return IoCallDriver(DevExt->LowerDevice, irp);
}

NTSTATUS DispatchPower(PDEVICE_OBJECT DeviceObject, PIRP irp)
{
	PDEVICE_EXTENSION DevExt;
	DevExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
	PoStartNextPowerIrp(irp);
	IoSkipCurrentIrpStackLocation(irp);
	return PoCallDriver(DevExt->LowerDevice, irp);
}

NTSTATUS DispatchPNP(PDEVICE_OBJECT DeviceObject, PIRP irp)
{
	PDEVICE_EXTENSION DevExt;
	PIO_STACK_LOCATION irps;
	NTSTATUS status = STATUS_SUCCESS;

	DevExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
	irps = IoGetCurrentIrpStackLocation(irp);

	switch (irps->MinorFunction)
	{
	case IRP_MN_REMOVE_DEVICE:
		IoSkipCurrentIrpStackLocation(irp);
		IoCallDriver(DevExt->LowerDevice, irp);
		IoDetachDevice(DevExt->LowerDevice);
		IoDeleteDevice(DeviceObject);
		status = STATUS_SUCCESS;
		break;
	default:
		IoSkipCurrentIrpStackLocation(irp);
		status = IoCallDriver(DevExt->LowerDevice, irp);
		break;
	}

	return status;
}

NTSTATUS DispatchRead(PDEVICE_OBJECT DeviceObject, PIRP irp)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_EXTENSION DevExt;

	if (irp->CurrentLocation == 1)
	{
		ULONG ReturnedInformation = 0;
		status = STATUS_INVALID_DEVICE_REQUEST;
		irp->IoStatus.Status = status;
		irp->IoStatus.Information = ReturnedInformation;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return status;
	}

	DevExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
	IoCopyCurrentIrpStackLocationToNext(irp);
	IoSetCompletionRoutine(irp, ReadComplete, DeviceObject, TRUE, TRUE, TRUE);

	return IoCallDriver(DevExt->LowerDevice, irp);
}


NTSTATUS ReadComplete(PDEVICE_OBJECT DeviceObject, PIRP irp, PVOID Context)
{
	//��ʼλ��
	static ULONG s_loc = 16;

	PKEYBOARD_INPUT_DATA KeyData = NULL;
	ULONG num = 0;

	if (NT_SUCCESS(irp->IoStatus.Status))
	{
		//��ü������ݺʹ�С
		KeyData = (PKEYBOARD_INPUT_DATA)irp->AssociatedIrp.SystemBuffer;
		num = irp->IoStatus.Information / sizeof(KEYBOARD_INPUT_DATA);

		//���˴���
		for (int i = 0; i < num; i++)
		{

			//W-17 A-30 S-31 D-32
			if (!KeyData->Flags)
			{
				g_Maze[s_loc] = '.';

				if (KeyData[i].MakeCode == W_DOWN)
				{
					if (s_loc / 16 != 0)
					{
						//����������
						s_loc -= 16;
					}
					else
					{
						//��������
						s_loc += 13 * 16;
					}
				}
				if (KeyData[i].MakeCode == S_DOWN)
				{
					if (s_loc / 16 != 13)
					{
						//����������
						s_loc += 16;
					}
					else
					{
						//��������
						s_loc -= 13 * 16;
					}
				}
				if (KeyData[i].MakeCode == A_DOWN)
				{
					if (s_loc % 16 != 0)
					{
						//���������
						s_loc--;
					}
					else
					{
						//�������
						s_loc += 15;
					}
				}
				if (KeyData[i].MakeCode == D_DOWN)
				{
					if (s_loc % 16 != 15)
					{
						//�������ұ�
						s_loc++;
					}
					else
					{
						//�����ұ�
						s_loc -= 15;
					}
				}

				if (g_Maze[s_loc] == '*')
				{
					s_loc = 16;
					DbgPrint("-1s\n");
				}
				else if (g_Maze[s_loc] == '7')
				{
					s_loc = 16;
					DbgPrint("The input is the flag!\n");
				}

				g_Maze[s_loc] = 'o';
			}

		}
	}

	if (irp->PendingReturned)
	{
		IoMarkIrpPending(irp);
	}
	return irp->IoStatus.Status;
}

NTSTATUS AttachDevices(PDRIVER_OBJECT DriverObject)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT FilterDeviceObject = NULL;
	PDEVICE_OBJECT TargetDeviceObject = NULL;
	PDEVICE_OBJECT LowerDeviceObject = NULL;
	PDRIVER_OBJECT KbdDriverObject = NULL;
	PDEVICE_EXTENSION DevExt = NULL;
	UNICODE_STRING KbdDriverName = RTL_CONSTANT_STRING(L"\\Driver\\kbdclass");

	status = ObReferenceObjectByName(&KbdDriverName, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, &KbdDriverObject);
	if (!NT_SUCCESS(status))
	{
		//DbgPrint("Reference kbdclass failed\n");
		return status;
	}
	else
	{
		ObDereferenceObject(KbdDriverObject);
	}

	//��kbdclass�µ������豸
	TargetDeviceObject = KbdDriverObject->DeviceObject;
	while (TargetDeviceObject)
	{
		status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), NULL, TargetDeviceObject->Type, TargetDeviceObject->Characteristics, FALSE, &FilterDeviceObject);
		if (!NT_SUCCESS(status))
		{
			//DbgPrint("Create FilterDevice failed\n");
			return status;
		}

		LowerDeviceObject = IoAttachDeviceToDeviceStack(FilterDeviceObject, TargetDeviceObject);
		if (!LowerDeviceObject)
		{
			//DbgPrint("Attach TargetDevice failed\n");
			IoDeleteDevice(FilterDeviceObject);
			return status;
		}

		//�豸����,��һ�β�����
		FilterDeviceObject->DeviceType = LowerDeviceObject->DeviceType;
		FilterDeviceObject->Characteristics = LowerDeviceObject->Characteristics;
		FilterDeviceObject->StackSize = LowerDeviceObject->StackSize + 1;
		FilterDeviceObject->Flags |= LowerDeviceObject->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE);
		FilterDeviceObject->Flags = LowerDeviceObject->Flags;

		//����豸��չ
		DevExt = (PDEVICE_EXTENSION)FilterDeviceObject->DeviceExtension;
		DevExt->TargetDevice = TargetDeviceObject;
		DevExt->LowerDevice = LowerDeviceObject;

		//��һ���豸
		TargetDeviceObject = TargetDeviceObject->NextDevice;
	}

	return status;
}