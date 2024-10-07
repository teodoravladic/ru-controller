#include "init-mplane.h"
#include "connect-mplane.h"
#include "get-mplane.h"
#include "subscribe-mplane.h"
#include "config-mplane.h"
#include "disconnect-mplane.h"
#include "xml/get-xml.h"

#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* use LOG_I[M-plane] or similar, instead of printf() */

static void sig_handler(int sig_num)
{
  printf("\nSignal number %d caught. Exiting.\n", sig_num);

  exit(EXIT_SUCCESS);
}


int main(int argc, char *argv[])
{
  struct sigaction action;
  memset(&action, 0, sizeof action);
  action.sa_handler = sig_handler;
  sigaction(SIGINT, &action, NULL);
  sigaction(SIGTERM, &action, NULL);

  // TODO: to import list of IP addresses from gnb.conf and allocate the correct length
  ru_session_t *ru_session = init_mplane(argc - 1, argv);
  nc_client_init();

  int num_ru;
  bool is_connect;
  if (argc == 1) {
    is_connect = false;
    num_ru = 1;
  } else {
    is_connect = true;
    num_ru = argc - 1;
  }

  ru_config_t *ru_config = calloc(num_ru, sizeof(ru_config_t));
  assert(ru_config != NULL);

  for (size_t i = 0; i < num_ru; i++) {
    (is_connect) ? cmd_connect(&ru_session[i]) : cmd_listen(&ru_session[i]);

    char *operational_ds = cmd_get(&ru_session[i]);
    ru_config[i].delay = get_ru_delay_profile(operational_ds);
    bool synced = get_ptp_sync_status(operational_ds);
    if(synced == false){
      cmd_subscribe(&ru_session[i]);
    }

    cmd_edit_config(&ru_session[i]);
    cmd_validate(&ru_session[i]);
    cmd_commit(&ru_session[i]);
    cmd_disconnect(&ru_session[i]);
  }

  nc_client_destroy();

  return EXIT_SUCCESS;
}
