#include "config.h"

#include "tennis.h"
#include "tennis_main.h"

#include "gui/mainbar/mainbar.h"
#include "gui/statusbar.h"
#include "gui/app.h"
#include "gui/widget.h"

#ifdef NATIVE_64BIT
    #include "utils/logging.h"
    #include "utils/millis.h"
#else
    #include <Arduino.h>
#endif

// app icon
icon_t *tennis = NULL;

// declare your images or fonts you need
LV_IMG_DECLARE(tennis_64px);

uint32_t tennis_main_tile_num;
uint32_t tennis_result_tile_num;

// app and widget icons
icon_t *tennis_app = NULL;
icon_t *tennis_widget = NULL;

// declare callback functions for the app and widget icon to enter the app
static void enter_tennis_event_cb( lv_obj_t * obj, lv_event_t event );

/*
 *
 */
void tennis_setup( void )
{
    #if defined( ONLY_ESSENTIAL )
        return;
    #endif
    // register 2 vertical tiles and get the first tile number and save it for later use
    tennis_main_tile_num = mainbar_add_app_tile( 1, 1, "Tennis" );
    tennis_result_tile_num = mainbar_add_app_tile( 1, 1, "Tennis" );

    // register app icon on the app tile
    // set your own icon and register her callback to activate by an click
    // remember, an app icon must have an size of 64x64 pixel with an alpha channel
    // use https://lvgl.io/tools/imageconverter to convert your images and set "true color with alpha" to get fancy images
    // the resulting c-file can put in /app/examples/images/ and declare it like LV_IMG_DECLARE( your_icon );
    tennis = app_register( "Tennis", &tennis_64px, enter_tennis_event_cb );
    app_set_indicator( tennis, ICON_INDICATOR_OK );

    // init main and setup tile, see tennis_main.cpp
    tennis_main_setup( tennis_main_tile_num );
    tennis_result_setup( tennis_result_tile_num );
}

/*
 *
 */
uint32_t tennis_get_app_main_tile_num( void )
{
    return( tennis_main_tile_num );
}

/*
 *
 */
uint32_t tennis_get_app_result_tile_num( void )
{
    return( tennis_result_tile_num );
}

/*
 *
 */
void tennis_app_hide_app_icon_info( bool show )
{
    if ( !show ) {
        app_set_indicator( tennis_app, ICON_INDICATOR_1 );
    }
    else {
        app_hide_indicator( tennis_app );
    }
}

/*
 *
 */
static void enter_tennis_event_cb( lv_obj_t * obj, lv_event_t event )
{
    switch( event ) {
        case( LV_EVENT_CLICKED ):       statusbar_hide( false );
                                        app_hide_indicator( tennis );
                                        mainbar_jump_to_tilenumber( tennis_main_tile_num, LV_ANIM_OFF, true);
                                        break;
    }    
}

/*
 *
 */
void tennis_add_widget( void ) 
{
    if (tennis_widget == NULL)
        tennis_widget = widget_register( "Tennis", &tennis_64px, enter_tennis_event_cb );
}

/*
 *
 */
void tennis_remove_widget( void ) 
{
    TENNIS_INFO_LOG("remove tennis widget");
    tennis_widget = widget_remove( tennis_widget );
}
