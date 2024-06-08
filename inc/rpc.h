/**
 * @file rpc.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief
 *
 * @version 0.1
 * @date 2024-05-15
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __INC_RPC_H
#define __INC_RPC_H

#include <inc/types.h>
#include <inc/env.h>

/**
 * @brief RPC server handler
 */
typedef int rpc_serve_handler_t(envid_t from, const void* request,
                                void* response, int* response_perm);

typedef int rpc_fallback_t(int32_t request_id, envid_t from,
                                   const void* request, void* response,
                                   int* response_perm);

struct RpcServer {
  void* ReceiveBuffer;
  void* SendBuffer;
  rpc_fallback_t* Fallback;
  size_t HandlerCount;
  rpc_serve_handler_t* Handlers[];
};

struct RpcFailure {
  envid_t Source;
  int32_t RequestId;
  void* Request;
  int Perm;
};

/**
 * @brief Listen for next IPC request and call appropriate handler
 *
 * If `failure` is `NULL` server will respond to any invalid request with
 * `-E_INVAL` and listen for next request, otherwise it will report invalid
 * request in `failure` and return -1
 *
 * @return 0 on successful handling, -1 upon invalid request with
 * non-NULL failure
 */
int rpc_listen(const struct RpcServer* server, struct RpcFailure* failure);

/**
 * @brief Execute RPC request: send request and await response.
 * NULL is written to *res_data if res_data!=NULL and no page was received from
 * server
 *
 * @note All buffers are assumed to be of size PAGE_SIZE or more
 */
int32_t rpc_execute(envid_t server, int32_t req_id, const void* req_data, void** res_data);

#endif /* rpc.h */
