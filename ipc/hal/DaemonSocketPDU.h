/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_ipc_DaemonSocketPDU_h
#define mozilla_ipc_DaemonSocketPDU_h

#include "mozilla/FileUtils.h"
#include "mozilla/ipc/SocketBase.h"
#include "mozilla/ipc/DaemonSocketMessageHandlers.h"
#include "nsTArray.h"

namespace mozilla {
namespace ipc {

static const size_t MAX_NFDS = 16;
class DaemonSocketIOConsumer;

/**
 * |DaemonSocketPDU| represents a single PDU that is transfered from or to
 * the HAL daemon. Each PDU contains exactly one command.
 *
 * A PDU as the following format
 *
 *    |    1    |    1    |        2       |    n    |
 *    | service |  opcode | payload length | payload |
 *
 * Service and Opcode each require 1 byte, the payload length requires 2
 * bytes, and the payload requires the number of bytes as stored in the
 * payload-length field.
 *
 * Each service and opcode can have a different payload with individual
 * length. For the exact details of the HAL protocol, please refer to
 *
 *    https://git.kernel.org/cgit/bluetooth/bluez.git/tree/android/hal-ipc-api.txt?id=5.24
 *
 */
class DaemonSocketPDU final : public UnixSocketIOBuffer {
 public:
  enum {
    PDU_OFF_SERVICE = 0,
    PDU_OFF_OPCODE = 1,
    PDU_OFF_LENGTH = 2,
    PDU_OFF_PAYLOAD = 4,
    PDU_HEADER_SIZE = PDU_OFF_PAYLOAD,
    PDU_MAX_PAYLOAD_LENGTH = 1 << 16
  };

  DaemonSocketPDU(uint8_t aService, uint8_t aOpcode, uint16_t aPayloadSize);
  explicit DaemonSocketPDU(size_t aPayloadSize);
  ~DaemonSocketPDU();

  void SetConsumer(DaemonSocketIOConsumer* aConsumer) { mConsumer = aConsumer; }

  void SetResultHandler(DaemonSocketResultHandler* aRes) { mRes = aRes; }

  DaemonSocketResultHandler* GetResultHandler() const { return mRes; }

  void GetHeader(uint8_t& aService, uint8_t& aOpcode, uint16_t& aPayloadSize);

  ssize_t Send(int aFd) override;
  ssize_t Receive(int aFd) override;

  nsTArray<int> AcquireFds();

  nsresult UpdateHeader();

 private:
  size_t GetPayloadSize() const;
  void OnError(const char* aFunction, int aErrno);

  DaemonSocketIOConsumer* mConsumer;
  RefPtr<DaemonSocketResultHandler> mRes;
  nsTArray<ScopedClose> mReceivedFds;
};

}  // namespace ipc
}  // namespace mozilla

#endif
