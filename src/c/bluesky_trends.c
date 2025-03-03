#include <pebble.h>

#define MAX_TOPICS 10
#define MAX_POSTS 10

static Window *s_topics_window;
static MenuLayer *s_topic_layer;

static Window *s_feed_window;
static MenuLayer *s_feed_layer;

static Window *s_post_window;
static ScrollLayer *s_post_layer;
static TextLayer *s_handle_layer;
static TextLayer *s_post_text_layer;

static char s_topic_text[32];

static int num_topics = 0;
static int loaded_topics = 0;

static int num_posts = 0;
static int loaded_posts = 0;

static char *feed_id;

static int selected_feed;
static int selected_post;

typedef struct Topic
{
  char name[32];
  char id[32];
} Topic;

typedef struct Post
{
  char handle[64];
  char text[352];
} Post;

static Topic topics[MAX_TOPICS];
static Post posts[MAX_POSTS];

static void select_feed_callback(struct MenuLayer *s_menu_layer, MenuIndex *cell_index,
                                 void *callback_context)
{
  loaded_posts = 0;
  for (int i = 0; i < MAX_POSTS; i++)
  {
    memset(posts[i].handle, 0, sizeof(posts[i].handle));
    memset(posts[i].text, 0, sizeof(posts[i].text));
  }
  feed_id = topics[cell_index->row].id;
  selected_feed = cell_index->row;
  window_stack_push(s_feed_window, false);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_cstring(iter, MESSAGE_KEY_MessageType, "feed");
  dict_write_cstring(iter, MESSAGE_KEY_TopicId, feed_id);
  app_message_outbox_send();
}

static void select_post_callback(struct MenuLayer *s_menu_layer, MenuIndex *cell_index,
                                 void *callback_context)
{
  selected_post = cell_index->row;
  window_stack_push(s_post_window, false);
}

static uint16_t get_topics_count_callback(struct MenuLayer *menulayer, uint16_t section_index,
                                          void *callback_context)
{
  return num_topics;
}

static uint16_t get_post_count_callback(struct MenuLayer *menulayer, uint16_t section_index,
                                        void *callback_context)
{
  return num_posts;
}

#ifdef PBL_ROUND
static int16_t get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index,
                                        void *callback_context)
{
  return 60;
}
#endif

static void draw_trend_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index,
                                   void *callback_context)
{
  char *name = topics[cell_index->row].name;

  snprintf(s_topic_text, sizeof(s_topic_text), "%s", name);
  menu_cell_basic_draw(ctx, cell_layer, name, NULL, NULL);
}

static void draw_post_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index,
                                  void *callback_context)
{
  char *handle = posts[cell_index->row].handle;

  char *preview = "";
  strncpy(preview, posts[cell_index->row].text, 24);
  preview[24] = '\0';

  menu_cell_basic_draw(ctx, cell_layer, preview, handle, NULL);
}

static void draw_topic_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index,
                              void *callback_context)
{
  menu_cell_basic_header_draw(ctx, cell_layer, "Trending");
}

static void draw_feed_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index,
                             void *callback_context)
{
  menu_cell_basic_header_draw(ctx, cell_layer, topics[selected_feed].name);
}

static int16_t get_header_height(MenuLayer *menu_layer, uint16_t section_index, void *data)
{
  return 16;
}

static void topic_window_load(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_topic_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_topic_layer, NULL, (MenuLayerCallbacks){
                                                    .get_num_rows = get_topics_count_callback,
                                                    .get_cell_height = PBL_IF_ROUND_ELSE(get_cell_height_callback, NULL),
                                                    .draw_row = draw_trend_row_handler,
                                                    .select_click = select_feed_callback,
                                                    .draw_header = draw_topic_header,
                                                    .get_header_height = get_header_height,
                                                });
  menu_layer_set_click_config_onto_window(s_topic_layer, window);
  menu_layer_set_highlight_colors(s_topic_layer, GColorPictonBlue, GColorBlack);
  layer_add_child(window_layer, menu_layer_get_layer(s_topic_layer));
}

static void topic_window_unload(Window *window)
{
  menu_layer_destroy(s_topic_layer);
}

static void feed_window_load(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_feed_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_feed_layer, NULL, (MenuLayerCallbacks){
                                                   .get_num_rows = get_post_count_callback,
                                                   .get_cell_height = PBL_IF_ROUND_ELSE(get_cell_height_callback, NULL),
                                                   .draw_row = draw_post_row_handler,
                                                   .select_click = select_post_callback,
                                                   .draw_header = draw_feed_header,
                                                   .get_header_height = get_header_height,
                                               });
  menu_layer_set_click_config_onto_window(s_feed_layer, window);
  menu_layer_set_highlight_colors(s_feed_layer, GColorPictonBlue, GColorBlack);
  layer_add_child(window_layer, menu_layer_get_layer(s_feed_layer));
}

static void feed_window_unload(Window *window)
{
  menu_layer_destroy(s_feed_layer);
}

static void post_window_load(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_post_layer = scroll_layer_create(bounds);
  scroll_layer_set_click_config_onto_window(s_post_layer, window);

  s_handle_layer = text_layer_create(GRect(2, 0, bounds.size.w - 4, 40));
  text_layer_set_text(s_handle_layer, posts[selected_post].handle);
  text_layer_set_background_color(s_handle_layer, GColorClear);
  text_layer_set_text_color(s_handle_layer, GColorBlack);
  text_layer_set_font(s_handle_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  scroll_layer_add_child(s_post_layer, text_layer_get_layer(s_handle_layer));

  s_post_text_layer = text_layer_create(GRect(0, 30, bounds.size.w, 500));
  text_layer_set_text(s_post_text_layer, posts[selected_post].text);
  text_layer_set_background_color(s_post_text_layer, GColorClear);
  text_layer_set_text_color(s_post_text_layer, GColorBlack);
  text_layer_set_font(s_post_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  scroll_layer_add_child(s_post_layer, text_layer_get_layer(s_post_text_layer));

  scroll_layer_set_content_size(s_post_layer, GSize(bounds.size.w, 530));
  layer_add_child(window_layer, scroll_layer_get_layer(s_post_layer));
}

static void post_window_unload(Window *window)
{
  text_layer_destroy(s_handle_layer);
  text_layer_destroy(s_post_text_layer);
  scroll_layer_destroy(s_post_layer);
}

static void inbox_recv_callback(DictionaryIterator *iterator, void *context)
{
  Tuple *msg_type = dict_find(iterator, MESSAGE_KEY_MessageType);
  if (!msg_type)
  {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "RequestType not found!");
    return;
  }

  if (strcmp(msg_type->value->cstring, "topic-count") == 0)
  {
    Tuple *count_tuple = dict_find(iterator, MESSAGE_KEY_Count);
    num_topics = count_tuple->value->int32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Total topics: %d", num_topics);
  }
  else if (strcmp(msg_type->value->cstring, "topics") == 0)
  {
    Tuple *name_tuple = dict_find(iterator, MESSAGE_KEY_TopicName);
    Tuple *id_tuple = dict_find(iterator, MESSAGE_KEY_TopicId);
    char *name = name_tuple->value->cstring;
    char *id = id_tuple->value->cstring;
    strncpy(topics[loaded_topics].name, name, sizeof(topics[loaded_topics].name) - 1);
    topics[loaded_topics].name[sizeof(topics[loaded_topics].name) - 1] = '\0';
    strncpy(topics[loaded_topics].id, id, sizeof(topics[loaded_topics].id) - 1);
    topics[loaded_topics].id[sizeof(topics[loaded_topics].id) - 1] = '\0';
    loaded_topics++;

    if (num_topics == loaded_topics)
    {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Reloading menu");
      menu_layer_reload_data(s_topic_layer);
    }
  }
  else if (strcmp(msg_type->value->cstring, "post-count") == 0)
  {
    Tuple *count_tuple = dict_find(iterator, MESSAGE_KEY_Count);
    num_posts = count_tuple->value->int32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Total posts: %d", num_posts);
  }
  else if (strcmp(msg_type->value->cstring, "posts") == 0)
  {
    Tuple *handle_tuple = dict_find(iterator, MESSAGE_KEY_PostHandle);
    Tuple *text_tuple = dict_find(iterator, MESSAGE_KEY_PostText);
    char *handle = handle_tuple->value->cstring;
    char *text = text_tuple->value->cstring;
    strncpy(posts[loaded_posts].handle, handle, sizeof(posts[loaded_posts].handle) - 1);
    posts[loaded_posts].handle[sizeof(posts[loaded_posts].handle) - 1] = '\0';
    strncpy(posts[loaded_posts].text, text, sizeof(posts[loaded_posts].text) - 1);
    posts[loaded_posts].text[sizeof(posts[loaded_posts].text) - 1] = '\0';
    loaded_posts++;

    if (num_posts == loaded_posts)
    {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Reloading feed");
      menu_layer_reload_data(s_feed_layer);
    }
  }
}

static void inbox_drop_callback(AppMessageResult reason, void *context)
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Message dropped!");
}

static void outbox_fail_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context)
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context)
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Outbox send success!");
}

static void init(void)
{
  s_topics_window = window_create();
  window_set_window_handlers(s_topics_window, (WindowHandlers){
                                                  .load = topic_window_load,
                                                  .unload = topic_window_unload,
                                              });
  s_feed_window = window_create();
  window_set_window_handlers(s_feed_window, (WindowHandlers){
                                                .load = feed_window_load,
                                                .unload = feed_window_unload,
                                            });
  s_post_window = window_create();
  window_set_window_handlers(s_post_window, (WindowHandlers){
                                                .load = post_window_load,
                                                .unload = post_window_unload,
                                            });

  window_stack_push(s_topics_window, false);

  app_message_register_inbox_received(inbox_recv_callback);
  app_message_register_inbox_dropped(inbox_drop_callback);
  app_message_register_outbox_failed(outbox_fail_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  const int inbox_size = 512;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);
}

static void deinit(void)
{
  window_destroy(s_topics_window);
  window_destroy(s_feed_window);
  window_destroy(s_post_window);
}

int main(void)
{
  init();
  app_event_loop();
  deinit();
}
