#include <redis-glib/redis-glib.h>

static GMainLoop *gMainLoop;

static void
test1_cb (GObject      *object,
          GAsyncResult *result,
          gpointer      user_data)
{
   RedisClient *client = (RedisClient *)object;
   gboolean *success = user_data;
   GError *error = NULL;

   *success = redis_client_connect_finish(client, result, &error);
   g_assert_no_error(error);
   g_assert(*success);
   g_main_loop_quit(gMainLoop);
}

static void
test1 (void)
{
   RedisClient *client;
   gboolean success = FALSE;

   client = redis_client_new();
   redis_client_connect_async(client, "127.0.0.1", 6379, test1_cb, &success);
   g_main_loop_run(gMainLoop);
   g_assert_cmpint(success, ==, TRUE);
}

gint
main (gint   argc,
      gchar *argv[])
{
   g_type_init();
   g_test_init(&argc, &argv, NULL);
   g_test_add_func("/RedisClient/connect_async", test1);
   gMainLoop = g_main_loop_new(NULL, FALSE);
   return g_test_run();
}
