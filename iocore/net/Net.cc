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


/************************************************************************

   Net.cc


************************************************************************/

#include "P_Net.h"

RecRawStatBlock *net_rsb = NULL;

static inline void
configure_net(void)
{

  IOCORE_RegisterConfigString(RECT_LOCAL, "proxy.local.incoming_ip_to_bind", NULL, RECU_RESTART_TS, RECC_NULL, NULL);

  IOCORE_RegisterConfigInteger(RECT_CONFIG, "proxy.config.net.connections_throttle", 8000, RECU_RESTART_TS, RECC_STR, "^[0-9]+$");
  IOCORE_RegisterConfigUpdateFunc("proxy.config.net.connections_throttle", change_net_connections_throttle, NULL);
  IOCORE_ReadConfigInteger(fds_throttle, "proxy.config.net.connections_throttle");

  IOCORE_ReadConfigInteger(throttle_enabled,"proxy.config.net.throttle_enabled");

  IOCORE_RegisterConfigInteger(RECT_CONFIG, "proxy.config.net.listen_backlog", 1024, RECU_RESTART_TS, RECC_NULL, NULL);
  IOCORE_RegisterConfigInteger(RECT_CONFIG, "proxy.config.net.max_poll_delay", 128, RECU_DYNAMIC, RECC_NULL, NULL);
}


static inline void
register_net_stats()
{

  //
  // Register statistics
  //
  RecRegisterRawStat(net_rsb, RECT_PROCESS, "proxy.process.net.net_handler_run",
                     RECD_INT, RECP_NULL, (int) net_handler_run_stat, RecRawStatSyncSum);
  NET_CLEAR_DYN_STAT(net_handler_run_stat);

  RecRegisterRawStat(net_rsb, RECT_PROCESS, "proxy.process.net.read_bytes",
                     RECD_INT, RECP_NULL, (int) net_read_bytes_stat, RecRawStatSyncSum);

  RecRegisterRawStat(net_rsb, RECT_PROCESS, "proxy.process.net.write_bytes",
                     RECD_INT, RECP_NULL, (int) net_write_bytes_stat, RecRawStatSyncSum);

  RecRegisterRawStat(net_rsb, RECT_PROCESS,
                     "proxy.process.net.connections_currently_open",
                     RECD_INT, RECP_NON_PERSISTENT, (int) net_connections_currently_open_stat, RecRawStatSyncSum);
  NET_CLEAR_DYN_STAT(net_connections_currently_open_stat);

  RecRegisterRawStat(net_rsb, RECT_PROCESS,
                     "proxy.process.net.accepts_currently_open",
                     RECD_INT, RECP_NON_PERSISTENT, (int) net_accepts_currently_open_stat, RecRawStatSyncSum);
  NET_CLEAR_DYN_STAT(net_accepts_currently_open_stat);

  RecRegisterRawStat(net_rsb, RECT_PROCESS,
                     "proxy.process.net.calls_to_readfromnet",
                     RECD_INT, RECP_NULL, (int) net_calls_to_readfromnet_stat, RecRawStatSyncSum);
  NET_CLEAR_DYN_STAT(net_calls_to_readfromnet_stat);

  RecRegisterRawStat(net_rsb, RECT_PROCESS,
                     "proxy.process.net.calls_to_readfromnet_afterpoll",
                     RECD_INT, RECP_NULL, (int) net_calls_to_readfromnet_afterpoll_stat, RecRawStatSyncSum);
  NET_CLEAR_DYN_STAT(net_calls_to_readfromnet_afterpoll_stat);

  RecRegisterRawStat(net_rsb, RECT_PROCESS,
                     "proxy.process.net.calls_to_read",
                     RECD_INT, RECP_NULL, (int) net_calls_to_read_stat, RecRawStatSyncSum);
  NET_CLEAR_DYN_STAT(net_calls_to_read_stat);

  RecRegisterRawStat(net_rsb, RECT_PROCESS,
                     "proxy.process.net.calls_to_read_nodata",
                     RECD_INT, RECP_NULL, (int) net_calls_to_read_nodata_stat, RecRawStatSyncSum);
  NET_CLEAR_DYN_STAT(net_calls_to_read_nodata_stat);

  RecRegisterRawStat(net_rsb, RECT_PROCESS,
                     "proxy.process.net.calls_to_writetonet",
                     RECD_INT, RECP_NULL, (int) net_calls_to_writetonet_stat, RecRawStatSyncSum);
  NET_CLEAR_DYN_STAT(net_calls_to_writetonet_stat);

  RecRegisterRawStat(net_rsb, RECT_PROCESS,
                     "proxy.process.net.calls_to_writetonet_afterpoll",
                     RECD_INT, RECP_NULL, (int) net_calls_to_writetonet_afterpoll_stat, RecRawStatSyncSum);
  NET_CLEAR_DYN_STAT(net_calls_to_writetonet_afterpoll_stat);

  RecRegisterRawStat(net_rsb, RECT_PROCESS,
                     "proxy.process.net.calls_to_write",
                     RECD_INT, RECP_NULL, (int) net_calls_to_write_stat, RecRawStatSyncSum);
  NET_CLEAR_DYN_STAT(net_calls_to_write_stat);

  RecRegisterRawStat(net_rsb, RECT_PROCESS,
                     "proxy.process.net.calls_to_write_nodata",
                     RECD_INT, RECP_NULL, (int) net_calls_to_write_nodata_stat, RecRawStatSyncSum);
  NET_CLEAR_DYN_STAT(net_calls_to_write_nodata_stat);

#ifndef INK_NO_SOCKS
  RecRegisterRawStat(net_rsb, RECT_PROCESS,
                     "proxy.process.socks.connections_successful",
                     RECD_INT, RECP_NULL, (int) socks_connections_successful_stat, RecRawStatSyncSum);

  RecRegisterRawStat(net_rsb, RECT_PROCESS,
                     "proxy.process.socks.connections_unsuccessful",
                     RECD_INT, RECP_NULL, (int) socks_connections_unsuccessful_stat, RecRawStatSyncSum);

  RecRegisterRawStat(net_rsb, RECT_PROCESS,
                     "proxy.process.socks.connections_currently_open",
                     RECD_INT, RECP_NON_PERSISTENT, (int) socks_connections_currently_open_stat, RecRawStatSyncSum);
  NET_CLEAR_DYN_STAT(socks_connections_currently_open_stat);
#endif

}

void
ink_net_init(ModuleVersion version)
{
  static int init_called = 0;

  ink_release_assert(!checkModuleVersion(version, NET_SYSTEM_MODULE_VERSION));
  if (!init_called) {
    // do one time stuff
    // create a stat block for NetStats
    net_rsb = RecAllocateRawStatBlock((int) Net_Stat_Count);
    configure_net();
    register_net_stats();
  }

  init_called = 1;
}
