/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include "get-mplane.h"
#include "rpc-send-recv.h"

#include <assert.h>
#include <stdio.h>

char *cmd_get(ru_session_t *ru_session)
{
  int timeout = CLI_RPC_REPLY_TIMEOUT;
  struct nc_rpc *rpc;
  NC_WD_MODE wd = NC_WD_UNKNOWN;  // try with explicit!
  NC_PARAMTYPE param = NC_PARAMTYPE_CONST;

  char *filter = NULL;   // e.g. "/o-ran-delay-management:delay-management";

  /* create request */
  rpc = nc_rpc_get(filter, wd, param);
  assert(rpc != NULL && "RPC creation failed.\n");

  char *answer = rpc_send_recv(ru_session, rpc, wd, timeout);
  printf("Successfully retreived operational datastore\n");

  nc_rpc_free(rpc);

  return answer;
}
