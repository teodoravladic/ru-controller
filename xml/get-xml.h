#ifndef GET_XML_H
#define GET_XML_H

#include <stdio.h>
#include <stdbool.h>

typedef struct {
  int T2a_min_up;
  int T2a_max_up;
  int T2a_min_cp_dl;
  int T2a_max_cp_dl;
  int Tcp_adv_dl;
  int Ta3_min;
  int Ta3_max;
  int T2a_min_cp_ul;
  int T2a_max_cp_ul;

} delay_profile_t;

typedef struct {
  
} antenna_conf_t;

typedef struct {
  delay_profile_t delay;

  size_t num_ant;
  antenna_conf_t ant_conf;

} ru_config_t;

delay_profile_t get_ru_delay_profile(const char *filename);

bool get_ptp_sync_status(const char *filename);

#endif /* GET_XML_H */
