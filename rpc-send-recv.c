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

#include "rpc-send-recv.h"
#include "disconnect-mplane.h"

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef V1
static char *recv_v1(ru_session_t * ru_session, struct nc_rpc *rpc, NC_MSG_TYPE msgtype, const uint64_t msgid, int timeout_s)
{
  char *answer = NULL;

  struct nc_reply *reply;
  struct nc_reply_data *data_rpl;
  struct nc_reply_error *error;
  char *str = NULL;
  uint32_t ly_wd;

  LYD_FORMAT output_format = LYD_XML;

  uint32_t output_flag = 0;    // other option is LYD_PRINT_SHRINK: Flag for output without indentation and formatting new lines.

  while(1){
    msgtype = nc_recv_reply(ru_session->session, rpc, msgid, timeout_s * 1000,
                            LYD_OPT_DESTRUCT | LYD_OPT_NOSIBLINGS, &reply);
    if (msgtype == NC_MSG_ERROR) {
      assert(false && "Failed to receive a reply.");
      cmd_disconnect(ru_session);
    } else if (msgtype == NC_MSG_WOULDBLOCK) {
      assert(false && "Timeout for receiving a reply expired.");
    } else if (msgtype == NC_MSG_NOTIF) {
      /* read again */
      continue;
    } else if (msgtype == NC_MSG_REPLY_ERR_MSGID) {
      /* unexpected message, try reading again to get the correct reply */
      printf("Unexpected reply received - ignoring and waiting for the correct reply.\n");
      nc_reply_free(reply);
      continue;
    }
    break;
  }

  switch (reply->type) {
    case NC_RPL_OK:
      fprintf(stdout, "OK\n");
      break;
    case NC_RPL_DATA:
      data_rpl = (struct nc_reply_data *)reply;

      if (nc_rpc_get_type(rpc) == NC_RPC_GETSCHEMA) {
        assert((!data_rpl->data || (data_rpl->data->schema->nodetype != LYS_RPC) || (data_rpl->data->child == NULL)
               || (data_rpl->data->child->schema->nodetype != LYS_ANYXML)) && "Cannot get schema");

        struct lyd_node_anydata *any = (struct lyd_node_anydata *)data_rpl->data->child;
        switch (any->value_type) {
          case LYD_ANYDATA_CONSTSTRING:
          case LYD_ANYDATA_STRING:
             answer = strdup(any->value.str); 
            break;
          case LYD_ANYDATA_DATATREE:
              lyd_print_mem(&answer, any->value.tree, LYD_XML, LYP_FORMAT | LYP_WITHSIBLINGS);
              break;
          case LYD_ANYDATA_XML:
              lyxml_print_mem(&answer, any->value.xml, LYXML_PRINT_SIBLINGS);
              break;
          default:
              assert(false && "cannot happen");
          }
          break;
      } else if (nc_rpc_get_type(rpc) == NC_RPC_GETCONFIG) {
        char *buffer = NULL;
        ly_wd = 0; // but try with EXPLICIT
        lyd_print_mem(&buffer, data_rpl->data, output_format, LYP_WITHSIBLINGS | LYP_NETCONF | ly_wd | output_flag);
        answer = calloc(strlen(buffer)+128, sizeof(char));
        sprintf(answer, "%s%s%s", "<config xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\">\n", buffer, "</config>");
      } else if (nc_rpc_get_type(rpc) == NC_RPC_GET) {
        char *buffer = NULL;
        ly_wd = 0; // but try with EXPLICIT
        lyd_print_mem(&buffer, data_rpl->data, output_format, LYP_WITHSIBLINGS | LYP_NETCONF | ly_wd | output_flag);
        answer = calloc(strlen(buffer)+128, sizeof(char));
        sprintf(answer, "%s%s%s", "<data xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\">\n", buffer, "</data>");
      }
      
      break;
    case NC_RPL_ERROR:
      fprintf(stdout, "ERROR\n");
      error = (struct nc_reply_error *)reply;
      for (int i = 0; i < error->count; ++i) {
        if (error->err[i].type) {
          fprintf(stdout, "\ttype:     %s\n", error->err[i].type);
        }
        if (error->err[i].tag) {
          fprintf(stdout, "\ttag:      %s\n", error->err[i].tag);
        }
        if (error->err[i].severity) {
          fprintf(stdout, "\tseverity: %s\n", error->err[i].severity);
        }
        if (error->err[i].apptag) {
          fprintf(stdout, "\tapp-tag:  %s\n", error->err[i].apptag);
        }
        if (error->err[i].path) {
          fprintf(stdout, "\tpath:     %s\n", error->err[i].path);
        }
        if (error->err[i].message) {
          fprintf(stdout, "\tmessage:  %s\n", error->err[i].message);
        }
        if (error->err[i].sid) {
          fprintf(stdout, "\tSID:      %s\n", error->err[i].sid);
        }
        for (int j = 0; j < error->err[i].attr_count; ++j) {
          fprintf(stdout, "\tbad-attr #%d: %s\n", j + 1, error->err[i].attr[j]);
        }
        for (int j = 0; j < error->err[i].elem_count; ++j) {
          fprintf(stdout, "\tbad-elem #%d: %s\n", j + 1, error->err[i].elem[j]);
        }
        for (int j = 0; j < error->err[i].ns_count; ++j) {
          fprintf(stdout, "\tbad-ns #%d:   %s\n", j + 1, error->err[i].ns[j]);
        }
        for (int j = 0; j < error->err[i].other_count; ++j) {
          lyxml_print_mem(&str, error->err[i].other[j], 0);
          fprintf(stdout, "\tother #%d:\n%s\n", j + 1, str);
          free(str);
        }
        fprintf(stdout, "\n");
      }
      assert(false && "Cannot continue editing RU config\n");
      break;
    default:
      assert(false && "Internal error.");
      nc_reply_free(reply);
  }
  nc_reply_free(reply);

  return answer;
}
#elif defined V2
static char *recv_v2(ru_session_t * ru_session, struct nc_rpc *rpc, NC_MSG_TYPE msgtype, const uint64_t msgid, int timeout_s)
{
  char *answer = NULL;

  struct lyd_node *envp, *op, *err, *node, *info;
  uint32_t ly_wd;
  LYD_FORMAT output_format = LYD_XML;
  uint32_t output_flag = 0;    // other option is LYD_PRINT_SHRINK: Flag for output without indentation and formatting new lines.

  while(1){
    msgtype = nc_recv_reply(ru_session->session, rpc, msgid, timeout_s * 1000, &envp, &op);
    if (msgtype == NC_MSG_ERROR) {
      assert(false && "[MPLANE] Failed to receive a reply for RPC.\n");    // pass rpc->type
      cmd_disconnect(ru_session);
    } else if (msgtype == NC_MSG_WOULDBLOCK) {
      assert(false && "Timeout for receiving a reply for RPC expired.\n");      // pass rpc->type
    } else if (msgtype == NC_MSG_NOTIF) {    // for SUBSCRIBE 
      /* read again */
      continue;
    } else if (msgtype == NC_MSG_REPLY_ERR_MSGID) {
      /* unexpected message, try reading again to get the correct reply */
      printf("[MPLANE] Unexpected reply received - ignoring and waiting for the correct reply.\n");
      lyd_free_tree(envp);
      lyd_free_tree(op);
      continue;
    }
    break;
  }

  /* get functionality */
  if (op) {
  /* data reply */
    if (nc_rpc_get_type(rpc) == NC_RPC_GETSCHEMA) {
      /* special case */
      if (!lyd_child(op) || (lyd_child(op)->schema->nodetype != LYS_ANYXML)) {
        assert(false && "Cannot happen");
      }
      struct lyd_node_any *any = (struct lyd_node_any *)lyd_child(op);
      switch (any->value_type) {
      case LYD_ANYDATA_STRING:
      case LYD_ANYDATA_XML:
          answer = strdup(any->value.str);
          break;
      case LYD_ANYDATA_DATATREE:
          lyd_print_mem(&answer, any->value.tree, LYD_XML, LYD_PRINT_WITHSIBLINGS);
          break;
      default:
        assert(false && "Cannot happen");
      }
    } else {

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

      // lyd_print_file(output, lyd_child(op), output_format, LYD_PRINT_WITHSIBLINGS | ly_wd | output_flag);
      lyd_print_mem(&answer, lyd_child(op), output_format, LYD_PRINT_WITHSIBLINGS | ly_wd | output_flag);
    }
  /* edit/validate/commit functionalities */
  } else if (!strcmp(LYD_NAME(lyd_child(envp)), "ok")) {
    /* ok reply */
    fprintf(stdout, "OK\n");
  } else {
    assert(!strcmp(LYD_NAME(lyd_child(envp)), "rpc-error"));

    /* make sure the following code is correct, and try to make it shorter? And do an assert, cause it cannot continue */
    fprintf(stdout, "ERROR\n");
    LY_LIST_FOR(lyd_child(envp), err) {
      lyd_find_sibling_opaq_next(lyd_child(err), "error-type", &node);
      if (node) {
          fprintf(stdout, "\ttype:     %s\n", ((struct lyd_node_opaq *)node)->value);
      }
      lyd_find_sibling_opaq_next(lyd_child(err), "error-tag", &node);
      if (node) {
          fprintf(stdout, "\ttag:      %s\n", ((struct lyd_node_opaq *)node)->value);
      }
      lyd_find_sibling_opaq_next(lyd_child(err), "error-severity", &node);
      if (node) {
          fprintf(stdout, "\tseverity: %s\n", ((struct lyd_node_opaq *)node)->value);
      }
      lyd_find_sibling_opaq_next(lyd_child(err), "error-app-tag", &node);
      if (node) {
          fprintf(stdout, "\tapp-tag:  %s\n", ((struct lyd_node_opaq *)node)->value);
      }
      lyd_find_sibling_opaq_next(lyd_child(err), "error-path", &node);
      if (node) {
          fprintf(stdout, "\tpath:     %s\n", ((struct lyd_node_opaq *)node)->value);
      }
      lyd_find_sibling_opaq_next(lyd_child(err), "error-message", &node);
      if (node) {
          fprintf(stdout, "\tmessage:  %s\n", ((struct lyd_node_opaq *)node)->value);
      }

      info = lyd_child(err);
      while (!lyd_find_sibling_opaq_next(info, "error-info", &info)) {
        fprintf(stdout, "\tinfo:\n");
        lyd_print_file(stdout, lyd_child(info), output_format, LYD_PRINT_WITHSIBLINGS);

        info = info->next;
      }
      fprintf(stdout, "\n");
    }
    assert(false && "Cannot continue editing RU config\n");
  }

  lyd_free_tree(envp);
  lyd_free_tree(op);

  return answer;
}
#else
  assert(false && "Unknown M-plane version\n");
#endif

char *rpc_send_recv(ru_session_t * ru_session, struct nc_rpc *rpc, NC_WD_MODE wd_mode, int timeout_s)
{
  uint64_t msgid;
  NC_MSG_TYPE msgtype;
  
  msgtype = nc_send_rpc(ru_session->session, rpc, 1000, &msgid);
  if (msgtype == NC_MSG_ERROR) {
    assert(false && "[MPLANE] Failed to send the RPC.\n");   // pass rpc->type and use AssertFatal() in OAI
    cmd_disconnect(ru_session); // should we pass session ID?
  } else if (msgtype == NC_MSG_WOULDBLOCK) {
    assert(false && "[MPLANE] Timeout for sending the RPC expired.\n");   // pass rpc->type
  }

#ifdef V1
  return recv_v1(ru_session, rpc, msgtype, msgid, timeout_s);
#elif defined V2
  return recv_v2(ru_session, rpc, msgtype, msgid, timeout_s);
#else
  assert(false && "Unknown M-plane version\n");
#endif
}
