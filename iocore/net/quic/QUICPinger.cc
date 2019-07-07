/** @file

  A brief file description

  @section license License

  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 */

#include "QUICPinger.h"

void
QUICPinger::request(QUICEncryptionLevel level)
{
  SCOPED_MUTEX_LOCK(lock, this->_mutex, this_ethread());
  ++this->_need_to_fire[static_cast<int>(level)];
}

void
QUICPinger::cancel(QUICEncryptionLevel level)
{
  SCOPED_MUTEX_LOCK(lock, this->_mutex, this_ethread());
  if (this->_need_to_fire[static_cast<int>(level)] > 0) {
    --this->_need_to_fire[static_cast<int>(level)];
  }
}

bool
QUICPinger::_will_generate_frame(QUICEncryptionLevel level, size_t current_packet_size, bool ack_eliciting)
{
  SCOPED_MUTEX_LOCK(lock, this->_mutex, this_ethread());
  // PING Frame is meaningless for ack_eliciting packet. Cancel it.
  if (ack_eliciting) {
    this->_ack_eliciting_packet_out = true;
    this->cancel(level);
    return false;
  }

  if (this->_ack_eliciting_packet_out == false && !ack_eliciting) {
    // force to send an PING Frame
    this->request(level);
  }

  return this->_need_to_fire[static_cast<int>(level)] > 0;
}

/**
 * @param connection_credit This is not used. Because PING frame is not flow-controlled
 */
QUICFrame *
QUICPinger::_generate_frame(uint8_t *buf, QUICEncryptionLevel level, uint64_t /* connection_credit */, uint16_t maximum_frame_size,
                            size_t current_packet_size)
{
  SCOPED_MUTEX_LOCK(lock, this->_mutex, this_ethread());
  QUICFrame *frame = nullptr;

  if (this->_need_to_fire[static_cast<int>(level)] > 0 && maximum_frame_size > 0) {
    // don't care ping frame lost or acked
    frame = QUICFrameFactory::create_ping_frame(buf, 0, nullptr);
    --this->_need_to_fire[static_cast<int>(level)];
    this->_ack_eliciting_packet_out = true;
  }

  return frame;
}

uint64_t
QUICPinger::count(QUICEncryptionLevel level)
{
  SCOPED_MUTEX_LOCK(lock, this->_mutex, this_ethread());
  return this->_need_to_fire[static_cast<int>(level)];
}