#include <pebble.h>
#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1

static Window *s_main_window;

static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_weather_layer;
//static TextLayer *s_connection_layer; //Old Text Bluetooth layer
static TextLayer *s_battery_layer;

static GFont s_time_font;
static GFont s_main_font;
static GFont s_sub_font;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;
static BitmapLayer *s_bluetooth_layer;
static GBitmap *s_bluetooth_status_bitmap;

//static InverterLayer *s_pulse_layer;
static GBitmap *s_pulse_bitmap;
static BitmapLayer *s_pulse_layer;
static PropertyAnimation*s_pulse_animation;


static void update_time()
{
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    
    //Create a long-lived buffer
    static char buffer[] = "00:00";
    
    if(clock_is_24h_style() == true)
    {
        strftime(buffer, sizeof("00:00"), "%k:%M", tick_time);
    }
    else
    {
        strftime(buffer, sizeof("00:00"), "%l:%M", tick_time);
    }
    text_layer_set_text(s_time_layer, buffer);
    
    static char dateBuffer[] = "JAN.01";
    strftime(dateBuffer, sizeof("JAN.01"), "%a%d", tick_time);
    text_layer_set_text(s_date_layer, dateBuffer);
}

static void update_connection(bool connected)
{
    if(connected == true)
    {
        //layer_set_hidden((Layer *)s_bluetooth_layer, true);
        layer_set_hidden(bitmap_layer_get_layer(s_bluetooth_layer), true);
    }
    else
    {
        //layer_set_hidden((Layer *)s_bluetooth_layer, false);
        layer_set_hidden(bitmap_layer_get_layer(s_bluetooth_layer), false);
    }
}

static void update_battery(BatteryChargeState charge)
{
    static char batbuffer[] = "/100%/"; //Max Characters
    
    if(charge.is_plugged == false)
    {
        snprintf(batbuffer, sizeof("/100%/"), "%d%s", charge.charge_percent, "%");
    }
    else if(charge.is_charging == true)
    {
        snprintf(batbuffer, sizeof("/100%/"), "%s%d%s", "/", charge.charge_percent, "%/");
    }
    else //Done Charging
    {
        snprintf(batbuffer, sizeof("/100%/"), "%s", "FULL!");
    }
    text_layer_set_text(s_battery_layer, batbuffer);
}

//Animation of the pulse
static void anim_stopped_handler(Animation *animation, bool finished, void *data)
{
    property_animation_destroy(s_pulse_animation);
}

static void run_animation()
{
    GRect start_frame = GRect(-20, 60, 10, 100);
    GRect end_frame = GRect(144, 60, 10, 100);
    //Animation
    s_pulse_animation = property_animation_create_layer_frame(bitmap_layer_get_layer(s_pulse_layer), &start_frame, &end_frame);
    animation_set_duration((Animation*)s_pulse_animation, 700);
    animation_set_delay((Animation*)s_pulse_animation, 100);
    animation_set_curve((Animation*)s_pulse_animation, AnimationCurveLinear);
    animation_set_handlers((Animation*)s_pulse_animation, (AnimationHandlers){ .stopped = anim_stopped_handler}, NULL);
    animation_schedule((Animation*)s_pulse_animation);
}



static void main_window_load(Window *window)
{
    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WATCHFACE_BACKGROUND_WHITE);//V2 for lower image
    s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
    bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
    layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
    
    //Pulse Layer
    s_pulse_bitmap = gbitmap_create_blank(GSize(10, 100));
    s_pulse_layer = bitmap_layer_create(GRect(-20, 60, 10, 100));
    bitmap_layer_set_bitmap(s_pulse_layer, s_pulse_bitmap);
    bitmap_layer_set_compositing_mode(s_pulse_layer, GCompOpSet);
    layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_pulse_layer));
    
    s_time_layer = text_layer_create(GRect(0, 65, 140, 50));
    //s_time_layer = text_layer_create(GRect(0, 90, 140, 50));//V2
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorBlack);
    
    s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_WATCH_FONT_36));
    text_layer_set_font(s_time_layer,s_time_font);
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
    
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
    
    s_main_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_WATCH_FONT_26));
    s_date_layer = text_layer_create(GRect(0, 110, 140, 30));
    //s_date_layer = text_layer_create(GRect(0, 135, 140, 30));//V2
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_text_color(s_date_layer, GColorBlack);
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
    text_layer_set_font(s_date_layer, s_main_font);

    
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
    
    s_weather_layer = text_layer_create(GRect(0, 12, 144, 30));
    text_layer_set_background_color(s_weather_layer, GColorClear);
    text_layer_set_text_color(s_weather_layer, GColorBlack);
    text_layer_set_text_alignment(s_weather_layer, GTextAlignmentRight);
    text_layer_set_text(s_weather_layer, "...");
    
    text_layer_set_font(s_weather_layer, s_main_font);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
    
    //s_connection_layer = text_layer_create(GRect(0,0,72,25));
    //text_layer_set_background_color(s_connection_layer, GColorClear);
    //text_layer_set_text_color(s_connection_layer, GColorBlack);
    //text_layer_set_text_alignment(s_connection_layer, GTextAlignmentLeft);
    
    s_bluetooth_status_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_OFF_WHITE);
    s_bluetooth_layer = bitmap_layer_create(GRect(0, 0, 32, 32));
    bitmap_layer_set_bitmap(s_bluetooth_layer, s_bluetooth_status_bitmap);
    layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bluetooth_layer));
    
    s_battery_layer = text_layer_create(GRect(72,0,72,20));
    text_layer_set_background_color(s_battery_layer, GColorClear);
    text_layer_set_text_color(s_battery_layer, GColorBlack);
    text_layer_set_text_alignment(s_battery_layer, GTextAlignmentRight);
    
    s_sub_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_WATCH_FONT_16));
    //text_layer_set_font(s_connection_layer, s_sub_font);
    text_layer_set_font(s_battery_layer, s_sub_font);
    //layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_connection_layer));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_battery_layer));
    
    update_time();
    update_connection(bluetooth_connection_service_peek());
    update_battery(battery_state_service_peek());
}

static void main_window_unload(Window *window)
{
    tick_timer_service_unsubscribe();
    bluetooth_connection_service_unsubscribe();
    battery_state_service_unsubscribe();
    
    gbitmap_destroy(s_background_bitmap);
    bitmap_layer_destroy(s_background_layer);
    
    fonts_unload_custom_font(s_time_font);
    fonts_unload_custom_font(s_main_font);
    fonts_unload_custom_font(s_sub_font);
    
    text_layer_destroy(s_time_layer);
    gbitmap_destroy(s_pulse_bitmap);
    bitmap_layer_destroy(s_pulse_layer);
    text_layer_destroy(s_date_layer);
    text_layer_destroy(s_weather_layer);
    //text_layer_destroy(s_connection_layer);
    gbitmap_destroy(s_bluetooth_status_bitmap);
    bitmap_layer_destroy(s_bluetooth_layer);
    
    text_layer_destroy(s_battery_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
    update_time();
    
    if(tick_time->tm_min % 30 == 0)
    {
        DictionaryIterator *iter;
        app_message_outbox_begin(&iter);
        
        dict_write_uint8(iter, 0, 0);
        
        app_message_outbox_send();
    }
    
    //if(tick_time->tm_sec == 0)
    run_animation();
}

static void bluetooth_handler(bool connected)
{
    update_connection(connected);
}

static void battery_handler(BatteryChargeState charge)
{
    update_battery(charge);
}

static void inbox_received_callback(DictionaryIterator *iterator, void * context)
{
    static char temperature_buffer[8];
    //static char conditions_buffer[32];
    static char weather_layer_buffer[32];
    
    Tuple *t = dict_read_first(iterator);
    
    while(t != NULL)
    {
        switch(t->key)
        {
            case KEY_TEMPERATURE:
            snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)t->value->int32);
            break;
            //case KEY_CONDITIONS:
            //snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
            //break;
            default:
            APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
            break;
        }
        
        t = dict_read_next(iterator);
    }
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s", temperature_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
}
static void inbox_dropped_callback(AppMessageResult reason, void * context)
{
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void * context)
{
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}
static void outbox_sent_callback(DictionaryIterator *iterator, void * context)
{
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send success!");
}

static void init()
{
    s_main_window = window_create();
    
    window_set_window_handlers(s_main_window, (WindowHandlers) {
            .load = main_window_load,
            .unload = main_window_unload
        });
    
    // Show the Window on the watch, with animated=true
    window_stack_push(s_main_window, true);
    
    //tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    bluetooth_connection_service_subscribe(&bluetooth_handler);
    battery_state_service_subscribe(&battery_handler);
    
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
    
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit()
{
    animation_unschedule_all();
    window_destroy(s_main_window);
}

int main(void)
{
    init();
    app_event_loop();
    deinit();
}








