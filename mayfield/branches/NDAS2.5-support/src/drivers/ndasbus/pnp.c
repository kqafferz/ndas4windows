DI

Arguments:
    
    None


Return Value:

    NTSTATUS -- Indicates whether registration succeeded

--*/

{
    NTSTATUS                    status;
    TDI_CLIENT_INTERFACE_INFO   info;
    UNICODE_STRING              clientName;
    
    PAGED_CODE ();

 
    //
    // Setup the TDI request structure
    //

    RtlZeroMemory (&info, sizeof (info));
    RtlInitUnicodeString(&clientName, L"ndasbus");
#ifdef TDI_CURRENT_VERSION
    info.TdiVersion = TDI_CURRENT_VERSION;
#else
    info.MajorTdiVersion = 2;
    info.MinorTdiVersion = 0;
#endif
    info.Unused = 0;
    info.ClientName = &clientName;
    info.BindingHandler = Reg_NetworkPnPBindingChange;
    info.AddAddressHandlerV2 = Reg_AddAddressHandler;
    info.DelAddressHandlerV2 = Reg_DelAddressHandler;
    info.PnPPowerHandler = NULL;

    //
    // Register handlers with TDI
    //

    status = TdiRegisterPnPHandlers (&info, sizeof(info), &FdoData->TdiClient);
    if (!NT_SUCCESS (status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, (
					"Failed to register PnP handlers: %lx .\n",
					status));
        return status;
    }

    return STATUS_SUCCESS;
}


VOID
Reg_DeregisterTdiPnPHandlers (
	PFDO_DEVICE_DATA	FdoData
){

    if (FdoData->TdiClient) {
        TdiDeregisterPnPHandlers (FdoData->TdiClient);
        FdoData->TdiClient = NULL;
	}
}

VOID
Reg_AddAddressHandler ( 
	IN PTA_ADDRESS NetworkAddress,
	IN PUNICODE_STRING  DeviceName,
	IN PTDI_PNP_CONTEXT Context
    )
/*++

Routine Description:

    TDI add address handler

Arguments:
    
    NetworkAddress  - new network address available on the system

    DeviceName      - name of the device to which address belongs

    Context         - PDO to which address belongs


Return Value:

    None

--*/
{
	UNICODE_STRING	lpxPrefix;

    PAGED_CODE ();

	UNREFERENCED_PARAMETER(Context);

	if (DeviceName==NULL) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, (
			"NO DEVICE NAME SUPPLIED when deleting address of type %d.\n",
			NetworkAddress->AddressType));
		return;
	}
	Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DeviceName=%ws AddrType=%u AddrLen=%u\n",
										DeviceName->Buffer,
										(ULONG)NetworkAddress->AddressType,
										(ULONG)NetworkAddress->AddressLength));

	//
	//	LPX
	//
	RtlInitUnicodeString(&lpxPrefix, LPX_BOUND_DEVICE_NAME_PREFIX);

	if(	RtlPrefixUnicodeString(&lpxPrefix, DeviceName, TRUE) &&
		NetworkAddress->AddressType == TDI_ADDRESS_TYPE_LPX
		){
			PTDI_ADDRESS_LPX	lpxAddr;

			lpxAddr = (PTDI_ADDRESS_LPX)NetworkAddress->Address;
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("LPX: %02x:%02x:%02x:%02x:%02x:%02x\n",
									lpxAddr->Node[0],
									lpxAddr->Node[1],
									lpxAddr->Node[2],
									lpxAddr->Node[3],
									lpxAddr->Node[4],
									lpxAddr->Node[5]));
			//
			//	LPX may leave dummy values.
			//
			RtlZeroMemory(lpxAddr->Reserved, sizeof(lpxAddr->Reserved));

			//
			//	Check to see if FdoData for TdiPnP is created.
			//

			ExAcquireFastMutex(&Globals.Mutex);
			if(Globals.PersistentPdo && Globals.FdoDataTdiPnP) {
				LSBUS_QueueWorker_PlugInByRegistry(Globals.FdoDataTdiPnP, NetworkAddress, 0);
			}
#if DBG
			else {
				Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("LPX: no FdoData for TdiPnP or enum disabled\n"));
			}
#endif
			ExReleaseFastMutex(&Globals.Mutex);

	//
	//	IP	address
	//
	} else if(NetworkAddress->AddressType == TDI_ADDRESS_TYPE_IP) {
		PTDI_ADDRESS_IP	ipAddr;
		PUCHAR			digit;

		ipAddr = (PTDI_ADDRESS_IP)NetworkAddress->Address;
		digit = (PUCHAR)&ipAddr->in_addr;
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("IP: %u.%u.%u.%u\n",digit[0],digit[1],digit[2],digit[3]));
	} else {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("AddressType %u discarded.\n", (ULONG)NetworkAddress->AddressType));
	}
}

VOID
Reg_DelAddressHandler ( 
	IN PTA_ADDRESS NetworkAddress,
	IN PUNICODE_STRING DeviceName,
	IN PTDI_PNP_CONTEXT Context
    )
/*++

Routine Description:

    TDI delete address handler

Arguments:
    
    NetworkAddress  - network address that is no longer available on the system

    Context1        - name of the device to which address belongs

    Context2        - PDO to which address belongs


Return Value:

    None

--*/
{
	UNICODE_STRING	lpxPrefix;

	PAGED_CODE ();

	UNREFERENCED_PARAMETER(Context);

	if (DeviceName==NULL) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, (
			"AfdDelAddressHandler: "
			"NO DEVICE NAME SUPPLIED when deleting address of type %d.\n",
			NetworkAddress->AddressType));
		return;
	}
	Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DeviceName=%ws AddrType=%u AddrLen=%u\n",
		DeviceName->Buffer,
		(ULONG)NetworkAddress->AddressType,
		(ULONG)NetworkAddress->AddressLength));

	//
	//	LPX
	//
	RtlInitUnicodeString(&lpxPrefix, LPX_BOUND_DEVICE_NAME_PREFIX);

	if(	RtlPrefixUnicodeString(&lpxPrefix, DeviceName, TRUE)){
		PTDI_ADDRESS_LPX	lpxAddr;

		lpxAddr = (PTDI_ADDRESS_LPX)NetworkAddress->Address;
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("LPX: %02x:%02x:%02x:%02x:%02x:%02x\n",
			lpxAddr->Node[0],
			lpxAddr->Node[1],
			lpxAddr->Node[2],
			lpxAddr->Node[3],
			lpxAddr->Node[4],
			lpxAddr->Node[5]));

		//
		//	IP	address
		//
	} else if(NetworkAddress->AddressType == TDI_ADDRESS_TYPE_IP) {
		PTDI_ADDRESS_IP	ipAddr;
		PUCHAR			digit;

		ipAddr = (PTDI_ADDRESS_IP)NetworkAddress->Address;
		digit = (PUCHAR)&ipAddr->in_addr;
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("IP: %u.%u.%u.%u\n",digit[0],digit[1],digit[2],digit[3]));
	} else {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("AddressType %u discarded.\n", (ULONG)NetworkAddress->AddressType));
	}
}


VOID
Reg_NetworkPnPBindingChange(
	IN TDI_PNP_OPCODE  PnPOpcode,
	IN PUNICODE_STRING  DeviceName,
	IN PWSTR  MultiSZBindList
){
	UNREFERENCED_PARAMETER(DeviceName);
	UNREFERENCED_PARAMETER(MultiSZBindList);

#if DBG
	if(DeviceName && DeviceName->Buffer) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DeviceName=%ws PnpOpcode=%x\n", DeviceName->Buffer, PnPOpcode));
	} else {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DeviceName=NULL PnpOpcode=%x\n", PnPOpcode));
	}
#endif

	switch(PnPOpcode) {
	case TDI_PNP_OP_ADD:
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("TDI_PNP_OP_ADD\n"));
	break;
	case TDI_PNP_OP_DEL:
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("TDI_PNP_OP_DEL\n"));
	break;
	case TDI_PNP_OP_PROVIDERREADY:
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("TDI_PNP_OP_PROVIDERREADY\n"));
	break;
	case TDI_PNP_OP_NETREADY:
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("TDI_PNP_OP_NETREADY\n"));

		//
		// Some NIC does not accept sending packets at early booting time.
		// So
		//

		ExAcquireFastMutex(&Globals.Mutex);
		if(Globals.PersistentPdo && Globals.FdoDataTdiPnP) {
			LSBUS_QueueWorker_PlugInByRegistry(Globals.FdoDataTdiPnP, NULL, 0);
		}
#if DBG
		else {
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("NETREADY: no FdoData for TdiPnP or enum disabled\n"));
		}
#endif
		ExReleaseFastMutex(&Globals.Mutex);
	break;
	default:
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("Unknown PnP code. %x\n", PnPOpcode));
	}
}


//////////////////////////////////////////////////////////////////////////
//
//	Exported functions to IOCTL.
//

//
//	Register a device by writing registry.
//

NTSTATUS
LSBus_RegisterDevice(
		PFDO_DEVICE_DATA				FdoData,
		PBUSENUM_PLUGIN_HARDWARE_EX2	Plugin
){
	NTSTATUS			status;
	HANDLE				busDevReg;
	HANDLE				ndasDevRoot;
	HANDLE				ndasDevInst;

	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	busDevReg = NULL;
	ndasDevRoot = NULL;
	ndasDevInst = NULL;


	//
	//	Open a BUS device registry, an NDAS device root, and device instance key.
	//
	KeEnterCriticalRegion();
	ExAcquireFastMutexUnsafe(&FdoData->RegMutex);

	status = Reg_OpenDeviceControlRoot(&busDevReg, KEY_READ|KEY_WRITE);
	if(!NT_SUCCESS(status)) {
		ExReleaseFastMutexUnsafe(&FdoData->RegMutex);
		KeLeaveCriticalRegion();

		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("OpenServiceRegistry() failed.\n"));
		return status;
	}

	status = Reg_OpenNdasDeviceRoot(&ndasDevRoot, KEY_READ|KEY_WRITE, busDevReg);
	if(!NT_SUCCESS(status)) {

		ZwClose(busDevReg);

		ExReleaseFastMutexUnsafe(&FdoData->RegMutex);
		KeLeaveCriticalRegion();

		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("OpenNdasDeviceRegistry() failed.\n"));
		return status;
	}

	status = Reg_OpenDeviceInst(&ndasDevInst, Plugin->SlotNo, TRUE, ndasDevRoot);
	if(!NT_SUCCESS(status)) {

		ZwClose(busDevReg);
		ZwClose(ndasDevRoot);

		ExReleaseFastMutexUnsafe(&FdoData->RegMutex);
		KeLeaveCriticalRegion();

		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("Reg_OpenDeviceInst() failed.\n"));
		return	status;
	}

	//
	//	Before writing information, clean up the device instance key.
	//

	DrDeleteAllSubKeys(ndasDevInst);

	//
	//	Write plug in information
	//

	status = WriteNDASDevToRegistry(ndasDevInst, Plugin);


	//
	//	Close handles
	//

	if(ndasDevInst)
		ZwClose(ndasDevInst);
	if(ndasDevRoot)
		ZwClose(ndasDevRoot);

	if(busDevReg)
		ZwClose(busDevReg);

	ExReleaseFastMutexUnsafe(&FdoData->RegMutex);
	KeLeaveCriticalRegion();

	return status;
}


NTSTATUS
LSBus_RegisterTarget(
	PFDO_DEVICE_DATA			FdoData,
	PLANSCSI_ADD_TARGET_DATA	AddTargetData
){
	NTSTATUS	status;
	HANDLE		busDevReg;
	HANDLE		ndasDevRoot;
	HANDLE		ndasDevInst;
	HANDLE		targetKey;

	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	busDevReg = NULL;
	ndasDevRoot = NULL;
	ndasDevInst = NULL;
	targetKey = NULL;

	//
	//	Open a BUS device registry, an NDAS device root, and device instance key.
	//
	KeEnterCriticalRegion();
	ExAcquireFastMutexUnsafe(&FdoData->RegMutex);

	status = Reg_OpenDeviceControlRoot(&busDevReg, KEY_READ|KEY_WRITE);
	if(!NT_SUCCESS(status)) {

		ExReleaseFastMutexUnsafe(&FdoData->RegMutex);
		KeLeaveCriticalRegion();

		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("Reg_OpenDeviceControlRoot() failed.\n"));
		return status;
	}

	status = Reg_OpenNdasDeviceRoot(&ndasDevRoot, KEY_READ|KEY_WRITE, busDevReg);
	if(!NT_SUCCESS(status)) {
		ZwClose(busDevReg);

		ExReleaseFastMutexUnsafe(&FdoData->RegMutex);
		KeLeaveCriticalRegion();

		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("Reg_OpenNdasDeviceRoot() failed.\n"));
		return status;
	}

	status = Reg_OpenDeviceInst(&ndasDevInst, AddTargetData->ulSlotNo, FALSE, ndasDevRoot);
	if(!NT_SUCCESS(status)) {
		ZwClose(busDevReg);
		ZwClose(ndasDevRoot);

		ExReleaseFastMutexUnsafe(&FdoData->RegMutex);
		KeLeaveCriticalRegion();

		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("Reg_OpenDeviceInst() failed.\n"));
		return	status;
	}

	status = Reg_OpenTarget(&targetKey, AddTargetData->ucTargetId, TRUE, ndasDevInst);
	if(!NT_SUCCESS(status)) {
		ZwClose(busDevReg);
		ZwClose(ndasDevRoot);
		ZwClose(ndasDevInst);

		ExReleaseFastMutexUnsafe(&FdoData->RegMutex);
		KeLeaveCriticalRegion();

		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("Reg_OpenTarget() failed.\n"));

		return	status;
	}

	//
	//	Before writing information, clean up the target key.
	//

	DrDeleteAllSubKeys(targetKey);

	//
	//	Write target information
	//

	status = WriteTargetToRegistry(targetKey, AddTargetData);


	//
	//	Close handles
	//
	if(targetKey)
		ZwClose(targetKey);
	if(ndasDevInst)
		ZwClose(ndasDevInst);
	if(ndasDevRoot)
		ZwClose(ndasDevRoot);
	if(busDevReg)
		ZwClose(busDevReg);

	ExReleaseFastMutexUnsafe(&FdoData->RegMutex);
	KeLeaveCriticalRegion();

	Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("Adding an NDAS device into registry is completed. NTSTATUS:%08lx\n", status));

	return status;
}



NTSTATUS
LSBus_UnregisterDevice(
		PFDO_DEVICE_DATA	FdoData,
		ULONG				SlotNo
) {
	NTSTATUS			status;
	HANDLE				busDevReg;
	HANDLE				ndasDevRoot;
	HANDLE				devInstTobeDeleted;

	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	status = STATUS_SUCCESS;
	busDevReg = NULL;
	ndasDevRoot = NULL;
	devInstTobeDeleted = NULL;


	//
	//	Open a BUS device registry, an NDAS device root, and device instance key.
	//
	KeEnterCriticalRegion();
	ExAcquireFastMutexUnsafe(&FdoData->RegMutex);

	status = Reg_OpenDeviceControlRoot(&busDevReg, KEY_READ|KEY_WRITE);
	if(!NT_SUCCESS(status)) {
		ExReleaseFastMutexUnsafe(&FdoData->RegMutex);
		KeLeaveCriticalRegion();

		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("Reg_OpenDeviceControlRoot() failed.\n"));
		return status;
	}
	status = Reg_OpenNdasDeviceRoot(&ndasDevRoot, KEY_READ|KEY_WRITE, busDevReg);
	if(!NT_SUCCESS(status)) {
		ZwClose(busDevReg);

		ExReleaseFastMutexUnsafe(&FdoData->RegMutex);
		KeLeaveCriticalRegion();

		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("Reg_OpenNdasDeviceRoot() failed.\n"));
		return status;
	}

	if(SlotNo != NDASBUS_SLOT_ALL) {
		status = Reg_OpenDeviceInst(&devInstTobeDeleted, SlotNo, FALSE, ndasDevRoot);
		if(NT_SUCCESS(status)) {

			//
			//	Delete a NDAS device instance.
			//
			status = DrDeleteAllSubKeys(devInstTobeDeleted);
			if(NT_SUCCESS(status)) {
				status = ZwDeleteKey(devInstTobeDeleted);
			}
#if DBG
			else {
				Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrDeleteAllSubkeys() failed. SlotNo:%u NTSTATUS:%08lx\n", SlotNo, status));
			}
#endif

			ZwClose(devInstTobeDeleted);

#if DBG
			if(NT_SUCCESS(status)) {
				Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("A device(Slot %d) is deleted.\n", SlotNo));
			} else {
				Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("ZwDeleteKey() failed. SlotNo:%u NTSTATUS:%08lx\n", SlotNo, status));
			}
#endif
		}
	} else {
		status = DrDeleteAllSubKeys(ndasDevRoot);
	}


	//
	//	Close handles
	//

	if(ndasDevRoot)
		ZwClose(ndasDevRoot);
	if(busDevReg)
		ZwClose(busDevReg);

	ExReleaseFastMutexUnsafe(&FdoData->RegMutex);
	KeLeaveCriticalRegion();

	Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("Removing a DNAS device from registry is completed. NTSTATUS:%08lx\n", status));

	return status;
}


NTSTATUS
LSBus_UnregisterTarget(
		PFDO_DEVICE_DATA	FdoData,
		ULONG				SlotNo,
		ULONG				TargetId
) {
	NTSTATUS			status;
	HANDLE				busDevReg;
	HANDLE				ndasDevRoot;
	HANDLE				ndasDevInst;
	HANDLE				targetIdTobeDeleted;

	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	status = STATUS_SUCCESS;
	busDevReg = NULL;
	ndasDevRoot = NULL;
	ndasDevInst = NULL;
	targetIdTobeDeleted = NULL;

	KeEnterCriticalRegion();
	ExAcquireFastMutexUnsafe(&FdoData->RegMutex);

	//
	//	Open a BUS device registry, an NDAS device root, and device instance key.
	//

	status = Reg_OpenDeviceControlRoot(&busDevReg, KEY_READ|KEY_WRITE);
	if(!NT_SUCCESS(status)) {
		ExReleaseFastMutexUnsafe(&FdoData->RegMutex);
		KeLeaveCriticalRegion();

		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("Reg_OpenDeviceControlRoot() failed.\n"));
		return status;
	}
	status = Reg_OpenNdasDeviceRoot(&ndasDevRoot, KEY_READ|KEY_WRITE, busDevReg);
	if(!NT_SUCCESS(status)) {
		ZwClose(busDevReg);
		ExReleaseFastMutexUnsafe(&FdoData->RegMutex);
		KeLeaveCriticalRegion();

		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("Reg_OpenNdasDeviceRoot() failed.\n"));
		return status;
	}

	status = Reg_OpenDeviceInst(&ndasDevInst, SlotNo, FALSE, ndasDevRoot);
	if(!NT_SUCCESS(status)) {
		ZwClose(busDevReg);
		ZwClose(ndasDevInst);
		ExReleaseFastMutexUnsafe(&FdoData->RegMutex);
		KeLeaveCriticalRegion();

		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("Reg_OpenDeviceInst() failed.\n"));
		return status;
	}

	status = Reg_OpenTarget(&targetIdTobeDeleted, TargetId, FALSE, ndasDevInst);
	if(NT_SUCCESS(status)) {

		//
		//	Delete an NDAS device instance.
		//
		status = DrDeleteAllSubKeys(targetIdTobeDeleted);
		if(NT_SUCCESS(status)) {
			status = ZwDeleteKey(targetIdTobeDeleted);
		}
#if DBG
		else {
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrDeleteAllSubKeys() failed. SlotNo:%u Target %u NTSTATUS:%08lx\n", SlotNo, TargetId, status));
		}
#endif
		ZwClose(targetIdTobeDeleted);
#if DBG
		if(NT_SUCCESS(status)) {
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("A device(Slot %d Target %u) is deleted.\n", SlotNo, TargetId));
		} else {
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("ZwDeleteKey() failed. SlotNo:%u Target %u NTSTATUS:%08lx\n", SlotNo, TargetId, status));
		}
#endif
	}


	//
	//	Close handles
	//
	if(ndasDevInst)
		ZwClose(ndasDevInst);
	if(ndasDevRoot)
		ZwClose(ndasDevRoot);
	if(busDevReg)
		ntenance parts, such as a reference count,
// ACL, and so on. All outstanding connection-oriented and connectionless
// data transfer requests are queued here.
//

#if DBG
#define AREF_TEMP_CREATE        0
#define AREF_OPEN               1
#define AREF_VERIFY            2
#define AREF_LOOKUP             3
#define AREF_CONNECTION         4
#define AREF_REQUEST            5

#define NUMBER_OF_AREFS        6
#endif

typedef struct _TP_ADDRESS {

#if DBG
    ULONG RefTypes[NUMBER_OF_AREFS];
#endif

    USHORT Size;
    CSHORT Type;

    LIST_ENTRY Linkage;                 // next address/this device object.
    LONG ReferenceCount;                // number of references to this object.

    //
    // The following spin lock is acquired to edit this TP_ADDRESS structure
    // or to scan down or edit the list of address files.
    //

    KSPIN_LOCK SpinLock;                // lock to manipulate this structure.

    //
    // The following fields comprise the actual address itself.
    //

    PLPX_ADDRESS NetworkName;    // this address

    //
    // The following fields are used to maintain state about this address.
    //

    ULONG Flags;                        // attributes of the address.
    struct _DEVICE_CONTEXT *Provider;   // device context to which we are attached.

    //
    // The following field points to a list of TP_CONNECTION structures,
    // one per active, connecting, or disconnecting connections on this
    // address.  By definition, if a connection is on this list, then
    // it is visible to the client in terms of receiving events and being
    // able to post requests by naming the ConnectionId.  If the connection
    // is not on this list, then it is not valid, and it is guaranteed that
    // no indications to the client will be made with reference to it, and
    // no requests specifying its ConnectionId will be accepted by the transport.
    //

    LIST_ENTRY AddressFileDatabase; // list of defined address file objects

    //
    // This structure is used for checking share access.
    //

    SHARE_ACCESS ShareAccess;

    //
    // Used for delaying LpxDestroyAddress to a thread so
    // we can access the security descriptor.
    //

    PIO_WORKITEM  DestroyAddressQueueItem;


    //
    // This structure is used to hold ACLs on the address.

    PSECURITY_DESCRIPTOR SecurityDescriptor;

    LIST_ENTRY				ConnectionServicePointList;	 // added for lpx

} TP_ADDRESS, *PTP_ADDRESS;

#define ADDRESS_FLAGS_STOPPING          0x00000040 // TpStopAddress is in progress.


//
// This structure defines the DEVICE_OBJECT and its extension allocated at
// the time the transport provider creates its device object.
//

#if DBG
#define DCREF_CREATION    0
#define DCREF_ADDRESS     1
#define DCREF_CONNECTION  2
#define DCREF_TEMP_USE    3

#define NUMBER_OF_DCREFS 4
#endif


typedef struct _DEVICE_CONTEXT {

    DEVICE_OBJECT DeviceObject;         // the I/O system's device object.
#if 1 /* Added for lpx */
	USHORT				LastPortNum;

	//
	// Packet descriptor pool handle
	//

	NDIS_HANDLE         LpxPacketPool;

	//
	// Packet buffer descriptor pool handle
	//

	NDIS_HANDLE			LpxBufferPool;

	// Received packet.
	KSPIN_LOCK			PacketInProgressQSpinLock;
	LIST_ENTRY			PacketInProgressList;

	BOOL				bDeviceInit;
#endif

#if DBG
    ULONG RefTypes[NUMBER_OF_DCREFS];
#endif

    CSHORT Type;                          // type of this structure
    USHORT Size;                          // size of this structure

    LIST_ENTRY DeviceListLinkage;                   // links them on LpxDeviceList
                                        
    LONG ReferenceCount;                // activity count/this provider.
    LONG CreateRefRemoved;              // has unload or unbind been called ?


    //
    // Following are protected by Global Device Context SpinLock
    //

    KSPIN_LOCK SpinLock;                // lock to manipulate this object.
                                        //  (used in KeAcquireSpinLock calls)

    //
    // the device context state, among open, closing
    //

    UCHAR State;

    //
    // Used when processing a STATUS_CLOSING indication.
    //

    PIO_WORKITEM StatusClosingQueueItem;
    
    //
    // The following queue holds free TP_ADDRESS objects available for allocation.
    //

    LIST_ENTRY AddressPool;

    //
    // These counters keep track of resources uses by TP_ADDRESS objects.
    //

    ULONG AddressAllocated;
    ULONG AddressInitAllocated;
    ULONG AddressMaxAllocated;
    ULONG AddressInUse;
    ULONG AddressMaxInUse;
    ULONG AddressExhausted;
    ULONG AddressTotal;


    //
    // The following queue holds free TP_ADDRESS_FILE objects available for allocation.
    //

    LIST_ENTRY AddressFilePool;

    //
    // These counters keep track of resources uses by TP_ADDRESS_FILE objects.
    //

    ULONG AddressFileAllocated;
    ULONG AddressFileInitAllocated;
    ULONG AddressFileMaxAllocated;
    ULONG AddressFileInUse;
    ULONG AddressFileMaxInUse;
    ULONG AddressFileTotal;

    //
    // The following field is a head of a list of TP_ADDRESS objects that
    // are defined for this transport provider.  To edit the list, you must
    // hold the spinlock of the device context object.
    //

    LIST_ENTRY AddressDatabase;        // list of defined transport addresses.
 
    //
    // following is used to keep adapter information.
    //

    NDIS_HANDLE NdisBindingHandle;

    ULONG MaxConnections;
    ULONG MaxAddressFiles;
    ULONG MaxAddresses;
    PWCHAR DeviceName;
    ULONG DeviceNameLength;

    //
    // This is the Mac type we must build the packet header for and know the
    // offsets for.
    //

    ULONG MaxReceivePacketSize;         // does not include the MAC header
    ULONG MaxSendPacketSize;            // includes the MAC header

    ULONG MaxUserData;
    //
    // some MAC addresses we use in the transport
    //

    HARDWARE_ADDRESS LocalAddress;      // our local hardware address.

    HANDLE TdiDeviceHandle;
    HANDLE ReservedAddressHandle;

    //
    // These are used while initializing the MAC driver.
    //

    KEVENT NdisRequestEvent;            // used for pended requests.
    NDIS_STATUS NdisRequestStatus;      // records request status.

    //
    // This information is used to keep track of the speed of
    // the underlying medium.
    //

    ULONG MediumSpeed;                    // in units of 100 bytes/sec

	//
	//	General Mac options supplied by underlying NIC drivers
	//

	ULONG	MacOptions;

	//
    // Counters for most of the statistics that LPX maintains;
    // some of these are kept elsewhere. Including the structure
    // itself wastes a little space but ensures that the alignment
    // inside the structure is correct.
    //

    TDI_PROVIDER_STATISTICS Statistics;

    //
    // This resource guards access to the ShareAccess
    // and SecurityDescriptor fields in addresses.
    //

    ERESOURCE AddressResource;

    //
    // This is to hold the underlying PDO of the device so
    // that we can answer DEVICE_RELATION IRPs from above
    //

    PVOID PnPContext;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;



typedef struct _CONTROL_CONTEXT {

    DEVICE_OBJECT DeviceObject;         // the I/O system's device object.
    BOOL				bDeviceInit;

#if DBG
    ULONG RefTypes[NUMBER_OF_DCREFS];
#endif

    CSHORT Type;                          // type of this structure
    USHORT Size;                          // size of this structure

    KSPIN_LOCK Interlock;               // GLOBAL spinlock for reference count.
                                        //  (used in ExInterlockedXxx calls)
                                        
    LONG ReferenceCount;                // activity count/this provider.
    LONG CreateRefRemoved;              // has unload or unbind been called ?

    //
    // Following are protected by Global Device Co �32 ���       � &} �� # +@   @֚ ��         # ,@   z֚ �  � } # -@       �  )} } # .@    �֚ �  *} zx # /@       �
  +} x # 0@        &  ,} �] # 1@       
  <} [{ # A@   �ך B�  =} �[ # B@   �ך �  >} �| # C@   �ך ��  ?} �O # D@       �� @}     # E@    �ך D�  A} .� # F@   �ך �;  B} �c # G@   �ך uX C} @| # H@   �ך   D} D| # I@       h!  E} �O # J@        K�  F} �{ # K@    �ך �  G} �| # L@   �ך �  H} �{ # M@       M�  I} b� # N@        N�  J} J� # O@        �3     ]� # P@    ؚ �  L} 7} # Q@   ؚ 	  M} } # R@       
  N} } # S@        ]  O} �o # T@       9  =� Ty # U@        9  Q} O} # V@        o�  R} o # W@        p�  B�  o # X@    �ؚ �  T} K} # Y@       F�  U} �� # Z@       G�  G� �� # [@    �ؚ �  W} S} # \@       �  X} T| # ]@        h;  Y} �% # ^@        �
  Z} �{ # _@        2  N� i] # `@    �ؚ �  \} V} # a@       �� ]}     # b@        �  S� �| # c@        �
  _} �{ # d@       �  `} �D # e@        ;c     �� # f@        �1  b} ,{ # g@        �;  c} �{ # h@        ��  d} �Y # i@        ��      { # j@        �
  f} ^} # k@        �� g}     # l@        ��  h} )\ # m@        ��         # n@        �
  j} e} !# o@        &  k} �p !# p@        �b l} �/ !# q@       ��      �q !# r@        �� n}     "# s@        �� o}     "# t@        �� p}     "# u@        �� q}     "# v@        �� r}     "# w@        �� s}     "# x@        hI t} ٪ "# y@        �� u}     "# z@        ��         "# {@        �
  w} i} ## |@        ̯ x} �] ## }@        m     �] ## ~@        �  z} �{ $# @        M   {} �X $# �@        ��         $# �@        �   }} Rr %# �@        �� ~}     %# �@        �  } (} %# �@        +  �} �q %# �@        �   �} 	Y %# �@        K�  �} r %# �@        !  �} nw %# �@        �� �} {} %# �@        �� �}     %# �@        �� �}     %# �@        �� �} �} %# �@        ��     �} %# �@    �ۚ � �} [| &# �@   �ۚ �� �} \| &# �@   �ۚ �� �} ]| &# �@   �ۚ �� �} ^| &# �@   ܚ �  �} [} &# �@   	ܚ �  �} =} &# �@   ܚ �� �} a| &# �@   ܚ �� �} b| &# �@   ܚ zu �} c| &# �@       �� _� �X &# �@    `ܚ �  �} �} '# �@   dܚ �   �} �| '# �@   oܚ H  �} vd '# �@   uܚ   �}  } '# �@        3 �} Nx '# �@          �} �| '# �@        �� �}     '# �@        �� �}     '# �@        �� �}     '# �@        �� k�     '# �@    �ܚ �  �} �} (# �@   �ܚ �  o� �B (# �@    ݚ �  �} �} )# �@   ݚ P�  �} �w )# �@       Q�  �} �� )# �@       R�  �} �� )# �@   ݚ S�  �} �� )# �@       T�  �} �� )# �@        �3     �n )# �@        S�  �} �} *# �@        �3 �� �w *# �@    iݚ �  �} �} +# �@   mݚ P�  �} �} +# �@       �3     �n +# �@    �ݚ �  �} �} ,# �@       ��  �} � ,# �@        �  �� ~} ,# �@        �� �}     -# �@        ��  �} �} -# �@        �  �� �} -# �@          �} �} .# �@    Mޚ   �} 1} .# �@       �� �}     .# �@        �� ��     .# �@          �} �} /# �@    �ޚ   �} �} /# �@       �� �}     /# �@  �32 ���           /# �@        �� �}     0# �@        �� �}     0# �@        ��         0# �@    Sߚ �  �} �} 1# �@   Xߚ �  �} �} 1# �@   `ߚ �  �} F} 1# �@       ��  �} �} 1# �@        �� �}     1# �@        ϥ �} "Y 1# �@        �� �     1# �@        D� �} c2 2# �@        �� �} �} 2# �@       �� �} �} 2# �@        �� �} �} 2# �@        �  �} gX 2# �@        �� 
� �} 2# �@                0.5  fd40e6a6ff4e3029fe3f4e4ab98e52a6 caspar 20101204-1 caspar-doc 20091115-1 20091115-1  06daae8a7ffe33cb7597844be4c911ed 20101204-1  b4d0af6863bd65c41ebb6d661937ead8 castle-combat                                                                       �� �0.    S     S �z t� \�     ��      `+     �d  �<         X� ��-     S     S { w�         �B            �C  �<         �� %.     S     S { 5�          �W	      @&     ��  �<         �� ���     S     S { &N         8�       �     ,  �<         �� �	.    !S     "S { x�         6y       �     '�  �<         gd� %.     �k     �k { {�         jr      �,     �  �<         �d� ��-     �k     �k -{ }�         |�      0	     ω  �<         De� ��-     �k     �k 2{ ~�         R9      �     ��  �<         �e� d�-    �k     �k 4{ ��         �            s'  �<         �e� �.    �k     �k 5{ ��         �H      �     �  �<         f� �	.     �k     �k 7{ ��         �O       ,     S�  �<         Vf� ��-     �k     �k <{ ��         ��D       �     ��  �<         �f� ..     �k     �k A{ ��         x�       �     �m  �<         g� �	.     �k     �k L{ ��         Ȃ       0     �)  �<         9g� ��-     �k     �k N{ �         �      �     ��  �<         �g� ��-     �k     �k T{ ��         j      �     `   =         �g� ,�-     �k     �k W{ ��         ��       P     ��  =         4h� pĎ     �k     �k `{ ��         �             S  =         qh� 0�-     �k     �k a{ ��         8^       @     ��  =         �h� ��-    �k     �k b{ ��         h       �      ��  =         �h� *�1     �k     �k e{ ��         ��      �     �/  =         ii� ��-     �k     �k o{ ��         N�       �     	�  =         �i� %.     �k     �k x{ ��         "           �t  =         �i� �	.     �k     �k z{ ��         l�       @     ٜ  =         "j� d�-    �k     �k {{ ��         ��      �     ��  	=         hj� �	.     �k     �k |{ ��         '      p     �e  
=         �j� %.    �k     �k �{ ��         |E      �     ��  =         �j� ,�-     �k     �k �{ ��         <              �  =         )k� ��-     �k     �k �{ ��         �X
           Me  
�  =         $l� �-     �k     �k �{ ��         �      �     ��  =         �l� ,�-     �k     �k �{ ��         (�             ��  =         �l� ��-     �k     �k �{ ��         T�       �     ^�  =         $m� �	.     �k     �k �{ ��         ��            28  =         bm� %.     �k     �k �{ ��         �k       �     �  =         �m� �.    �k     �k �{ ��         �       �      Z  =         �m� ��-    �k     �k �{ ��         �       t      ��  =         n� ��-    �k     �k �{ �         �       �
GGf�f;�u�USPh� �5h� �փ���|C��	W|h� �h� �5h� �փ���|"W�� f�|G�
t�5h� h� �� YY�� �������   h  �D$ Ph,  �5|� �� �L$�D$�f��/t
@@f�f��u�3�f9��tf�f��mt
GGf�f;�u�USPh� �5h� �փ���|C��	W|h� �h� �5h� �փ���|"W�� f�|G�
t�5h� h� �� YY�� �������   h  �D$ Ph-  �5|� �� �L$�D$�f��/t
@@f�f��u�3�f9��tf�f��mt
GGf�f;�u�USPh� �5h� �փ���|C��	W|h� �h� �5h� �փ���|"W�� f�|G�
t�5h� h� �� YY�� ��D$�� �T$�����u�D$D$��   h  �D$ Ph.  �5|� �� �L$�D$�f��/t
@@f�f��u�3�f9��tf�f��mtf��nt
GGf�f;�u��t$�t$USPh� �5h� �փ���|C��	W|h� �h� �5h� �փ���|"W�� f�|G�
t�5h� h� �� YYh/  �5� �"���3�9� t/9� tG9� t?� � �t6�t2�t.h   j������%9� u9� u� � 
@@f�f;�u�f9��tf�f��mt
GGf�f;�u��t$UPh� �5h� �փ�;�|C��	W|h� �h� �5h� �փ�;�|"W�� f�|G�
t�5h� h� �� YY�0� ;�]t94� t	h  j�h  P�%���h  �54� ����h�  �n����<� S3�FV�h�  RP������8� ���  ��SVh�  �RP�����h�  �5�� �����hA  �!���hD  �5�� ����hC  �5�� ����hE  �5�� ����hF  �5�� �~���hG  �5�� �n���hH  �5�� �^���hJ  �5�� �N���hI  �5�� �>���hP  �5�� �.���hQ  �5�� ����3�9� hK  ��P����hL  �58� ������v���hO  �5�� ������5h� �� Y[��$(  ��L$�_  ��$   _d�
u
f�HII;�u�3�f9�����t!��������f�	f�� tf��	u
G@@f9��u獴}����f�f;���   f;Ӊ�����t"�΋�f�	f��/tf��\u������@@f9��u�f��-tkjh4 V�� ����tVjh$ V�� ����tAjh V�� ����t,jh V�� ����tf�>/u	������tV�r����
  h W�օ�u�� � � �� ��h  W�օ�u!4� �0� �
  h� W�օ�u!0� �4� �
  h� W�օ�u�0� ��h� W�օ�u!� �[
  h� W�օ�u�8� �D
  h� W�օ�u! � �-
  h� W�օ�u!$� �
  h� W�օ�u�D� ��	  h� W�օ�u�H� ��	  h� W�օ�u�L� ��	  h� W�օ�u�P� �	  h� W�օ�u�T� �	  h� W�օ�u�X� �	  h� W�օ�u�\� �u	  h� W�օ��e	  h| W�օ��U	  ht W�օ�u�t�    �:	  hl W�օ�u��    �	  hd W�օ�u!d� �`�    �%h�  ��  h\ W�օ�u!`� �d�    ��hT W�օ�u!`� !d� �h�    �  �5� jhH W�փ�����   3Ƀ�
�
   ���jRPh�0B ��   �@rB ��uf�=�rB t]h�  h��B h�;C ��  Ph �A h�zB �C�  ��Ph�zB h rB ��<C h0<B j	h�rB ���  ��rB ��<yt<Y�!  f�";C f= r
f= ��   f= ��   f= t}f= ww蹥  ��zB �D;C ��   �<v��
   ��h  ��j ���jRPh�0B h��B h�;C �  ��Phx�A h�zB �l�  �� �j�
�  ��Ph�zB h rB ��<C ����  ��_^][�9rB t*h �  hP<B ��;C P<B �X ��P�X ���^����   ���Q���_^]�2   [�9@rB ��   94rB uz�rB �(1B ;�u� mB 9=rB t� mB ���zB �P�1B ��u�1B SQPh��B h�;C ��  ��Ph1B h��A h�zB �'�  ��Ph�zB h rB ��<C ����%  ��;��O  ��2��   �@rB ;á4rB tZ;�uZ�����A t���A h  h��A Ph��A h�zB ��  ��Ph�zB h rB ��<C ��3Ƀ�����   ����  ;�t������A t���A h  h��B h�;C ��
� mB ��0B h  h�0B h��B h�;C �
  Ph �A h�zB �k�  ��Ph�zB h rB ��<C ���2   �Z  ��_^][þ   �	���[  ��:C �;C ;С@rB ��   ;á4rB t;�u�?;�t;h  h��B h�;C �	  Ph�0B h�zB ��  ��Ph�zB h rB ��<C ���;C �
  ��_^][�;�t>��:C ;�t%3�f�
  ��_^][�94rB u;Sh�0B �94rB u+;�u'Sh�0B h�zB ��  ��Ph�zB h rB ��<C ����	  ��_^][Ð������������D$SU3�V��W��  �|$W�3  �؍GP�(  �L$ f������  �����;���  ��%��  =SD  ��   t��	��   =M3  ��   �Q  ����  �G����  h��@ jVW�  �؃����"  �4rB ��t8jh��B h�;C �  Ph�0B h�zB ���  ��Ph�zB h rB ��<C ������  ����=�܅���z� ��<����F�:H1z��9�6�a�ܮ�"�
|���O#�0��Lc6��3��<d�=O&�!�g�d d�}~-a�� x���5�\�n7r��a�\$�ʭ�Z|��v�����Y���8�թ�~9(s�[��O,���O�.pH���F���3��ϯ�d�&��>bw��<p(V�:�S���52O���%�Lߐ%I7�"�9 }� '�n�cM��|��+��tO�)݃��95B�:JEb���DԿ$b7�/�Nִi/a5���*��N�
�/xU���`���QO8}D��juV��_I�U�~H,�����vJ^��I��	��'Ku`r<$�c���d�
>#C��B���h��
C&ze|��s�8·�����~�f�s2��c�4(��P�PdܔIF��/��iΣA~b'��uo�Ǿ���R�u���?ol&6AB�9'"��KGD�"�'�	"Xq"AD$'V�!�[W6q�LD� m�Z)��
؟s�{����sJ���!�t��`��O��C�|K^���1��cy���HWQ�c����#�!~�u��ֹ�����5����Aj�k�>��F�ßH�6��ʧ��/�����Y�k{�!.�S��	��,KұhO;�|��2)#N�r�I뺽aڌ�ːS���I��W��Rm���D~&��.�F��^��gR����p��B���.'�ޙ�n{"���v��x[���-�l��
a��A��w
��� ��vɰ��=-=v?1Q��۟���X^"v<�2��P�����x��a�+	�k�Xu����	6'�,�:tIۻk����c̣�"?d�=���IO^��*�]���{���q6bŤg�V#�B�GPlb�����.u'H?/B>A��c���%7��fX�3�i��~�����#�y��1�����o�>xl�,%}Z��0̇�.J�6�|��l#-��6�/�δKo{�hYVyW�_l!�G���|�2�E�X���]�V?a�Gs�Qf�E��@Y�y����ף�*[fnh���3�i8���Ȕm������U�e����[��|�m��,m��6w�����y��7q~��=�Ȩw��i�p\F�~��`���ٷ[�U�{)��:�J)�a}w��M�4x�ت]#�=Zv!p�Oc���.�:���y��F�kZ���K���xG*�'�S��gf��"=BK��.�� ڃߜmoP*���a_�K�Qg��B�N�G=�Q�1��Qϙ�B��p���krX����E�Ag` *�"�o(��< =�~
5��ѿ��� �0�������H����"e��B��ⷵ W����W�k�ߺ����`�%�|c�?1���-�YE���G���;H���>?K��-N�o%��
a�y��~0NA���b�<�oi{&"��uL�sn0�T�sj|����?����潯�ip�n�f��wJN���{v����G߱gn�"L4���i�����6��I�G�׳����a\�w"d"��$�����T��]�e��t)��@,4�BY��
(�����!?%�i�tA�i����K��:�
uY����t�@�Mk�3�r�K�^�]'�ޕꮓ�mH��(�ܖ���̅m.ɠ�׏���wF�-m��4mJ����S�=z��9W6r�>��u�ٯ�9��Y{;<K[
sjw�*rް��5��]�*��J�E�Ma����i�JF�=��IByz��H\��ϛ��f�Bl�-i	���RW��t+�"6�0�""d�"C!�H`	!�"DDD$KH�%$	R$���,˶�4���{�o|��I��������x�{ι���%졥p������pH��>��>�a��Q�G�^�%��۝��� � ��d���w����N��,���߭����~��3��g�=��~P�|��S�op��l��Epϳo~v��ﱬS�w��n�Ժ%ȳ��2�m�>2�k�q-ybyg�3o�.y�
T�R���/+�T�9���CN��M��U�]���	�E���A)X)rĉ�gV�Ӫso�����{)h�C��>����S4ީ����;G=��2��z;ٗByTͯ�A�~������&�Q�i�s���M�C�#�ʼEm@��9���x_����#�%�'�뉬���k��	�t��a��@��Y�3�
*���8)pԒ�}�}|�{������}w[K���������P	��ɰ���%���;w�.�K!�˻rfݖ���+�S���G*P�}?�+���䎥���	��t�I��"d$@Ж���W1>K�7����>��8�����E�7���U�$�?����#��Ź�rս'g
$�y���C)��ZBw���(���Ȥ��g��	q�&�Zze��ު��2����h��dr�,�f����T�[�ْs�1�r��҆��\$'��         �   ����������������@������ �d8 ���@0: ���        x�            �20N    8{G    8&�J            ��$N    �W�    	 	            �!      `              � 0: ���� 0: ���                                � 0: ���� 0: ���� 0: ���� 0: ���� 0: ���� 0: ���0: ���0: ���                       `������          0: ���                                    p0: ���p0: ���       �0: ����0: ���                        @�������      89� ���        �0: ����0: ���                        �0: ����0: ���                                S,                            ��  ��  x�     ��     `0: ���`0: ���p0: ���p0: ���                �0: ����0: ����0: ����0: ���@0: ���C% h
	 ��         �   ����������������@������ �d8 ��� 0: ���        v�            �20N    �\�    8&�J            ��$N    �W�    	 	                  `              h0: ���h0: ���                                �0: ����0: ����0: ����0: ����0: ����0: ����0: ����0: ���                       `������        �0: ���                                    00: ���00: ���       H0: ���H0: ���                        @�������      89� ���        �0: ����0: ���                        �0: ����0: ���                                Q,                            ��      v�     ��      0: ��� 0: ���00: ���00: ���    l
	         P0: ���P0: ���`0: ���`0: ����0: ���    & ��         �   ����������������@������ �d8 ����0: ���        w�            �20N    @��    8&�J            ��$N    �W�    	 	            m	      `              (0: ���(0: ���                                X0: ���X0: ���h0: ���h0: ���x0: ���x0: ����0: ����0: ���                       `������        �0: ���                                    �0: ����0: ���       0: ���0: ���                        @�������      89� ���        P0: ���P0: ���                        x0: ���x0: ���                                R,                            ��  u
	 w�     ��     �0: ����0: ����0: ����0: ���    /     �, 0: ���0: ��� 0: ��� 0: ���  0: ���       ��         �   ����������������@������ �d8 ����	0: ���        y�            �20N    0�Z    8&�J            ��$N    �W�          @       Wp      `              �0: ����0: ���                                	0: ���	0: ���(	0: ���(	0: ���8	0: ���8	0: ���H	0: ���H	0: ���                       `������        @0: ���                                    �	0: ����	0: ���       �	0: ����	0: ���                        @�������      89� ���        
0: ���
0: ���                        8
0: ���8
0: ���                                T,                            ��  8 y�     ��     �
0: ����
0: ����
0: ����
0: ���            �- �
0: ����
0: ����
0: ����
0: ����0: ��� �     �A         �   ���������������� ������ �d8 ���@0: ���        B�            )��N    �:0    ��$N    Xt/    ��$N    Xt/    