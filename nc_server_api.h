#ifndef M_PLANE_SERVER_API_H
#define M_PLANE_SERVER_API_H

#include <libyang/libyang.h>
#include <nc_client.h>


typedef struct{
  struct nc_session *session;
  char *ru_ip_add;

} ru_session_t;

#endif
