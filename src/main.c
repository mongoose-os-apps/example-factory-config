/*
 * Copyright (c) 2021 Deomid "rojer" Ryabkov
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mgos.h"
#include "mgos_rpc.h"
#include "mgos_vfs_dev.h"

static void set_factory_config_handler(struct mg_rpc_request_info *ri,
                                       void *cb_arg,
                                       struct mg_rpc_frame_info *fi,
                                       struct mg_str args) {
  size_t off = (size_t) cb_arg;
  struct mgos_vfs_dev *dev = NULL;
  struct mgos_config_factory cfg = {0};
  struct mbuf mb;
  struct json_out mbout = JSON_OUT_MBUF(&mb);

  mbuf_init(&mb, 0);

  mgos_config_factory_set_defaults(&cfg);
  // Alternatively,
  // mgos_config_factory_copy(mgos_sys_config_get_factory(), &cfg);

  json_scanf(args.p, args.len, ri->args_fmt, &cfg.foo, &cfg.bar);

  json_printf(&mbout, "{factory: ");
  if (!mgos_config_factory_emit(&cfg, false /* pretty */, &mbout)) {
    mg_rpc_send_errorf(ri, 500, "%s failed", "emit");
    goto out;
  }
  mbuf_append(&mb, "}", 1 + 1);  // Close and NUL-terminate.
  while (mb.len % 16 != 0) {
    mbuf_append(&mb, "", 1);  // Pad to AES block size.
  }

  mg_hexdumpf(stderr, mb.buf, mb.len);

  dev = mgos_vfs_dev_open("conf");
  if (dev == NULL) goto out;
  if (mgos_vfs_dev_erase(dev, off, 0x1000) != MGOS_VFS_DEV_ERR_NONE) {
    mg_rpc_send_errorf(ri, 500, "%s failed", "erase");
    goto out;
  }
  if (mgos_vfs_dev_write(dev, off, mb.len, mb.buf) != MGOS_VFS_DEV_ERR_NONE) {
    mg_rpc_send_errorf(ri, 500, "%s failed", "write");
    goto out;
  }

  mg_rpc_send_responsef(ri, "%s", mb.buf);

  mgos_system_restart_after(200);

out:
  mgos_config_factory_free(&cfg);
  mgos_vfs_dev_close(dev);
  mbuf_free(&mb);
}

static void timer_cb(void *arg) {
  LOG(LL_INFO,
      ("foo is %d, bar is '%s'", mgos_sys_config_get_factory_foo(),
       (mgos_sys_config_get_factory_bar() ? mgos_sys_config_get_factory_bar()
                                          : "")));
  (void) arg;
}

enum mgos_app_init_result mgos_app_init(void) {
  mgos_set_timer(1000, MGOS_TIMER_REPEAT | MGOS_TIMER_RUN_NOW, timer_cb, NULL);
  mg_rpc_add_handler(mgos_rpc_get_global(), "Test.SetFactoryConfig1",
                     "{foo: %d, bar: %Q}", set_factory_config_handler,
                     (void *) 0);
  mg_rpc_add_handler(mgos_rpc_get_global(), "Test.SetFactoryConfig2",
                     "{foo: %d, bar: %Q}", set_factory_config_handler,
                     (void *) 4096);
  return MGOS_APP_INIT_SUCCESS;
}
