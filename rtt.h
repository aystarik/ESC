#ifndef RTT_H
#define RTT_H
#if 0
static size_t _writes(void *ip, const uint8_t *bp, size_t n) {
  (void)ip;
  return SEGGER_RTT_Write(0, bp, n);
}

static size_t _reads(void *ip, uint8_t *bp, size_t n) {
  (void)ip;
  return SEGGER_RTT_Read(0, bp, n);
}

static msg_t _put(void *ip, uint8_t b) {
  (void)ip;
  SEGGER_RTT_PutChar(0, b);
  return MSG_OK;
}

static msg_t _get(void *ip) {
  uint8_t b;
  (void)ip;
  if (SEGGER_RTT_Read(0, &b, 1) == 1)
    return b;
  return MSG_RESET;
}

struct RTTStreamVMT {
  _base_sequential_stream_methods
};

/**
 * @extends BaseSequentialStream
 *
 * @brief RTT stream object.
 */
struct RTTStream {
  /** @brief Virtual Methods Table.*/
  const struct RTTStreamVMT *vmt;
};

static const struct RTTStreamVMT vmt = {.write = _writes, .read = _reads, .put = _put, .get = _get};

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

static const struct RTTStream rtt = {.vmt = &vmt};
#endif

#endif // RTT_H
