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
  char *content = NULL, *config_m = NULL, *cont_start;
  NC_DATASTORE target = NC_DATASTORE_CANDIDATE;  // also, can be RUNNING, but by M-plane spec, we should modify CANDIDATE, then verify if it's ok and then COMMIT
  struct nc_rpc *rpc;
  NC_RPC_EDIT_DFLTOP op = NC_RPC_EDIT_DFLTOP_REPLACE;  // check which one is better: MERGE or REPLACE
  NC_RPC_EDIT_TESTOPT test = NC_RPC_EDIT_TESTOPT_UNKNOWN;
  NC_RPC_EDIT_ERROPT err = NC_RPC_EDIT_ERROPT_UNKNOWN;

  /* open edit configuration data from the file */
  const char *input = "../working-100.xml";
  config_fd = open(input, O_RDONLY);
  assert(config_fd != -1 && "Unable to open the local datastore file.\n");    // pass input and strerror(errno));

  /* map content of the file into the memory */
  if (fstat(config_fd, &config_stat) != 0) {
    assert(false && "fstat failed.");    // pass strerror(errno)
    close(config_fd);
  }
  config_m = mmap(NULL, config_stat.st_size, PROT_READ, MAP_PRIVATE, config_fd, 0);
  if (config_m == MAP_FAILED) {
    assert(false && "mmap of the local datastore file failed.\n");    // pass strerror(errno)
    close(config_fd);
  }

  /* make a copy of the content to allow closing the file */
  content = strdup(config_m);

  /* unmap local datastore file and close it */
  munmap(config_m, config_stat.st_size);
  close(config_fd);

  rpc = nc_rpc_edit(target, op, test, err, content, NC_PARAMTYPE_CONST);
  assert(rpc != NULL && "RPC creation failed.\n");

  rpc_send_recv(ru_session, rpc, stdout, 0, timeout);
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

  rpc_send_recv(ru_session, rpc, stdout, 0, timeout);
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

  rpc_send_recv(ru_session, rpc, stdout, 0, timeout);
  printf("RU config is successfully committed\n");

  nc_rpc_free(rpc);
}
