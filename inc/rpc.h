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

struct RpcServer {
  void* ReceiveBuffer;
  void* SendBuffer;
  size_t HanlderCount;
  rpc_serve_handler_t* Handlers[];
};

/**
 * @brief Listen for incoming IPC requests and call appropriate handlers
 */
void rpc_serve(const struct RpcServer* server);


#endif /* rpc.h */
