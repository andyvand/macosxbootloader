//********************************************************************
//	created:	6:11:2009   23:55
//	filename: 	BootDebugger.cpp
//	author:		tiamo
//	purpose:	boot debugger
//********************************************************************

#include "StdAfx.h"
#include "BootDebuggerPrivate.h"
#include "Debug1394.h"
#include "DebugUsb.h"

#ifdef __cplusplus
extern "C"
{
#endif
UINT8 BdDebuggerType														= KDP_TYPE_NONE;
BOOLEAN BdSubsystemInitialized												= FALSE;
BOOLEAN	BdConnectionActive													= FALSE;
BOOLEAN	BdArchBlockDebuggerOperation										= FALSE;
BdDebugRoutine BdDebugTrap													= NULL;
BdSendPacketRoutine BdSendPacket											= NULL;
BdReceivePacketRoutine BdReceivePacket										= NULL;
CHAR8 BdMessageBuffer[0x1000]												= {0};
CHAR8 BdDebugMessage[4096]													= {0};
BOOLEAN BdControlCPressed													= FALSE;
BOOLEAN	BdControlCPending													= FALSE;
BOOLEAN	BdDebuggerNotPresent												= TRUE;
UINT32 BdNextPacketIdToSend													= 0;
UINT32 BdPacketIdExpected													= 0;
UINT32 BdNumberRetries														= 5;
UINT32 BdRetryCount															= 5;
KDP_BREAKPOINT_TYPE	BdBreakpointInstruction									= KDP_BREAKPOINT_VALUE;
BREAKPOINT_ENTRY BdBreakpointTable[BREAKPOINT_TABLE_SIZE]					= {{0}};
UINT64 BdRemoteFiles[0x10]													= {0};
CHAR8 BdFileTransferBuffer[0x2000]											= {0};
LIST_ENTRY BdModuleList														= {0};
LDR_DATA_TABLE_ENTRY BdModuleDataTableEntry									= {{0}};
#ifdef __cplusplus
}
#endif
//
// Check write access.
//
STATIC BOOLEAN BdpWriteCheck(VOID* writeBuffer)
{
	UINT64 physicalAddress;
	return MmTranslateVirtualAddress(writeBuffer, &physicalAddress);
}

//
// Low write context.
//
STATIC BOOLEAN BdpLowWriteContent(UINT32 index)
{
	if (BdBreakpointTable[index].Flags & KD_BREAKPOINT_NEEDS_WRITE)
	{
		//
		// The breakpoint was never written out. Clear the flag and we are done.
		//
		BdBreakpointTable[index].Flags										&= ~KD_BREAKPOINT_NEEDS_WRITE;

		return TRUE;
	}

	//
	// The instruction is a breakpoint anyway.
	//
	if (BdBreakpointTable[index].Content == BdBreakpointInstruction)
		return TRUE;

	//
	// Restore old content.
	//
	if (BdMoveMemory(ArchConvertAddressToPointer(BdBreakpointTable[index].Address, VOID*), &BdBreakpointTable[index].Content, sizeof(KDP_BREAKPOINT_TYPE)) != sizeof(KDP_BREAKPOINT_TYPE))
	{
		BdBreakpointTable[index].Flags										|= KD_BREAKPOINT_NEEDS_REPLACE;
		return FALSE;
	}
	return TRUE;
}

//
// Add breakpoint.
//
STATIC UINT32 BdpAddBreakpoint(UINT64 address)
{
	//
	// Unable to write.
	//
	KDP_BREAKPOINT_TYPE oldContent;
	BOOLEAN addressValid													= BdMoveMemory(&oldContent, ArchConvertAddressToPointer(address, VOID*), sizeof(oldContent)) == sizeof(oldContent);

	if (addressValid && !BdpWriteCheck(ArchConvertAddressToPointer(address, VOID*)))
		return 0;

	//
	// Search free slot.
	//
	UINT32 index															= 0;

	for (; index < ARRAYSIZE(BdBreakpointTable); index ++)
	{
		if (!BdBreakpointTable[index].Flags)
			break;
	}

	//
	// Free slot not found.
	//
	if (index == ARRAYSIZE(BdBreakpointTable))
		return 0;

	//
	// Save address.
	//
	BdBreakpointTable[index].Address										= address;

	if (addressValid)
	{
		//
		// Write breakpoint instruction.
		//
		BdBreakpointTable[index].Content									= oldContent;
		BdBreakpointTable[index].Flags										= KD_BREAKPOINT_IN_USE;
		BdMoveMemory(ArchConvertAddressToPointer(address, VOID*), &BdBreakpointInstruction, sizeof(BdBreakpointInstruction));
	}
	else
	{
		//
		// Write pending.
		//
		BdBreakpointTable[index].Flags										= KD_BREAKPOINT_IN_USE | KD_BREAKPOINT_NEEDS_WRITE;
	}

	return index + 1;
}

//
// Delete breakpoint from index.
//
STATIC BOOLEAN BdpDeleteBreakpoint(UINT32 breakpointHandle)
{
	//
	// The specified handle is not valid.
	//
	UINT32 index															= breakpointHandle - 1;

	if (breakpointHandle == 0 || breakpointHandle > ARRAYSIZE(BdBreakpointTable))
		return FALSE;

	//
	// The specified breakpoint table entry is not valid.
	//
	if (!BdBreakpointTable[index].Flags)
		return FALSE;

	//
	// If the breakpoint is already suspended, just delete it from the table.
	//
	BOOLEAN removeIt														= FALSE;

	if (!(BdBreakpointTable[index].Flags & KD_BREAKPOINT_SUSPENDED) || (BdBreakpointTable[index].Flags & KD_BREAKPOINT_NEEDS_REPLACE))
		removeIt															= BdpLowWriteContent(index);
	else
		removeIt															= TRUE;

	//
	// Delete breakpoint table entry.
	//
	if (removeIt)
		BdBreakpointTable[index].Flags										= 0;

	return TRUE;
}

//
// Suspend breakpoint.
//
STATIC VOID BdpSuspendBreakpoint(UINT32 breakpointHandle)
{
	UINT32 index															= breakpointHandle - 1;

	if ((BdBreakpointTable[index].Flags & KD_BREAKPOINT_IN_USE) && !(BdBreakpointTable[index].Flags & KD_BREAKPOINT_SUSPENDED))
	{
		BdBreakpointTable[index].Flags										|= KD_BREAKPOINT_SUSPENDED;
		BdpLowWriteContent(index);
	}
}

//
// Suspend all breakpoints.
//
STATIC VOID BdpSuspendAllBreakpoints()
{
	for(UINT32 i = 1; i <= ARRAYSIZE(BdBreakpointTable); i ++)
		BdpSuspendBreakpoint(i);
}

//
// Set state change.
//
STATIC VOID BdpSetStateChange(DBGKD_WAIT_STATE_CHANGE64* waitStateChange, EXCEPTION_RECORD* exceptionRecord, CONTEXT* contextRecord)
{
	BdSetContextState(waitStateChange, contextRecord);
	waitStateChange->u.Exception.FirstChance								= TRUE;
}

//
// Convert exception record.
//
STATIC VOID BdpExceptionRecord32To64(EXCEPTION_RECORD* exceptionRecord, EXCEPTION_RECORD64* exceptionRecord64)
{
	//
	// Sign extend.
	//
	exceptionRecord64->ExceptionAddress										= (UINT64)((INTN)(exceptionRecord->ExceptionAddress));
	exceptionRecord64->ExceptionCode										= exceptionRecord->ExceptionCode;
	exceptionRecord64->ExceptionFlags										= exceptionRecord->ExceptionFlags;
	exceptionRecord64->ExceptionRecord										= (UINT64)(exceptionRecord->ExceptionRecord);
	exceptionRecord64->NumberParameters										= exceptionRecord->NumberParameters;

	for (UINT32 i = 0; i < ARRAYSIZE(exceptionRecord->ExceptionInformation); i ++)
		exceptionRecord64->ExceptionInformation[i]							= (UINT64)((INTN)(exceptionRecord->ExceptionInformation[i]));
}

//
// Read virtual memory.
//
STATIC VOID BdpReadVirtualMemory(DBGKD_MANIPULATE_STATE64* manipulateState, STRING* additionalData, CONTEXT* contextRecord)
{
	STRING messageHeader;
	messageHeader.Length													= sizeof(DBGKD_MANIPULATE_STATE64);
	messageHeader.Buffer													= (CHAR8*)((VOID*)(manipulateState));
	UINT32 readLength														= manipulateState->ReadMemory.TransferCount;
	VOID* readAddress														= ArchConvertAddressToPointer(manipulateState->ReadMemory.TargetBaseAddress, VOID*);

	if (readLength > PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64))
		readLength															= PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64);

	additionalData->Length													= (UINT16)(BdMoveMemory(additionalData->Buffer, readAddress, readLength));
	manipulateState->ReturnStatus											= additionalData->Length == readLength ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
	manipulateState->ReadMemory.ActualBytesRead								= additionalData->Length;
	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, additionalData);
}

//
// Write virtual memory.
//
STATIC VOID BdpWriteVirtualMemory(DBGKD_MANIPULATE_STATE64* manipulateState, STRING* additionalData, CONTEXT* contextRecord)
{
	STRING messageHeader;
	messageHeader.Length													= sizeof(DBGKD_MANIPULATE_STATE64);
	messageHeader.Buffer													= (CHAR8*)((VOID*)(manipulateState));
	VOID* writeAddress														= ArchConvertAddressToPointer(manipulateState->WriteMemory.TargetBaseAddress, VOID*);
	UINT32 writenLength														= BdMoveMemory(writeAddress, additionalData->Buffer, additionalData->Length);
	manipulateState->WriteMemory.ActualBytesWritten							= writenLength;
	manipulateState->ReturnStatus											= additionalData->Length == writenLength ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, NULL);
}

//
// Get context.
//
STATIC VOID BdpGetcontextRecord(DBGKD_MANIPULATE_STATE64* manipulateState, STRING* additionalData, CONTEXT* contextRecord)
{
	STRING messageHeader;
	messageHeader.Length													= sizeof(DBGKD_MANIPULATE_STATE64);
	messageHeader.Buffer													= (CHAR8*)((VOID*)(manipulateState));
	manipulateState->ReturnStatus											= STATUS_SUCCESS;
	additionalData->Length													= sizeof(CONTEXT);
	BdCopyMemory(additionalData->Buffer, contextRecord, sizeof(CONTEXT));
	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, additionalData);
}

//
// Set context.
//
STATIC VOID BdpSetcontextRecord(DBGKD_MANIPULATE_STATE64* manipulateState, STRING* additionalData, CONTEXT* contextRecord)
{
	STRING messageHeader;
	messageHeader.Length													= sizeof(DBGKD_MANIPULATE_STATE64);
	messageHeader.Buffer													= (CHAR8*)((VOID*)(manipulateState));
	manipulateState->ReturnStatus											= STATUS_SUCCESS;
	BdCopyMemory(contextRecord, additionalData->Buffer, sizeof(CONTEXT));
	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, NULL);
}

//
// Write breakpoint.
//
STATIC VOID BdpWriteBreakpoint(DBGKD_MANIPULATE_STATE64* manipulateState, STRING* additionalData, CONTEXT* contextRecord)
{
	STRING messageHeader;
	messageHeader.Length													= sizeof(DBGKD_MANIPULATE_STATE64);
	messageHeader.Buffer													= (CHAR8*)((VOID*)(manipulateState));
	manipulateState->WriteBreakPoint.BreakPointHandle						= BdpAddBreakpoint(manipulateState->WriteBreakPoint.BreakPointAddress);
	manipulateState->ReturnStatus											= manipulateState->WriteBreakPoint.BreakPointHandle ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, NULL);
}

//
// Restore breakpoint.
//
STATIC VOID BdpRestoreBreakpoint(DBGKD_MANIPULATE_STATE64* manipulateState, STRING* additionalData, CONTEXT* contextRecord)
{
	STRING messageHeader;
	messageHeader.Length													= sizeof(DBGKD_MANIPULATE_STATE64);
	messageHeader.Buffer													= (CHAR8*)((VOID*)(manipulateState));
	manipulateState->ReturnStatus											= BdpDeleteBreakpoint(manipulateState->RestoreBreakPoint.BreakPointHandle) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, NULL);
}

//
// Read port.
//
STATIC VOID BdpReadIoSpace(DBGKD_MANIPULATE_STATE64* manipulateState, STRING* additionalData, CONTEXT* contextRecord)
{
	STRING messageHeader;
	DBGKD_READ_WRITE_IO64* readIo											= &manipulateState->ReadWriteIo;
	messageHeader.Length													= sizeof(DBGKD_MANIPULATE_STATE64);
	messageHeader.Buffer													= (CHAR8*)((VOID*)(manipulateState));
	manipulateState->ReturnStatus											= STATUS_SUCCESS;
	readIo->DataValue														= 0;

	switch (readIo->DataSize)
	{
		case 1:
			readIo->DataValue												= ARCH_READ_PORT_UINT8(ArchConvertAddressToPointer(readIo->IoAddress, UINT8*));
			break;

		case 2:
			if (readIo->IoAddress & 1)
				manipulateState->ReturnStatus								= STATUS_DATATYPE_MISALIGNMENT;
			else
				readIo->DataValue											= ARCH_READ_PORT_UINT16(ArchConvertAddressToPointer(readIo->IoAddress, UINT16*));
			break;

		case 4:
			if (readIo->IoAddress & 3)
				manipulateState->ReturnStatus								= STATUS_DATATYPE_MISALIGNMENT;
			else
				readIo->DataValue											= ARCH_READ_PORT_UINT32(ArchConvertAddressToPointer(readIo->IoAddress, UINT32*));
			break;

		default:
			manipulateState->ReturnStatus									= STATUS_INVALID_PARAMETER;
			break;
	}

	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, NULL);
}

//
// Write port.
//
STATIC VOID BdpWriteIoSpace(DBGKD_MANIPULATE_STATE64* manipulateState, STRING* additionalData, CONTEXT* contextRecord)
{
	STRING messageHeader;
	DBGKD_READ_WRITE_IO64* writeIo											= &manipulateState->ReadWriteIo;
	messageHeader.Length													= sizeof(DBGKD_MANIPULATE_STATE64);
	messageHeader.Buffer													= (CHAR8*)((VOID*)(manipulateState));
	manipulateState->ReturnStatus											= STATUS_SUCCESS;

	switch (writeIo->DataSize)
	{
		case 1:
			ARCH_WRITE_PORT_UINT8(ArchConvertAddressToPointer(writeIo->IoAddress, UINT8*), (UINT8)(writeIo->DataValue & 0xff));
			break;

		case 2:
			if (writeIo->IoAddress & 1)
				manipulateState->ReturnStatus								= STATUS_DATATYPE_MISALIGNMENT;
			else
				ARCH_WRITE_PORT_UINT16(ArchConvertAddressToPointer(writeIo->IoAddress, UINT16*), (UINT16)(writeIo->DataValue & 0xffff));
			break;

		case 4:
			if (writeIo->IoAddress & 3)
				manipulateState->ReturnStatus								= STATUS_DATATYPE_MISALIGNMENT;
			else
				ARCH_WRITE_PORT_UINT32(ArchConvertAddressToPointer(writeIo->IoAddress, UINT32*), writeIo->DataValue);
			break;

		default:
			manipulateState->ReturnStatus									= STATUS_INVALID_PARAMETER;
			break;
	}

	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, NULL);
}

//
// Read port.
//
STATIC VOID BdpReadIoSpaceEx(DBGKD_MANIPULATE_STATE64* manipulateState, STRING* additionalData, CONTEXT* contextRecord)
{
	STRING messageHeader;
	DBGKD_READ_WRITE_IO_EXTENDED64* readIo									= &manipulateState->ReadWriteIoExtended;
	messageHeader.Length													= sizeof(DBGKD_MANIPULATE_STATE64);
	messageHeader.Buffer													= (CHAR8*)((VOID*)(manipulateState));
	manipulateState->ReturnStatus											= STATUS_SUCCESS;
	readIo->DataValue														= 0;

	switch (readIo->DataSize)
	{
		case 1:
			readIo->DataValue												= ARCH_READ_PORT_UINT8(ArchConvertAddressToPointer(readIo->IoAddress, UINT8*));
			break;

		case 2:
			if (readIo->IoAddress & 1)
				manipulateState->ReturnStatus								= STATUS_DATATYPE_MISALIGNMENT;
			else
				readIo->DataValue											= ARCH_READ_PORT_UINT16(ArchConvertAddressToPointer(readIo->IoAddress, UINT16*));
			break;

		case 4:
			if (readIo->IoAddress & 3)
				manipulateState->ReturnStatus								= STATUS_DATATYPE_MISALIGNMENT;
			else
				readIo->DataValue											= ARCH_READ_PORT_UINT32(ArchConvertAddressToPointer(readIo->IoAddress, UINT32*));
			break;

		default:
			manipulateState->ReturnStatus									= STATUS_INVALID_PARAMETER;
			break;
	}

	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, NULL);
}

//
// Write port.
//
STATIC VOID BdpWriteIoSpaceEx(DBGKD_MANIPULATE_STATE64* manipulateState, STRING* additionalData, CONTEXT* contextRecord)
{
	STRING messageHeader;
	DBGKD_READ_WRITE_IO_EXTENDED64* writeIo									= &manipulateState->ReadWriteIoExtended;
	messageHeader.Length													= sizeof(DBGKD_MANIPULATE_STATE64);
	messageHeader.Buffer													= (CHAR8*)((VOID*)(manipulateState));
	manipulateState->ReturnStatus											= STATUS_SUCCESS;

	switch (writeIo->DataSize)
	{
		case 1:
			ARCH_WRITE_PORT_UINT8(ArchConvertAddressToPointer(writeIo->IoAddress, UINT8*), (UINT8)(writeIo->DataValue & 0xff));
			break;

		case 2:
			if (writeIo->IoAddress & 1)
				manipulateState->ReturnStatus								= STATUS_DATATYPE_MISALIGNMENT;
			else
				ARCH_WRITE_PORT_UINT16(ArchConvertAddressToPointer(writeIo->IoAddress, UINT16*), (UINT16)(writeIo->DataValue & 0xffff));
			break;

		case 4:
			if (writeIo->IoAddress & 3)
				manipulateState->ReturnStatus								= STATUS_DATATYPE_MISALIGNMENT;
			else
				ARCH_WRITE_PORT_UINT32(ArchConvertAddressToPointer(writeIo->IoAddress, UINT32*), writeIo->DataValue);
			break;

		default:
			manipulateState->ReturnStatus									= STATUS_INVALID_PARAMETER;
			break;
	}

	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, NULL);
}

//
// Read physical memory.
//
STATIC VOID BdpReadPhysicalMemory(DBGKD_MANIPULATE_STATE64* manipulateState, STRING* additionalData, CONTEXT* contextRecord)
{
	STRING messageHeader;
	DBGKD_READ_MEMORY64* readMemory											= &manipulateState->ReadMemory;
	messageHeader.Length													= sizeof(DBGKD_MANIPULATE_STATE64);
	messageHeader.Buffer													= (CHAR8*)((VOID*)(manipulateState));
	UINT64 startAddress														= readMemory->TargetBaseAddress;
	UINT32 readLength														= readMemory->TransferCount;

	if (readLength > PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64))
		readLength															= PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64);

	UINT64 endAddress														= startAddress + readLength;

	if (PAGE_ALIGN(startAddress) == PAGE_ALIGN(endAddress))
	{
		VOID* address														= BdTranslatePhysicalAddress(startAddress);

		if (address)
			additionalData->Length											= (UINT16)(BdMoveMemory(additionalData->Buffer, address, readLength));
		else
			additionalData->Length											= 0;
	}
	else
	{
		do
		{
			UINT32 leftCount												= readLength;
			CHAR8* buffer													= additionalData->Buffer;
			VOID* address													= BdTranslatePhysicalAddress(startAddress);

			if (!address)
			{
				additionalData->Length										= 0;
				break;
			}

			UINT32 lengthThisRun											= (UINT32)(EFI_PAGE_SIZE - BYTE_OFFSET(address));
			additionalData->Length											= (UINT16)(BdMoveMemory(buffer, address, lengthThisRun));
			leftCount														-= lengthThisRun;
			startAddress													+= lengthThisRun;
			buffer															+= lengthThisRun;

			while (leftCount)
			{
				address														= BdTranslatePhysicalAddress(startAddress);

				if (!address)
					break;

				lengthThisRun												= EFI_PAGE_SIZE > leftCount ? EFI_PAGE_SIZE : leftCount;
				additionalData->Length										+= (UINT16)(BdMoveMemory(buffer, address, lengthThisRun));
				leftCount													-= lengthThisRun;
				startAddress												+= lengthThisRun;
				buffer														+= lengthThisRun;
			}
		} while (0);
	}

	readMemory->ActualBytesRead												= additionalData->Length;
	manipulateState->ReturnStatus											= readLength == additionalData->Length ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, additionalData);
}

//
// Write physical memory.
//
STATIC VOID BdpWritePhysicalMemory(DBGKD_MANIPULATE_STATE64* manipulateState, STRING* additionalData, CONTEXT* contextRecord)
{
	STRING messageHeader;
	DBGKD_WRITE_MEMORY64* writeMemory										= &manipulateState->WriteMemory;
	messageHeader.Length													= sizeof(DBGKD_MANIPULATE_STATE64);
	messageHeader.Buffer													= (CHAR8*)((VOID*)(manipulateState));
	UINT64 startAddress														= writeMemory->TargetBaseAddress;
	UINT64 endAddress														= startAddress + writeMemory->TransferCount;
	UINT32 writtenCount														= 0;

	if (PAGE_ALIGN(startAddress) == PAGE_ALIGN(endAddress))
	{
		writtenCount														= BdMoveMemory(BdTranslatePhysicalAddress(startAddress), additionalData->Buffer, writeMemory->TransferCount);
	}
	else
	{
		UINT32 leftCount													= writeMemory->TransferCount;
		CHAR8* buffer														= additionalData->Buffer;
		VOID* address														= BdTranslatePhysicalAddress(startAddress);
		UINT32 lengthThisRun												= (UINT32)(EFI_PAGE_SIZE - BYTE_OFFSET(address));
		UINT32 thisRun														= BdMoveMemory(address, buffer, lengthThisRun);
		writtenCount														+= thisRun;
		leftCount															-= lengthThisRun;
		startAddress														+= lengthThisRun;
		buffer																+= lengthThisRun;

		while (leftCount)
		{
			lengthThisRun													= EFI_PAGE_SIZE > leftCount ? EFI_PAGE_SIZE : leftCount;
			thisRun															= BdMoveMemory(BdTranslatePhysicalAddress(startAddress), buffer, lengthThisRun);
			writtenCount													+= thisRun;
			leftCount														-= lengthThisRun;
			startAddress													+= lengthThisRun;
			buffer															+= lengthThisRun;
		}
	}

	writeMemory->ActualBytesWritten											= writtenCount;
	manipulateState->ReturnStatus											= writtenCount == additionalData->Length ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, NULL);
}

//
// Get version.
//
STATIC VOID BdpGetVersion(DBGKD_MANIPULATE_STATE64* manipulateState)
{
	memset(&manipulateState->GetVersion64, 0, sizeof(manipulateState->GetVersion64));

#if (defined(_DEBUG) || defined(DEBUG))
	manipulateState->GetVersion64.MajorVersion								= 0x040c;
#else
	manipulateState->GetVersion64.MajorVersion								= 0x040f;
#endif

	manipulateState->GetVersion64.MinorVersion								= 6000;
	manipulateState->GetVersion64.ProtocolVersion							= DBGKD_64BIT_PROTOCOL_VERSION2;
	manipulateState->GetVersion64.KdSecondaryVersion						= EFI_IMAGE_MACHINE_TYPE == EFI_IMAGE_MACHINE_X64 ? 2 : 0;
	manipulateState->GetVersion64.Flags										= DBGKD_VERS_FLAG_DATA | (sizeof(UINT64) == sizeof(VOID*) ? DBGKD_VERS_FLAG_PTR64 : 0);
	manipulateState->GetVersion64.MachineType								= EFI_IMAGE_MACHINE_TYPE;
	manipulateState->GetVersion64.MaxPacketType								= PACKET_TYPE_MAX;
	manipulateState->GetVersion64.MaxStateChange							= DbgKdCommandStringStateChange - DbgKdMinimumStateChange;
	manipulateState->GetVersion64.MaxManipulate								= DbgKdCheckLowMemoryApi - DbgKdMinimumManipulate;
	manipulateState->GetVersion64.DebuggerDataList							= 0;
	manipulateState->GetVersion64.PsLoadedModuleList						= ArchConvertPointerToAddress(&BdModuleList);
	manipulateState->GetVersion64.KernBase									= ArchConvertPointerToAddress(BdModuleDataTableEntry.DllBase);
	manipulateState->ReturnStatus											= STATUS_SUCCESS;

	STRING messageHeader;
	messageHeader.Length													= sizeof(DBGKD_MANIPULATE_STATE64);
	messageHeader.Buffer													= (CHAR8*)((VOID*)(manipulateState));
	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, NULL);
}

//
// Write breakpoint ext.
//
STATIC NTSTATUS BdpWriteBreakPointEx(DBGKD_MANIPULATE_STATE64* manipulateState, STRING* additionalData, CONTEXT* contextRecord)
{
	STRING messageHeader;
	DBGKD_BREAKPOINTEX* breakpointEx										= &manipulateState->BreakPointEx;
	messageHeader.Length													= sizeof(DBGKD_MANIPULATE_STATE64);
	messageHeader.Buffer													= (CHAR8*)((VOID*)(manipulateState));

	//
	// Verify that the packet size is correct.
	//
	if (additionalData->Length != breakpointEx->BreakPointCount * sizeof(DBGKD_WRITE_BREAKPOINT64) || breakpointEx->BreakPointCount > ARRAYSIZE(BdBreakpointTable))
	{
		manipulateState->ReturnStatus										= STATUS_UNSUCCESSFUL;
		BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, additionalData);
		return STATUS_UNSUCCESSFUL;
	}

	DBGKD_WRITE_BREAKPOINT64 bpBuf[ARRAYSIZE(BdBreakpointTable)];
	BdMoveMemory(bpBuf, additionalData->Buffer, breakpointEx->BreakPointCount * sizeof(DBGKD_WRITE_BREAKPOINT64));
	manipulateState->ReturnStatus											= STATUS_SUCCESS;
	DBGKD_WRITE_BREAKPOINT64* b												= bpBuf;

	for (UINT32 i = 0; i < breakpointEx->BreakPointCount; i ++, b ++)
	{
		if (b->BreakPointHandle)
		{
			if (!BdpDeleteBreakpoint(b->BreakPointHandle))
				manipulateState->ReturnStatus								= STATUS_UNSUCCESSFUL;

			b->BreakPointHandle												= 0;
		}
	}

	b																		= bpBuf;

	for (UINT32 i = 0; i < breakpointEx->BreakPointCount; i ++, b ++)
	{
		if (b->BreakPointAddress)
		{
			b->BreakPointHandle												= BdpAddBreakpoint(b->BreakPointAddress);
			if (!b->BreakPointHandle)
				manipulateState->ReturnStatus								= STATUS_UNSUCCESSFUL;
		}
	}

	BdMoveMemory(additionalData->Buffer, bpBuf, breakpointEx->BreakPointCount * sizeof(DBGKD_WRITE_BREAKPOINT64));
	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, additionalData);

	return breakpointEx->ContinueStatus;
}

//
// Restore breakpoint ex.
//
STATIC VOID BdpRestoreBreakPointEx(DBGKD_MANIPULATE_STATE64* manipulateState, STRING* additionalData, CONTEXT* contextRecord)
{
	STRING messageHeader;
	DBGKD_BREAKPOINTEX* breakpointEx										= &manipulateState->BreakPointEx;
	messageHeader.Length													= sizeof(DBGKD_MANIPULATE_STATE64);
	messageHeader.Buffer													= (CHAR8*)((VOID*)(manipulateState));

	//
	// Verify that the packet size is correct.
	//
	if (additionalData->Length != breakpointEx->BreakPointCount * sizeof(DBGKD_RESTORE_BREAKPOINT) || breakpointEx->BreakPointCount > ARRAYSIZE(BdBreakpointTable))
	{
		manipulateState->ReturnStatus										= STATUS_UNSUCCESSFUL;
		BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, additionalData);

		return;
	}

	DBGKD_RESTORE_BREAKPOINT bpBuf[ARRAYSIZE(BdBreakpointTable)];
	BdMoveMemory(bpBuf, additionalData->Buffer, breakpointEx->BreakPointCount * sizeof(DBGKD_RESTORE_BREAKPOINT));
	manipulateState->ReturnStatus											= STATUS_SUCCESS;
	DBGKD_RESTORE_BREAKPOINT* b												= bpBuf;

	for (UINT32 i = 0; i < breakpointEx->BreakPointCount; i ++, b ++)
	{
		if (!BdpDeleteBreakpoint(b->BreakPointHandle))
			manipulateState->ReturnStatus									= STATUS_UNSUCCESSFUL;
	}

	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, additionalData);
}

//
// Validate PCI slot.
//
STATIC VOID BdpReadPciConfigSpace(UINT32 bus, UINT32 device, UINT32 func, UINT32 offset, VOID* dataBuffer, UINT32 length);
STATIC BOOLEAN BdpValidatePciSlot(UINT32 bus, UINT32 device, UINT32 func)
{
	if (bus > PCI_MAX_BUS || device > PCI_MAX_DEVICE || func > PCI_MAX_FUNC)
		return FALSE;

	if (!func)
		return TRUE;

	UINT8 headerType														= 0;
	UINT32 length															= sizeof(headerType);
	BdpReadPciConfigSpace(bus, device, 0, EFI_FIELD_OFFSET(PCI_DEVICE_INDEPENDENT_REGION, HeaderType), &headerType, length);

	return headerType != 0xff && (headerType & HEADER_TYPE_MULTI_FUNCTION);
}

//
// Read PCI config.
//
STATIC VOID BdpReadWritePciConfigSpace(UINT32 bus, UINT32 device, UINT32 func, UINT32 offset, VOID* dataBuffer, UINT32 length, BOOLEAN readMode)
{
	UINT32 cf8BaseValue														= 0x80000000 | ((bus & 0xff) << 16) | ((device & 0x1f) << 11) | ((func & 0x07) << 8);

	while (length)
	{
		ARCH_WRITE_PORT_UINT32(ArchConvertAddressToPointer(0xcf8, UINT32*), cf8BaseValue | (offset & 0xfc));
		STATIC UINT32 pciReadWriteLength[3]									= {sizeof(UINT32), sizeof(UINT8), sizeof(UINT16)};
		STATIC UINT32 pciReadWriteType[4][4]								= {{0, 1, 2, 2}, {1, 1, 1, 1}, {2, 1, 2, 2}, {1, 1, 1, 1}};
		UINT32 type															= pciReadWriteType[offset % sizeof(UINT32)][length % sizeof(UINT32)];

		switch (type)
		{
			case 0:
				if (readMode)
					*(UINT32*)(dataBuffer)						= ARCH_READ_PORT_UINT32(ArchConvertAddressToPointer(0xcfc + (offset % sizeof(UINT32)), UINT32*));
				else
					ARCH_WRITE_PORT_UINT32(ArchConvertAddressToPointer(0xcfc + (offset % sizeof(UINT32)), UINT32*), *(UINT32*)(dataBuffer));
				break;

			case 1:
				if (readMode)
					*(UINT8*)(dataBuffer)						= ARCH_READ_PORT_UINT8(ArchConvertAddressToPointer(0xcfc + (offset % sizeof(UINT32)), UINT8*));
				else
					ARCH_WRITE_PORT_UINT8(ArchConvertAddressToPointer(0xcfc + (offset % sizeof(UINT32)), UINT8*), *(UINT8*)(dataBuffer));
				break;

			case 2:
				if (readMode)
					*(UINT16*)(dataBuffer)						= ARCH_READ_PORT_UINT16(ArchConvertAddressToPointer(0xcfc + (offset % sizeof(UINT32)), UINT16*));
				else
					ARCH_WRITE_PORT_UINT16(ArchConvertAddressToPointer(0xcfc + (offset % sizeof(UINT32)), UINT16*), *(UINT16*)(dataBuffer));
				break;
		}

		if (type < 3)
		{
			length															-= pciReadWriteLength[type];
			offset															+= pciReadWriteLength[type];
			dataBuffer														= Add2Ptr(dataBuffer, pciReadWriteLength[type], VOID*);
		}
	}

	ARCH_WRITE_PORT_UINT32(ArchConvertAddressToPointer(0xcf8, UINT32*), 0);
}

//
// Read CPI config.
//
STATIC VOID BdpReadPciConfigSpace(UINT32 bus, UINT32 device, UINT32 func, UINT32 offset, VOID* dataBuffer, UINT32 length)
{
	if (!BdpValidatePciSlot(bus, device, func))
		memset(dataBuffer, 0xff, length);
	else
		BdpReadWritePciConfigSpace(bus, device, func, offset, dataBuffer, length, TRUE);
}

//
// Read PCI config space.
//
STATIC UINT32 BdpReadPciConfigSpace2(UINT32 bus, UINT32 device, UINT32 func, UINT32 offset, UINT32 length, VOID* dataBuffer)
{
	if (offset >= PCI_MAX_CONFIG_OFFSET)
		return 0;

	if (length + offset > PCI_MAX_CONFIG_OFFSET)
		length																= PCI_MAX_CONFIG_OFFSET - offset;

	UINT32 readLength														= 0;
	PCI_TYPE_GENERIC localHeader											= {{{0}}};

	if (offset >= sizeof(localHeader))
	{
		BdpReadPciConfigSpace(bus, device, func, 0, &localHeader, EFI_FIELD_OFFSET(PCI_DEVICE_INDEPENDENT_REGION, Command));
		if (localHeader.Device.Hdr.VendorId == 0xffff || localHeader.Device.Hdr.VendorId == 0)
			return 0;
	}
	else
	{
		readLength															= sizeof(localHeader);
		BdpReadPciConfigSpace(bus, device, func, 0, &localHeader, sizeof(localHeader));

		if (localHeader.Device.Hdr.VendorId == 0xffff || localHeader.Device.Hdr.VendorId == 0)
		{
			localHeader.Device.Hdr.VendorId									= 0xffff;
			readLength														= EFI_FIELD_OFFSET(PCI_DEVICE_INDEPENDENT_REGION, VendorId) + sizeof(localHeader.Device.Hdr.VendorId);
		}

		if (readLength < offset)
			return 0;

		readLength															-= offset;
		if (readLength > length)
			readLength														= length;

		memcpy(dataBuffer, Add2Ptr(&localHeader, offset, VOID*), readLength);

		offset																+= readLength;
		dataBuffer															= Add2Ptr(dataBuffer, readLength, VOID*);
		length																-= readLength;
	}

	if (length && offset >= sizeof(localHeader))
	{
		BdpReadPciConfigSpace(bus, device, func, offset, dataBuffer, length);
		readLength															+= length;
	}

	return readLength;
}

//
// Get bus data.
//
STATIC VOID BdpGetBusData(DBGKD_MANIPULATE_STATE64* manipulateState, STRING* additionalData, CONTEXT* contextRecord)
{
	STRING messageHeader;
	DBGKD_GET_SET_BUS_DATA* getBusData										= &manipulateState->GetSetBusData;
	messageHeader.Length													= sizeof(DBGKD_MANIPULATE_STATE64);
	messageHeader.Buffer													= (CHAR8*)((VOID*)(manipulateState));
	UINT32 readLength														= getBusData->Length;
	getBusData->Length														= 0;

	if (readLength > PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64))
		readLength															= PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64);

	//
	// Only support PCI bus.
	//
	if (getBusData->BusDataType != 4)
	{
		manipulateState->ReturnStatus										= STATUS_UNSUCCESSFUL;
		BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, additionalData);

		return;
	}

	//
	// Read it.
	//
	UINT32 device															= getBusData->SlotNumber & 0x1f;
	UINT32 func																= (getBusData->SlotNumber >> 5) & 0x07;
	getBusData->Length														= BdpReadPciConfigSpace2(getBusData->BusNumber, device, func, getBusData->Offset, readLength, additionalData->Buffer);
	additionalData->Length													= (UINT16)(getBusData->Length);
	manipulateState->ReturnStatus											= getBusData->Length == readLength ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, additionalData);
}

//
// Write CPI config.
//
STATIC VOID BdpWritePciConfigSpace(UINT32 bus, UINT32 device, UINT32 func, UINT32 offset, VOID* dataBuffer, UINT32 length)
{
	if (BdpValidatePciSlot(bus, device, func))
		BdpReadWritePciConfigSpace(bus, device, func, offset, dataBuffer, length, FALSE);
}

//
// Write PCI config space.
//
STATIC UINT32 BdpWritePciConfigSpace2(UINT32 bus, UINT32 device, UINT32 func, UINT32 offset, UINT32 length, VOID* dataBuffer)
{
	if (offset >= PCI_MAX_CONFIG_OFFSET)
		return 0;

	if (length + offset > PCI_MAX_CONFIG_OFFSET)
		length																= PCI_MAX_CONFIG_OFFSET - offset;

	UINT32 writeLength														= 0;
	PCI_TYPE_GENERIC localHeader											= {{{0}}};

	if (offset >= sizeof(localHeader))
	{
		BdpReadPciConfigSpace(bus, device, func, 0, &localHeader, EFI_FIELD_OFFSET(PCI_DEVICE_INDEPENDENT_REGION, Command));

		if (localHeader.Device.Hdr.VendorId == 0xffff || localHeader.Device.Hdr.VendorId == 0)
			return 0;
	}
	else
	{
		writeLength															= sizeof(localHeader);
		BdpReadPciConfigSpace(bus, device, func, 0, &localHeader, sizeof(localHeader));

		if (localHeader.Device.Hdr.VendorId == 0xffff || localHeader.Device.Hdr.VendorId == 0)
			return 0;

		writeLength															-= offset;

		if (writeLength > length)
			writeLength														= length;

		memcpy(Add2Ptr(&localHeader, offset, VOID*), dataBuffer, writeLength);

		BdpWritePciConfigSpace(bus, device, func, 0, Add2Ptr(&localHeader, offset, VOID*), writeLength);

		offset																+= writeLength;
		dataBuffer															= Add2Ptr(dataBuffer, writeLength, VOID*);
		length																-= writeLength;
	}

	if (length && offset >= sizeof(localHeader))
	{
		BdpWritePciConfigSpace(bus, device, func, offset, dataBuffer, length);
		writeLength															+= length;
	}

	return writeLength;
}

//
// Set bus data.
//
STATIC VOID BdpSetBusData(DBGKD_MANIPULATE_STATE64* manipulateState, STRING* additionalData, CONTEXT* contextRecord)
{
	STRING messageHeader;
	DBGKD_GET_SET_BUS_DATA* setBusData										= &manipulateState->GetSetBusData;
	messageHeader.Length													= sizeof(DBGKD_MANIPULATE_STATE64);
	messageHeader.Buffer													= (CHAR8*)((VOID*)(manipulateState));
	UINT32 writeLength														= setBusData->Length;
	setBusData->Length														= 0;

	//
	// Only support PCI bus.
	//
	if (setBusData->BusDataType != 4)
	{
		manipulateState->ReturnStatus										= STATUS_UNSUCCESSFUL;
		BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, additionalData);
		return;
	}

	//
	// Write it.
	//
	UINT32 device															= setBusData->SlotNumber & 0x1f;
	UINT32 func																= (setBusData->SlotNumber >> 5) & 0x07;
	setBusData->Length														= BdpWritePciConfigSpace2(setBusData->BusNumber, device, func, setBusData->Offset, writeLength, additionalData->Buffer);
	manipulateState->ReturnStatus											= setBusData->Length == writeLength ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &messageHeader, NULL);
}


//
// Send and wait.
//
STATIC UINT32 BdpSendWaitContinue(UINT32 packetType, STRING* outputHead, STRING* outputData, CONTEXT* contextRecord)
{
	STRING inputHead;
	STRING inputData;
	DBGKD_MANIPULATE_STATE64 manipulateState;
	inputHead.MaximumLength													= sizeof(DBGKD_MANIPULATE_STATE64);
	inputHead.Buffer														= (CHAR8*)((VOID*)(&manipulateState));
	inputData.MaximumLength													= sizeof(BdMessageBuffer);
	inputData.Buffer														= BdMessageBuffer;
	BOOLEAN sendOutputPacket												= TRUE;

	while (TRUE)
	{
		if (sendOutputPacket)
		{
			BdSendPacket(packetType, outputHead, outputData);

			if (BdDebuggerNotPresent)
				return KD_CONTINUE_SUCCESS;
		}

		UINT32 replyCode													= KDP_PACKET_TIMEOUT;
		UINT32 length														= 0;
		sendOutputPacket													= FALSE;

		do
		{
			//
			// Wait a reply packet.
			//
			replyCode														= BdReceivePacket(PACKET_TYPE_KD_STATE_MANIPULATE, &inputHead, &inputData, &length);
			if (replyCode == KDP_PACKET_RESEND)
				sendOutputPacket											= TRUE;

		} while (replyCode == KDP_PACKET_TIMEOUT);

		//
		// Resend packet.
		//
		if (sendOutputPacket)
			continue;

		//
		// Case on API number.
		//
		switch(manipulateState.ApiNumber)
		{
			case DbgKdReadVirtualMemoryApi:
				BdpReadVirtualMemory(&manipulateState, &inputData, contextRecord);
				break;

			case DbgKdWriteVirtualMemoryApi:
				BdpWriteVirtualMemory(&manipulateState, &inputData, contextRecord);
				break;

			case DbgKdGetContextApi:
				BdpGetcontextRecord(&manipulateState, &inputData, contextRecord);
				break;

			case DbgKdSetContextApi:
				BdpSetcontextRecord(&manipulateState, &inputData, contextRecord);
				break;

			case DbgKdWriteBreakPointApi:
				BdpWriteBreakpoint(&manipulateState, &inputData, contextRecord);
				break;

			case DbgKdRestoreBreakPointApi:
				BdpRestoreBreakpoint(&manipulateState, &inputData, contextRecord);
				break;

			case DbgKdContinueApi:
				return NT_SUCCESS(manipulateState.Continue.ContinueStatus) ? KD_CONTINUE_SUCCESS : KD_CONTINUE_ERROR;
				break;

			case DbgKdReadControlSpaceApi:
				BdReadControlSpace(&manipulateState, &inputData, contextRecord);
				break;

			case DbgKdWriteControlSpaceApi:
				BdWriteControlSpace(&manipulateState, &inputData, contextRecord);
				break;

			case DbgKdReadIoSpaceApi:
				BdpReadIoSpace(&manipulateState, &inputData, contextRecord);
				break;

			case DbgKdWriteIoSpaceApi:
				BdpWriteIoSpace(&manipulateState, &inputData, contextRecord);
				break;

			case DbgKdRebootApi:
				EfiRuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
				break;

			case DbgKdContinueApi2:

				if (!NT_SUCCESS(manipulateState.Continue2.ContinueStatus))
					return KD_CONTINUE_ERROR;

				BdGetStateChange(&manipulateState, contextRecord);

				return KD_CONTINUE_SUCCESS;
				break;

			case DbgKdReadPhysicalMemoryApi:
				BdpReadPhysicalMemory(&manipulateState, &inputData, contextRecord);
				break;

			case DbgKdWritePhysicalMemoryApi:
				BdpWritePhysicalMemory(&manipulateState, &inputData, contextRecord);
				break;

			case DbgKdReadIoSpaceExtendedApi:
				BdpReadIoSpaceEx(&manipulateState, &inputData, contextRecord);
				break;

			case DbgKdWriteIoSpaceExtendedApi:
				BdpWriteIoSpaceEx(&manipulateState, &inputData, contextRecord);
				break;

			case DbgKdGetVersionApi:
				BdpGetVersion(&manipulateState);
				break;

			case DbgKdWriteBreakPointExApi:

				if (BdpWriteBreakPointEx(&manipulateState, &inputData, contextRecord) != STATUS_SUCCESS)
					return KD_CONTINUE_ERROR;
				break;

			case DbgKdRestoreBreakPointExApi:
				BdpRestoreBreakPointEx(&manipulateState, &inputData, contextRecord);
				break;

			case DbgKdGetBusDataApi:
				BdpGetBusData(&manipulateState, &inputData, contextRecord);
				break;

			case DbgKdSetBusDataApi:
				BdpSetBusData(&manipulateState, &inputData, contextRecord);
				break;

			default:
				inputData.Length												= 0;
				manipulateState.ReturnStatus									= STATUS_UNSUCCESSFUL;
				BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &inputHead, &inputData);
				break;
		}
	}
}

//
// Close debug device.
//
STATIC VOID BdpCloseDebuggerDevice()
{
	switch (BdDebuggerType)
	{
		case KDP_TYPE_COM:
			break;

		case KDP_TYPE_1394:
			Bd1394CloseDebuggerDevice();
			break;

		case KDP_TYPE_USB:
			BdUsbCloseDebuggerDevice();
			break;
	}
}

//
// Setup debugger loader data entry.
//
STATIC EFI_STATUS BdpPopulateDataTableEntry(LDR_DATA_TABLE_ENTRY* loaderDataEntry)
{
	//
	// Get image base.
	//
	UINT64 imageBase64														= 0;
	memset(loaderDataEntry, 0, sizeof(LDR_DATA_TABLE_ENTRY));
	BlGetApplicationBaseAndSize(&imageBase64, NULL);

	//
	// Get nt header.
	//
	VOID* imageBase															= ArchConvertAddressToPointer(imageBase64, VOID*);
	EFI_IMAGE_NT_HEADERS* ntHeaders											= PeImageNtHeader(imageBase);

	if (!ntHeaders)
		return EFI_INVALID_PARAMETER;

	//
	// Setup.
	//
	loaderDataEntry->Flags													= 0;
	loaderDataEntry->LoadCount												= 1;
	loaderDataEntry->DllBase												= imageBase;
	loaderDataEntry->SizeOfImage											= PeImageGetSize(ntHeaders);
	loaderDataEntry->EntryPoint												= PeImageGetEntryPoint(imageBase);
	loaderDataEntry->SectionAndCheckSum.CheckSum							= PeImageGetChecksum(ntHeaders);
	loaderDataEntry->BaseDllName.Buffer										= CHAR16_STRING(L"boot.efi");
	loaderDataEntry->BaseDllName.Length										= 16;
	loaderDataEntry->BaseDllName.MaximumLength								= loaderDataEntry->BaseDllName.Length;
	loaderDataEntry->FullDllName.Buffer										= loaderDataEntry->BaseDllName.Buffer;
	loaderDataEntry->FullDllName.Length										= loaderDataEntry->BaseDllName.Length;
	loaderDataEntry->FullDllName.MaximumLength								= loaderDataEntry->FullDllName.Length;

	return EFI_SUCCESS;
}

//
// Start debugger.
//
STATIC EFI_STATUS BdpStart(CHAR8 CONST* loaderOptions)
{
	//
	// Debugger has not been initialised.
	//
	if (!BdSubsystemInitialized)
		return EFI_SUCCESS;

	//
	// Already connected to host machine.
	//
	if (BdConnectionActive)
		return EFI_SUCCESS;

	//
	// Set as connected
	//
	BdConnectionActive														= TRUE;

	//
	// Try to connect to host machine.
	//
	for (UINT32 i = 0; i < 10; i ++)
	{
		BdDebuggerNotPresent												= FALSE;
		DbgPrint(CHAR8_CONST_STRING("<?dml?><col fg=\"changed\">BD: Boot Debugger Initialized</col>\n"));
		if (!BdDebuggerNotPresent)
			break;
	}

	//
	// Let debugger load symbols.
	//
	DbgLoadImageSymbols(&BdModuleDataTableEntry.BaseDllName, BdModuleDataTableEntry.DllBase, (UINTN)(-1));

	//
	// Boot break.
	//
	if (loaderOptions && strstr(loaderOptions, CHAR8_CONST_STRING("/break")))
		DbgBreakPoint();
	else
		BdPollConnection();

	return EFI_SUCCESS;
}

//
// Stop debugger.
//
STATIC VOID BdpStop()
{
	//
	// Not connected.
	//
	if (!BdConnectionActive)
		return;

	//
	// Special unload symbols packet to tell debugger we are stopping.
	//
	DbgUnLoadImageSymbols2((STRING*)(NULL), ArchConvertAddressToPointer(-1, VOID*), 0);
	BdConnectionActive														= FALSE;
}

//
// Free loader data entry.
//
STATIC VOID BdpFreeDataTableEntry(LDR_DATA_TABLE_ENTRY* ldrDataTableEntry)
{
	ldrDataTableEntry->FullDllName.Buffer									= NULL;
	ldrDataTableEntry->FullDllName.Length									= 0;
	ldrDataTableEntry->BaseDllName.Buffer									= NULL;
	ldrDataTableEntry->BaseDllName.Length									= 0;
}

//
// Copy memory.
//
VOID BdCopyMemory(VOID* dstBuffer, VOID* srcBuffer, UINT32 bytesCount)
{
	UINT8* dstBuffer2														= (UINT8*)(dstBuffer);
	UINT8* srcBuffer2														= (UINT8*)(srcBuffer);

	for (UINT32 i = 0; i < bytesCount; i ++)
		dstBuffer2[i]														= srcBuffer2[i];
}

//
// Move memory.
//
UINT32 BdMoveMemory(VOID* dstBuffer, VOID* srcBuffer, UINT32 bytesCount)
{
	UINT32 moveLength														= bytesCount > EFI_PAGE_SIZE ? EFI_PAGE_SIZE : bytesCount;
	bytesCount																= moveLength;
	UINT8* dstBuffer2														= (UINT8*)(dstBuffer);
	UINT8* srcBuffer2														= (UINT8*)(srcBuffer);

	while (Add2Ptr(srcBuffer2, 0, UINT32) & 3)
	{
		if (!moveLength)
			break;

		if (!BdpWriteCheck(dstBuffer2) || !BdpWriteCheck(srcBuffer2))
			break;

		*dstBuffer2															= *srcBuffer2;
		dstBuffer2															+= sizeof(UINT8);
		srcBuffer2															+= sizeof(UINT8);
		moveLength															-= sizeof(UINT8);
	}

	while (moveLength > 3)
	{
		if (!BdpWriteCheck(dstBuffer2) || !BdpWriteCheck(srcBuffer2))
			break;

		*(UINT32*)(dstBuffer2)								= *(UINT32*)(srcBuffer2);
		dstBuffer2															+= sizeof(UINT32);
		srcBuffer2															+= sizeof(UINT32);
		moveLength															-= sizeof(UINT32);
	}

	while (moveLength)
	{
		if (!BdpWriteCheck(dstBuffer2) || !BdpWriteCheck(srcBuffer2))
			break;

		*dstBuffer2															= *srcBuffer2;
		dstBuffer2															+= sizeof(UINT8);
		srcBuffer2															+= sizeof(UINT8);
		moveLength															-= sizeof(UINT8);
	}

	//
	// Sweep instruction change.
	//
	bytesCount																-= moveLength;
	ArchSweepIcacheRange(dstBuffer, bytesCount);

	return bytesCount;
}

//
// Compute checksum.
//
UINT32 BdComputeChecksum(VOID* dataBuffer, UINT32 bufferLength)
{
	UINT32 checksum															= 0;
	UINT8* buffer															= (UINT8*)(dataBuffer);

	while (bufferLength > 0)
	{
		checksum															= checksum + (UINT32)(*buffer++);
		bufferLength														-= 1;
	}

	return checksum;
}

//
// Remove breakpoint in range.
//
BOOLEAN BdDeleteBreakpointRange(UINT64 lowerAddress, UINT64 upperAddress)
{
	BOOLEAN returnStatus													= FALSE;

	for (UINT32 index = 0; index < ARRAYSIZE(BdBreakpointTable); index ++)
	{
		if ((BdBreakpointTable[index].Flags & KD_BREAKPOINT_IN_USE) && BdBreakpointTable[index].Address >= lowerAddress && BdBreakpointTable[index].Address <= upperAddress)
			returnStatus													= returnStatus || BdpDeleteBreakpoint(index + 1);
	}

	return returnStatus;
}

//
// Report exception state change.
//
VOID BdReportExceptionStateChange(EXCEPTION_RECORD* exceptionRecord, CONTEXT* contextRecord)
{
	UINT32 status															= KD_CONTINUE_SUCCESS;

	do
	{
		DBGKD_WAIT_STATE_CHANGE64 waitStateChange;
		BdSetCommonState(DbgKdExceptionStateChange, contextRecord, &waitStateChange);
		BdpExceptionRecord32To64(exceptionRecord, &waitStateChange.u.Exception.ExceptionRecord);
		BdpSetStateChange(&waitStateChange, exceptionRecord, contextRecord);

		STRING messageHeader;
		messageHeader.Length												= sizeof(DBGKD_WAIT_STATE_CHANGE64);
		messageHeader.Buffer												= (CHAR8*)((VOID*)(&waitStateChange));
		STRING messageData													= {0};
		status																= BdpSendWaitContinue(PACKET_TYPE_KD_STATE_CHANGE64, &messageHeader, &messageData, contextRecord);
	} while (status == KD_CONTINUE_PROCESSOR_RESELECTED);
}

//
// Report load symbols state change.
//
VOID BdReportLoadSymbolsStateChange(STRING* moduleName, KD_SYMBOLS_INFO* SymbolsInfo, BOOLEAN unloadModule, CONTEXT* contextRecord)
{
	UINT32 status;

	do
	{
		DBGKD_WAIT_STATE_CHANGE64 waitStateChange;
		BdSetCommonState(DbgKdLoadSymbolsStateChange, contextRecord, &waitStateChange);
		BdSetContextState(&waitStateChange, contextRecord);

		STRING messageHeader;
		messageHeader.Length												= sizeof(DBGKD_WAIT_STATE_CHANGE64);
		messageHeader.Buffer												= (CHAR8*)((VOID*)(&waitStateChange));
		STRING messageData													= {0};

		if (moduleName)
		{
			UINT32 length													= BdMoveMemory(BdMessageBuffer, moduleName->Buffer, moduleName->Length);
			BdMessageBuffer[length]											= 0;
			messageData.Buffer												= BdMessageBuffer;
			messageData.Length												= (UINT16)(length + 1);
		}

		waitStateChange.u.LoadSymbols.PathNameLength						= messageData.Length;
		waitStateChange.u.LoadSymbols.UnloadSymbols							= unloadModule;
		waitStateChange.u.LoadSymbols.BaseOfDll								= (UINT64)((INTN)(SymbolsInfo->BaseOfDll));
		waitStateChange.u.LoadSymbols.CheckSum								= SymbolsInfo->CheckSum;
		waitStateChange.u.LoadSymbols.ProcessId								= SymbolsInfo->ProcessId;
		waitStateChange.u.LoadSymbols.SizeOfImage							= SymbolsInfo->SizeOfImage;
		status																= BdpSendWaitContinue(PACKET_TYPE_KD_STATE_CHANGE64, &messageHeader, &messageData, contextRecord);
	} while (status == KD_CONTINUE_PROCESSOR_RESELECTED);
}

//
// Print string.
//
BOOLEAN BdPrintString(STRING* printString)
{
	//
	// Move the output string to the message buffer.
	//
	UINT32 length															= BdMoveMemory(BdMessageBuffer, printString->Buffer, printString->Length);

	if (sizeof(DBGKD_DEBUG_IO) + length > PACKET_MAX_SIZE)
		length																= PACKET_MAX_SIZE - sizeof(DBGKD_DEBUG_IO);

	//
	// Construct the print string message and message descriptor.
	//
	STRING messageHeader;
	STRING messageData;
	DBGKD_DEBUG_IO debugIo;
	debugIo.ApiNumber														= DbgKdPrintStringApi;
	debugIo.ProcessorLevel													= 0;
	debugIo.Processor														= 0;
	debugIo.u.PrintString.LengthOfString									= length;
	messageHeader.Length													= sizeof(DBGKD_DEBUG_IO);
	messageHeader.Buffer													= (CHAR8*)((VOID*)(&debugIo));
	messageData.Length														= (UINT16)(length);
	messageData.Buffer														= BdMessageBuffer;
	BdSendPacket(PACKET_TYPE_KD_DEBUG_IO, &messageHeader, &messageData);

	return BdPollBreakIn();
}

//
// Prompt string.
//
BOOLEAN BdPromptString(STRING* inputString, STRING* outputString)
{
	//
	// Move the output string to the message buffer.
	//
	UINT32 length															= BdMoveMemory(BdMessageBuffer, inputString->Buffer, inputString->Length);

	if (sizeof(DBGKD_DEBUG_IO) + length > PACKET_MAX_SIZE)
		length																= PACKET_MAX_SIZE - sizeof(DBGKD_DEBUG_IO);

	//
	// Construct the prompt string message and message descriptor.
	//
	DBGKD_DEBUG_IO debugIo;
	STRING messageHeader;
	STRING messageData;
	debugIo.ApiNumber														= DbgKdGetStringApi;
	debugIo.ProcessorLevel													= 0;
	debugIo.Processor														= 0;
	debugIo.u.GetString.LengthOfPromptString								= length;
	debugIo.u.GetString.LengthOfStringRead									= outputString->MaximumLength;
	messageHeader.Length													= sizeof(DBGKD_DEBUG_IO);
	messageHeader.Buffer													= (CHAR8*)((VOID*)(&debugIo));
	messageData.Length														= (UINT16)(length);
	messageData.Buffer														= BdMessageBuffer;
	BdSendPacket(PACKET_TYPE_KD_DEBUG_IO, &messageHeader, &messageData);

	messageHeader.MaximumLength												= sizeof(DBGKD_DEBUG_IO);
	messageData.MaximumLength												= sizeof(BdMessageBuffer);
	UINT32 replyCode														= KDP_PACKET_TIMEOUT;

	do
	{
		//
		// BdTrap will recall us again if we return TRUE
		//
		replyCode															= BdReceivePacket(PACKET_TYPE_KD_DEBUG_IO, &messageHeader, &messageData, &length);

		if (replyCode == KDP_PACKET_RESEND)
			return TRUE;

	} while (replyCode != KDP_PACKET_RECEIVED);

	//
	// Copy to output buffer.
	//
	if (length > outputString->MaximumLength)
		length																= outputString->MaximumLength;

	outputString->Length													= (UINT16)(BdMoveMemory(outputString->Buffer, BdMessageBuffer, length));

	return FALSE;
}

//
// Initialise boot debugger.
//
EFI_STATUS BdInitialize(CHAR8 CONST* loaderOptions)
{
	EFI_STATUS status														= EFI_SUCCESS;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// Already initialised.
		//
        if (BdSubsystemInitialized || !loaderOptions || !loaderOptions[0]) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if (EFI_ERROR(status))
            {
                BdpCloseDebuggerDevice();
                BdpFreeDataTableEntry(&BdModuleDataTableEntry);
            }
            
            return status;
#endif
        }

		//
		// Case on debug type.
		//
		if (strstr(loaderOptions, CHAR8_CONST_STRING("/debug=1394")))
		{
			BdDebuggerType													= KDP_TYPE_1394;
			BdSendPacket													= &Bd1394SendPacket;
			BdReceivePacket													= &Bd1394ReceivePacket;
			status															= Bd1394ConfigureDebuggerDevice(loaderOptions);
		}
		else if (strstr(loaderOptions, CHAR8_CONST_STRING("/debug=usb")))
		{
			BdDebuggerType													= KDP_TYPE_USB;
			BdSendPacket													= &BdUsbSendPacket;
			BdReceivePacket													= &BdUsbReceivePacket;
			status															= BdUsbConfigureDebuggerDevice(loaderOptions);
		}
		else
		{
#if defined(_MSC_VER)
			try_leave(status = EFI_SUCCESS);
#else
            status = EFI_SUCCESS;

            if (EFI_ERROR(status))
            {
                BdpCloseDebuggerDevice();
                BdpFreeDataTableEntry(&BdModuleDataTableEntry);
            }

            return status;
#endif
		}

		//
		// Setup debug device failed.
		//
        if (EFI_ERROR(status)) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            BdpCloseDebuggerDevice();
            BdpFreeDataTableEntry(&BdModuleDataTableEntry);
            
            return status;
#endif
        }

		//
		// Initialise breakpoint table.
		//
		memset(BdBreakpointTable, 0, sizeof(BdBreakpointTable));

		//
		// Setup loader table entry.
		//
		BdBreakpointInstruction												= KDP_BREAKPOINT_VALUE;
		BdDebugTrap															= (BdDebugRoutine)&BdTrap;
        if (EFI_ERROR(status = BdpPopulateDataTableEntry(&BdModuleDataTableEntry))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if (EFI_ERROR(status))
            {
                BdpCloseDebuggerDevice();
                BdpFreeDataTableEntry(&BdModuleDataTableEntry);
            }
            
            return status;
#endif
        }

		//
		// Link debugger module to modules list.
		//
		BdModuleList.Flink													= &BdModuleDataTableEntry.InLoadOrderLinks;
		BdModuleList.Blink													= &BdModuleDataTableEntry.InLoadOrderLinks;
		BdModuleDataTableEntry.InLoadOrderLinks.Flink						= &BdModuleList;
		BdModuleDataTableEntry.InLoadOrderLinks.Blink						= &BdModuleList;

		//
		// Initialise arch.
		//
        if (EFI_ERROR(status = BdArchInitialize())) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            BdpCloseDebuggerDevice();
            BdpFreeDataTableEntry(&BdModuleDataTableEntry);
            
            return status;
#endif
        }

		//
		// Start debugger.
		//
		BdSubsystemInitialized												= TRUE;
		status																= BdpStart(loaderOptions);
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if (EFI_ERROR(status))
		{
			BdpCloseDebuggerDevice();
			BdpFreeDataTableEntry(&BdModuleDataTableEntry);
		}
#if defined(_MSC_VER)
	}
#endif

	return status;
}

//
// Destroy debugger.
//
EFI_STATUS BdFinalize()
{
	//
	// Stop connection.
	//
	if (BdConnectionActive)
		BdpStop();

	//
	// Debugger system has been initialised.
	//
	if (BdSubsystemInitialized)
	{
		BdpSuspendAllBreakpoints();
		BdpCloseDebuggerDevice();
		BdArchDestroy();
		BdSubsystemInitialized												= FALSE;
	}

	//
	// Free ldr.
	//
	BdpFreeDataTableEntry(&BdModuleDataTableEntry);

	return EFI_SUCCESS;
}

//
// Debugger is enabled.
//
BOOLEAN BdDebuggerEnabled()
{
	//
	// Debugger is not initialised or debugger operation is blocked.
	//
	if (!BdSubsystemInitialized || BdArchBlockDebuggerOperation)
		return FALSE;

	return TRUE;
}

//
// Poll break in.
//
BOOLEAN BdPollBreakIn()
{
	//
	// If the debugger is enabled, see if a breakin by the kernel debugger is pending.
	//
	BOOLEAN breakIn															= FALSE;

	if (BdDebuggerEnabled())
	{
		if (BdControlCPending)
		{
			breakIn															= TRUE;
			BdControlCPending												= FALSE;
		}
		else
		{
			breakIn															= BdReceivePacket(PACKET_TYPE_KD_POLL_BREAKIN, NULL, NULL, NULL) == KDP_PACKET_RECEIVED;
		}

		if (breakIn)
			BdControlCPressed												= TRUE;
	}
	return breakIn;
}

//
// Poll connection.
//
VOID BdPollConnection()
{
	if (!BdPollBreakIn())
		return;

	DbgPrint(CHAR8_CONST_STRING("User requested boot debugger break!\r\n"));
	DbgBreakPoint();
}

//
// Load symbols.
//
VOID DbgLoadImageSymbols(UNICODE_STRING* fileName, VOID* imageBase, UINTN processId)
{
	STRING ansiName;
	ansiName.Length															= fileName->Length / sizeof(CHAR16) * sizeof(CHAR8);
	ansiName.MaximumLength													= sizeof(BdDebugMessage);
	ansiName.Buffer															= BdDebugMessage;

	if (!EFI_ERROR(BlUnicodeToAnsi(fileName->Buffer, fileName->Length / sizeof(CHAR16), ansiName.Buffer, ansiName.MaximumLength)))
		DbgLoadImageSymbols2(&ansiName, imageBase, processId);
}

//
// Unload symbols.
//
VOID DbgUnLoadImageSymbols(UNICODE_STRING* fileName, VOID* imageBase, UINTN processId)
{
	STRING ansiName;
	ansiName.Length															= fileName->Length / sizeof(CHAR16) * sizeof(CHAR8);
	ansiName.MaximumLength													= sizeof(BdDebugMessage);
	ansiName.Buffer															= BdDebugMessage;

	if (!EFI_ERROR(BlUnicodeToAnsi(fileName->Buffer, fileName->Length / sizeof(CHAR16), ansiName.Buffer, ansiName.MaximumLength)))
		DbgUnLoadImageSymbols2(&ansiName, imageBase, processId);
}

//
// Load symbols.
//
VOID DbgLoadImageSymbols2(STRING* fileName, VOID* imageBase, UINTN processId)
{
	KD_SYMBOLS_INFO symbolsInfo;
	EFI_IMAGE_NT_HEADERS* imageNtHeader										= PeImageNtHeader(imageBase);
	symbolsInfo.BaseOfDll													= imageBase;
	symbolsInfo.ProcessId													= processId;
	symbolsInfo.CheckSum													= imageNtHeader ? PeImageGetChecksum(imageNtHeader) : 0;
	symbolsInfo.SizeOfImage													= imageNtHeader ? PeImageGetSize(imageNtHeader) : 0x100000;

	DbgService2(fileName, &symbolsInfo, BREAKPOINT_LOAD_SYMBOLS);
}

//
// Unload symbols.
//
VOID DbgUnLoadImageSymbols2(STRING* fileName, VOID* imageBase, UINTN processId)
{
	KD_SYMBOLS_INFO symbolsInfo;
	symbolsInfo.BaseOfDll													= imageBase;
	symbolsInfo.ProcessId													= processId;
	symbolsInfo.CheckSum													= 0;
	symbolsInfo.SizeOfImage													= 0;

	DbgService2(fileName, &symbolsInfo, BREAKPOINT_UNLOAD_SYMBOLS);
}

//
// Debug print.
//
UINT32 DbgPrint(CHAR8 CONST* printFormat, ...)
{
	VA_LIST list;
	VA_START(list, printFormat);
	vsnprintf(BdDebugMessage, ARRAYSIZE(BdDebugMessage) - 1, printFormat, list);
	VA_END(list);

	STRING outputString;
	outputString.Length														= (UINT16)(strlen(BdDebugMessage));
	outputString.MaximumLength												= sizeof(BdDebugMessage);
	outputString.Buffer														= BdDebugMessage;
	DbgService(BREAKPOINT_PRINT, ArchConvertPointerToAddress(outputString.Buffer), outputString.Length, 0, 0);

	return outputString.Length;
}
