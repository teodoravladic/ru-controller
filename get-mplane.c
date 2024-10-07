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
