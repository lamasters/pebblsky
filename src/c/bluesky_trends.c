#include <pebble.h>

#define MAX_TOPICS 10

static Window *s_topics_window;
static MenuLayer *s_topic_layer;

static char s_topic_text[32];

static int num_topics = 0;
static int loaded_topics = 0;

typedef struct Topic
{
  char name[32];
  char url[128];
} Topic;

static Topic topics[MAX_TOPICS];

static void select_callback(struct MenuLayer *s_menu_layer, MenuIndex *cell_index,
                            void *callback_context)
{
  // Switch to countdown window
  // window_stack_push(s_countdown_window, false);
}

static uint16_t get_sections_count_callback(struct MenuLayer *menulayer, uint16_t section_index,
                                            void *callback_context)
{
  return num_topics;
}

#ifdef PBL_ROUND
static int16_t get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index,
                                        void *callback_context)
{
  return 60;
}
#endif

static void draw_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index,
                             void *callback_context)
{
  char *name = topics[cell_index->row].name;

  snprintf(s_topic_text, sizeof(s_topic_text), "%s", name);
  menu_cell_basic_draw(ctx, cell_layer, name, NULL, NULL);
}

static void menu_window_load(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_topic_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_topic_layer, NULL, (MenuLayerCallbacks){.get_num_rows = get_sections_count_callback, .get_cell_height = PBL_IF_ROUND_ELSE(get_cell_height_callback, NULL), .draw_row = draw_row_handler, .select_click = select_callback});
  menu_layer_set_click_config_onto_window(s_topic_layer, window);
  menu_layer_set_highlight_colors(s_topic_layer, GColorPictonBlue, GColorWhite);
  layer_add_child(window_layer, menu_layer_get_layer(s_topic_layer));
}

static void menu_window_unload(Window *window)
{
  menu_layer_destroy(s_topic_layer);
}

static void inbox_recv_callback(DictionaryIterator *iterator, void *context)
{
  Tuple *msg_type = dict_find(iterator, MESSAGE_KEY_MessageType);
  if (!msg_type)
  {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "RequestType not found!");
    return;
  }

  if (strcmp(msg_type->value->cstring, "count") == 0)
  {
    Tuple *count_tuple = dict_find(iterator, MESSAGE_KEY_Count);
    num_topics = count_tuple->value->int32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Total topics: %d", num_topics);
  }
  if (strcmp(msg_type->value->cstring, "topics") == 0)
  {
    Tuple *name_tuple = dict_find(iterator, MESSAGE_KEY_TopicName);
    Tuple *url_tuple = dict_find(iterator, MESSAGE_KEY_TopicURL);
    char *name = name_tuple->value->cstring;
    char *url = url_tuple->value->cstring;
    strncpy(topics[loaded_topics].name, name, sizeof(topics[loaded_topics].name) - 1);
    topics[loaded_topics].name[sizeof(topics[loaded_topics].name) - 1] = '\0';
    loaded_topics++;
  }

  if (num_topics == loaded_topics)
  {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Reloading menu");
    menu_layer_reload_data(s_topic_layer);
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
                                                  .load = menu_window_load,
                                                  .unload = menu_window_unload,
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
}

int main(void)
{
  init();
  app_event_loop();
  deinit();
}
