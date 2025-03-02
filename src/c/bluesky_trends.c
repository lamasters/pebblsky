#include <pebble.h>

#define MAX_TOPICS 10
#define MAX_TOPIC_LENGTH 32

static Window *s_topics_window;
static MenuLayer *s_topic_layer;

static char s_topic_text[32];

static char* topics[MAX_TOPICS];
static int num_topics = 0;

void split_string_into_array(const char *input) {
  // Clear previous topics
  for (int i = 0; i < num_topics; i++) {
    free(topics[i]);
  }
  num_topics = 0;

  // Allocate memory for the input copy
  char *input_copy = malloc(strlen(input) + 1);
  if (!input_copy) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to allocate memory for input copy");
    return;
  }

  // Copy the input string to the allocated memory
  strcpy(input_copy, input);

  // Split the string by commas manually
  char *start = input_copy;
  char *end = input_copy;
  while (*end != '\0' && num_topics < MAX_TOPICS) {
    if (*end == ',') {
      *end = '\0';
      topics[num_topics] = malloc(MAX_TOPIC_LENGTH);
      if (topics[num_topics] == NULL) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to allocate memory for topic");
        break;
      }
      strncpy(topics[num_topics], start, MAX_TOPIC_LENGTH - 1);
      topics[num_topics][MAX_TOPIC_LENGTH - 1] = '\0'; // Ensure null-termination
      num_topics++;
      start = end + 1;
    }
    end++;
  }

  // Add the last topic
  if (num_topics < MAX_TOPICS) {
    topics[num_topics] = malloc(MAX_TOPIC_LENGTH);
    if (topics[num_topics] == NULL) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to allocate memory for topic");
    } else {
      strncpy(topics[num_topics], start, MAX_TOPIC_LENGTH - 1);
      topics[num_topics][MAX_TOPIC_LENGTH - 1] = '\0'; // Ensure null-termination
      num_topics++;
    }
  }

  free(input_copy);
}

static void select_callback(struct MenuLayer *s_menu_layer, MenuIndex *cell_index, 
                            void *callback_context) {
  // Switch to countdown window
  // window_stack_push(s_countdown_window, false);
}

static uint16_t get_sections_count_callback(struct MenuLayer *menulayer, uint16_t section_index, 
                                            void *callback_context) {
  return num_topics;
}

#ifdef PBL_ROUND
static int16_t get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, 
                                        void *callback_context) {
  return 60;
}
#endif

static void draw_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, 
                             void *callback_context) {
  char* name = topics[cell_index->row];

  snprintf(s_topic_text, sizeof(s_topic_text), "%s", name);
  menu_cell_basic_draw(ctx, cell_layer, name, NULL, NULL);
}

static void menu_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_topic_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_topic_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = get_sections_count_callback,
    .get_cell_height = PBL_IF_ROUND_ELSE(get_cell_height_callback, NULL),
    .draw_row = draw_row_handler,
    .select_click = select_callback
  }); 
  menu_layer_set_click_config_onto_window(s_topic_layer,	window);
  layer_add_child(window_layer, menu_layer_get_layer(s_topic_layer));
}

static void menu_window_unload(Window *window) {
  menu_layer_destroy(s_topic_layer);
}

static void inbox_recv_callback(DictionaryIterator *iterator, void *context)
{
  Tuple *count_tuple = dict_find(iterator, MESSAGE_KEY_Count);
  Tuple *topics_tuple = dict_find(iterator, MESSAGE_KEY_Topics);

  if (count_tuple && topics_tuple) {
    int count = count_tuple->value->int32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Count: %d", count);

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Topics: %s", topics_tuple->value->cstring);
    split_string_into_array(topics_tuple->value->cstring);

    // Log the topics
    for (int i = 0; i < num_topics; i++) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Topic %d: %s", i, topics[i]);
    }
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

static void init(void) {
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

  const int inbox_size = 256;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);
}

static void deinit(void) {
  window_destroy(s_topics_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
