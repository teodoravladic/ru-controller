#ifndef CONFIGURE_MPLANE_H
#define CONFIGURE_MPLANE_H

#include "ru-session-api.h"

void cmd_edit_config(ru_session_t *ru_session);
void cmd_validate(ru_session_t *ru_session);
void cmd_commit(ru_session_t *ru_session);

#endif /* CONFIGURE_MPLANE_H */
