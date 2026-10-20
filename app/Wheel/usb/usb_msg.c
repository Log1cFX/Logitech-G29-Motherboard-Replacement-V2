/*
 * usb_msg.c
 *
 *  Created on: Oct 13, 2026
 *      Author: raffi
 *
 *  The file link to the PC: the buffer that holds the files of both
 *  directions, and the protocol that carries them in blocks.
 *  See usb_msg.h for the interface. Nothing of TinyUSB nor of the HAL is used:
 *  usb_processing.c and usb_callbacks.c move the blocks and give the time
 */

#include "usb_internal.h"
#include <assert.h>
#include <string.h>

/*
 * The protocol. Both sides follow the same rules. "PC" is the application of
 * the PC that opened the interface.
 *
 * BLOCKS
 * A block is one report of USB_MSG_BLOCK_SIZE bytes. Its first byte holds
 * BLOCK_START, BLOCK_END and the number of data bytes that follow it. What is
 * left of the block is not used.
 * A message is the data of one or more blocks, from a BLOCK_START to a
 * BLOCK_END. Only its last block may hold less than BLOCK_MAX_DATA bytes, and
 * no more than TIMEOUT_MS pass between two of its blocks.
 *
 * MESSAGES
 * Told apart by their length:
 *   8 bytes or more   a file: wheel_file_header_t, then the content without
 *                     its padding
 *   SYNC_SIZE         MAGIC, sent by the PC to connect
 *   ACK_SIZE          ACK_CODE, then the crc field of the acknowledged file
 *   NACK_SIZE         NACK_CODE, a nack_reason_t, then the same crc field
 *
 * HANDSHAKES
 * An ACK or a NACK, like the handshake packet of USB. The receiver of a file
 * sends ACK once the file is stored, and NACK when the message is complete
 * but its content is refused. It sends no handshake, and drops the message,
 * when the blocks break the rules above or when it has no room for the file.
 * A sync gets its handshake the same way, with MAGIC in place of the crc field.
 * A handshake is a message of its own: it goes between two files, never between
 * two blocks of one. It is sent once and gets no handshake itself.
 *
 * SENDING A FILE
 * One file at a time: the file in flight. Once its last block is sent, the
 * sender waits for the handshake that carries the crc field of the file, and
 * ignores any other.
 * ACK: the file is delivered. NACK: the file is dropped. Nothing for
 * TIMEOUT_MS: the file is sent again from its first block, MAX_TRIES in total.
 *
 * CONNECTION
 * Files only travel while connected: from a sync until a file went MAX_TRIES
 * without a handshake, or until the bus leaves the configured state.
 * Not connected, the outgoing files wait in the buffer, the one without a
 * handshake included, and a file of the PC gets NACK_NOT_CONNECTED.
 * A sync while connected starts the file in flight again.
 */

// First byte of a block
#define BLOCK_START 0x80u // first block of a message
#define BLOCK_END 0x40u // last block of a message
#define BLOCK_COUNT_MASK 0x3Fu // data bytes of the block
#define BLOCK_MAX_DATA (USB_MSG_BLOCK_SIZE - 1u)

#define SYNC_SIZE 4u
#define ACK_SIZE 5u
#define NACK_SIZE 6u
#define ACK_CODE 0x06u // ASCII ACK
#define NACK_CODE 0x15u // ASCII NAK
// On the cable: 'G' '2' '9' 0x01, in this order. The word is these 4 bytes
// read little-endian: 'G' is its low byte. It never changes, the version is
// the one of every file (wheel_version.h)
#define MAGIC 0x01393247u

// Sent in a NACK: the values are part of the protocol
typedef enum {
  NACK_NONE, // never sent: no NACK is owed
  NACK_NOT_CONNECTED, // a file before the sync
  NACK_BAD_MAGIC, // a sync that is not MAGIC
  NACK_MALFORMED, // the size of the file header is not the size received
  NACK_TOO_BIG, // content over USB_MSG_MAX_CONTENT_SIZE
  // What wheel_file_check() found, see reason_of()
  NACK_BAD_CRC,
  NACK_UNKNOWN_TYPE,
  NACK_BAD_VERSION,
  NACK_BAD_SIZE // a content size its type does not allow
} nack_reason_t;

#define TIMEOUT_MS 250u // for a handshake, and between two blocks of a message
#define MAX_TRIES 4u // sends of a file before the connection is given up

#define FILE_HEADER_SIZE sizeof(wheel_file_header_t)

/*
 * BUFFER
 * Cut into pieces that follow each other from its start to its end. A piece is
 * a piece_t, then a file as it is in flash: header, content, zeros up to a
 * multiple of 4 bytes. Every piece starts on a multiple of 4 bytes.
 * A file goes to the first free piece that is big enough, so its place says
 * nothing of its age: the order is kept by two chains, one per direction.
 */
typedef struct {
  uint16_t size; // of the piece in bytes, this struct included
  uint16_t next; // offset of the next piece of its chain, NO_PIECE or FREE_PIECE
} piece_t;
// next does not only serve the purpose of indicating the offset of the next block
// in its chain (IN or OUT). It's also used as a flag that indicates that the
// block is free (in case of FREE_PIECE)

#define NO_PIECE 0xFFFFu // end of a chain, or no piece at all
#define FREE_PIECE 0xFFFEu

static_assert(USB_MSG_BUFFER_SIZE % 4 == 0 && USB_MSG_BUFFER_SIZE < FREE_PIECE,
    "USB_MSG_BUFFER_SIZE does not fit the offsets of piece_t");

// The files of one direction, from the first that arrived to the last. Both
// fields are offsets of pieces, NO_PIECE when the chain is empty
typedef struct {
  uint16_t oldest;
  uint16_t newest;
} chain_t;

// What is done with the blocks of the PC
typedef enum {
  RX_IDLE, // no message is open
  RX_FILLING, // the blocks of a file are copied to rx_piece
  RX_SKIPPING // the blocks are dropped until the end of the message
} rx_state_t;

// Main only, like the rest of the USB module (see usb_processing.c)
static struct {
  // On a multiple of 4 bytes: its pieces are read as piece_t and wheel_file_t
  _Alignas(4) uint8_t buffer[USB_MSG_BUFFER_SIZE];
  chain_t outgoing; // files for the PC
  chain_t incoming; // files of the PC, for usb_msg_peek()
  bool connected; // see CONNECTION above

  // Outgoing. The file in flight is always the oldest of its chain
  bool tx_in_flight; // from its first block given to TinyUSB until its handshake
  uint16_t tx_offset; // bytes given to TinyUSB in this try
  uint8_t tx_tries; // tries that got no handshake
  uint32_t tx_time; // of the last block given, or of the start of this try
  // What usb_msg_next_block() built last, for usb_msg_block_sent()
  bool built_handshake;
  uint8_t built_count; // data bytes of the file, when it is not a handshake

  // Incoming. The message being received
  rx_state_t rx_state;
  uint16_t rx_piece; // RX_FILLING: taken, not in a chain yet
  uint16_t rx_size; // of the file, from its header
  uint16_t rx_received;
  uint32_t rx_crc; // crc field of its header, for the handshake
  nack_reason_t rx_reason; // RX_SKIPPING: the NACK to send at the end
  uint32_t rx_time; // of the last block

  // Handshake owed to the PC: 0, ACK_CODE or NACK_CODE
  uint8_t handshake;
  uint8_t handshake_reason;
  uint32_t handshake_crc;

  // For the debugger
  uint32_t tx_refused; // outgoing files that got a NACK
  uint8_t tx_last_reason; // nack_reason_t of the last one
  uint32_t rx_refused; // incoming messages that got a NACK
  uint32_t rx_dropped; // incoming blocks and messages left without a handshake
} msg;

// The piece that starts offset bytes after the start of the buffer
static piece_t* piece_at(uint16_t offset) {
  return (piece_t*)&msg.buffer[offset];
}

// The file stored in a piece: it starts right after the piece_t
static wheel_file_t* file_at(uint16_t offset) {
  return (wheel_file_t*)&msg.buffer[offset + sizeof(piece_t)];
}

// The same file as the bytes that travel: its header, then its content
static uint8_t* bytes_at(uint16_t offset) {
  return &msg.buffer[offset + sizeof(piece_t)];
}

// Takes a piece for a file of file_size bytes, header and content. Returns its
// offset, NO_PIECE when no free piece is big enough.
// The piece is in no chain yet: it goes to chain_add(), or back to piece_free()
static uint16_t piece_take(uint16_t file_size) {
  // The piece_t, then the file with its padding
  uint16_t needed = sizeof(piece_t) + WHEEL_FILE_PADDED_SIZE(file_size);

  // From the first piece to the last: a piece ends where the next one starts
  for (uint16_t offset = 0; offset < USB_MSG_BUFFER_SIZE;
      offset += piece_at(offset)->size) {
    piece_t *piece = piece_at(offset);
    // Only a free piece is a contender
    if (piece->next != FREE_PIECE) {
      continue;
    }

    // Free pieces that touch become one
    uint16_t end = offset + piece->size;
    while (end < USB_MSG_BUFFER_SIZE && piece_at(end)->next == FREE_PIECE) {
      piece->size += piece_at(end)->size;
      end = offset + piece->size;
    }
    // Final check after merging: still too small, on to the next piece
    if (piece->size < needed) {
      continue;
    }

    // A piece big enough is found. What is left over becomes a free piece of
    // its own, right after it
    if (piece->size > needed) {
      piece_t *rest = piece_at(offset + needed);
      rest->size = piece->size - needed;
      rest->next = FREE_PIECE;
      piece->size = needed;
    }
    // No longer free, and in no chain yet
    piece->next = NO_PIECE;
    return offset;
  }
  return NO_PIECE;
}

// Marks a piece as free. Its content is left as it is, and the free pieces
// around it are only joined by the next piece_take()
static void piece_free(uint16_t offset) {
  piece_at(offset)->next = FREE_PIECE;
}

// Adds a taken piece to a chain, as its newest
static void chain_add(chain_t *chain, uint16_t offset) {
  // Nothing follows the newest piece
  piece_at(offset)->next = NO_PIECE;
  // If the chain is empty
  if (chain->newest == NO_PIECE) {
    // The piece is also the oldest (the head)
    chain->oldest = offset;
  } else {
    // Else, the piece is only added to the tail: after the newest so far
    piece_at(chain->newest)->next = offset;
  }
  // The piece always becomes the newest
  chain->newest = offset;
}

// Removes the oldest piece of a chain and frees it. The chain must not be
// empty
static void chain_remove_oldest(chain_t *chain) {
  uint16_t offset = chain->oldest;
  // The piece that followed it becomes the oldest
  chain->oldest = piece_at(offset)->next;
  // It was the only one: the chain is empty
  if (chain->oldest == NO_PIECE) {
    chain->newest = NO_PIECE;
  }
  piece_free(offset);
}

/* OUTGOING */

// Size on the cable of the file stored in a piece: header and content
static uint16_t stored_size(uint16_t offset) {
  return FILE_HEADER_SIZE + file_at(offset)->header.size;
}

// Nothing is in flight anymore: the oldest outgoing file starts from its first
// block, with all its tries. Called when the file in flight gets its
// handshake, and when the connection opens or closes
static void tx_reset(void) {
  msg.tx_in_flight = false;
  msg.tx_offset = 0;
  msg.tx_tries = 0;
}

// Handshake of the PC. Removes the file in flight, if the handshake is for it.
// code is ACK_CODE or NACK_CODE, reason only counts for a NACK, crc is the crc
// field the handshake carries
static void on_handshake(uint8_t code, uint8_t reason, uint32_t crc) {
  // No file is being sent: the handshake is for one that is already gone
  if (!msg.tx_in_flight) {
    return;
  }
  uint16_t offset = msg.outgoing.oldest;
  // Only a file sent to its end takes a handshake, and only the one with its
  // crc: a handshake that comes twice is not taken for the next file
  bool sent_once = msg.tx_tries > 0 || msg.tx_offset == stored_size(offset);
  if (!sent_once || crc != file_at(offset)->header.crc) {
    return;
  }

  // Refused: the same bytes would be refused again, the file is not kept
  if (code == NACK_CODE) {
    msg.tx_refused++;
    msg.tx_last_reason = reason;
  }
  // Delivered or refused, the file leaves the buffer and the next one can go
  chain_remove_oldest(&msg.outgoing);
  tx_reset();
}

/* INCOMING */

// Drops the open message, and frees the piece it was filling
static void rx_forget(void) {
  if (msg.rx_state == RX_FILLING) {
    piece_free(msg.rx_piece);
  }
  msg.rx_state = RX_IDLE;
}

// Queues the handshake that usb_msg_next_block() sends next. One at a time: a
// new one replaces the one that did not leave yet
static void send_handshake(uint8_t code, nack_reason_t reason, uint32_t crc) {
  msg.handshake = code;
  msg.handshake_reason = reason;
  msg.handshake_crc = crc;
}

// Sends NACK for a message of the PC, and counts it
static void refuse(nack_reason_t reason, uint32_t crc) {
  msg.rx_refused++;
  send_handshake(NACK_CODE, reason, crc);
}

// Reason sent for a file that wheel_file_check() refused
static nack_reason_t reason_of(wheel_file_status_t status) {
  switch (status) {
  case WHEEL_FILE_BAD_CRC:
    return NACK_BAD_CRC;
  case WHEEL_FILE_UNKNOWN_TYPE:
    return NACK_UNKNOWN_TYPE;
  case WHEEL_FILE_BAD_VERSION:
    return NACK_BAD_VERSION;
  default:
    return NACK_BAD_SIZE;
  }
}

// The connection ends: after MAX_TRIES without a handshake, or when the bus is
// lost. The files stay: the outgoing ones are sent after the next sync
static void link_close(void) {
  msg.connected = false;
  // The file in flight keeps its place, and will start from its first block
  tx_reset();
  // A file half received will never get its end
  rx_forget();
}

// A complete message shorter than a file header: a sync, an ACK or a NACK
// (see MESSAGES). Anything else is dropped
static void on_short_message(const uint8_t *data, uint8_t count) {
  // The 4 bytes each of the three carries: MAGIC, or a crc field. Copied out:
  // data is not aligned
  uint32_t word;

  if (count == SYNC_SIZE) {
    // Sync: the 4 bytes are the whole message
    memcpy(&word, data, sizeof(word));
    if (word == MAGIC) {
      // Also when already connected: the application of the PC started again
      msg.connected = true;
      tx_reset();
      send_handshake(ACK_CODE, NACK_NONE, word);
    } else {
      refuse(NACK_BAD_MAGIC, word);
    }
  } else if (count == ACK_SIZE && data[0] == ACK_CODE) {
    // ACK: the code, then the crc field
    memcpy(&word, &data[1], sizeof(word));
    on_handshake(ACK_CODE, NACK_NONE, word);
  } else if (count == NACK_SIZE && data[0] == NACK_CODE) {
    // NACK: the code, the reason, then the crc field
    memcpy(&word, &data[2], sizeof(word));
    on_handshake(NACK_CODE, data[1], word);
  } else {
    // None of the three: no handshake
    msg.rx_dropped++;
  }
}

// First block of a file. Its data starts with the file header, which gives the
// size of the piece to take. Without a piece the blocks are skipped up to the
// end of the message, and rx_reason is the NACK that rx_end() sends
static void rx_begin(const uint8_t *data) {
  // Copied out: data is not aligned, the fields cannot be read in place
  wheel_file_header_t header;
  memcpy(&header, data, sizeof(header));

  // Skipped without a handshake, unless one of the cases below changes it
  msg.rx_crc = header.crc;
  msg.rx_received = 0;
  msg.rx_state = RX_SKIPPING;
  msg.rx_reason = NACK_NONE;

  if (!msg.connected) {
    msg.rx_reason = NACK_NOT_CONNECTED;
    return;
  }
  if (header.size > USB_MSG_MAX_CONTENT_SIZE) {
    msg.rx_reason = NACK_TOO_BIG;
    return;
  }

  // What has to arrive: the header and the content, without the padding
  msg.rx_size = FILE_HEADER_SIZE + header.size;
  msg.rx_piece = piece_take(msg.rx_size);
  if (msg.rx_piece == NO_PIECE) {
    // No room: no handshake, the PC sends the file again later
    return;
  }
  // The padding of the content, which no block brings
  memset(bytes_at(msg.rx_piece) + msg.rx_size, 0,
  WHEEL_FILE_PADDED_SIZE(msg.rx_size) - msg.rx_size);
  msg.rx_state = RX_FILLING;
}

// Data of a block of the open message, the first one included
static void rx_add(const uint8_t *data, uint8_t count) {
  // A skipped message: its data goes nowhere
  if (msg.rx_state != RX_FILLING) {
    return;
  }
  // More bytes than the file header announced: the piece is given back, and
  // what is left of the message is skipped
  if (msg.rx_received + count > msg.rx_size) {
    rx_forget();
    msg.rx_state = RX_SKIPPING;
    msg.rx_reason = NACK_MALFORMED;
    return;
  }
  // After the bytes of the blocks before
  memcpy(bytes_at(msg.rx_piece) + msg.rx_received, data, count);
  msg.rx_received += count;
}

// The last block of the open message arrived: the message gets its handshake,
// and its file is kept if it is a valid one
static void rx_end(void) {
  // Whatever happens below, no message is open after this
  rx_state_t state = msg.rx_state;
  msg.rx_state = RX_IDLE;

  // A skipped message: refused with the reason found on the way, or left
  // without a handshake when there was no room for it
  if (state == RX_SKIPPING) {
    if (msg.rx_reason != NACK_NONE) {
      refuse(msg.rx_reason, msg.rx_crc);
    } else {
      msg.rx_dropped++;
    }
    return;
  }

  // A file was filled: it has to be complete, then valid
  uint16_t offset = msg.rx_piece;
  nack_reason_t reason = NACK_NONE;
  // Fewer bytes than the file header announced
  if (msg.rx_received != msg.rx_size) {
    reason = NACK_MALFORMED;
  } else {
    wheel_file_status_t status = wheel_file_check(file_at(offset));
    if (status != WHEEL_FILE_OK) {
      reason = reason_of(status);
    }
  }

  // Not kept: its piece is given back
  if (reason != NACK_NONE) {
    piece_free(offset);
    refuse(reason, msg.rx_crc);
    return;
  }
  // From here usb_msg_peek() sees the file
  chain_add(&msg.incoming, offset);
  send_handshake(ACK_CODE, NACK_NONE, msg.rx_crc);
}

/* FOR THE USB MODULE (usb_internal.h) */

// Starts with an empty buffer, no connection and nothing to send
void usb_msg_init(void) {
  memset(&msg, 0, sizeof(msg));
  // One free piece: the whole buffer
  piece_at(0)->size = USB_MSG_BUFFER_SIZE;
  piece_at(0)->next = FREE_PIECE;
  // Two empty chains. NO_PIECE is not 0: the memset above does not do it
  msg.outgoing.oldest = NO_PIECE;
  msg.outgoing.newest = NO_PIECE;
  msg.incoming.oldest = NO_PIECE;
  msg.incoming.newest = NO_PIECE;
}

// Runs the two timeouts of the protocol: the one of the message being
// received, and the one of the file in flight
void usb_msg_task(uint32_t now_ms) {
  // The next block of the open message did not come in time
  if (msg.rx_state != RX_IDLE && now_ms - msg.rx_time >= TIMEOUT_MS) {
    rx_forget();
    msg.rx_dropped++;
  }

  // The file in flight got no handshake, or the PC stopped collecting its
  // blocks: one try is spent. After the last one the connection is given up,
  // and the file stays the oldest of its chain
  if (msg.tx_in_flight && now_ms - msg.tx_time >= TIMEOUT_MS) {
    msg.tx_tries++;
    if (msg.tx_tries >= MAX_TRIES) {
      link_close();
    } else {
      // Next try: from the first block, with a full TIMEOUT_MS
      msg.tx_offset = 0;
      msg.tx_time = now_ms;
    }
  }
}

// Builds the block to send next: a handshake first, else the next bytes of the
// file in flight. False when there is nothing to send.
// It only builds. Nothing counts as sent before usb_msg_block_sent(): a block
// that TinyUSB did not take is simply built again at the next call
bool usb_msg_next_block(uint8_t block[USB_MSG_BLOCK_SIZE]) {
  // The oldest outgoing file and its size on the cable, 0 when none waits
  uint16_t offset = msg.outgoing.oldest;
  uint16_t size = (offset != NO_PIECE) ? stored_size(offset) : 0;
  // Where the data of the block goes: after its first byte
  uint8_t *data = &block[1];

  // A handshake goes between two files, never between two blocks of one.
  bool mid_file = msg.tx_offset > 0 && msg.tx_offset < size;

  /* A handshake has been queued */
  if (msg.handshake != 0 && !mid_file) {
    uint8_t count;
    memset(block, 0, USB_MSG_BLOCK_SIZE);
    // ACK: the code, then the crc field. NACK: the code, the reason, then the
    // crc field
    data[0] = msg.handshake;
    if (msg.handshake == ACK_CODE) {
      memcpy(&data[1], &msg.handshake_crc, sizeof(msg.handshake_crc));
      count = ACK_SIZE;
    } else {
      data[1] = msg.handshake_reason;
      memcpy(&data[2], &msg.handshake_crc, sizeof(msg.handshake_crc));
      count = NACK_SIZE;
    }
    // The whole message is in this block: both flags
    block[0] = BLOCK_START | BLOCK_END | count;
    msg.built_handshake = true;
    return true;
  }

  /* Not connected, no file, or the file in flight waits for its handshake */
  if (!msg.connected || msg.tx_offset >= size) {
    return false;
  }

  /* Continue sending the next block of the file */
  // A full block, or what is left of the file
  uint16_t left = size - msg.tx_offset;
  uint8_t count = (left < BLOCK_MAX_DATA) ? left : BLOCK_MAX_DATA;
  memset(block, 0, USB_MSG_BLOCK_SIZE);
  block[0] = count;
  // Nothing was sent in this try: its first block
  if (msg.tx_offset == 0) {
    block[0] |= BLOCK_START;
  }
  // Nothing is left after this block: its last one
  if (count == left) {
    block[0] |= BLOCK_END;
  }
  memcpy(data, bytes_at(offset) + msg.tx_offset, count);
  // For usb_msg_block_sent()
  msg.built_handshake = false;
  msg.built_count = count;
  return true;
}

// TinyUSB took the block of the last usb_msg_next_block(): what it carried
// now counts as sent
void usb_msg_block_sent(uint32_t now_ms) {
  // The handshake was queued and sent; we can now clear the queue
  if (msg.built_handshake) {
    // Built_handshake is cleared when the next block of a file is sent
    msg.handshake = 0;
    return;
  }
  // A block of the file. The file is in flight from its first block on, and
  // the TIMEOUT_MS of usb_msg_task() starts again from here
  msg.tx_in_flight = true;
  msg.tx_offset += msg.built_count;
  msg.tx_time = now_ms;
}

// A block of the PC arrived. It is checked against the rules of BLOCKS, then
// its data goes to the message it belongs to: a short message is handled at
// once, a file is filled block after block and handled at its last one
void usb_msg_on_block(const uint8_t *block, uint16_t len, uint32_t now_ms) {
  if (len == 0) {
    return;
  }
  // The first byte of the block, then where its data starts
  uint8_t count = block[0] & BLOCK_COUNT_MASK;
  bool start = (block[0] & BLOCK_START) != 0;
  bool end = (block[0] & BLOCK_END) != 0;
  const uint8_t *data = &block[1];

  // The rules of BLOCKS: the data fits in what was received, only the last
  // block is short, and only a BLOCK_START opens a message. A block that
  // breaks them gets no handshake, and neither does its message
  bool valid = count > 0 && count < len && (end || count == BLOCK_MAX_DATA)
               && (start || msg.rx_state != RX_IDLE);
  if (!valid) {
    rx_forget();
    msg.rx_dropped++;
    return;
  }

  if (start) {
    // A message left open is forgotten
    rx_forget();
    // A whole message, too short to be a file: a sync, an ACK or a NACK
    if (end && count < FILE_HEADER_SIZE) {
      on_short_message(data, count);
      return;
    }
    // A file. Its first block always holds its whole header: it is either a
    // full block, or the last one of a message of 8 bytes or more
    rx_begin(data);
  }
  // First block or not, its data joins the file being filled
  rx_add(data, count);
  // The TIMEOUT_MS of usb_msg_task() starts again from here
  msg.rx_time = now_ms;
  if (end) {
    rx_end();
  }
}

// The bus left the configured state: the connection closes
void usb_msg_link_down(void) {
  link_close();
  // Nobody is left to read the handshake that was owed
  msg.handshake = 0;
}

/* INTERFACE (usb_msg.h) */

// Stores a file for the PC: takes a piece, builds the file in it and adds it
// to the outgoing chain. usb_msg_next_block() sends it when its turn comes
usb_msg_status_t usb_msg_send(uint8_t type, const void *content, uint16_t size) {
  if (size > USB_MSG_MAX_CONTENT_SIZE) {
    return USB_MSG_INVALID;
  }
  uint16_t offset = piece_take(FILE_HEADER_SIZE + size);
  if (offset == NO_PIECE) {
    return USB_MSG_FULL;
  }
  // The header with its CRC, the copy of the content and the padding, written
  // straight in the piece
  wheel_file_make(file_at(offset), type, content, size);
  chain_add(&msg.outgoing, offset);
  return USB_MSG_OK;
}

// Oldest file of the PC, where it lies in the buffer: nothing is copied.
// NULL when the incoming chain is empty
const wheel_file_t* usb_msg_peek(void) {
  uint16_t offset = msg.incoming.oldest;
  return (offset != NO_PIECE) ? file_at(offset) : NULL;
}

// Removes the oldest file of the PC and frees its piece: the pointer of
// usb_msg_peek() is no longer valid. Does nothing when no file waits
void usb_msg_pop(void) {
  if (msg.incoming.oldest != NO_PIECE) {
    chain_remove_oldest(&msg.incoming);
  }
}

// See CONNECTION above
bool usb_msg_connected(void) {
  return msg.connected;
}

// Walks the outgoing chain, from the oldest file to the newest
bool usb_msg_pending(uint32_t crc) {
  for (uint16_t offset = msg.outgoing.oldest; offset != NO_PIECE;
      offset = piece_at(offset)->next) {
    if (file_at(offset)->header.crc == crc) {
      return true;
    }
  }
  return false;
}
