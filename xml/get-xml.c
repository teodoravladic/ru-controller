#include "get-xml.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <libxml/parser.h>

static void log_ru_delay_profile(delay_profile_t *delay)
{
  printf("\
  T2a_min_up %d\n\
  T2a_max_up %d\n\
  T2a_min_cp_dl %d\n\
  T2a_max_cp_dl %d\n\
  Tcp_adv_dl %d\n\
  Ta3_min %d\n\
  Ta3_max %d\n\
  T2a_min_cp_ul %d\n\
  T2a_max_cp_ul %d\n",
    delay->T2a_min_up,
    delay->T2a_max_up,
    delay->T2a_min_cp_dl,
    delay->T2a_max_cp_dl,
    delay->Tcp_adv_dl,
    delay->Ta3_min,
    delay->Ta3_max,
    delay->T2a_min_cp_ul,
    delay->T2a_max_cp_ul);
}

static void store_ru_delay_profile(xmlNode *node, delay_profile_t *delay)
{
  for (xmlNode *cur_child = node; cur_child; cur_child = cur_child->next) {
    if(cur_child->type == XML_ELEMENT_NODE){
      int value = atoi((const char *)xmlNodeGetContent(cur_child));

      if (strcmp((const char *)cur_child->name, "t2a-min-up") == 0) {
        delay->T2a_min_up = value;
      } else if (strcmp((const char *)cur_child->name, "t2a-max-up") == 0) {
        delay->T2a_max_up = value;
      } else if (strcmp((const char *)cur_child->name, "t2a-min-cp-dl") == 0) {
        delay->T2a_min_cp_dl = value;
      } else if (strcmp((const char *)cur_child->name, "t2a-max-cp-dl") == 0) {
        delay->T2a_max_cp_dl = value;
      } else if (strcmp((const char *)cur_child->name, "tcp-adv-dl") == 0) {
        delay->Tcp_adv_dl = value;
      } else if (strcmp((const char *)cur_child->name, "ta3-min") == 0) {
        delay->Ta3_min = value;
      } else if (strcmp((const char *)cur_child->name, "ta3-max") == 0) {
        delay->Ta3_max = value;
      } else if (strcmp((const char *)cur_child->name, "t2a-min-cp-ul") == 0) {
        delay->T2a_min_cp_ul = value;
      } else if (strcmp((const char *)cur_child->name, "t2a-max-cp-ul") == 0) {
        delay->T2a_max_cp_ul = value;
      }
    }
  }
}

static void find_ru_delay_profile(xmlNode *node, delay_profile_t *delay)
{
  for(xmlNode *cur_node = node; cur_node; cur_node = cur_node->next){
    if(cur_node->type == XML_ELEMENT_NODE){
      if(strcmp((const char*)cur_node->name, "ru-delay-profile") == 0){
        store_ru_delay_profile(cur_node->children, delay);
        break;
      } else {
        find_ru_delay_profile(cur_node->children, delay);
      }
    }
  }
}

delay_profile_t get_ru_delay_profile(const char *filename)
{
  delay_profile_t delay = {0};

  // Initialize the xml file
  xmlDoc *doc = xmlReadFile(filename, NULL, 0);
  xmlNode *root_element = xmlDocGetRootElement(doc);

  find_ru_delay_profile(root_element->children, &delay);
  log_ru_delay_profile(&delay);

  return delay;
}

static bool find_ptp_status(xmlNode *node)
{
  for (xmlNode *cur_node = node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (strcmp((const char *)cur_node->name, "sync-state") == 0 && strcmp((char *)xmlNodeGetContent(cur_node), "LOCKED") == 0) {
          printf("RU is already PTP synchronized\n");
          return true;
      }
      if (find_ptp_status(cur_node->children)) {
        return true;
      }
    }
  }
  return false;
}

bool get_ptp_sync_status(const char *filename)
{
  // Initialize the xml file
  xmlDoc *doc = xmlReadFile(filename, NULL, 0);
  xmlNode *root_element = xmlDocGetRootElement(doc);

  return find_ptp_status(root_element->children);
}
