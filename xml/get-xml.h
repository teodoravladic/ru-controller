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
