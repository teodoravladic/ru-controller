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

#include "get-yang-models.h"
#include "../rpc-send-recv.h"

#include <libxml/parser.h>

static void store_schemas(xmlNode *node, ru_session_t *ru_session)
{
  int timeout = CLI_RPC_REPLY_TIMEOUT;

#ifdef V1
  struct ly_ctx *ctx = ly_ctx_new(NULL, 0);
#else
  struct ly_ctx *ctx = NULL;
  ly_ctx_new(NULL, 0, &ctx);
#endif

  for (xmlNode *cur_node = node; cur_node; cur_node = cur_node->next) {
    if (strcmp((const char *)cur_node->name, "schema") == 0) {

      xmlNode *name_node = xmlFirstElementChild(cur_node);
      char *module_name = (char *)xmlNodeGetContent(name_node);
      xmlNode *revision_node = xmlNextElementSibling(name_node);
      char *module_revision = (char *)xmlNodeGetContent(revision_node);
      xmlNode *format_node = xmlNextElementSibling(revision_node);
      char *module_format = (char *)xmlNodeGetContent(format_node);

      if (strcmp(module_format, "ncm:yang") != 0) continue;
      struct nc_rpc *get_schema_rpc = nc_rpc_getschema(module_name, module_revision, "yang", NC_PARAMTYPE_CONST);
      char *schema_data = rpc_send_recv(ru_session, get_schema_rpc, 0, timeout);

      if (schema_data) {
#ifdef V1
        const struct lys_module *mod = lys_parse_mem(ctx, schema_data, LYS_IN_YANG);
#else
        struct lys_module *mod = NULL;
        lys_parse_mem(ctx, schema_data, LYS_IN_YANG, &mod);
#endif
        if (!mod) {
          printf("Failed to load module %s\n", module_name);
        } else {
          printf("Module %s loaded successfully\n", module_name);
        }
        free(schema_data);
      }

      nc_rpc_free(get_schema_rpc);
    }
  }
}

static void load_yang_models(xmlNode *node, ru_session_t *ru_session)
{
  for (xmlNode *schemas_node = node; schemas_node; schemas_node = schemas_node->next) {
    if(schemas_node->type == XML_ELEMENT_NODE){
      if (strcmp((const char *)schemas_node->name, "schemas") == 0) {
        store_schemas(schemas_node->children, ru_session);
      } else {
        load_yang_models(schemas_node->children, ru_session);
      }
    }
  }
}

void get_yang_models(const char* buffer, ru_session_t *ru_session)
{
  // Initialize the xml file
  size_t len = strlen(buffer) + 1;
  xmlDoc *doc = xmlReadMemory(buffer, len, NULL, NULL, 0);
  xmlNode *root_element = xmlDocGetRootElement(doc);

  load_yang_models(root_element->children, ru_session);
}
