/* redis-client.c
 *
 * Copyright (C) 2012 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib/gi18n.h>

#include "redis-adapter.h"
#include "redis-client.h"

G_DEFINE_TYPE(RedisClient, redis_client, G_TYPE_OBJECT)

struct _RedisClientPrivate
{
   GSimpleAsyncResult *async_connect;
   guint context_source;
   redisAsyncContext *context;
   GMainContext *main_context;
};

enum
{
   PROP_0,
   LAST_PROP
};

//static GParamSpec *gParamSpecs[LAST_PROP];

RedisClient *
redis_client_new (void)
{
   return g_object_new(REDIS_TYPE_CLIENT, NULL);
}

static void
redis_client_command_cb (redisAsyncContext *context,
                         gpointer           r,
                         gpointer           user_data)
{
   GSimpleAsyncResult *simple = user_data;

   g_return_if_fail(context);
   g_return_if_fail(G_IS_SIMPLE_ASYNC_RESULT(simple));

   if (r) {
      g_simple_async_result_set_op_res_gboolean(simple, TRUE);
      /*
       * TODO: Copy asyncReply?
       */
   }

   g_simple_async_result_complete_in_idle(simple);
   g_object_unref(simple);
}

void
redis_client_command_async (RedisClient         *client,
                            GAsyncReadyCallback  callback,
                            gpointer             user_data,
                            const gchar         *format,
                            ...)
{
   RedisClientPrivate *priv;
   GSimpleAsyncResult *simple;
   va_list args;

   g_return_if_fail(REDIS_IS_CLIENT(client));
   g_return_if_fail(callback);
   g_return_if_fail(format);

   priv = client->priv;

   simple = g_simple_async_result_new(G_OBJECT(client), callback, user_data,
                                      redis_client_command_async);

   va_start(args, format);
   redisvAsyncCommand(priv->context,
                      redis_client_command_cb,
                      simple,
                      format,
                      args);
   va_end(args);
}

gboolean
redis_client_command_finish (RedisClient   *client,
                             GAsyncResult  *result,
                             GError       **error)
{
   GSimpleAsyncResult *simple = (GSimpleAsyncResult *)result;
   gboolean ret;

   g_return_val_if_fail(REDIS_IS_CLIENT(client), FALSE);
   g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(simple), FALSE);

   if (!(ret = g_simple_async_result_get_op_res_gboolean(simple))) {
      g_simple_async_result_propagate_error(simple, error);
   }

   return ret;
}

static void
redis_client_connect_cb (const redisAsyncContext *ac,
                         int                      status)
{
   RedisClientPrivate *priv;
   RedisSource *source;
   gboolean success;

   g_return_if_fail(ac);
   g_return_if_fail(ac->ev.data);

   source = ac->ev.data;
   priv = REDIS_CLIENT(source->user_data)->priv;

   if (priv->async_connect) {
      success = (status == REDIS_OK);
      g_simple_async_result_set_op_res_gboolean(priv->async_connect, success);
      if (!success) {
         g_simple_async_result_set_error(priv->async_connect,
                                         REDIS_CLIENT_ERROR,
                                         REDIS_CLIENT_ERROR_HIREDIS,
                                         "%s", ac->errstr);
      }
      g_simple_async_result_complete_in_idle(priv->async_connect);
      g_clear_object(&priv->async_connect);
   }
}

static void
redis_client_disconnect_cb (const redisAsyncContext *ac,
                            int                      status)
{
   g_printerr("%s\n", G_STRFUNC);
}

void
redis_client_connect_async (RedisClient         *client,
                            const gchar         *hostname,
                            guint16              port,
                            GAsyncReadyCallback  callback,
                            gpointer             user_data)
{
   RedisClientPrivate *priv;
   GSimpleAsyncResult *simple;
   GSource *source;

   g_return_if_fail(REDIS_IS_CLIENT(client));
   g_return_if_fail(hostname);
   g_return_if_fail(callback);

   priv = client->priv;

   if (!port) {
      port = 6379;
   }

   /*
    * Make sure we haven't already connected.
    */
   if (priv->context) {
      simple =
         g_simple_async_result_new_error(G_OBJECT(client),
                                         callback,
                                         user_data,
                                         REDIS_CLIENT_ERROR,
                                         REDIS_CLIENT_ERROR_INVALID_STATE,
                                         _("%s() has already been called."),
                                         G_STRFUNC);
      g_simple_async_result_complete_in_idle(simple);
      g_object_unref(simple);
      return;
   }

   /*
    * Start connecting asynchronously.
    */
   priv->context = redisAsyncConnect(hostname, port);
   if (priv->context->err) {
      simple =
         g_simple_async_result_new_error(G_OBJECT(client),
                                         callback,
                                         user_data,
                                         REDIS_CLIENT_ERROR,
                                         REDIS_CLIENT_ERROR_HIREDIS,
                                         "%s", priv->context->errstr);
      g_simple_async_result_complete_in_idle(simple);
      g_object_unref(simple);
      redisAsyncFree(priv->context);
      priv->context = NULL;
      return;
   }

   /*
    * Save our GSimpleAsyncResult for later callback.
    */
   g_assert(!priv->async_connect);
   priv->async_connect =
      g_simple_async_result_new(G_OBJECT(client), callback, user_data,
                                redis_client_connect_async);

   /*
    * Create an event source so that we get our callbacks.
    */
   source = redis_source_new(priv->context, client);

   /*
    * This needs to come after the call redis_source_new().
    */
   redisAsyncSetConnectCallback(priv->context, redis_client_connect_cb);
   redisAsyncSetDisconnectCallback(priv->context, redis_client_disconnect_cb);

   /*
    * Start processing events using our GSource.
    */
   priv->context_source = g_source_attach(source, priv->main_context);

#if 0
   g_source_unref(source);
#endif
}

gboolean
redis_client_connect_finish (RedisClient   *client,
                             GAsyncResult  *result,
                             GError       **error)
{
   GSimpleAsyncResult *simple = (GSimpleAsyncResult *)result;
   gboolean ret;

   g_return_val_if_fail(REDIS_IS_CLIENT(client), FALSE);
   g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(simple), FALSE);

   if (!(ret = g_simple_async_result_get_op_res_gboolean(simple))) {
      g_simple_async_result_propagate_error(simple, error);
   }

   return ret;
}

static void
redis_client_finalize (GObject *object)
{
   G_OBJECT_CLASS(redis_client_parent_class)->finalize(object);
}

static void
redis_client_class_init (RedisClientClass *klass)
{
   GObjectClass *object_class;

   object_class = G_OBJECT_CLASS(klass);
   object_class->finalize = redis_client_finalize;
   g_type_class_add_private(object_class, sizeof(RedisClientPrivate));
}

static void
redis_client_init (RedisClient *client)
{
   client->priv =
      G_TYPE_INSTANCE_GET_PRIVATE(client,
                                  REDIS_TYPE_CLIENT,
                                  RedisClientPrivate);
}

GQuark
redis_client_error_quark (void)
{
   return g_quark_from_string("redis-client-error-quark");
}
