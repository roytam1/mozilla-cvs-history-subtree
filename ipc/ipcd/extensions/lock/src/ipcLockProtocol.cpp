#include <stdlib.h>
#include <string.h>
#include "prlog.h"
#include "ipcLockProtocol.h"

//-----------------------------------------------------------------------------

static inline PRUint8 get_opcode(const PRUint8 *buf)
{
    return (buf[0] & 0x0f);
}

static inline PRUint8 get_flags(const PRUint8 *buf)
{
    return (buf[0] & 0xf0) >> 4;
}

static inline const char *get_key(const PRUint8 *buf)
{
    return ((const char *) buf) + 1;
}

//-----------------------------------------------------------------------------

PRUint8 *
IPC_FlattenLockMsg(const ipcLockMsg *msg, PRUint32 *bufLen)
{
    PRUint32 len = 1                 // header byte
                 + strlen(msg->key)  // key
                 + 1;                // null terminator

    PRUint8 *buf = (PRUint8 *) malloc(len);
    if (!buf)
        return NULL;

    buf[0] = (msg->opcode | (msg->flags << 4));

    memcpy(&buf[1], msg->key, len - 1);
    *bufLen = len;
    return buf;
}

void
IPC_UnflattenLockMsg(const PRUint8 *buf, PRUint32 bufLen, ipcLockMsg *msg)
{
    PR_ASSERT(bufLen > 2); // malformed buffer otherwise
    msg->opcode = get_opcode(buf);
    msg->flags = get_flags(buf);
    msg->key = get_key(buf);
}
