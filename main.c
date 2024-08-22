#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <libyang/libyang.h>
#include <nc_client.h>

#define CLI_CH_TIMEOUT 30         // time to wait for call-home
#define CLI_RPC_REPLY_TIMEOUT 5   // time to wait for server reply

#define HOME_DIR (getenv("HOME"))

static struct nc_session *session;   // make this an array or RB tree
static char *ru_ip_add = NULL;


/* use LOG_I[M-plane] or similar, instead of printf() */


static void lnc2_print_clb(NC_VERB_LEVEL level, const char *msg)
{
  printf("netconf2 verb level = %d\n", level);

  switch (level) {
    case NC_VERB_ERROR:
      fprintf(stderr, "nc ERROR: %s\n", msg);
      break;
    case NC_VERB_WARNING:
      fprintf(stderr, "nc WARNING: %s\n", msg);
      break;
    case NC_VERB_VERBOSE:
      fprintf(stderr, "nc VERBOSE: %s\n", msg);
      break;
    case NC_VERB_DEBUG:
    case NC_VERB_DEBUG_LOWLVL:
      fprintf(stderr, "nc DEBUG: %s\n", msg);
      break;
  }
}

static void ly_print_clb(LY_LOG_LEVEL level, const char *msg, const char *path)
{
  printf("yang verb level = %d\n", level);

  switch (level) {
    case LY_LLERR:
      if (path) {
          fprintf(stderr, "ly ERROR: %s (%s)\n", msg, path);
      } else {
          fprintf(stderr, "ly ERROR: %s\n", msg);
      }
      break;
    case LY_LLWRN:
      if (path) {
          fprintf(stderr, "ly WARNING: %s (%s)\n", msg, path);
      } else {
          fprintf(stderr, "ly WARNING: %s\n", msg);
      }
      break;
    case LY_LLVRB:
      if (path) {
          fprintf(stderr, "ly VERBOSE: %s (%s)\n", msg, path);
      } else {
          fprintf(stderr, "ly VERBOSE: %s\n", msg);
      }
      break;
    case LY_LLDBG:
      if (path) {
          fprintf(stderr, "ly DEBUG: %s (%s)\n", msg, path);
      } else {
          fprintf(stderr, "ly DEBUG: %s\n", msg);
      }
      break;
    default:
      /* silent, just to cover enum, shouldn't be here in real world */
      return;
  }
}

static void cmd_disconnect(void)
{
  assert(session != NULL);  // probably can be taken out

  nc_session_free(session, NULL);
  session = NULL;

  printf("Successfully disconnected from RU with IP address %s\n", ru_ip_add);
}

int my_auth_hostkey_check(const char *hostname, ssh_session session, void *priv)
{
  (void)hostname;
  (void)session;
  (void)priv;

  return 0;
}

/* be aware that this function might need to be expanded to unix and TLS use cases */
static void cmd_listen(void)
{
  NC_TRANSPORT_IMPL ti = NC_TI_LIBSSH;

  int port = NC_PORT_CH_SSH;
  char *host = "0.0.0.0";       // better IPv4
  int timeout = CLI_CH_TIMEOUT;
  char *user = "root";

  /* create the session */
  nc_client_ssh_ch_set_username(user);
  nc_client_ssh_ch_add_bind_listen(host, port);

  nc_client_ssh_ch_set_auth_pref(NC_SSH_AUTH_PASSWORD, -1);
  nc_client_ssh_ch_set_auth_pref(NC_SSH_AUTH_PUBLICKEY, 1);  // ssh-key identification
  nc_client_ssh_ch_set_auth_pref(NC_SSH_AUTH_INTERACTIVE, -1);

  char pub_key[32], priv_key[32];
  sprintf(pub_key, "%s%s", HOME_DIR, "/.ssh/id_rsa.pub");
  sprintf(priv_key, "%s%s", HOME_DIR, "/.ssh/id_rsa");
  printf("pub_key = %s, prv_key = %s\n", pub_key, priv_key);
  int keypair_ret = nc_client_ssh_ch_add_keypair(pub_key, priv_key);
  assert(keypair_ret == 0 && "Unable to authenticate RU\n");
  nc_client_ssh_ch_set_auth_hostkey_check_clb(my_auth_hostkey_check, "DATA");  // host-key identification

  printf("Waiting %ds for an SSH Call Home connection on port %u...\n", timeout, port);

  int ret = nc_accept_callhome(timeout * 1000, NULL, &session); // check the right session; maybe session[0] is already ongoing, and need to create session[1]
  nc_client_ssh_ch_del_bind(host, port);

  assert(ret == 1 && "SSH Call Home failed.");

  printf("Successfuly connected to RU with IP address %s\n", ru_ip_add);
}


static void cli_send_recv(struct nc_rpc *rpc, FILE *output, NC_WD_MODE wd_mode, int timeout_s)
{
  uint32_t ly_wd;
  uint64_t msgid;
  struct lyd_node *envp, *op, *err, *node, *info;
  NC_MSG_TYPE msgtype;
  uint32_t output_flag = 0;    // other option is LYD_PRINT_SHRINK: Flag for output without indentation and formatting new lines.
  LYD_FORMAT output_format = LYD_XML;

  msgtype = nc_send_rpc(session, rpc, 1000, &msgid);
  if (msgtype == NC_MSG_ERROR) {
    assert(false && "[MPLANE] Failed to send the RPC.\n");   // pass rpc->type and use AssertFatal() in OAI
    cmd_disconnect(); // should we pass session ID?
  } else if (msgtype == NC_MSG_WOULDBLOCK) {
    assert(false && "[MPLANE] Timeout for sending the RPC expired.\n");   // pass rpc->type
  }

recv_reply:
  msgtype = nc_recv_reply(session, rpc, msgid, timeout_s * 1000, &envp, &op);
  if (msgtype == NC_MSG_ERROR) {
    assert(false && "[MPLANE] Failed to receive a reply for RPC.\n");    // pass rpc->type
    cmd_disconnect();
  } else if (msgtype == NC_MSG_WOULDBLOCK) {
    assert(false && "Timeout for receiving a reply for RPC expired.\n");      // pass rpc->type
  } else if (msgtype == NC_MSG_NOTIF) {    // for SUBSCRIBE 
    /* read again */
    goto recv_reply;
  } else if (msgtype == NC_MSG_REPLY_ERR_MSGID) {
    /* unexpected message, try reading again to get the correct reply */
    printf("[MPLANE] Unexpected reply received - ignoring and waiting for the correct reply.\n");
    lyd_free_tree(envp);
    lyd_free_tree(op);
    goto recv_reply;
  }

  /* get functionality */
  if (op) {
    /* data reply */
            
    ly_wd = 0;  // but try with EXPLICIT
    // switch (wd_mode) {
    // case NC_WD_ALL:
    //     ly_wd = LYD_PRINT_WD_ALL;
    //     break;
    // case NC_WD_ALL_TAG:
    //     ly_wd = LYD_PRINT_WD_ALL_TAG;
    //     break;
    // case NC_WD_TRIM:
    //     ly_wd = LYD_PRINT_WD_TRIM;
    //     break;
    // case NC_WD_EXPLICIT:
    //     ly_wd = LYD_PRINT_WD_EXPLICIT;
    //     break;
    // default:
    //     ly_wd = 0;
    //     break;
    // }

    lyd_print_file(output, lyd_child(op), output_format, LYD_PRINT_WITHSIBLINGS | ly_wd | output_flag);

    /* edit/validate/commit functionalities */
    } else if (!strcmp(LYD_NAME(lyd_child(envp)), "ok")) {
      /* ok reply */
      fprintf(output, "OK\n");
    } else {
      assert(!strcmp(LYD_NAME(lyd_child(envp)), "rpc-error"));

      /* make sure the following code is correct, and try to make it shorter? And do an assert, cause it cannot continue */
      fprintf(output, "ERROR\n");
      LY_LIST_FOR(lyd_child(envp), err) {
        lyd_find_sibling_opaq_next(lyd_child(err), "error-type", &node);
        if (node) {
            fprintf(output, "\ttype:     %s\n", ((struct lyd_node_opaq *)node)->value);
        }
        lyd_find_sibling_opaq_next(lyd_child(err), "error-tag", &node);
        if (node) {
            fprintf(output, "\ttag:      %s\n", ((struct lyd_node_opaq *)node)->value);
        }
        lyd_find_sibling_opaq_next(lyd_child(err), "error-severity", &node);
        if (node) {
            fprintf(output, "\tseverity: %s\n", ((struct lyd_node_opaq *)node)->value);
        }
        lyd_find_sibling_opaq_next(lyd_child(err), "error-app-tag", &node);
        if (node) {
            fprintf(output, "\tapp-tag:  %s\n", ((struct lyd_node_opaq *)node)->value);
        }
        lyd_find_sibling_opaq_next(lyd_child(err), "error-path", &node);
        if (node) {
            fprintf(output, "\tpath:     %s\n", ((struct lyd_node_opaq *)node)->value);
        }
        lyd_find_sibling_opaq_next(lyd_child(err), "error-message", &node);
        if (node) {
            fprintf(output, "\tmessage:  %s\n", ((struct lyd_node_opaq *)node)->value);
        }

        info = lyd_child(err);
        while (!lyd_find_sibling_opaq_next(info, "error-info", &info)) {
          fprintf(output, "\tinfo:\n");
          lyd_print_file(stdout, lyd_child(info), LYD_XML, LYD_PRINT_WITHSIBLINGS);

          info = info->next;
        }
        fprintf(output, "\n");
      }
    }

    lyd_free_tree(envp);
    lyd_free_tree(op);
}

static void cmd_commit(void)
{
  struct nc_rpc *rpc;
  int confirmed = 0, timeout = CLI_RPC_REPLY_TIMEOUT;
  int32_t confirm_timeout = 0;
  char *persist = NULL, *persist_id = NULL;

  /* creat request */
  rpc = nc_rpc_commit(confirmed, confirm_timeout, persist, persist_id, NC_PARAMTYPE_CONST);
  assert(rpc != NULL && "RPC creation failed.");

  cli_send_recv(rpc, stdout, 0, timeout);
  printf("RU config is successfully committed\n");

  nc_rpc_free(rpc);
}

static void cmd_validate(void)
{
  struct nc_rpc *rpc;
  int timeout = CLI_RPC_REPLY_TIMEOUT;
  char *src_start = NULL;
  NC_DATASTORE source = NC_DATASTORE_CANDIDATE;

  /* create requests */
  rpc = nc_rpc_validate(source, src_start, NC_PARAMTYPE_CONST);
  assert(rpc != NULL && "RPC val creation failed.");

  cli_send_recv(rpc, stdout, 0, timeout);
  printf("RU config successfully validated. Ready for the changes to be commited\n");

  nc_rpc_free(rpc);
}

static void cmd_edit_config(void)
{
  int c, config_fd, ret = EXIT_FAILURE, content_param = 0, timeout = CLI_RPC_REPLY_TIMEOUT;
  struct stat config_stat;
  char *content = NULL, *config_m = NULL, *cont_start;
  NC_DATASTORE target = NC_DATASTORE_CANDIDATE;  // also, can be RUNNING, but by M-plane spec, we should modify CANDIDATE, then verify if it's ok and then COMMIT
  struct nc_rpc *rpc;
  NC_RPC_EDIT_DFLTOP op = NC_RPC_EDIT_DFLTOP_MERGE;  // defop merge, save the existing values + modify the ones requested + add new ones if requested
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

  cli_send_recv(rpc, stdout, 0, timeout);
  printf("Successfully edited RU config\n");

  nc_rpc_free(rpc);
  free(content);
}

static void cmd_get_delay_profile(void)
{
  int timeout = CLI_RPC_REPLY_TIMEOUT;
  struct nc_rpc *rpc;
  NC_WD_MODE wd = NC_WD_UNKNOWN;  // try with explicit!
  NC_PARAMTYPE param = NC_PARAMTYPE_CONST;
  FILE *output = fopen("delay.xml", "w");
  char filter[] = "/o-ran-delay-management:delay-management";

  /* create request */
  rpc = nc_rpc_get(filter, wd, param);
  assert(rpc != NULL && "RPC creation failed.\n");

  cli_send_recv(rpc, output, wd, timeout);
  printf("Successfully retreived RU delay profile\n");

  fclose(output);
  nc_rpc_free(rpc);
}

int main(void)
{
  nc_client_init();

  /* check if this is needed, probably yes;
    also, is signal() sufficient or we need sigaction() */
//   struct sigaction action;
//   memset(&action, 0, sizeof action);
//   action.sa_handler = SIG_IGN;
//   sigaction(SIGPIPE, &action, NULL);   // why SIGPIPE?


  // logs for netconf2 and yang libraries
  nc_set_print_clb(lnc2_print_clb); // TODO: use nc_set_print_clb_session() function!
  ly_set_log_clb(ly_print_clb, 1);   // called in previous func?

  cmd_listen();
  cmd_get_delay_profile();
  cmd_edit_config();
  cmd_validate();
  cmd_commit();
  cmd_disconnect();

  nc_client_destroy();

  return EXIT_SUCCESS;
}
