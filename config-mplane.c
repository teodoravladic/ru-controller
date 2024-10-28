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

#include "config-mplane.h"
#include "rpc-send-recv.h"

#include <assert.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>


void cmd_edit_config(ru_session_t *ru_session)
{
  int c, config_fd, ret = EXIT_FAILURE, content_param = 0, timeout = CLI_RPC_REPLY_TIMEOUT;
  struct stat config_stat;
  char *config_m = NULL, *cont_start;
  NC_DATASTORE target = NC_DATASTORE_CANDIDATE;  // also, can be RUNNING, but by M-plane spec, we should modify CANDIDATE, then verify if it's ok and then COMMIT
  struct nc_rpc *rpc;
  NC_RPC_EDIT_DFLTOP op = NC_RPC_EDIT_DFLTOP_REPLACE;  // check which one is better: MERGE or REPLACE
  NC_RPC_EDIT_TESTOPT test = NC_RPC_EDIT_TESTOPT_UNKNOWN;
  NC_RPC_EDIT_ERROPT err = NC_RPC_EDIT_ERROPT_UNKNOWN;

  /* open edit configuration data from the file */
  const char *input = "<add-file-name>.xml";
  FILE *f = fopen(input, "r");
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *content = calloc(len + 1, sizeof(char));
  fread(content, 1, len, f);
  content[len] = '\0';
  fclose(f);

  rpc = nc_rpc_edit(target, op, test, err, content, NC_PARAMTYPE_CONST);
  assert(rpc != NULL && "RPC creation failed.\n");

  rpc_send_recv(ru_session, rpc, 0, timeout);
  printf("Successfully edited RU config\n");

  nc_rpc_free(rpc);
  free(content);
}

void cmd_validate(ru_session_t *ru_session)
{
  struct nc_rpc *rpc;
  int timeout = CLI_RPC_REPLY_TIMEOUT;
  char *src_start = NULL;
  NC_DATASTORE source = NC_DATASTORE_CANDIDATE;

  /* create requests */
  rpc = nc_rpc_validate(source, src_start, NC_PARAMTYPE_CONST);
  assert(rpc != NULL && "RPC val creation failed.");

  rpc_send_recv(ru_session, rpc, 0, timeout);
  printf("RU config successfully validated. Ready for the changes to be commited\n");

  nc_rpc_free(rpc);
}

void cmd_commit(ru_session_t *ru_session)
{
  struct nc_rpc *rpc;
  int confirmed = 0, timeout = CLI_RPC_REPLY_TIMEOUT;
  int32_t confirm_timeout = 0;
  char *persist = NULL, *persist_id = NULL;

  /* creat request */
  rpc = nc_rpc_commit(confirmed, confirm_timeout, persist, persist_id, NC_PARAMTYPE_CONST);
  assert(rpc != NULL && "RPC creation failed.");

  rpc_send_recv(ru_session, rpc, 0, timeout);
  printf("RU config is successfully committed\n");

  nc_rpc_free(rpc);
}
