#ifndef UAVCAN_H
#define UAVCAN_H
/*
 * Internal loopback mode, 1000KBaud, automatic wakeup, automatic recover
 * from abort mode.
 */
static const CANConfig cancfg = {
  CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP,
  /*CAN_BTR_LBKM |*/ CAN_BTR_SJW(3) | CAN_BTR_TS2(3) |
  CAN_BTR_TS1(12) | CAN_BTR_BRP(1)
};

#define UID_SIZE (16)
#define UAVCAN_MEM_BLOCK_SIZE (20)
#define MEMORY_POOL_SIZE (64)
struct MemBlock {
    uint8_t data[UAVCAN_MEM_BLOCK_SIZE];
} rxobjects[MEMORY_POOL_SIZE], txobjects[MEMORY_POOL_SIZE];

MEMORYPOOL_DECL(rx_mempool, sizeof(struct MemBlock), 4, NULL);
MEMORYPOOL_DECL(tx_mempool, sizeof(struct MemBlock), 4, NULL);

struct UAVCANTxQueueItem
{
    struct UAVCANTxQueueItem* next;
    CANTxFrame frame;
};

enum TransferType {
    TT_SERVICE_RESP = 0,
    TT_SERVICE_REQ = 1,
    TT_MESSAGE = 2,
};

union TransferDesc {
    uint32_t raw;
    struct {
        uint16_t data_type_id;
        unsigned dst_node_id:7;
        unsigned src_node_id:7;
        enum TransferType tt:2;
    };
};

struct UAVCANBufferBlock {
    struct UAVCANBufferBlock* next;
    uint8_t data[];
};

struct UAVCANRxState {
    struct UAVCANRxState* next;
    struct UAVCANBufferBlock* buffer_blocks;

    unsigned timestamp;

    union TransferDesc dtid_tt_snid_dnid;

    uint16_t payload_crc;
    uint16_t calculated_crc;

    uint16_t payload_len;
    unsigned transfer_id:5;
    bool next_toggle;
};



struct UAVCAN;
typedef bool (*UAVCANAccept_cb)(struct UAVCAN *state,
        unsigned *sign_crc, union TransferDesc tdesc);
typedef void (*UAVCANReceive_cb)(struct UAVCAN *state, struct UAVCANRxState *rxstate);

struct UAVCANRxCallbacks {
  UAVCANAccept_cb accept;
  UAVCANReceive_cb receive;
};

#define UAVCAN_BUFFER_BLOCK_DATA_SIZE (UAVCAN_MEM_BLOCK_SIZE - offsetof(struct UAVCANBufferBlock, data))

#define TRANSFER_TIMEOUT TIME_S2I(2)
#define ID_MASK     (0x7f)

#define ID_SERVICE_BIT  (1U << 7)
#define ID_REQUEST_BIT  (1U << 15)

#define ID_SRC_ID_OFFSET (0)
#define ID_SRC_ID_MASK (ID_MASK << ID_SRC_ID_OFFSET)
#define ID_SRC_ID(x) ((x & ID_SRC_ID_MASK) >> ID_SRC_ID_OFFSET)

#define ID_DEST_ID_OFFSET (8)
#define ID_DEST_ID_MASK (ID_MASK << ID_DEST_ID_OFFSET)
#define ID_DEST_ID(x) ((x & ID_DEST_ID_MASK) >> ID_DEST_ID_OFFSET)

#define ID_MSGTYPE_OFFSET (8)
#define ID_MSGTYPE_MASK (0xffff << ID_MSGTYPE_OFFSET)
#define ID_MSGTYPE(x) ((x & ID_MSGTYPE_MASK) >> ID_MSGTYPE_OFFSET)

#define ANON_MSGTYPE_MASK (0x3)

#define ID_SERVICEID_OFFSET (16)
#define ID_SERVICEID_MASK (0xff << ID_SERVICEID_OFFSET)
#define ID_SERVICEID(x) ((x & ID_SERVICEID_MASK) >> ID_SERVICEID_OFFSET)

#define ID_PRIORITY_OFFSET (24)
#define ID_PRIORITY_MASK    (0x1f << ID_PRIORITY_OFFSET)
#define ID_PRIORITY(x) ((x & ID_PRIORITY_MASK) >> ID_PRIORITY_OFFSET)

#define TAIL_START_BIT  (1 << 7)
#define TAIL_END_BIT    (1 << 6)
#define TAIL_SINGLE_FRAME_MASK  (TAIL_START_BIT|TAIL_END_BIT)
#define TAIL_TOGGLE_BIT (1 << 5)
#define TAIL_TRANSFER_ID_MASK    (0x1f)
#define TDESC(data_type_id, transfer_type, src_node_id, dst_node_id)             \
    ((data_type_id) | (transfer_type) << 16 |                                 \
    (src_node_id) << 18 | (dst_node_id) << 25)

#define UAVCAN_LOW_PRIORITY (31)
#define UAVCAN_ALLOC_OFFSET_TIME (600)
#define UAVCAN_ALLOC_RANGE (400)

struct UAVCAN;

typedef bool (*UAVCANAccept_cb)(struct UAVCAN *ucan, unsigned *crc, union TransferDesc tdesc);
typedef void (*UAVCANReceive_cb)(struct UAVCAN *ucan, struct UAVCANRxState *state);

struct UAVCAN {
    uint8_t uid[UID_SIZE];
    struct UAVCANRxCallbacks *cbs;
    struct UAVCANRxState *rx_states;
    uint8_t node_id;
};

static void uavcanPoolsInit(void) {
  chPoolLoadArray(&rx_mempool, rxobjects, MEMORY_POOL_SIZE);
  chPoolLoadArray(&tx_mempool, txobjects, MEMORY_POOL_SIZE);
}

static void getUniqueId(struct UAVCAN *ucan)
{
    uint32_t *ptr = (uint32_t *)ucan->uid;
    uint32_t *uid_ptr = (uint32_t *)UID_BASE;
    for (unsigned i = 0; i < UID_SIZE / 4; ++i)
        *ptr++ = *uid_ptr++;
}

static void crc16Init(void) {
   rccEnableCRC(FALSE);
   CRC->INIT = 0xFFFFU;
   CRC->POL = 0x1021U;
   CRC->CR = CRC_CR_POLYSIZE_0 | CRC_CR_RESET;
}

static uint16_t crcAdd(uint16_t crc_val, const uint8_t *bytes, size_t len)
{
   CRC->INIT = crc_val;
   __IO uint8_t *DR8 = (__IO uint8_t *)&CRC->DR;
   while (len--)
      *DR8 = *bytes++;
   return CRC->DR;
}

void uavcanSendReqNodeId(struct UAVCAN *ucan, unsigned preferred_id);
int uavcanAcquireNodeId(struct UAVCAN *ucan, unsigned preferred_id);
void uavcanSendNodeStatus(struct UAVCAN *ucan);
void uavcanHandleRxFrame(struct UAVCAN *ucan, CANRxFrame *frame, systime_t timestamp);
/*
uavcan.equipment.esc.RPMCommand                         1031   0xce0f9f621cf7e70b     365
uavcan.equipment.esc.RawCommand                         1030   0x217f5c87d7ec951d     285
uavcan.equipment.esc.Status                             1034   0xa9af28aea2fbb254     110
uavcan.protocol.GetNodeInfo                                1   0xee468a8121c46a9e       0 / 3015
*/
#define UAVCAN_ESC_RPM_COMMAND_SIGN_CRC (0xf8b4)
#define UAVCAN_ESC_RAW_COMMAND_SIGN_CRC (0xe4b8)
#define UAVCAN_ESC_STATUS_SIGN_CRC (0x1591)

#define UAVCAN_GET_NODE_INFO_SIGN_CRC  (0xd9a7)
#define UAVCAN_GET_SET_SIGN_CRC  (0xfb10)

static bool workAccept(struct UAVCAN *ucan, unsigned *crc, union TransferDesc tdesc)
{
  (void)ucan;
  if (tdesc.tt == TT_SERVICE_REQ) {
      if (tdesc.data_type_id == 1) {
        *crc = UAVCAN_GET_NODE_INFO_SIGN_CRC;
        return true;
      }
      if (tdesc.data_type_id == 11) {
        *crc = UAVCAN_GET_SET_SIGN_CRC;
        return true;
      }
  }
  return false;
}

void uavcanSendNodeInfo(struct UAVCAN *ucan, unsigned dest_id);
static void uavcanGetSet(struct UAVCAN *ucan, struct UAVCANRxState *state);

static void workReceive(struct UAVCAN *ucan, struct UAVCANRxState *state)
{
    if (state->dtid_tt_snid_dnid.tt == TT_SERVICE_REQ) {
        if (state->dtid_tt_snid_dnid.data_type_id == 1) {
            uavcanSendNodeInfo(ucan, state->dtid_tt_snid_dnid.src_node_id);
        }
        if (state->dtid_tt_snid_dnid.data_type_id == 11) {
            uavcanGetSet(ucan, state);
        }
    }
}
struct UAVCANRxCallbacks workCbs = {
    .accept = workAccept,
    .receive = workReceive
};

/*
 * Receiver thread.
 */
static THD_WORKING_AREA(can_rx_wa, 256);
static THD_FUNCTION(can_rx, p) {
#if 1
    struct UAVCAN *ucan = (struct UAVCAN *)p;

  chRegSetThreadName("receiver");
  uavcanAcquireNodeId(ucan, 0);
  ucan->cbs = &workCbs;
  event_listener_t el;
  chEvtRegister(&CAND1.rxfull_event, &el, 0);
  while (true) {
    systime_t start = chVTGetSystemTime();
    systime_t end = chTimeAddX(start, TIME_MS2I(1000));
    while (chVTIsSystemTimeWithin(start, end)) {
        if (chEvtWaitAnyTimeout(ALL_EVENTS, TIME_MS2I(10)) == 0)
          continue;
        CANRxFrame rxmsg;
        while (canReceive(&CAND1, CAN_ANY_MAILBOX, &rxmsg, TIME_IMMEDIATE) == MSG_OK) {
          /* Process message.*/
            uavcanHandleRxFrame(ucan, &rxmsg, chVTGetSystemTime());
        }
    }
    uavcanSendNodeStatus(ucan);
  }
  chEvtUnregister(&CAND1.rxfull_event, &el);
#endif
}

static struct UAVCANTxQueueItem *tx_queue = NULL;
MUTEX_DECL(tx_mtx);
CONDVAR_DECL(tx_cv);

static void pushTxQueue(struct UAVCAN *ucan, struct UAVCANTxQueueItem* item)
{
    (void)ucan;
    chMtxLock(&tx_mtx);
    item->next = NULL;
    struct UAVCANTxQueueItem** queue = &tx_queue;
    while (*queue) {
        if ((*queue)->frame.EID > item->frame.EID) { // lower number wins
            item->next = *queue;
            *queue = item;
            return;
        }
        queue = &((*queue)->next);
    }
    *queue = item;
    chCondSignal(&tx_cv);
    chMtxUnlock(&tx_mtx);
}

static inline void bumpTransferId(unsigned* transfer_id)
{
    *transfer_id += 1;
    *transfer_id &= 0x1f;
}

int enqueueTxFrame(struct UAVCAN *ucan,
                   uint32_t can_id,
                   unsigned* transfer_id,
                   const uint8_t* payload,
                   unsigned payload_len)
{
    //assert(payload_len <= 7);
    struct UAVCANTxQueueItem* queue_item = (struct UAVCANTxQueueItem*)chPoolAlloc(&tx_mempool);
    if (!queue_item)
        return -1;
    for (unsigned i = 0; i < payload_len; ++i)
        queue_item->frame.data8[i] = *payload++;
    queue_item->frame.DLC = payload_len + 1;
    queue_item->frame.data8[payload_len] = TAIL_START_BIT | TAIL_END_BIT | *transfer_id;
    queue_item->frame.EID = can_id;
    queue_item->frame.IDE = CAN_IDE_EXT;
    pushTxQueue(ucan, queue_item);
    bumpTransferId(transfer_id);
    return 0;
}

static int uavcanBroadcastSingle(struct UAVCAN *ucan,
                    unsigned data_type_id,
                    unsigned* inout_transfer_id,
                    unsigned priority,
                    const uint8_t *payload,
                    unsigned payload_len)
{
    //assert(payload_len <= 7);
    uint32_t can_id = (priority << 24) | (data_type_id << 8) | ucan->node_id;
    if (!ucan->node_id) {
        //assert(data_type_id < 4);
        // anonymous transfer, random discriminator
        can_id |= (crcAdd(0xFFFFU, payload, payload_len) & 0x7FFEU) << 9;
    }
    return enqueueTxFrame(ucan, can_id, inout_transfer_id, payload, payload_len);
}

static void pushTxQueueMulti(struct UAVCAN *ucan, struct UAVCANTxQueueItem* head, struct UAVCANTxQueueItem *tail)
{
    (void)ucan;
    chMtxLock(&tx_mtx);
    struct UAVCANTxQueueItem** queue = &tx_queue;
    while (*queue) {
        if ((*queue)->frame.EID > head->frame.EID) { // lower number wins
            tail->next = *queue;
            *queue = head;
            return;
        }
        queue = &((*queue)->next);
    }
    *queue = head;
    tail->next = NULL;
    chCondSignal(&tx_cv);
    chMtxUnlock(&tx_mtx);
}

int enqueueTxFrames(struct UAVCAN *ucan,
                    uint32_t can_id,
                    unsigned* transfer_id,
                    uint16_t crc,
                    const uint8_t* payload,
                    uint16_t payload_len)
{
    struct UAVCANTxQueueItem* head = (struct UAVCANTxQueueItem*)chPoolAlloc(&tx_mempool);
    if (!head)
            return -1;
    head->frame.data16[0] = crc;
    unsigned i = 2;
    payload_len += 2;
    unsigned toggle = 0;
    struct UAVCANTxQueueItem* tail = head;
    while (payload_len) {
        unsigned data_len = payload_len;
        if (data_len > 7)
            data_len = 7;
        for (;i < data_len; ++i)
            tail->frame.data8[i] = *payload++;
        payload_len -= i;
        tail->frame.data8[i] = (toggle << 5) | *transfer_id;
        tail->frame.EID = can_id;
        tail->frame.IDE = CAN_IDE_EXT;
        tail->frame.DLC = i + 1;
        i = 0;
        if (!payload_len)
            break;
        toggle ^= 1;
        tail->next = (struct UAVCANTxQueueItem*)chPoolAlloc(&tx_mempool);
        if (!tail->next) {
            while (head) {
                struct UAVCANTxQueueItem *ptr = head->next;
                chPoolFree(&tx_mempool, head);
                head = ptr;
            }
            return -1;
        }
        tail = tail->next;
    }
    head->frame.data8[head->frame.DLC - 1] |= TAIL_START_BIT;
    tail->frame.data8[tail->frame.DLC - 1] |= TAIL_END_BIT;
    pushTxQueueMulti(ucan, head, tail);
    bumpTransferId(transfer_id);
    return 0;
}
#if 0
static int uavcanBroadcastMulti(struct UAVCAN *ucan,
                    unsigned data_type_signature_crc,
                    unsigned data_type_id,
                    unsigned* inout_transfer_id,
                    unsigned priority,
                    const uint8_t *payload,
                    unsigned payload_len)
{
    //assert(uci->node_id);
    uint32_t can_id = (priority << 24) | (data_type_id << 8) | ucan->node_id;
    unsigned crc = crcAdd(data_type_signature_crc, payload, payload_len);
    return enqueueTxFrames(ucan, can_id, inout_transfer_id, crc, payload, payload_len);
}
#endif
static int uavcanServiceReplySingle(struct UAVCAN *ucan,
                    unsigned data_type_id,
                    unsigned destination_id,
                    unsigned* inout_transfer_id,
                    unsigned priority,
                    const uint8_t *payload,
                    unsigned payload_len)
{
    //assert(uci->node_id);
    uint32_t can_id = (priority << 24) | (data_type_id << 16) |
        (destination_id << 8) | (1U << 7) | ucan->node_id;
    return enqueueTxFrame(ucan, can_id, inout_transfer_id, payload, payload_len);
}

static int uavcanServiceReply(struct UAVCAN *ucan,
                    unsigned data_type_signature_crc,
                    unsigned data_type_id,
                    unsigned destination_id,
                    unsigned* inout_transfer_id,
                    unsigned priority,
                    const uint8_t *payload,
                    unsigned payload_len)
{
    //assert(uci->node_id);
    uint32_t can_id = (priority << 24) | (data_type_id << 16) |
        (destination_id << 8) | (1U << 7) | ucan->node_id;
    unsigned crc = crcAdd(data_type_signature_crc, payload, payload_len);
    return enqueueTxFrames(ucan, can_id, inout_transfer_id, crc, payload, payload_len);
}

struct UAVCANTxQueueItem* uavcanPopTxQueue(struct UAVCAN* ucan)
{
  (void)ucan;
  chMtxLock(&tx_mtx);
  while (!tx_queue) {
    chCondWait(&tx_cv);
  }
  struct UAVCANTxQueueItem *item = tx_queue;
  tx_queue = item->next;
  chMtxUnlock(&tx_mtx);
  return item;
}

void releaseTxQueueItem(struct UAVCANTxQueueItem *item)
{
  chPoolFree(&tx_mempool, item);
}

/*
 * Transmitter thread.
 */
static THD_WORKING_AREA(can_tx_wa, 256);
static THD_FUNCTION(can_tx, ctx) {
  struct UAVCAN *ucan = (struct UAVCAN *)ctx;
  chRegSetThreadName("uavcan_tx");
  while (true) {
    struct UAVCANTxQueueItem *item = uavcanPopTxQueue(ucan);
    canTransmit(&CAND1, CAN_ANY_MAILBOX, &item->frame, TIME_MS2I(100));
    releaseTxQueueItem(item);
  }
}

// node allocator code
static unsigned unique_id_offset = 0;
void uavcanSendReqNodeId(struct UAVCAN *ucan, unsigned preferred_id) {
    static unsigned transfer_id = 0;
    uint8_t packet[8];
    packet[0] = (unique_id_offset)? 0 : (1 | ((preferred_id & 0x7f) << 1));
    unsigned len = UID_SIZE - unique_id_offset;
    if (len > 6)
        len = 6;
    for (unsigned i = 0; i < len; ++i)
        packet[i + 1] = ucan->uid[unique_id_offset + i];
    uavcanBroadcastSingle(ucan, 1, &transfer_id, UAVCAN_LOW_PRIORITY, packet, len + 1);
}

#define UAVCAN_ALLOCATE_TYPE_ID (1U)
#define UAVCAN_ALLOCATE_SIGNATURE_CRC (0xf258)

static bool allocNodeAccept(struct UAVCAN *ucan, unsigned *crc, union TransferDesc tdesc)
{
  (void)ucan;
  if (tdesc.tt == TT_MESSAGE &&
      tdesc.data_type_id == UAVCAN_ALLOCATE_TYPE_ID) {
    *crc = UAVCAN_ALLOCATE_SIGNATURE_CRC;
    return true;
  }
  return false;
}

static unsigned req_send_delay = UAVCAN_ALLOC_OFFSET_TIME;

void copyBitArray(uint8_t *dst, unsigned dst_offset,
                  const uint8_t *src, unsigned src_offset,
                  unsigned len) {
    dst += dst_offset >> 3;
    dst_offset &= 7;

    src += src_offset >> 3;
    src_offset &= 7;

    while (len) {
        unsigned max_offset = src_offset;
        if (dst_offset > max_offset)
            max_offset = dst_offset;
        unsigned bits  = 8 - max_offset;
        if (bits > len)
            bits = len;
        unsigned shift = 8 - (src_offset + bits);
        unsigned sel_src = (*src >> shift) & ((1U << bits) - 1);
        shift = 8 - (dst_offset + bits);
        const unsigned shift_src = sel_src << shift;

        *dst |= shift_src;

        dst_offset += bits;
        src_offset += bits;

        dst += dst_offset >> 3;
        dst_offset &= 7;

        src += src_offset >> 3;
        src_offset &= 7;

        len -= bits;
    }
}

static unsigned gatherPayload(const struct UAVCANRxState* state,
                              unsigned bit_offset,
                              unsigned bit_length,
                              uint8_t* output)
{
    unsigned remaining_bits = state->payload_len * 8U;

    if (bit_offset >= remaining_bits)
        return 0;
    if (bit_length > remaining_bits - bit_offset)
        bit_length = remaining_bits - bit_offset;
    unsigned output_bit_offset = 0;
    unsigned remaining_bit_length = bit_length;
    const struct UAVCANBufferBlock* block = state->buffer_blocks;

    while (remaining_bit_length) {
        while (bit_offset > UAVCAN_BUFFER_BLOCK_DATA_SIZE * 8) {
            bit_offset -= UAVCAN_BUFFER_BLOCK_DATA_SIZE * 8;
            remaining_bits -= UAVCAN_BUFFER_BLOCK_DATA_SIZE * 8;
            block = block->next;
        }
        unsigned amount = UAVCAN_BUFFER_BLOCK_DATA_SIZE * 8 - bit_offset;
        if (amount > remaining_bits)
            amount = remaining_bits;
        if (amount > remaining_bit_length)
            amount = remaining_bit_length;
        copyBitArray(output, output_bit_offset, &block->data[0], bit_offset, amount);
        bit_offset += amount;
        output_bit_offset += amount;
        remaining_bit_length -= amount;
    }
    return bit_length;
}

int decodeUnsigned(const struct UAVCANRxState *state,
                   unsigned bit_offset, unsigned bit_length,
                   uint8_t *bytes)
{
    unsigned result = gatherPayload(state, bit_offset, bit_length, bytes);
    if (bit_length & 7)
        bytes[bit_length >> 3] >>= 8 - (bit_length & 7);
    return result;
}

static void allocNodeReceive(struct UAVCAN *ucan, struct UAVCANRxState *state)
{
    if (state->dtid_tt_snid_dnid.tt != TT_MESSAGE ||
        state->dtid_tt_snid_dnid.data_type_id != 1)
        return;
    uint8_t node_id = 0;
    uint8_t first_part = 0;
    uint8_t unique_id[16] = {0};
    decodeUnsigned(state, 0, 7, &node_id);
    decodeUnsigned(state, 7, 1, &first_part);
    unsigned len = gatherPayload(state, 8, 16 * 8, unique_id);
    len >>= 3;
    req_send_delay = UAVCAN_ALLOC_OFFSET_TIME;
    if (!state->dtid_tt_snid_dnid.src_node_id)
        return;
    if (memcmp(ucan->uid, unique_id, len))
        return;
    if (len == 16)
        ucan->node_id = node_id;
    else {
        unique_id_offset = len;
        req_send_delay = 0;
    }
}

struct UAVCANRxCallbacks allocCbs = {
    .accept = allocNodeAccept,
    .receive = allocNodeReceive
};
static enum TransferType extractTransferType(uint32_t id)
{
    if (!(id & ID_SERVICE_BIT))
        return TT_MESSAGE;
    if (id & ID_REQUEST_BIT)
        return TT_SERVICE_REQ;
    return TT_SERVICE_RESP;
}

static unsigned extractDataType(uint32_t id)
{
    if (id & ID_SERVICE_BIT)
        return ID_SERVICEID(id);
    unsigned dtid = ID_MSGTYPE(id);
    if (!ID_SRC_ID(id))
        dtid &= ANON_MSGTYPE_MASK;
    return dtid;
}

static unsigned transferIdDistance(unsigned a, unsigned b)
{
    return (b - a) & TAIL_TRANSFER_ID_MASK;
}

static struct UAVCANRxState* findRxState(struct UAVCANRxState* state, union TransferDesc td)
{
    while (state != NULL) {
        if (state->dtid_tt_snid_dnid.raw == td.raw)
            return state;
        state = state->next;
    }
    return NULL;
}

static int bufferBlockPushBytes(struct UAVCANRxState* state,
                                const uint8_t* data,
                                unsigned data_len)
{
    unsigned block_offset = state->payload_len;
    struct UAVCANBufferBlock **bptr = &state->buffer_blocks;
    while (data_len) {
        // get to current block
        while (block_offset >= UAVCAN_BUFFER_BLOCK_DATA_SIZE) {
            bptr = &((*bptr)->next);
            block_offset -= UAVCAN_BUFFER_BLOCK_DATA_SIZE;
        }
        if (*bptr == NULL) {
            *bptr = (struct UAVCANBufferBlock*) chPoolAlloc(&rx_mempool);
            if (!*bptr)
                return -3;
            (*bptr)->next = NULL;
        }
        while (block_offset < UAVCAN_BUFFER_BLOCK_DATA_SIZE && data_len) {
            (*bptr)->data[block_offset++] = *data++;
            --data_len;
            ++state->payload_len;
        }
    }
    return 0;
}

static void releaseStatePayload(struct UAVCANRxState *rx_state)
{
    while (rx_state->buffer_blocks) {
        struct UAVCANBufferBlock *blk = rx_state->buffer_blocks->next;
        chPoolFree(&rx_mempool, rx_state->buffer_blocks);
        rx_state->buffer_blocks = blk;
    }
    rx_state->payload_len = 0;
}

void uavcanHandleRxFrame(struct UAVCAN *ucan, CANRxFrame *frame, systime_t timestamp)
{
    union TransferDesc tdesc;
    tdesc.tt = extractTransferType(frame->EID);
    tdesc.dst_node_id = (tdesc.tt == TT_MESSAGE) ? 0 : ID_DEST_ID(frame->EID);
    tdesc.src_node_id = ID_SRC_ID(frame->EID);
    tdesc.data_type_id = extractDataType(frame->EID);

    struct UAVCANRxState* rx_state = findRxState(ucan->rx_states, tdesc);

    unsigned tail = frame->data8[frame->DLC - 1];
    bool start_frame = tail & TAIL_START_BIT;
    bool end_frame = tail & TAIL_END_BIT;
    if (start_frame) {
        unsigned data_type_signature_crc = 0;
        if (!ucan->cbs->accept(ucan, &data_type_signature_crc, tdesc))
            return;      // The application doesn't want this transfer
        if (!rx_state) {
            rx_state = (struct UAVCANRxState*) chPoolAlloc(&rx_mempool);
            if (!rx_state)
                return;
            rx_state->timestamp = 0;
            rx_state->dtid_tt_snid_dnid = tdesc;
            rx_state->buffer_blocks = NULL;
            rx_state->next = ucan->rx_states;
            ucan->rx_states = rx_state;
        }
        rx_state->calculated_crc = data_type_signature_crc;
    }

    if (!rx_state)
        return;

    unsigned transfer_id = tail & TAIL_TRANSFER_ID_MASK;
    unsigned offset = 0;

    bool toggle = tail & TAIL_TOGGLE_BIT;
    if ((!rx_state->timestamp) ||
        !chTimeIsInRangeX(timestamp, rx_state->timestamp, chTimeAddX(rx_state->timestamp, TRANSFER_TIMEOUT)) ||
        (start_frame && transferIdDistance(rx_state->transfer_id, transfer_id) > 1)) {
        rx_state->transfer_id = transfer_id;
        rx_state->next_toggle = 0;
        releaseStatePayload(rx_state);
        if (!start_frame) // missed the first frame
            goto cleanup_state;
        else
            rx_state->timestamp = timestamp;
    }

    if (toggle != rx_state->next_toggle || transfer_id != rx_state->transfer_id)
        goto cleanup_state;
    if (start_frame && !end_frame) {
        offset = 2;
        if (frame->DLC < 3)
            goto cleanup_state;
        rx_state->payload_crc = *frame->data16;
    }
    if (bufferBlockPushBytes(rx_state, frame->data8 + offset, frame->DLC - 1 - offset) < 0)
        goto cleanup_state;
    if (!start_frame || !end_frame)
        rx_state->calculated_crc = crcAdd(rx_state->calculated_crc, frame->data8 + offset, frame->DLC - 1 - offset);
    if (end_frame) {
        if (start_frame || rx_state->calculated_crc == rx_state->payload_crc)
            ucan->cbs->receive(ucan, rx_state);
        goto cleanup_state;
    }
    rx_state->next_toggle = !rx_state->next_toggle;
    return;
cleanup_state:
    releaseStatePayload(rx_state);
    ++rx_state->transfer_id;
    rx_state->next_toggle = 0;
    return;
}

int uavcanAcquireNodeId(struct UAVCAN *ucan, unsigned preferred_id)
{
  ucan->cbs = &allocCbs;
  req_send_delay = UAVCAN_ALLOC_OFFSET_TIME;
  //allocNodeFilters(ts.sock);
  while (!ucan->node_id) {
    systime_t start = chVTGetSystemTime();
    systime_t end = chTimeAddX(start, TIME_MS2I(req_send_delay + UAVCAN_ALLOC_RANGE / 2));
//      (rand() * UAVCAN_ALLOC_RANGE) / RAND_MAX;
    while (chVTIsSystemTimeWithin(start, end)) {
        CANRxFrame rxmsg;
        if (canReceive(&CAND1, CAN_ANY_MAILBOX, &rxmsg, TIME_MS2I(10)) == MSG_OK) {
            uavcanHandleRxFrame(ucan, &rxmsg, chVTGetSystemTime());
        }
        if (ucan->node_id)
            return 0;
    }
    uavcanSendReqNodeId(ucan, preferred_id);
    unique_id_offset = 0;
  }
  return 0;
}

static enum UAVCANHealth {
    HEALTH_OK = 0,
    HEALTH_WARNING = 1,
    HEALTH_ERROR = 2,
    HEALTH_CRITICAL = 3
} health = HEALTH_OK;

static enum UAVCANMode {
    MODE_OPERATIONAL      = 0,
    MODE_INITIALIZATION   = 1,
    MODE_MAINTENANCE      = 2,
    MODE_SOFTWARE_UPDATE  = 3,
    MODE_OFFLINE          = 7,
} mode = MODE_INITIALIZATION;


void encodeUnsigned(uint8_t *buf, unsigned *bit_offset, unsigned bit_length, void *data)
{
    uint8_t *bytes = (uint8_t *)data;
    if (bit_length & 7)
        bytes[bit_length >> 3] <<= 8 - (bit_length & 7);
    copyBitArray(buf, *bit_offset, bytes, 0, bit_length);
    if (bit_length & 7) // revert
        bytes[bit_length >> 3] >>= 8 - (bit_length & 7);
    *bit_offset += bit_length;
}

void uavcanSendNodeStatus(struct UAVCAN *ucan)
{
    static unsigned transfer_id = 0;
    uint8_t packet[8] = {0};
    uint32_t uptime = TIME_I2S(chVTGetSystemTime());
    for (unsigned i = 0; i < sizeof(uptime); ++i) {
        packet[i] = uptime & 0xff;
        uptime >>= 8;
    }
    unsigned bit_offset = 4 * 8;
    encodeUnsigned(packet, &bit_offset, 2, &health);
    encodeUnsigned(packet, &bit_offset, 3, &mode);
    uavcanBroadcastSingle(ucan, 341, &transfer_id, UAVCAN_LOW_PRIORITY, packet, 7);
}

void uavcanSendNodeInfo(struct UAVCAN *ucan, unsigned dest_id)
{
    static unsigned transfer_id = 0;
    uint8_t packet[64];
    // NodeStatus
    uint32_t uptime = TIME_I2S(chVTGetSystemTime());
    unsigned i = 0;
    for (; i < sizeof(uptime); ++i) {
        packet[i] = uptime & 0xff;
        uptime >>= 8;
    }
    packet[i++] = (((unsigned)health) & 3) << 6 | (((unsigned)mode) & 3) << 3;
    packet[i++] = 0;
    packet[i++] = 0;
    // SoftwareVersion
    packet[i++] = 0; // major
    packet[i++] = 1; // minor
    packet[i++] = 0; // no optional fields : no VCS, no CRC
    for (unsigned k = 0; k < 12; ++k)
        packet[i++] = 0;
    // HardwareVersion
    packet[i++] = 1; // major
    packet[i++] = 1; // minor
    for (unsigned k = 0; k < UID_SIZE; ++k)
        packet[i++] = ucan->uid[k];
    packet[i++] = 0; // no certificate
    static const uint8_t name[] = "com.topcon.drone.esc";
    for (unsigned k = 0; k < sizeof(name) - 1; ++k)
        packet[i++] = name[k];
    uavcanServiceReply(ucan, UAVCAN_GET_NODE_INFO_SIGN_CRC, 1, dest_id, &transfer_id,
        UAVCAN_LOW_PRIORITY, packet, i);
}

static int64_t esc_idx = 20;
static int64_t motor_poles = 7;

enum UAVCANValueType {
    UAVCAN_EMPTY = 0,
    UAVCAN_INTEGER = 1,
    UAVCAN_FLOAT = 2,
    UAVCAN_BOOL = 3,
    UAVCAN_STRING = 4,
};

static struct UAVCANParameter {
    enum UAVCANValueType type;
    const char *name;
    uint8_t *ptr;
} params[] = {
    {.type = UAVCAN_INTEGER, "index", (uint8_t *)&esc_idx},
    {.type = UAVCAN_INTEGER, "poles", (uint8_t *)&motor_poles},
};

#define UAVCAN_NUM_PARAMS (sizeof(params)/sizeof(struct UAVCANParameter))

static void uavcanGetSet(struct UAVCAN *ucan, struct UAVCANRxState *state)
{
    static unsigned transfer_id = 0;
    uint8_t packet[64];
    uint16_t param_idx = 0;
    if (gatherPayload(state, 0, 16, (uint8_t *)&param_idx) != 16)
        return;
    unsigned param_type = (param_idx >> 13) & 7;
    param_idx  &= 0x1fff;
    unsigned i = 0;
    if (param_idx < UAVCAN_NUM_PARAMS) {
        // send actual param
        packet[i++] = params[param_idx].type;
        switch (params[param_idx].type) {
        case UAVCAN_INTEGER:
            for (unsigned k = 0; k < 8; ++k)
                packet[i++] = params[param_idx].ptr[k];
            break;
        case UAVCAN_FLOAT:
            for (unsigned k = 0; k < 4; ++k)
                packet[i++] = params[param_idx].ptr[k];
            break;
        case UAVCAN_BOOL:
            packet[i++] = *(bool *)params[param_idx].ptr;
            break;
        case UAVCAN_STRING: {
            unsigned len = strlen((char *)params[param_idx].ptr);
            packet[i++] = len;
            for (unsigned k = 0; k < len; ++k)
                packet[i++] = params[param_idx].ptr[k];
            }
            break;
        default:
            return;
        }
        // skip optional default, min and max
        for (unsigned k = 0; k < 3; ++k)
            packet[i++] = 0;

        unsigned len = strlen(params[param_idx].name);
        for (unsigned k = 0; k < len; ++k)
            packet[i++] = params[param_idx].name[k];
        if (param_type == params[param_idx].type) {
            if (param_type == UAVCAN_INTEGER) {
                int64_t tmp = 0;
                gatherPayload(state, 16, 64, (uint8_t *)&tmp);
                *(int64_t *)params[param_idx].ptr = tmp;
            }
        }
    } else {
        // send empty
        for (; i < 4; ++i)
            packet[i] = 0;
    }
    if (i < 8)
        uavcanServiceReplySingle(ucan, 11,
            state->dtid_tt_snid_dnid.src_node_id, &transfer_id,
            UAVCAN_LOW_PRIORITY, packet, i);
    else
        uavcanServiceReply(ucan, UAVCAN_GET_SET_SIGN_CRC, 11,
            state->dtid_tt_snid_dnid.src_node_id, &transfer_id,
            UAVCAN_LOW_PRIORITY, packet, i);

}

#endif // UAVCAN_H
