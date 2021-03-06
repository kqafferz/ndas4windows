 �22 ���                        \
    ExAcquireFastMutexUnsafe( &(V)->CheckpointMutex );      \
}

#define NtfsReleaseCheckpoint(IC,V) {                       \
    ExReleaseFastMutexUnsafe( &(V)->CheckpointMutex );      \
}

#define NtfsWaitOnCheckpointNotify(IC,V) {                          \
    NTSTATUS _Status;                                               \
    _Status = KeWaitForSingleObject( &(V)->CheckpointNotifyEvent,   \
                                     Executive,                     \
                                     KernelMode,                    \
                                     FALSE,                         \
                                     NULL );                        \
    if (!NT_SUCCESS( _Status )) {                                   \
        NtfsRaiseStatus( IrpContext, _Status, NULL, NULL );         \
    }                                                               \
}

#define NtfsSetCheckpointNotify(IC,V) {                             \
    KeSetEvent( &(V)->CheckpointNotifyEvent, 0, FALSE );            \
}

#define NtfsResetCheckpointNotify(IC,V) {                           \
    KeClearEvent( &(V)->CheckpointNotifyEvent );                    \
}

#define NtfsAcquireUsnNotify(V) {                           \
    ExAcquireFastMutex( &(V)->CheckpointMutex );            \
}

#define NtfsReleaseUsnNotify(V) {                           \
    ExReleaseFastMutex( &(V)->CheckpointMutex );            \
}

#define NtfsAcquireReservedClusters(V) {                    \
    ExAcquireFastMutexUnsafe( &(V)->ReservedClustersMutex );\
}

#define NtfsReleaseReservedClusters(V) {                    \
    ExReleaseFastMutexUnsafe( &(V)->ReservedClustersMutex );\
}

#define NtfsAcquireFsrtlHeader(S) {                         \
    ExAcquireFastMutex((S)->Header.FastMutex);              \
}

#define NtfsReleaseFsrtlHeader(S) {                         \
    ExReleaseFastMutex((S)->Header.FastMutex);              \
}

#define NtfsReleaseVcb(IC,V) {                              \
    ExReleaseResource( &(V)->Resource );                    \
}

//
//  Macros to test resources for exclusivity.
//

#define NtfsIsExclusiveResource(R) (                            \
    ExIsResourceAcquiredExclusive(R)                            \
)

#define NtfsIsExclusiveFcb(F) (                                 \
    (NtfsIsExclusiveResource((F)->Resource))                    \
)

#define NtfsIsExclusiveFcbPagingIo(F) (                         \
    (NtfsIsExclusiveResource((F)->PagingIoResource))            \
)

#define NtfsIsExclusiveScb(S) (                                 \
    (NtfsIsExclusiveFcb((S)->Fcb))                              \
)

#define NtfsIsExclusiveVcb(V) (                                 \
    (NtfsIsExclusiveResource(&(V)->Resource))                   \
)

//
//  Macros to test resources for shared acquire
//

#define NtfsIsSharedResource(R) (                               \
    ExIsResourceAcquiredShared(R)                               \
)

#define NtfsIsSharedFcb(F) (                                    \
    (NtfsIsSharedResource((F)->Resource))                       \
)

#define NtfsIsSharedFcbPagingIo(F) (                            \
    (NtfsIsSharedResource((F)->PagingIoResource))               \
)

#define NtfsIsSharedScb(S) (                                    \
    (NtfsIsSharedFcb((S)->Fcb))                                 \
)

#define NtfsIsSharedVcb(V) (                                    \
    (NtfsIsSharedResource(&(V)->Resource))                      \
)

__inline
VOID
NtfsReleaseExclusiveScbIfOwned(
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    )
/*++

Routine Description:

    This routine is called release an Scb that may or may not be currently
    owned exclusive.

Arguments:

    IrpContext - Context of call

    Scb - Scb to be released

Return Value:

    None.

--*/
{
    if (Scb->Fcb->ExclusiveFcbLinks.Flink != NULL &&
        NtfsIsEx �22 ���       @�A  ���       ���  ���       IrpContext, Scb );
    }
}

//
//  The following are cache manager call backs.  They return FALSE
//  if the resource cannot be acquired with waiting and wait is false.
//

BOOLEAN
NtfsAcquireVolumeForClose (
    IN PVOID Vcb,
    IN BOOLEAN Wait
    );

VOID
NtfsReleaseVolumeFromClose (
    IN PVOID Vcb
    );

BOOLEAN
NtfsAcquireScbForLazyWrite (
    IN PVOID Null,
    IN BOOLEAN Wait
    );

VOID
NtfsReleaseScbFromLazyWrite (
    IN PVOID Null
    );

NTSTATUS
NtfsAcquireFileForModWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER EndingOffset,
    OUT PERESOURCE *ResourceToRelease,
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
NtfsAcquireFileForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
NtfsReleaseFileForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
NtfsAcquireForCreateSection (
    IN PFILE_OBJECT FileObject
    );

VOID
NtfsReleaseForCreateSection (
    IN PFILE_OBJECT FileObject
    );


BOOLEAN
NtfsAcquireScbForReadAhead (
    IN PVOID Null,
    IN BOOLEAN Wait
    );

VOID
NtfsReleaseScbFromReadAhead (
    IN PVOID Null
    );

BOOLEAN
NtfsAcquireVolumeFileForClose (
    IN PVOID Vcb,
    IN BOOLEAN Wait
    );

VOID
NtfsReleaseVolumeFileFromClose (
    IN PVOID Vcb
    );

BOOLEAN
NtfsAcquireVolumeFileForLazyWrite (
    IN PVOID Vcb,
    IN BOOLEAN Wait
    );

VOID
NtfsReleaseVolumeFileFromLazyWrite (
    IN PVOID Vcb
    );


//
//  Ntfs Logging Routine interfaces in RestrSup.c
//

BOOLEAN
NtfsRestartVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PBOOLEAN UnrecognizedRestart
    );

VOID
NtfsAbortTransaction (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PTRANSACTION_ENTRY Transaction OPTIONAL
    );

NTSTATUS
NtfsCloseAttributesFromRestart (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );


//
//  Security support routines, implemented in SecurSup.c
//

//
//  VOID
//  NtfsTraverseCheck (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB ParentFcb,
//      IN PIRP Irp
//      );
//
//  VOID
//  NtfsOpenCheck (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN PFCB ParentFcb OPTIONAL,
//      IN PIRP Irp
//      );
//
//  VOID
//  NtfsCreateCheck (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB ParentFcb,
//      IN PIRP Irp
//      );
//

#define NtfsTraverseCheck(IC,F,IR) { \
    NtfsAccessCheck( IC,             \
                     F,              \
                     NULL,           \
                     IR,             \
                     FILE_TRAVERSE,  \
                     TRUE );         \
}

#define NtfsOpenCheck(IC,F,PF,IR) {                                                                      \
    NtfsAccessCheck( IC,                                                                                 \
                     F,                                                                                  \
                     PF,                                                                                 \
                     IR,                                                                                 \
                     IoGetCurrentIrpStackLocation(IR)->Parameters.Create.SecurityContext->DesiredAccess, \
                     FALSE );                                                                            \
}

#define NtfsCreateCheck(IC,PF,IR) {                                                                              \
    NtfsAccessCheck( IC,                                                                                         \
                     PF,                                                                                         \
                     NULL,                                                                                       \
                     IR �22 ���                                                                                 \
                     (FlagOn(IoGetCurrentIrpStackLocation(IR)->Parameters.Create.Options, FILE_DIRECTORY_FILE) ? \
                        FILE_ADD_SUBDIRECTORY : FILE_ADD_FILE),                                                  \
                     TRUE );                                                                                     \
}

VOID
NtfsAssignSecurity (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ParentFcb,
    IN PIRP Irp,
    IN PFCB NewFcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,      //  BUGBUG delete
    IN PBCB FileRecordBcb,                          //  BUGBUG delete
    IN LONGLONG FileOffset,                         //  BUGBUG delete
    IN OUT PBOOLEAN LogIt      