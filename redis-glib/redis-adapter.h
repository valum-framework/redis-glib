#ifndef __HIREDIS_GLIB_H__
#define __HIREDIS_GLIB_H__

#include <glib.h>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>

typedef struct
{
    GSource source;
    redisAsyncContext *ac;
    GPollFD poll_fd;
    gpointer user_data;
} RedisSource;

static void
redis_source_add_read (gpointer data)
{
    RedisSource *source = data;
    g_return_if_fail(source);
    source->poll_fd.events |= G_IO_IN;
    g_main_context_wakeup(g_source_get_context(data));
}

static void
redis_source_del_read (gpointer data)
{
    RedisSource *source = data;
    g_return_if_fail(source);
    source->poll_fd.events &= ~G_IO_IN;
    g_main_context_wakeup(g_source_get_context(data));
}

static void
redis_source_add_write (gpointer data)
{
    RedisSource *source = data;
    g_return_if_fail(source);
    source->poll_fd.events |= G_IO_OUT;
    g_main_context_wakeup(g_source_get_context(data));
}

static void
redis_source_del_write (gpointer data)
{
    RedisSource *source = data;
    g_return_if_fail(source);
    source->poll_fd.events &= ~G_IO_OUT;
    g_main_context_wakeup(g_source_get_context(data));
}

static void
redis_source_cleanup (gpointer data)
{
    RedisSource *source = data;

    g_return_if_fail(source);

    redis_source_del_read(source);
    redis_source_del_write(source);
    /*
     * It is not our responsibility to remove ourself from the
     * current main loop. However, we will remove the GPollFD.
     */
    if (source->poll_fd.fd >= 0) {
        g_source_remove_poll(data, &source->poll_fd);
        source->poll_fd.fd = -1;
    }
}

static gboolean
redis_source_prepare (GSource *source,
                      gint    *timeout_)
{
    RedisSource *redis = (RedisSource *)source;
    *timeout_ = -1;
    return !!(redis->poll_fd.events & redis->poll_fd.revents);
}

static gboolean
redis_source_check (GSource *source)
{
    RedisSource *redis = (RedisSource *)source;
    return !!(redis->poll_fd.events & redis->poll_fd.revents);
}

static gboolean
redis_source_dispatch (GSource      *source,
                       GSourceFunc   callback,
                       gpointer      user_data)
{
    RedisSource *redis = (RedisSource *)source;

    if ((redis->poll_fd.revents & G_IO_OUT)) {
        redisAsyncHandleWrite(redis->ac);
        redis->poll_fd.revents &= ~G_IO_OUT;
    }

    if ((redis->poll_fd.revents & G_IO_IN)) {
        redisAsyncHandleRead(redis->ac);
        redis->poll_fd.revents &= ~G_IO_IN;
    }

    if (callback) {
        return callback(user_data);
    }

    return TRUE;
}

static void
redis_source_finalize (GSource *source)
{
    RedisSource *redis = (RedisSource *)source;

    if (redis->poll_fd.fd >= 0) {
        g_source_remove_poll(source, &redis->poll_fd);
        redis->poll_fd.fd = -1;
    }
}

static GSource *
redis_source_new (redisAsyncContext *ac,
                  gpointer           user_data)
{
    static GSourceFuncs source_funcs = {
        redis_source_prepare,
        redis_source_check,
        redis_source_dispatch,
        redis_source_finalize,
        0,
        0
    };
    redisContext *c = &ac->c;
    RedisSource *source;

    g_return_val_if_fail(ac != NULL, NULL);

    source = (RedisSource *)g_source_new(&source_funcs, sizeof *source);
    source->ac = ac;
    source->poll_fd.fd = c->fd;
    source->poll_fd.events = 0;
    source->poll_fd.revents = 0;
    source->user_data = user_data;
    g_source_add_poll((GSource *)source, &source->poll_fd);

    ac->ev.addRead = redis_source_add_read;
    ac->ev.delRead = redis_source_del_read;
    ac->ev.addWrite = redis_source_add_write;
    ac->ev.delWrite = redis_source_del_write;
    ac->ev.cleanup = redis_source_cleanup;
    ac->ev.data = source;

    return (GSource *)source;
}

#endif /* __HIREDIS_GLIB_H__ */
