#ifndef ipcLockProtocol_h__
#define ipcLockProtocol_h__

#include "prtypes.h"

//
// ipc lock message format:
//
//   +----------------------------------+
//   | opcode : 4 bits                  |
//   +----------------------------------+
//   | flags  : 4 bits                  |
//   +----------------------------------+
//   | key    : null terminated string  |
//   +----------------------------------+
//

// lock opcodes
#define IPC_LOCK_OP_ACQUIRE          1
#define IPC_LOCK_OP_RELEASE          2
#define IPC_LOCK_OP_STATUS_ACQUIRED  3
#define IPC_LOCK_OP_STATUS_FAILED    4
#define IPC_LOCK_OP_STATUS_BUSY      5

// lock flags
#define IPC_LOCK_FL_NONBLOCKING 1

// data structure for representing lock request message
struct ipcLockMsg
{
    PRUint8      opcode;
    PRUint8      flags;
    const char * key;
};

//
// flatten a lock message
//
// returns a malloc'd buffer containing the flattened message.  on return,
// bufLen contains the length of the flattened message.
//
PRUint8 *IPC_FlattenLockMsg(const ipcLockMsg *msg, PRUint32 *bufLen);

//
// unflatten a lock message
//
void IPC_UnflattenLockMsg(const PRUint8 *buf, PRUint32 bufLen, ipcLockMsg *msg);

//
// TargetID for message passing
//
#define IPC_LOCK_TARGETID \
{ /* 703ada8a-2d38-4d5d-9d39-03d1ccceb567 */         \
    0x703ada8a,                                      \
    0x2d38,                                          \
    0x4d5d,                                          \
    {0x9d, 0x39, 0x03, 0xd1, 0xcc, 0xce, 0xb5, 0x67} \
}

#endif // !ipcLockProtocol_h__
