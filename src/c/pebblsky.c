#include <pebble.h>

#define MAX_TOPICS 10
#define MAX_FEEDS 15
#define MAX_POSTS PBL_PLATFORM_SWITCH_DEFAULT(PBL_PLATFORM_TYPE_CURRENT, 15, 15, 25, 25, 25, 25, 25, 25)

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

static int num_user_feeds = 0;
static int loaded_user_feeds = 0;

static int num_topics = 0;
static int loaded_topics = 0;

static int num_posts = 0;
static int loaded_posts = 0;

static char loaded_buffer[64];
static char s_feed_title[64];

static char *feed_id;

static int selected_feed;
static int selected_post;
static bool s_feed_is_topic = false;

static uint16_t get_sections_count_callback(struct MenuLayer *s_menu_layer, uint16_t section_index,
                                            void *callback_context);
static uint16_t get_user_feeds_count_callback(struct MenuLayer *menulayer, uint16_t section_index,
                                              void *callback_context);
static uint16_t get_topics_count_callback(struct MenuLayer *menulayer, uint16_t section_index,
                                          void *callback_context);
static uint16_t get_post_count_callback(struct MenuLayer *menulayer, uint16_t section_index,
                                        void *callback_context);
static void draw_section_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index,
                                     void *callback_context);
static void draw_user_feed_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index,
                                       void *callback_context);
static void draw_trend_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index,
                                   void *callback_context);
static void draw_post_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index,
                                  void *callback_context);
static void draw_section_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index,
                                void *callback_context);
static void draw_user_feeds_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index,
                                   void *callback_context);
static void draw_topic_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index,
                              void *callback_context);
static void draw_feed_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index,
                             void *callback_context);
static int16_t get_header_height(MenuLayer *menu_layer, uint16_t section_index, void *data);

#if defined(_PBL_API_EXISTS_tap_recognizer_get_tap_point)
typedef struct {
  MenuLayer **menu_layer;
  void (*select_callback)(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
  MenuIndex armed_index;
  int16_t pan_base;
} MenuTouchState;

typedef struct {
  ScrollLayer **scroll_layer;
  int16_t pan_base;
} ScrollTouchState;

static MenuTouchState s_sections_touch = {
  .menu_layer = &s_sections_layer,
  .select_callback = NULL,
  .armed_index = {.section = MENU_INDEX_NOT_FOUND, .row = MENU_INDEX_NOT_FOUND},
  .pan_base = 0,
};

static MenuTouchState s_user_feeds_touch = {
  .menu_layer = &s_user_feeds_layer,
  .select_callback = NULL,
  .armed_index = {.section = MENU_INDEX_NOT_FOUND, .row = MENU_INDEX_NOT_FOUND},
  .pan_base = 0,
};

static MenuTouchState s_topics_touch = {
  .menu_layer = &s_topic_layer,
  .select_callback = NULL,
  .armed_index = {.section = MENU_INDEX_NOT_FOUND, .row = MENU_INDEX_NOT_FOUND},
  .pan_base = 0,
};

static MenuTouchState s_feed_touch = {
  .menu_layer = &s_feed_layer,
  .select_callback = NULL,
  .armed_index = {.section = MENU_INDEX_NOT_FOUND, .row = MENU_INDEX_NOT_FOUND},
  .pan_base = 0,
};

static ScrollTouchState s_post_touch = {
  .scroll_layer = &s_post_layer,
  .pan_base = 0,
};

static void reset_menu_touch_state(MenuTouchState *state);
static void reset_scroll_touch_state(ScrollTouchState *state);
static void sections_tap_handler(const Recognizer *recognizer, RecognizerEvent event);
static void sections_pan_handler(const Recognizer *recognizer, RecognizerEvent event);
static void user_feeds_tap_handler(const Recognizer *recognizer, RecognizerEvent event);
static void user_feeds_pan_handler(const Recognizer *recognizer, RecognizerEvent event);
static void topics_tap_handler(const Recognizer *recognizer, RecognizerEvent event);
static void topics_pan_handler(const Recognizer *recognizer, RecognizerEvent event);
static void feed_tap_handler(const Recognizer *recognizer, RecognizerEvent event);
static void feed_pan_handler(const Recognizer *recognizer, RecognizerEvent event);
static void post_pan_handler(const Recognizer *recognizer, RecognizerEvent event);
#endif

typedef struct Feed
{
  char name[64];
  char id[64];
} Feed;

typedef struct Post
{
  char name[64];
  char handle[64];
  char text[448];
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
  num_posts = 0;
  s_feed_is_topic = false;
  strncpy(s_feed_title, user_feeds[cell_index->row].name, sizeof(s_feed_title) - 1);
  s_feed_title[sizeof(s_feed_title) - 1] = '\0';
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
  num_posts = 0;
  s_feed_is_topic = true;
  for (int i = 0; i < MAX_POSTS; i++)
  {
    memset(posts[i].handle, 0, sizeof(posts[i].handle));
    memset(posts[i].text, 0, sizeof(posts[i].text));
    memset(posts[i].name, 0, sizeof(posts[i].name));
    posts[i].seconds_ago = 0;
  }
  strncpy(s_feed_title, topics[cell_index->row].name, sizeof(s_feed_title) - 1);
  s_feed_title[sizeof(s_feed_title) - 1] = '\0';
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

#if defined(_PBL_API_EXISTS_tap_recognizer_get_tap_point)

static MenuIndex invalid_menu_index(void)
{
  return (MenuIndex){.section = MENU_INDEX_NOT_FOUND, .row = MENU_INDEX_NOT_FOUND};
}

static bool menu_index_is_valid(MenuIndex index)
{
  return index.section != MENU_INDEX_NOT_FOUND && index.row != MENU_INDEX_NOT_FOUND;
}

static bool menu_index_equals(MenuIndex left, MenuIndex right)
{
  return left.section == right.section && left.row == right.row;
}

static uint16_t touch_row_count_for_menu(MenuLayer *menu_layer)
{
  if (menu_layer == s_sections_layer)
  {
    return 2;
  }
  if (menu_layer == s_user_feeds_layer)
  {
    return num_user_feeds;
  }
  if (menu_layer == s_topic_layer)
  {
    return num_topics;
  }
  if (menu_layer == s_feed_layer)
  {
    return num_posts;
  }
  return 0;
}

static int16_t touch_cell_height_for_menu(MenuLayer *menu_layer, MenuIndex index)
{
  (void)menu_layer;
  (void)index;
  return PBL_IF_ROUND_ELSE(60, 44);
}

static MenuIndex touch_index_for_point(MenuLayer *menu_layer, GPoint point)
{
  MenuIndex invalid = invalid_menu_index();
  if (!menu_layer)
  {
    return invalid;
  }

  GRect bounds = layer_get_bounds(menu_layer_get_layer(menu_layer));
  if (!grect_contains_point(&bounds, &point))
  {
    return invalid;
  }

  ScrollLayer *scroll_layer = menu_layer_get_scroll_layer(menu_layer);
  int16_t content_y = point.y - scroll_layer_get_content_offset(scroll_layer).y;
  int16_t header_height = 16;
  if (content_y < header_height)
  {
    return invalid;
  }

  content_y -= header_height;
  MenuIndex first_row = MenuIndex(0, 0);
  int16_t cell_height = touch_cell_height_for_menu(menu_layer, first_row);
  if (cell_height <= 0)
  {
    return invalid;
  }

  uint16_t row = content_y / cell_height;
  if (row >= touch_row_count_for_menu(menu_layer))
  {
    return invalid;
  }

  return MenuIndex(0, row);
}

static void reset_menu_touch_state(MenuTouchState *state)
{
  state->armed_index = invalid_menu_index();
  if (!state->menu_layer || !*state->menu_layer)
  {
    state->pan_base = 0;
    return;
  }

  state->pan_base = scroll_layer_get_content_offset(menu_layer_get_scroll_layer(*state->menu_layer)).y;
}

static void reset_scroll_touch_state(ScrollTouchState *state)
{
  if (!state->scroll_layer || !*state->scroll_layer)
  {
    state->pan_base = 0;
    return;
  }

  state->pan_base = scroll_layer_get_content_offset(*state->scroll_layer).y;
}

static void handle_menu_tap(MenuTouchState *state, const Recognizer *recognizer, RecognizerEvent event)
{
  if (event != RecognizerEvent_Completed || !state->menu_layer || !*state->menu_layer)
  {
    return;
  }

  MenuLayer *menu_layer = *state->menu_layer;
  MenuIndex target = touch_index_for_point(menu_layer, tap_recognizer_get_tap_point(recognizer));
  if (!menu_index_is_valid(target))
  {
    return;
  }

  if (menu_index_equals(state->armed_index, target) && state->select_callback)
  {
    state->select_callback(menu_layer, &target, NULL);
    state->armed_index = invalid_menu_index();
    return;
  }

  menu_layer_set_selected_index(menu_layer, target, MenuRowAlignCenter, true);
  state->armed_index = target;
}

static void handle_menu_pan(MenuTouchState *state, const Recognizer *recognizer, RecognizerEvent event)
{
  if (!state->menu_layer || !*state->menu_layer)
  {
    return;
  }

  ScrollLayer *scroll_layer = menu_layer_get_scroll_layer(*state->menu_layer);
  switch (event)
  {
    case RecognizerEvent_Started:
      state->pan_base = scroll_layer_get_content_offset(scroll_layer).y;
      state->armed_index = invalid_menu_index();
      break;
    case RecognizerEvent_Updated:
    {
      GPoint delta = pan_recognizer_get_delta_since_start(recognizer);
      scroll_layer_set_content_offset(scroll_layer, (GPoint){.x = 0, .y = state->pan_base + delta.y}, false);
      break;
    }
    case RecognizerEvent_Completed:
      state->pan_base = scroll_layer_get_content_offset(scroll_layer).y;
      break;
    case RecognizerEvent_Cancelled:
      scroll_layer_set_content_offset(scroll_layer, (GPoint){.x = 0, .y = state->pan_base}, true);
      break;
  }
}

static void handle_scroll_pan(ScrollTouchState *state, const Recognizer *recognizer, RecognizerEvent event)
{
  if (!state->scroll_layer || !*state->scroll_layer)
  {
    return;
  }

  ScrollLayer *scroll_layer = *state->scroll_layer;
  switch (event)
  {
    case RecognizerEvent_Started:
      state->pan_base = scroll_layer_get_content_offset(scroll_layer).y;
      break;
    case RecognizerEvent_Updated:
    {
      GPoint delta = pan_recognizer_get_delta_since_start(recognizer);
      scroll_layer_set_content_offset(scroll_layer, (GPoint){.x = 0, .y = state->pan_base + delta.y}, false);
      break;
    }
    case RecognizerEvent_Completed:
      state->pan_base = scroll_layer_get_content_offset(scroll_layer).y;
      break;
    case RecognizerEvent_Cancelled:
      scroll_layer_set_content_offset(scroll_layer, (GPoint){.x = 0, .y = state->pan_base}, true);
      break;
  }
}

static void back_swipe_handler(const Recognizer *recognizer, RecognizerEvent event)
{
  if (event != RecognizerEvent_Completed)
  {
    return;
  }

  if (swipe_recognizer_get_direction(recognizer) == SwipeDirection_Right)
  {
    window_stack_pop(true);
  }
}

static void sections_tap_handler(const Recognizer *recognizer, RecognizerEvent event)
{
  handle_menu_tap(&s_sections_touch, recognizer, event);
}

static void sections_pan_handler(const Recognizer *recognizer, RecognizerEvent event)
{
  handle_menu_pan(&s_sections_touch, recognizer, event);
}

static void user_feeds_tap_handler(const Recognizer *recognizer, RecognizerEvent event)
{
  handle_menu_tap(&s_user_feeds_touch, recognizer, event);
}

static void user_feeds_pan_handler(const Recognizer *recognizer, RecognizerEvent event)
{
  handle_menu_pan(&s_user_feeds_touch, recognizer, event);
}

static void topics_tap_handler(const Recognizer *recognizer, RecognizerEvent event)
{
  handle_menu_tap(&s_topics_touch, recognizer, event);
}

static void topics_pan_handler(const Recognizer *recognizer, RecognizerEvent event)
{
  handle_menu_pan(&s_topics_touch, recognizer, event);
}

static void feed_tap_handler(const Recognizer *recognizer, RecognizerEvent event)
{
  handle_menu_tap(&s_feed_touch, recognizer, event);
}

static void feed_pan_handler(const Recognizer *recognizer, RecognizerEvent event)
{
  handle_menu_pan(&s_feed_touch, recognizer, event);
}

static void post_pan_handler(const Recognizer *recognizer, RecognizerEvent event)
{
  handle_scroll_pan(&s_post_touch, recognizer, event);
}

static void attach_menu_touch_recognizers(Window *window, RecognizerEventCb tap_handler,
                                          RecognizerEventCb pan_handler, bool enable_back_swipe)
{
  window_set_touch_bridge_disabled(window, true);

  Recognizer *pan = pan_recognizer_create(pan_handler, NULL, PanAxis_Vertical);
  window_attach_recognizer(window, pan);

  Recognizer *tap = tap_recognizer_create(tap_handler, NULL);
  recognizer_set_fail_after(tap, pan);
  window_attach_recognizer(window, tap);

  if (enable_back_swipe)
  {
    Recognizer *back_swipe = swipe_recognizer_create(back_swipe_handler, NULL, SwipeDirection_Right);
    recognizer_set_fail_after(back_swipe, pan);
    window_attach_recognizer(window, back_swipe);
  }
}

static void attach_scroll_touch_recognizers(Window *window, RecognizerEventCb pan_handler,
                                            bool enable_back_swipe)
{
  window_set_touch_bridge_disabled(window, true);

  Recognizer *pan = pan_recognizer_create(pan_handler, NULL, PanAxis_Vertical);
  window_attach_recognizer(window, pan);

  if (enable_back_swipe)
  {
    Recognizer *back_swipe = swipe_recognizer_create(back_swipe_handler, NULL, SwipeDirection_Right);
    recognizer_set_fail_after(back_swipe, pan);
    window_attach_recognizer(window, back_swipe);
  }
}

#endif

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

  menu_cell_basic_draw(ctx, cell_layer, name, NULL, NULL);
}

static void draw_trend_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index,
                                   void *callback_context)
{
  char *name = topics[cell_index->row].name;

  menu_cell_basic_draw(ctx, cell_layer, name, NULL, NULL);
}

static void draw_post_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index,
                                  void *callback_context)
{
  char *handle = posts[cell_index->row].handle;

  char preview[25];
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
  strncat(round_name, s_feed_title, sizeof(round_name) - strlen(round_name) - 1);
  menu_cell_basic_header_draw(ctx, cell_layer, PBL_IF_ROUND_ELSE(round_name, s_feed_title));
}

static int16_t get_header_height(MenuLayer *menu_layer, uint16_t section_index, void *data)
{
  return 16;
}

#if defined(_PBL_API_EXISTS_tap_recognizer_get_tap_point)
static void sections_window_appear(Window *window)
{
  reset_menu_touch_state(&s_sections_touch);
}

static void user_feeds_window_appear(Window *window)
{
  reset_menu_touch_state(&s_user_feeds_touch);
}

static void topic_window_appear(Window *window)
{
  reset_menu_touch_state(&s_topics_touch);
}

static void feed_window_appear(Window *window)
{
  reset_menu_touch_state(&s_feed_touch);
}

static void post_window_appear(Window *window)
{
  reset_scroll_touch_state(&s_post_touch);
}
#else
static void sections_window_appear(Window *window)
{
  (void)window;
}

static void user_feeds_window_appear(Window *window)
{
  (void)window;
}

static void topic_window_appear(Window *window)
{
  (void)window;
}

static void feed_window_appear(Window *window)
{
  (void)window;
}

static void post_window_appear(Window *window)
{
  (void)window;
}
#endif

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
  menu_layer_set_highlight_colors(s_sections_layer, PBL_IF_BW_ELSE(GColorBlack, GColorPictonBlue), PBL_IF_BW_ELSE(GColorWhite, GColorBlack));
  layer_add_child(window_layer, menu_layer_get_layer(s_sections_layer));
#if defined(_PBL_API_EXISTS_tap_recognizer_get_tap_point)
  attach_menu_touch_recognizers(window, sections_tap_handler, sections_pan_handler, false);
  reset_menu_touch_state(&s_sections_touch);
#endif
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
  menu_layer_set_highlight_colors(s_user_feeds_layer, PBL_IF_BW_ELSE(GColorBlack, GColorPictonBlue), PBL_IF_BW_ELSE(GColorWhite, GColorBlack));
  layer_add_child(window_layer, menu_layer_get_layer(s_user_feeds_layer));
  layer_set_hidden(menu_layer_get_layer(s_user_feeds_layer), true);
#if defined(_PBL_API_EXISTS_tap_recognizer_get_tap_point)
  attach_menu_touch_recognizers(window, user_feeds_tap_handler, user_feeds_pan_handler, true);
  reset_menu_touch_state(&s_user_feeds_touch);
#endif

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
  menu_layer_set_highlight_colors(s_topic_layer, PBL_IF_BW_ELSE(GColorBlack, GColorPictonBlue), PBL_IF_BW_ELSE(GColorWhite, GColorBlack));
  layer_add_child(window_layer, menu_layer_get_layer(s_topic_layer));
  layer_set_hidden(menu_layer_get_layer(s_topic_layer), true);
#if defined(_PBL_API_EXISTS_tap_recognizer_get_tap_point)
  attach_menu_touch_recognizers(window, topics_tap_handler, topics_pan_handler, true);
  reset_menu_touch_state(&s_topics_touch);
#endif

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
  menu_layer_set_highlight_colors(s_feed_layer, PBL_IF_BW_ELSE(GColorBlack, GColorPictonBlue), PBL_IF_BW_ELSE(GColorWhite, GColorBlack));
  layer_add_child(window_layer, menu_layer_get_layer(s_feed_layer));
  layer_set_hidden(menu_layer_get_layer(s_feed_layer), true);
#if defined(_PBL_API_EXISTS_tap_recognizer_get_tap_point)
  attach_menu_touch_recognizers(window, feed_tap_handler, feed_pan_handler, true);
  reset_menu_touch_state(&s_feed_touch);
#endif

  s_feed_loaded = text_layer_create(GRect(0, bounds.size.h / 2 - 10, bounds.size.w, 20));
  text_layer_set_text(s_feed_loaded, "Loading posts...");
  text_layer_set_background_color(s_feed_loaded, GColorClear);
  text_layer_set_text_color(s_feed_loaded, GColorBlack);
  text_layer_set_text_alignment(s_feed_loaded, GTextAlignmentCenter);
  text_layer_set_font(s_feed_loaded, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_feed_loaded));
  layer_set_hidden(text_layer_get_layer(s_feed_loaded), false);
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
#if defined(_PBL_API_EXISTS_tap_recognizer_get_tap_point)
  attach_scroll_touch_recognizers(window, post_pan_handler, true);
#endif

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
#if defined(_PBL_API_EXISTS_tap_recognizer_get_tap_point)
  reset_scroll_touch_state(&s_post_touch);
#endif
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
    if (num_posts == 0)
    {
      text_layer_set_text(s_feed_loaded, "No posts found!");
      layer_set_hidden(menu_layer_get_layer(s_feed_layer), true);
      layer_set_hidden(text_layer_get_layer(s_feed_loaded), false);
    }
    else
    {
      text_layer_set_text(s_feed_loaded, "Loading posts...");
      layer_set_hidden(menu_layer_get_layer(s_feed_layer), true);
      layer_set_hidden(text_layer_get_layer(s_feed_loaded), false);
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
    } else if (num_user_feeds == 0) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "No feeds found!");
      text_layer_set_text(s_user_feeds_loaded, "No feeds found!");
      layer_set_hidden(menu_layer_get_layer(s_user_feeds_layer), true);
      layer_set_hidden(text_layer_get_layer(s_user_feeds_loaded), false);
      return;
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
#if defined(_PBL_API_EXISTS_app_touch_navigation_enable)
  app_touch_navigation_enable(true);
#endif

  s_sections_window = window_create();
  window_set_window_handlers(s_sections_window, (WindowHandlers){
                                                    .load = sections_window_load,
                            .appear = sections_window_appear,
                                                    .unload = sections_window_unload,
                                                });
  s_topics_window = window_create();
  window_set_window_handlers(s_topics_window, (WindowHandlers){
                                                  .load = topic_window_load,
                            .appear = topic_window_appear,
                                                  .unload = topic_window_unload,
                                              });
  s_user_feeds_window = window_create();
  window_set_window_handlers(s_user_feeds_window, (WindowHandlers){
                                                      .load = user_feeds_window_load,
                              .appear = user_feeds_window_appear,
                                                      .unload = user_feeds_window_unload,
                                                  });
  s_feed_window = window_create();
  window_set_window_handlers(s_feed_window, (WindowHandlers){
                                                .load = feed_window_load,
                          .appear = feed_window_appear,
                                                .unload = feed_window_unload,
                                            });
  s_post_window = window_create();
  window_set_window_handlers(s_post_window, (WindowHandlers){
                                                .load = post_window_load,
                          .appear = post_window_appear,
                                                .unload = post_window_unload,
                                            });

#if defined(_PBL_API_EXISTS_tap_recognizer_get_tap_point)

    s_sections_touch.select_callback = select_section_callback;
    s_user_feeds_touch.select_callback = select_user_feed_callback;
    s_topics_touch.select_callback = select_trending_feed_callback;
    s_feed_touch.select_callback = select_post_callback;
#endif

  window_stack_push(s_sections_window, false);

  app_message_register_inbox_received(inbox_recv_callback);
  app_message_register_inbox_dropped(inbox_drop_callback);
  app_message_register_outbox_failed(outbox_fail_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  const int inbox_size = 592;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);
}

static void deinit(void)
{
#if defined(_PBL_API_EXISTS_app_touch_navigation_enable)
  app_touch_navigation_enable(false);
#endif

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
