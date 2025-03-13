#include <pebble.h>

#define MAX_TOPICS 10
#define MAX_POSTS 25
#define MAX_FEEDS 15

static Window *s_sections_window;
static MenuLayer *s_sections_layer;

static Window *s_user_feeds_window;
static MenuLayer *s_user_feeds_layer;
static TextLayer *s_user_feeds_loaded;

static Window *s_topics_window;
static MenuLayer *s_topic_layer;
static TextLayer *s_topics_loaded;

static Window *s_feed_window;
static MenuLayer *s_feed_layer;
static TextLayer *s_feed_loaded;

static Window *s_post_window;
static ScrollLayer *s_post_layer;
static TextLayer *s_handle_layer;
static TextLayer *s_post_text_layer;
static TextLayer *s_name_layer;
static TextLayer *s_time_layer;

char time_buffer[24];

static char s_user_feed_text[64];

static char s_topic_text[64];

static int num_user_feeds = 0;
static int loaded_user_feeds = 0;

static int num_topics = 0;
static int loaded_topics = 0;

static int num_posts = 0;
static int loaded_posts = 0;

static char loaded_buffer[64];

static char *feed_id;

static int selected_feed;
static int selected_post;

typedef struct Feed
{
  char name[64];
  char id[64];
} Feed;

typedef struct Post
{
  char name[64];
  char handle[64];
  char text[352];
  int seconds_ago;
} Post;

static Feed user_feeds[MAX_FEEDS];
static Feed topics[MAX_TOPICS];
static Post posts[MAX_POSTS];

static void select_section_callback(struct MenuLayer *s_menu_layer, MenuIndex *cell_index, void *callback_context)
{
  memset(loaded_buffer, 0, sizeof(loaded_buffer));
  if (cell_index->row == 0)
  {
    loaded_topics = 0;
    for (int i = 0; i < MAX_TOPICS; i++)
    {
      memset(topics[i].name, 0, sizeof(topics[i].name));
      memset(topics[i].id, 0, sizeof(topics[i].id));
    }
    window_stack_push(s_topics_window, true);

    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_cstring(iter, MESSAGE_KEY_MessageType, "topics");
    app_message_outbox_send();
  }
  else if (cell_index->row == 1)
  {
    loaded_user_feeds = 0;
    for (int i = 0; i < MAX_FEEDS; i++)
    {
      memset(user_feeds[i].name, 0, sizeof(user_feeds[i].name));
      memset(user_feeds[i].id, 0, sizeof(user_feeds[i].id));
    }
    window_stack_push(s_user_feeds_window, true);

    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_cstring(iter, MESSAGE_KEY_MessageType, "user-feeds");
    app_message_outbox_send();
  }
}

static void select_user_feed_callback(struct MenuLayer *s_menu_layer, MenuIndex *cell_index, void *callback_context)
{
  if (num_user_feeds == 0 || num_user_feeds != loaded_user_feeds)
  {
    return;
  }
  memset(loaded_buffer, 0, sizeof(loaded_buffer));
  loaded_posts = 0;
  for (int i = 0; i < MAX_POSTS; i++)
  {
    memset(posts[i].handle, 0, sizeof(posts[i].handle));
    memset(posts[i].text, 0, sizeof(posts[i].text));
    memset(posts[i].name, 0, sizeof(posts[i].name));
    posts[i].seconds_ago = 0;
  }
  feed_id = user_feeds[cell_index->row].id;
  selected_feed = cell_index->row;
  window_stack_push(s_feed_window, true);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_cstring(iter, MESSAGE_KEY_MessageType, "feed");
  dict_write_cstring(iter, MESSAGE_KEY_FeedId, feed_id);
  app_message_outbox_send();
}

static void select_trending_feed_callback(struct MenuLayer *s_menu_layer, MenuIndex *cell_index,
                                          void *callback_context)
{
  if (num_topics == 0 || num_topics != loaded_topics)
  {
    return;
  }
  memset(loaded_buffer, 0, sizeof(loaded_buffer));
  loaded_posts = 0;
  for (int i = 0; i < MAX_POSTS; i++)
  {
    memset(posts[i].handle, 0, sizeof(posts[i].handle));
    memset(posts[i].text, 0, sizeof(posts[i].text));
    memset(posts[i].name, 0, sizeof(posts[i].name));
    posts[i].seconds_ago = 0;
  }
  feed_id = topics[cell_index->row].id;
  selected_feed = cell_index->row;
  window_stack_push(s_feed_window, true);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_cstring(iter, MESSAGE_KEY_MessageType, "topic");
  dict_write_cstring(iter, MESSAGE_KEY_TopicId, feed_id);
  app_message_outbox_send();
}

static void select_post_callback(struct MenuLayer *s_menu_layer, MenuIndex *cell_index,
                                 void *callback_context)
{
  if (num_posts == 0 || num_posts != loaded_posts)
  {
    return;
  }
  memset(time_buffer, 0, sizeof(time_buffer));
  selected_post = cell_index->row;
  if (posts[selected_post].seconds_ago < 60)
  {
    snprintf(time_buffer, sizeof(time_buffer), "Just now");
  }
  else if (posts[selected_post].seconds_ago < 3600)
  {
    snprintf(time_buffer, sizeof(time_buffer), "%d minutes ago", posts[selected_post].seconds_ago / 60);
  }
  else if (posts[selected_post].seconds_ago < 86400)
  {
    snprintf(time_buffer, sizeof(time_buffer), "%d hours ago", posts[selected_post].seconds_ago / 3600);
  }
  else
  {
    snprintf(time_buffer, sizeof(time_buffer), "%d days ago", posts[selected_post].seconds_ago / 86400);
  }
  window_stack_push(s_post_window, true);
}

static uint16_t get_sections_count_callback(struct MenuLayer *s_menu_layer, uint16_t section_index, void *callback_context)
{
  return 2;
}

static uint16_t get_user_feeds_count_callback(struct MenuLayer *menulayer, uint16_t section_index,
                                              void *callback_context)
{
  return num_user_feeds;
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

static void draw_section_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index,
                                     void *callback_context)
{
  if (cell_index->row == 0)
  {
    menu_cell_basic_draw(ctx, cell_layer, "Trending", NULL, NULL);
  }
  else if (cell_index->row == 1)
  {
    menu_cell_basic_draw(ctx, cell_layer, "My Feeds", NULL, NULL);
  }
}

static void draw_user_feed_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index,
                                       void *callback_context)
{
  char *name = user_feeds[cell_index->row].name;

  snprintf(s_user_feed_text, sizeof(s_user_feed_text), "%s", name);
  menu_cell_basic_draw(ctx, cell_layer, name, NULL, NULL);
}

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

static void draw_section_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index,
                                void *callback_context)
{
  menu_cell_basic_header_draw(ctx, cell_layer, PBL_IF_ROUND_ELSE("     Pebblsky", "Pebblsky"));
}

static void draw_user_feeds_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index,
                                   void *callback_context)
{
  menu_cell_basic_header_draw(ctx, cell_layer, PBL_IF_ROUND_ELSE("     My Feeds", "My Feeds"));
}

static void draw_topic_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index,
                              void *callback_context)
{
  menu_cell_basic_header_draw(ctx, cell_layer, PBL_IF_ROUND_ELSE("     Trending", "Trending"));
}

static void draw_feed_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index,
                             void *callback_context)
{
  char round_name[32] = "     ";
  strcat(round_name, topics[selected_feed].name);
  menu_cell_basic_header_draw(ctx, cell_layer, PBL_IF_ROUND_ELSE(round_name, topics[selected_feed].name));
}

static int16_t get_header_height(MenuLayer *menu_layer, uint16_t section_index, void *data)
{
  return 16;
}

static void sections_window_load(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_sections_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_sections_layer, NULL, (MenuLayerCallbacks){
                                                       .get_num_rows = get_sections_count_callback,
                                                       .get_cell_height = PBL_IF_ROUND_ELSE(get_cell_height_callback, NULL),
                                                       .draw_row = draw_section_row_handler,
                                                       .select_click = select_section_callback,
                                                       .draw_header = draw_section_header,
                                                       .get_header_height = get_header_height,
                                                   });
  menu_layer_set_click_config_onto_window(s_sections_layer, window);
  menu_layer_set_highlight_colors(s_sections_layer, GColorPictonBlue, GColorBlack);
  layer_add_child(window_layer, menu_layer_get_layer(s_sections_layer));
}

static void sections_window_unload(Window *window)
{
  menu_layer_destroy(s_sections_layer);
}

static void user_feeds_window_load(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_user_feeds_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_user_feeds_layer, NULL, (MenuLayerCallbacks){
                                                         .get_num_rows = get_user_feeds_count_callback,
                                                         .get_cell_height = PBL_IF_ROUND_ELSE(get_cell_height_callback, NULL),
                                                         .draw_row = draw_user_feed_row_handler,
                                                         .select_click = select_user_feed_callback,
                                                         .draw_header = draw_user_feeds_header,
                                                         .get_header_height = get_header_height,
                                                     });
  menu_layer_set_click_config_onto_window(s_user_feeds_layer, window);
  menu_layer_set_highlight_colors(s_user_feeds_layer, GColorPictonBlue, GColorBlack);
  layer_add_child(window_layer, menu_layer_get_layer(s_user_feeds_layer));
  layer_set_hidden(menu_layer_get_layer(s_user_feeds_layer), true);

  s_user_feeds_loaded = text_layer_create(GRect(0, bounds.size.h / 2 - 20, bounds.size.w, 40));
  text_layer_set_text(s_user_feeds_loaded, "Catching butterflies...");
  text_layer_set_background_color(s_user_feeds_loaded, GColorClear);
  text_layer_set_text_color(s_user_feeds_loaded, GColorBlack);
  text_layer_set_text_alignment(s_user_feeds_loaded, GTextAlignmentCenter);
  text_layer_set_font(s_user_feeds_loaded, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_user_feeds_loaded));
}

static void user_feeds_window_unload(Window *window)
{
  text_layer_destroy(s_user_feeds_loaded);
  menu_layer_destroy(s_user_feeds_layer);
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
                                                    .select_click = select_trending_feed_callback,
                                                    .draw_header = draw_topic_header,
                                                    .get_header_height = get_header_height,
                                                });
  menu_layer_set_click_config_onto_window(s_topic_layer, window);
  menu_layer_set_highlight_colors(s_topic_layer, GColorPictonBlue, GColorBlack);
  layer_add_child(window_layer, menu_layer_get_layer(s_topic_layer));
  layer_set_hidden(menu_layer_get_layer(s_topic_layer), true);

  s_topics_loaded = text_layer_create(GRect(0, bounds.size.h / 2 - 10, bounds.size.w, 20));
  text_layer_set_text(s_topics_loaded, "");
  text_layer_set_background_color(s_topics_loaded, GColorClear);
  text_layer_set_text_color(s_topics_loaded, GColorBlack);
  text_layer_set_text_alignment(s_topics_loaded, GTextAlignmentCenter);
  text_layer_set_font(s_topics_loaded, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_topics_loaded));
  layer_set_hidden(text_layer_get_layer(s_topics_loaded), true);
}

static void topic_window_unload(Window *window)
{
  text_layer_destroy(s_topics_loaded);
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
  layer_set_hidden(menu_layer_get_layer(s_feed_layer), true);

  s_feed_loaded = text_layer_create(GRect(0, bounds.size.h / 2 - 10, bounds.size.w, 20));
  text_layer_set_text(s_feed_loaded, "");
  text_layer_set_background_color(s_feed_loaded, GColorClear);
  text_layer_set_text_color(s_feed_loaded, GColorBlack);
  text_layer_set_text_alignment(s_feed_loaded, GTextAlignmentCenter);
  text_layer_set_font(s_feed_loaded, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_feed_loaded));
  layer_set_hidden(text_layer_get_layer(s_feed_loaded), true);
}

static void feed_window_unload(Window *window)
{
  text_layer_destroy(s_feed_loaded);
  menu_layer_destroy(s_feed_layer);
}

static void post_window_load(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_post_layer = scroll_layer_create(bounds);
  scroll_layer_set_click_config_onto_window(s_post_layer, window);

  int x_padding = PBL_IF_ROUND_ELSE(10, 3);
  int y_padding = PBL_IF_ROUND_ELSE(40, 2);

  int total_height = y_padding;
  s_name_layer = text_layer_create(GRect(x_padding, y_padding, bounds.size.w - x_padding * 2, 40));
  text_layer_set_text(s_name_layer, posts[selected_post].name);
  text_layer_set_background_color(s_name_layer, GColorClear);
  text_layer_set_text_color(s_name_layer, GColorBlack);
  text_layer_set_text_alignment(s_name_layer, GTextAlignmentCenter);
  text_layer_set_font(s_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  scroll_layer_add_child(s_post_layer, text_layer_get_layer(s_name_layer));
  GSize name_bounds = text_layer_get_content_size(s_name_layer);
  text_layer_set_size(s_name_layer, (GSize){.h = name_bounds.h + 2, .w = bounds.size.w - x_padding * 2});
  total_height += name_bounds.h + 2;

  s_handle_layer = text_layer_create(GRect(x_padding, total_height, bounds.size.w - x_padding * 2, 40));
  text_layer_set_text(s_handle_layer, posts[selected_post].handle);
  text_layer_set_background_color(s_handle_layer, GColorClear);
  text_layer_set_text_color(s_handle_layer, GColorBlack);
  text_layer_set_text_alignment(s_handle_layer, GTextAlignmentCenter);
  text_layer_set_font(s_handle_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  scroll_layer_add_child(s_post_layer, text_layer_get_layer(s_handle_layer));
  GSize handle_bounds = text_layer_get_content_size(s_handle_layer);
  text_layer_set_size(s_handle_layer, (GSize){.h = handle_bounds.h + 2, .w = bounds.size.w - x_padding * 2});
  total_height += handle_bounds.h + 2;

  s_time_layer = text_layer_create(GRect(x_padding, total_height, bounds.size.w - x_padding * 2, 40));
  text_layer_set_text(s_time_layer, time_buffer);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  scroll_layer_add_child(s_post_layer, text_layer_get_layer(s_time_layer));
  GSize time_bounds = text_layer_get_content_size(s_time_layer);
  text_layer_set_size(s_time_layer, (GSize){.h = time_bounds.h + 2, .w = bounds.size.w - x_padding * 2});
  total_height += time_bounds.h + 2;

  s_post_text_layer = text_layer_create(GRect(x_padding, total_height, bounds.size.w - x_padding * 2, 500));
  text_layer_set_text(s_post_text_layer, posts[selected_post].text);
  text_layer_set_background_color(s_post_text_layer, GColorClear);
  text_layer_set_text_color(s_post_text_layer, GColorBlack);
  text_layer_set_text_alignment(s_post_text_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft));
  text_layer_set_font(s_post_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  scroll_layer_add_child(s_post_layer, text_layer_get_layer(s_post_text_layer));
  GSize text_bounds = text_layer_get_content_size(s_post_text_layer);
  text_layer_set_size(s_post_text_layer, (GSize){.h = text_bounds.h + 2, .w = bounds.size.w - x_padding * 2});
  total_height += text_bounds.h + 2;

  scroll_layer_set_content_size(s_post_layer, GSize(bounds.size.w, total_height + y_padding));
  layer_add_child(window_layer, scroll_layer_get_layer(s_post_layer));
}

static void post_window_unload(Window *window)
{
  text_layer_destroy(s_name_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_handle_layer);
  text_layer_destroy(s_post_text_layer);
  scroll_layer_destroy(s_post_layer);
}

static void inbox_recv_callback(DictionaryIterator *iterator, void *context)
{
  Tuple *msg_type = dict_find(iterator, MESSAGE_KEY_MessageType);
  if (!msg_type)
  {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "MessageType not found!");
    return;
  }

  if (strcmp(msg_type->value->cstring, "topic-count") == 0)
  {
    Tuple *count_tuple = dict_find(iterator, MESSAGE_KEY_Count);
    num_topics = count_tuple->value->int32;
    if (num_topics > MAX_TOPICS)
    {
      num_topics = MAX_TOPICS;
    }
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

    snprintf(loaded_buffer, sizeof(loaded_buffer), "Loaded %d/%d", loaded_topics, num_topics);
    text_layer_set_text(s_topics_loaded, loaded_buffer);
    layer_set_hidden(text_layer_get_layer(s_topics_loaded), false);

    if (num_topics == loaded_topics)
    {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Reloading menu");
      menu_layer_reload_data(s_topic_layer);
      layer_set_hidden(menu_layer_get_layer(s_topic_layer), false);
      layer_set_hidden(text_layer_get_layer(s_topics_loaded), true);
    }
  }
  else if (strcmp(msg_type->value->cstring, "post-count") == 0)
  {
    Tuple *count_tuple = dict_find(iterator, MESSAGE_KEY_Count);
    num_posts = count_tuple->value->int32;
    if (num_posts > MAX_POSTS)
    {
      num_posts = MAX_POSTS;
    }
  }
  else if (strcmp(msg_type->value->cstring, "posts") == 0)
  {
    Tuple *handle_tuple = dict_find(iterator, MESSAGE_KEY_PostHandle);
    Tuple *text_tuple = dict_find(iterator, MESSAGE_KEY_PostText);
    Tuple *name_tuple = dict_find(iterator, MESSAGE_KEY_PostName);
    Tuple *seconds_ago_tuple = dict_find(iterator, MESSAGE_KEY_PostTime);
    char *handle = handle_tuple->value->cstring;
    char *text = text_tuple->value->cstring;
    char *name = name_tuple->value->cstring;
    strncpy(posts[loaded_posts].handle, handle, sizeof(posts[loaded_posts].handle) - 1);
    posts[loaded_posts].handle[sizeof(posts[loaded_posts].handle) - 1] = '\0';
    strncpy(posts[loaded_posts].text, text, sizeof(posts[loaded_posts].text) - 1);
    posts[loaded_posts].text[sizeof(posts[loaded_posts].text) - 1] = '\0';
    strncpy(posts[loaded_posts].name, name, sizeof(posts[loaded_posts].name) - 1);
    posts[loaded_posts].name[sizeof(posts[loaded_posts].name) - 1] = '\0';
    posts[loaded_posts].seconds_ago = seconds_ago_tuple->value->int32;
    loaded_posts++;

    snprintf(loaded_buffer, sizeof(loaded_buffer), "Loaded %d/%d", loaded_posts, num_posts);
    text_layer_set_text(s_feed_loaded, loaded_buffer);
    layer_set_hidden(text_layer_get_layer(s_feed_loaded), false);

    if (num_posts == loaded_posts)
    {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Reloading feed");
      menu_layer_reload_data(s_feed_layer);
      layer_set_hidden(menu_layer_get_layer(s_feed_layer), false);
      layer_set_hidden(text_layer_get_layer(s_feed_loaded), true);
    }
  }
  else if (strcmp(msg_type->value->cstring, "feed-count") == 0)
  {
    Tuple *count_tuple = dict_find(iterator, MESSAGE_KEY_Count);
    num_user_feeds = count_tuple->value->int32;
    if (num_user_feeds > MAX_FEEDS)
    {
      num_user_feeds = MAX_FEEDS;
    }
  }
  else if (strcmp(msg_type->value->cstring, "feeds") == 0)
  {
    Tuple *name_tuple = dict_find(iterator, MESSAGE_KEY_FeedName);
    Tuple *id_tuple = dict_find(iterator, MESSAGE_KEY_FeedId);
    char *name = name_tuple->value->cstring;
    char *id = id_tuple->value->cstring;
    strncpy(user_feeds[loaded_user_feeds].name, name, sizeof(user_feeds[loaded_user_feeds].name) - 1);
    user_feeds[loaded_user_feeds].name[sizeof(user_feeds[loaded_user_feeds].name) - 1] = '\0';
    strncpy(user_feeds[loaded_user_feeds].id, id, sizeof(user_feeds[loaded_user_feeds].id) - 1);
    user_feeds[loaded_user_feeds].id[sizeof(user_feeds[loaded_user_feeds].id) - 1] = '\0';
    loaded_user_feeds++;

    snprintf(loaded_buffer, sizeof(loaded_buffer), "Loaded %d/%d", loaded_user_feeds, num_user_feeds);
    text_layer_set_text(s_user_feeds_loaded, loaded_buffer);
    layer_set_hidden(text_layer_get_layer(s_user_feeds_loaded), false);

    if (num_user_feeds == loaded_user_feeds)
    {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Reloading menu");
      menu_layer_reload_data(s_user_feeds_layer);
      layer_set_hidden(menu_layer_get_layer(s_user_feeds_layer), false);
      layer_set_hidden(text_layer_get_layer(s_user_feeds_loaded), true);
    }
  }
  else if (strcmp(msg_type->value->cstring, "session-error") == 0)
  {
    Tuple *error_tuple = dict_find(iterator, MESSAGE_KEY_Error);
    strncpy(loaded_buffer, error_tuple->value->cstring, sizeof(loaded_buffer) - 1);
    loaded_buffer[sizeof(loaded_buffer) - 1] = '\0';
    text_layer_set_text(s_user_feeds_loaded, loaded_buffer);
    layer_set_hidden(menu_layer_get_layer(s_user_feeds_layer), true);
    layer_set_hidden(text_layer_get_layer(s_user_feeds_loaded), false);
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
  s_sections_window = window_create();
  window_set_window_handlers(s_sections_window, (WindowHandlers){
                                                    .load = sections_window_load,
                                                    .unload = sections_window_unload,
                                                });
  s_topics_window = window_create();
  window_set_window_handlers(s_topics_window, (WindowHandlers){
                                                  .load = topic_window_load,
                                                  .unload = topic_window_unload,
                                              });
  s_user_feeds_window = window_create();
  window_set_window_handlers(s_user_feeds_window, (WindowHandlers){
                                                      .load = user_feeds_window_load,
                                                      .unload = user_feeds_window_unload,
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

  window_stack_push(s_sections_window, false);

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
  window_destroy(s_sections_window);
  window_destroy(s_user_feeds_window);
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
