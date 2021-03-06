
        DbgPrint("LPX: Tried to cancel receive datagram %p on %p, not found\n",
                Irp, AddressFile);
#endif

    }

}   /* LpxCancelReceiveDatagram */


NTSTATUS
LpxTdiReceiveDatagram(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the TdiReceiveDatagram request for the transport
    provider. Receive datagrams just get queued up to an address, and are
    completed when a DATAGRAM or DATAGRAM_BROADCAST frame is received at
    the address.

Arguments:

    Irp - I/O Request Packet for this request.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS status;
    KIRQL oldirql;
    PTP_ADDRESS address;
    PTP_ADDRESS_FILE addressFile;
    PIO_STACK_LOCATION irpSp;
    KIRQL cancelIrql;

    //
    // verify that the operation is taking place on an address. At the same
    // time we do this, we reference the address. This ensures it does not
    // get removed out from under us. Note also that we do the address
    // lookup within a try/except clause, thus protecting ourselves against
    // really bogus handles
    //

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    if (irpSp->FileObject->FsContext2 != (PVOID) TDI_TRANSPORT_ADDRESS_FILE) {
        return STATUS_INVALID_ADDRESS;
    }

    addressFile = irpSp->FileObject->FsContext;

    status = LpxVerifyAddressObject (addressFile);

    if (!NT_SUCCESS (status)) {
        return status;
    }

    address = addressFile->Address;

    IoAcquireCancelSpinLock(&cancelIrql);
    ACQUIRE_SPIN_LOCK (&address->SpinLock,&oldirql);

    if ((address->Flags & ADDRESS_FLAGS_STOPPING) != 0) {

        RELEASE_SPIN_LOCK (&address->SpinLock,oldirql);
        IoReleaseCancelSpinLock(cancelIrql);

        Irp->IoStatus.Information = 0;
        status =  (address->Flags & ADDRESS_FLAGS_STOPPING) ?
                    STATUS_NETWORK_NAME_DELETED : STATUS_DUPLICATE_NAME;
    } else {

        //
        // If this IRP has been cancelled, then call the
        // cancel routine.
        //

        if (Irp->Cancel) {

            RELEASE_SPIN_LOCK (&address->SpinLock, oldirql);
            IoReleaseCancelSpinLock(cancelIrql);

            Irp->IoStatus.Information = 0;
            status =  STATUS_CANCELLED;
//            Irp->IoStatus.Status = STATUS_CANCELLED;
//            IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
        } else {
            IoSetCancelRoutine(Irp, LpxCancelReceiveDatagram);
            LpxReferenceAddress ("Receive datagram", address, AREF_REQUEST);
            InsertTailList (&addressFile->ReceiveDatagramQueue,&Irp->Tail.Overlay.ListEntry);
            RELEASE_SPIN_LOCK (&address->SpinLock,oldirql);
            IoReleaseCancelSpinLock(cancelIrql);
            status = STATUS_PENDING;
        }
    }

    LpxDereferenceAddress ("Temp rcv datagram", address, AREF_VERIFY);

    return status;

} /* TdiReceiveDatagram */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /*++

Copyright (C)2002-2005 XIMETA, Inc.
All rights reserved.

--*/

#include "precomp.h"
#pragma hdrstop
 
#if DBG
VOID
LpxRefSendIrp(
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine increments the reference count on a send IRP.

Arguments:

    IrpSp - Pointer to the IRP's stack location.

Return Value:

    none.

--*/

{

    IF_LPXDBG (LPX_DEBUG_REQUEST) {
        LpxPrint1 ("LpxRefSendIrp:  Entered, ReferenceCount: %x\n",
            IRP_SEND_REFCOUNT(IrpSp));
    }

    ASSERT (IRP_SEND_REFCOUNT(IrpSp) > 0);

    InterlockedIncrement (&IRP_SEND_REFCOUNT(IrpSp));

} /* LpxRefSendIrp */


VOID
LpxDerefSendIrp(
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine dereferences a transport send IRP by decrementing the
    reference count contained in the structure.  If, after being
    decremented, the reference count is zero, then this routine calls
    IoCompleteRequest to actually complete the IRP.

Arguments:

    Request - Pointer to a transport send IRP's stack location.

Return Value:

    none.

--*/

{
    LONG result;

    IF_LPXDBG (LPX_DEBUG_REQUEST) {
        LpxPrint1 ("LpxDerefSendIrp:  Entered, ReferenceCount: %x\n",
            IRP_SEND_REFCOUNT(IrpSp));
    }

    result = InterlockedDecrement (&IRP_SEND_REFCOUNT(IrpSp));


    ASSERT (result >= 0);

    //
    // If we have deleted all references to this request, then we can
    // destroy the object.  It is okay to have already released the spin
    // lock at this point because there is no possible way that another
    // stream of execution has access to the request any longer.
    //

    if (result == 0) {
        KIRQL  cancelIrql;

        PIRP Irp = IRP_SEND_IRP(IrpSp);

        IRP_SEND_REFCOUNT(IrpSp) = 0;
        IRP_SEND_IRP (IrpSp) = NULL;


        IoAcquireCancelSpinLock(&cancelIrql);
        IoSetCancelRoutine(Irp, NULL);
        IoReleaseCancelSpinLock(cancelIrql);
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
    }

} /* LpxDerefSendIrp */
#endif

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /*++

Copyright (C)2002-2005 XIMETA, Inc.
All rights reserved.

--*/

#include "precomp.h"
#pragma hdrstop


NTSTATUS
LpxTdiSend(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the TdiSend request for the transport provider.

    NOTE: THIS FUNCTION MUST BE CALLED AT DPC LEVEL.

Arguments:

    Irp - Pointer to the I/O Request Packet for this request.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PTP_CONNECTION connection;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    //
    // Determine which connection this send belongs on.
    //

    irpSp = IoGetCurrentIrpStackLocation (Irp);
    connection  = irpSp->FileObject->FsContext;

    //
    // Check that this is really a connection.
    //

    if ((irpSp->FileObject->FsContext2 == (PVOID)LPX_FILE_TYPE_CONTROL) ||
        (connection->Size != sizeof (TP_CONNECTION)) ||
        (connection->Type != LPX_CONNECTION_SIGNATURE) ||
		(connection->Flags2 & CONNECTION_FLAGS2_STOPPING)
		){//||
//		(connection->IsDisconnted == TRUE)) {
#if DBG
        LpxPrint2 ("TdiSend: Invalid Connection %p Irp %p\n", connection, Irp);
#endif
        status = STATUS_INVALID_CONNECTION;
//        Irp->IoStatus.Status = status;
//        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);            
        return status;
    }
 
    status = LpxSend(
		connection,
		Irp
		);
    return status;
} /* TdiSend */


NTSTATUS
LpxTdiSendDatagram(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the TdiSendDatagram request for the transport
    provider.

Arguments:

    Irp - Pointer to the I/O Request Packet for this request.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS status;
    PTP_ADDRESS_FILE addressFile;
    PTP_ADDRESS address;
    PIO_STACK_LOCATION irpSp;

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    if (irpSp->FileObject->FsContext2 != (PVOID) TDI_TRANSPORT_ADDRESS_FILE) {
        return STATUS_INVALID_ADDRESS;
    }

    addressFile  = irpSp->FileObject->FsContext;

    status = LpxVerifyAddressObject (addressFile);
    if (!NT_SUCCESS (status)) {
        IF_LPXDBG (LPX_DEBUG_SENDENG) {
            LpxPrint2 ("TdiSendDG: Invalid address %p Irp %p\n",
                    addressFile, Irp);
        }
        return status;
    }

    address = addressFile->Address;

    status = LpxSendDatagram(
        address,
        Irp
        );

    LpxDereferenceAddress("tmp send datagram", address, AREF_VERIFY);

    return status;
} /* LpxTdiSendDatagram */

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              TARGETNAME=lpx
TARGETPATH=obj
TARGETTYPE=DRIVER

LPX=lpx
MINORCOMP=lpx
NETDISK_BINPLACE_TYPE=sys

TARGETLIBS=$(DDK_LIB_PATH)\tdi.lib \
           $(DDK_LIB_PATH)\ndis.lib

C_DEFINES=-D__LPX__ -D__VERSION_CONTROL__ -DNDIS40 -D_PNP_POWER_ -DNDIS_POOL_TAGGING=1

!IFDEF XP
C_DEFINES=-D__XP__ $(C_DEFINES)
!ENDIF

AMD64_WARNING_LEVEL=/W3 /WX /Wp64 -wd4296
# -Wp64 will not work with ntddk.h for w2k,wxp headers
386_WARNING_LEVEL=/W3 /WX

INCLUDES=$(NDAS_DRIVER_INC_PATH);$(NDAS_INC_PATH)
RCOPTIONS=/i $(NDAS_INC_PATH)

PRECOMPILED_INCLUDE=precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES=address.c \
        connect.c \
        devctx.c \
        event.c \
        info.c \
        lpx.rc \
        lpxcnfg.c \
        lpxdrvr.c \
        lpxmac.c \
        lpxndis.c \
        lpxpnp.c    \
        rcv.c \
        request.c \
        send.c \
        lpx.c \
        lpxpacket.c

#
# CDF Copy
#
NTTARGETFILES=cdfcopy
!IF "$(_BUILDARCH)" == "AMD64"
CDFSRCFILE=netlpx.amd64.cdf
!ELSE
CDFSRCFILE=netlpx.cdf
!ENDIF
CDFFILE=$(O)\netlpx.cdf

#
# Set INF File Version
#

INFFILENAME=netlpx
!IF "$(_BUILDARCH)" == "AMD64"
INFSRCNAME=$(INFFILENAME).amd64
!ELSE
INFSRCNAME=$(INFFILENAME)
!ENDIF

POST_BUILD_CMD=$(NDAS_TOOLS)\setinfver $(O)\$(TARGETNAME).sys $(INFSRCNAME).inf $(O)\$(INFFILENAME).inf
BINPLACE_FLAGS=$(O)\$(INFFILENAME).inf $(CDFFILE)

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           rvicePoint,
	IN	ULONG                PacketLength,
	IN	PDEVICE_CONTEXT        DeviceContext,
	IN	BOOLEAN                Send,
	IN	PUCHAR                CopyData,
	IN	ULONG                CopyDataLength,
	IN	PIO_STACK_LOCATION    IrpSp,
	OUT	PNDIS_PACKET        *Packet
	);

PNDIS_PACKET
PacketCopy(
	IN	PNDIS_PACKET Packet,
	OUT	PLONG    Cloned
	) ;

PNDIS_PACKET
PacketClone(
	IN	PNDIS_PACKET Packet
	);

BOOLEAN
PacketQueueEmpty(
	PLIST_ENTRY	PacketQueue,
	PKSPIN_LOCK	QSpinLock
	);

PNDIS_PACKET
PacketPeek(
	PLIST_ENTRY	PacketQueue,
	PKSPIN_LOCK	QSpinLock
	);

void
CallUserDisconnectHandler(
	IN	PSERVICE_POINT    pServicePoint,
	IN	ULONG            DisconnectFlags
	);

VOID LpxChangeState(
	IN PTP_CONNECTION Connection,
	IN SMP_STATE NewState,
	IN BOOLEAN Locked	
);


#endif // def _LPXPROCS_
