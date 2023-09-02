/****************************************************************************
 *   Su Jan 17 23:05:51 2021
 *   Copyright  2021  Dirk Brosswick
 *   Email: dirk.brosswick@googlemail.com
 ****************************************************************************/
 
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "quickbar.h"
#include "screenshot.h"
#include "gui/mainbar/mainbar.h"
#include "gui/mainbar/setup_tile/setup_tile.h"
#include "gui/widget_factory.h"

#include "hardware/blectl.h"
#include "hardware/wifictl.h"
#include "hardware/button.h"
#include "hardware/powermgm.h"
#include "hardware/motor.h"
#include "hardware/timesync.h"

#ifdef NATIVE_64BIT
    #include "utils/logging.h"
#else
    #include <Arduino.h>
#endif

static bool quickbar_init = false;
static bool wifistate = false;
static bool bluetoothstate = false;

static lv_obj_t *quickbar = NULL;
static lv_obj_t *quickbar_time_label = NULL;
static lv_obj_t *quickbar_maintile_btn = NULL;
static lv_obj_t *quickbar_setup_btn = NULL;
static lv_obj_t *quickbar_screenshot_btn = NULL;
static lv_obj_t *quickbar_bluetooth_btn = NULL;
static lv_obj_t *quickbar_wifi_btn = NULL;
static lv_style_t quickbarstyle[ QUICKBAR_STYLE_NUM ];

LV_IMG_DECLARE(maintile_32px);
LV_IMG_DECLARE(setup_32px);
LV_IMG_DECLARE(camera_32px);
LV_IMG_DECLARE(bluetooth_64px);
LV_IMG_DECLARE(wifi_64px);
LV_FONT_DECLARE(Ubuntu_48px);

lv_timer_t * quickbar_task;
static uint32_t quickbar_counter = 0;

/**
 * quickbar icon events
 */
static lv_event_cb_t quickbar_maintile_event_cb( lv_obj_t *obj, lv_event_t event );
static lv_event_cb_t quickbar_setup_event_cb( lv_obj_t *obj, lv_event_t event );
static lv_event_cb_t quickbar_screenshot_event_cb( lv_obj_t *obj, lv_event_t event );
static lv_event_cb_t quickbar_wifi_event_cb( lv_obj_t *obj, lv_event_t event );
static lv_event_cb_t quickbar_bluetooth_event_cb( lv_obj_t *obj, lv_event_t event );
/**
 * quickbar external events
 */
static bool quickbar_blectl_event_cb( EventBits_t event, void *arg );
static bool quickbar_wifictl_event_cb( EventBits_t event, void *arg );
static bool quickbar_button_event_cb( EventBits_t event, void *arg );
static bool quickbar_powermgm_event_cb( EventBits_t event, void *arg );
static void quickbar_counter_task( lv_timer_t * task );

void quickbar_setup( void ){
    /*
     * check if quickbar already initialized
     */
    if ( quickbar_init ) {
        log_e("quickbar already initialized");
        return;
    }

    /*Copy a built-in style to initialize the new style*/
    lv_style_init( &quickbarstyle[ QUICKBAR_STYLE_NORMAL ] );
    lv_style_set_text_font( &quickbarstyle[ QUICKBAR_STYLE_NORMAL ], &Ubuntu_48px);
    lv_style_set_radius( &quickbarstyle[ QUICKBAR_STYLE_NORMAL ], 0 );
    lv_style_set_bg_color( &quickbarstyle[ QUICKBAR_STYLE_NORMAL ], LV_COLOR_WHITE );
    lv_style_set_bg_opa( &quickbarstyle[ QUICKBAR_STYLE_NORMAL ], LV_OPA_80 );
    lv_style_set_border_width( &quickbarstyle[ QUICKBAR_STYLE_NORMAL ], 0 );
    lv_style_set_text_color( &quickbarstyle[ QUICKBAR_STYLE_NORMAL ], LV_COLOR_WHITE );
    lv_style_set_radius( &quickbarstyle[ QUICKBAR_STYLE_NORMAL ], 16 );
    lv_obj_set_style_img_recolor( &quickbarstyle[ QUICKBAR_STYLE_NORMAL ], LV_COLOR_WHITE, LV_PART_MAIN );

    lv_style_init( &quickbarstyle[ QUICKBAR_STYLE_LIGHT ] );
    lv_style_copy( &quickbarstyle[ QUICKBAR_STYLE_LIGHT ], &quickbarstyle[ QUICKBAR_STYLE_NORMAL ] );
    lv_style_set_bg_color( &quickbarstyle[ QUICKBAR_STYLE_LIGHT ], LV_COLOR_WHITE );
    lv_style_set_bg_opa( &quickbarstyle[ QUICKBAR_STYLE_LIGHT ], LV_OPA_0 );
    lv_style_set_text_color( &quickbarstyle[ QUICKBAR_STYLE_LIGHT ], LV_COLOR_WHITE );
    lv_obj_set_style_img_recolor( &quickbarstyle[ QUICKBAR_STYLE_LIGHT ], LV_COLOR_WHITE, LV_PART_MAIN );
    lv_obj_set_style_img_recolor_opa( &quickbarstyle[ QUICKBAR_STYLE_DARK ], LV_OPA_COVER, LV_PART_MAIN );

    lv_style_init( &quickbarstyle[ QUICKBAR_STYLE_DARK ] );
    lv_style_copy( &quickbarstyle[ QUICKBAR_STYLE_DARK ], &quickbarstyle[ QUICKBAR_STYLE_LIGHT ] );
    lv_style_set_bg_color( &quickbarstyle[ QUICKBAR_STYLE_DARK ], LV_COLOR_BLACK );
    lv_style_set_text_color( &quickbarstyle[ QUICKBAR_STYLE_DARK ], LV_COLOR_BLACK );
    lv_obj_set_style_img_recolor( &quickbarstyle[ QUICKBAR_STYLE_DARK ], LV_COLOR_BLACK, LV_PART_MAIN );
    lv_obj_set_style_img_recolor_opa( &quickbarstyle[ QUICKBAR_STYLE_DARK ], LV_OPA_COVER, LV_PART_MAIN );

    lv_style_init( &quickbarstyle[ QUICKBAR_STYLE_GREEN ] );
    lv_style_copy( &quickbarstyle[ QUICKBAR_STYLE_GREEN ], &quickbarstyle[ QUICKBAR_STYLE_LIGHT ]  );
    lv_style_set_bg_color( &quickbarstyle[ QUICKBAR_STYLE_GREEN ], LV_COLOR_BLACK );
    lv_style_set_text_color( &quickbarstyle[ QUICKBAR_STYLE_GREEN ], LV_COLOR_BLACK );
    lv_obj_set_style_img_recolor( &quickbarstyle[ QUICKBAR_STYLE_GREEN ], LV_COLOR_GREEN, LV_PART_MAIN );
    lv_obj_set_style_img_recolor_opa( &quickbarstyle[ QUICKBAR_STYLE_GREEN ], LV_PART_MAIN, LV_OPA_COVER );

    lv_style_init( &quickbarstyle[ QUICKBAR_STYLE_RED ] );
    lv_style_copy( &quickbarstyle[ QUICKBAR_STYLE_RED ], &quickbarstyle[ QUICKBAR_STYLE_LIGHT ]  );
    lv_style_set_bg_color( &quickbarstyle[ QUICKBAR_STYLE_RED ], LV_COLOR_BLACK );
    lv_style_set_text_color( &quickbarstyle[ QUICKBAR_STYLE_RED ], LV_COLOR_BLACK );
    lv_obj_set_style_img_recolor( &quickbarstyle[ QUICKBAR_STYLE_RED ], LV_COLOR_RED, LV_PART_MAIN );
    lv_obj_set_style_img_recolor_opa( &quickbarstyle[ QUICKBAR_STYLE_RED ], LV_OPA_COVER, LV_PART_MAIN );

    lv_style_init( &quickbarstyle[ QUICKBAR_STYLE_GRAY ] );
    lv_style_copy( &quickbarstyle[ QUICKBAR_STYLE_GRAY ], &quickbarstyle[ QUICKBAR_STYLE_LIGHT ]  );
    lv_style_set_bg_color( &quickbarstyle[ QUICKBAR_STYLE_GRAY ], LV_COLOR_BLACK );
    lv_style_set_text_color( &quickbarstyle[ QUICKBAR_STYLE_GRAY ], LV_COLOR_BLACK );
    lv_obj_set_style_img_recolor( &quickbarstyle[ QUICKBAR_STYLE_GRAY ], LV_COLOR_GRAY, LV_PART_MAIN );
    lv_obj_set_style_img_recolor_opa( &quickbarstyle[ QUICKBAR_STYLE_GRAY ], LV_OPA_COVER, LV_PART_MAIN );

    lv_style_init( &quickbarstyle[ QUICKBAR_STYLE_TRANS ] );
    lv_style_copy( &quickbarstyle[ QUICKBAR_STYLE_TRANS ], &quickbarstyle[ QUICKBAR_STYLE_LIGHT ]  );
    lv_style_set_bg_color( &quickbarstyle[ QUICKBAR_STYLE_TRANS ], LV_COLOR_BLACK );
    lv_style_set_bg_opa( &quickbarstyle[ QUICKBAR_STYLE_TRANS ], LV_OPA_TRANSP );
    lv_style_set_text_color( &quickbarstyle[ QUICKBAR_STYLE_TRANS ], LV_COLOR_BLACK );
    lv_obj_set_style_img_recolor_opa( &quickbarstyle[ QUICKBAR_STYLE_TRANS ], LV_OPA_COVER, LV_PART_MAIN );

    quickbar = lv_obj_create( lv_scr_act() );
    lv_obj_set_width( quickbar, 48 * 3 );
    lv_obj_remove_style_all( quickbar );
    lv_obj_add_style( quickbar, &quickbarstyle[ QUICKBAR_STYLE_NORMAL ], LV_PART_MAIN );
    
    #if defined( ROUND_DISPLAY )
        lv_obj_set_height( quickbar, 48 * 2 + 64 );
        lv_obj_align_to( quickbar, lv_scr_act(), LV_ALIGN_CENTER, 0, 0 );
    #else
        lv_obj_set_height( quickbar, 48 * 2 );
        lv_obj_align_to( quickbar, lv_scr_act(), LV_ALIGN_BOTTOM_MID, 0, 0 );
    #endif

    quickbar_time_label = lv_label_create( quickbar );
    lv_label_set_text( quickbar_time_label, "00:00");
    lv_obj_remove_style_all( quickbar_time_label );
    lv_obj_add_style( quickbar_time_label, &quickbarstyle[ QUICKBAR_STYLE_DARK ], LV_PART_MAIN  );
    lv_obj_align_to( quickbar_time_label, NULL, LV_ALIGN_TOP_MID, 0, 0);

    quickbar_setup_btn = wf_add_image_button( quickbar, setup_32px, quickbar_setup_event_cb, &quickbarstyle[ QUICKBAR_STYLE_DARK ] );
    lv_obj_set_width( quickbar_setup_btn, 48 );
    lv_obj_set_height( quickbar_setup_btn, 48 );
    lv_obj_align_to( quickbar_setup_btn, quickbar_time_label, LV_ALIGN_OUT_BOTTOM_MID, 0, -8 );

    quickbar_maintile_btn = wf_add_image_button( quickbar, maintile_32px, quickbar_maintile_event_cb, &quickbarstyle[ QUICKBAR_STYLE_DARK ] );
    lv_obj_set_width( quickbar_maintile_btn, 48 );
    lv_obj_set_height( quickbar_maintile_btn, 48 );
    lv_obj_align_to( quickbar_maintile_btn, quickbar_setup_btn, LV_ALIGN_OUT_LEFT_MID, 0, 0 );

    quickbar_screenshot_btn = wf_add_image_button( quickbar, camera_32px, quickbar_screenshot_event_cb, &quickbarstyle[ QUICKBAR_STYLE_DARK ] );
    lv_obj_set_width( quickbar_screenshot_btn, 48 );
    lv_obj_set_height( quickbar_screenshot_btn, 48 );
    lv_obj_align_to( quickbar_screenshot_btn, quickbar_setup_btn, LV_ALIGN_OUT_RIGHT_MID, 0, 0 );
    
    quickbar_wifi_btn = wf_add_image_button( quickbar, wifi_64px, quickbar_wifi_event_cb, &quickbarstyle[ QUICKBAR_STYLE_RED ] );
    lv_obj_set_width( quickbar_wifi_btn, wifi_64px.header.w + THEME_ICON_PADDING);
    lv_obj_set_height( quickbar_wifi_btn, wifi_64px.header.h + THEME_ICON_PADDING );
    lv_obj_align_to( quickbar_wifi_btn, quickbar, LV_ALIGN_BOTTOM_LEFT, 0, 0 );

    quickbar_bluetooth_btn = wf_add_image_button( quickbar, bluetooth_64px, quickbar_bluetooth_event_cb, &quickbarstyle[ QUICKBAR_STYLE_RED ] );
    lv_obj_set_width( quickbar_bluetooth_btn, bluetooth_64px.header.w + THEME_ICON_PADDING );
    lv_obj_set_height( quickbar_bluetooth_btn, bluetooth_64px.header.h + THEME_ICON_PADDING );
    lv_obj_align_to( quickbar_bluetooth_btn, quickbar, LV_ALIGN_BOTTOM_RIGHT, 0, 0 );
    /*
     * quickbar init complete
     */
    quickbar_init = true;
    quickbar_hide( true );
    #ifndef ROUND_DISPLAY
        lv_obj_add_flag( quickbar_wifi_btn, LV_OBJ_FLAG_HIDDEN );
        lv_obj_add_flag( quickbar_bluetooth_btn, LV_OBJ_FLAG_HIDDEN );
    #endif
    /*
     * register pmu callback to detect long press and powermgm callback
     */
    blectl_register_cb( BLECTL_CONNECT | BLECTL_DISCONNECT | BLECTL_ON | BLECTL_OFF, quickbar_blectl_event_cb, "quickbar bluetooth" );
    wifictl_register_cb( WIFICTL_CONNECT | WIFICTL_DISCONNECT | WIFICTL_OFF | WIFICTL_ON, quickbar_wifictl_event_cb, "quickbar wifi" );
    button_register_cb( BUTTON_QUICKBAR, quickbar_button_event_cb, "quickbar pmu event");
    powermgm_register_cb( POWERMGM_SILENCE_WAKEUP | POWERMGM_STANDBY | POWERMGM_WAKEUP, quickbar_powermgm_event_cb, "quickbar powermgm event" );

    return;
}

static bool quickbar_blectl_event_cb( EventBits_t event, void *arg ) {
    switch( event ) {
        case BLECTL_ON:
            bluetoothstate = true;
            wf_image_button_set_style( quickbar_bluetooth_btn, &quickbarstyle[ QUICKBAR_STYLE_GRAY ] );
            break;
        case BLECTL_OFF:
            bluetoothstate = false;
            wf_image_button_set_style( quickbar_bluetooth_btn, &quickbarstyle[ QUICKBAR_STYLE_RED ] );
            break;
        case BLECTL_CONNECT:
            if( bluetoothstate )
                wf_image_button_set_style( quickbar_bluetooth_btn, &quickbarstyle[ QUICKBAR_STYLE_GREEN ] );
            break;
        case BLECTL_DISCONNECT:
            if( bluetoothstate )
                wf_image_button_set_style( quickbar_bluetooth_btn, &quickbarstyle[ QUICKBAR_STYLE_GRAY ] );
            break;
    }
    return( true );
}
static bool quickbar_wifictl_event_cb( EventBits_t event, void *arg ) {
    switch( event ) {
        case WIFICTL_ON:
            wifistate = true;
            wf_image_button_set_style( quickbar_wifi_btn, &quickbarstyle[ QUICKBAR_STYLE_GRAY ] );
            break;
        case WIFICTL_OFF:
            wifistate = false;
            wf_image_button_set_style( quickbar_wifi_btn, &quickbarstyle[ QUICKBAR_STYLE_RED ] );
            break;
        case WIFICTL_CONNECT:
            if( wifistate )
                wf_image_button_set_style( quickbar_wifi_btn, &quickbarstyle[ QUICKBAR_STYLE_GREEN ] );
            break;
        case WIFICTL_DISCONNECT:
            if( wifistate )
                wf_image_button_set_style( quickbar_wifi_btn, &quickbarstyle[ QUICKBAR_STYLE_GRAY ] );
            break;
    }
    return( true );
}

bool quickbar_powermgm_event_cb( EventBits_t event, void *arg ) {
    /*
     * check if quickar already initialized
     */
    if ( !quickbar_init ) {
        log_e("quickbar not initialized");
        return( true );
    }

    bool retval = true;
    
    switch (event) {
        case POWERMGM_WAKEUP:
            quickbar_hide( true );
            break;
        case POWERMGM_STANDBY:
            quickbar_hide( true );
            break;
        case POWERMGM_SILENCE_WAKEUP:
            quickbar_hide( true );
            break;
    }
    return( retval );
}

static bool quickbar_button_event_cb( EventBits_t event, void *arg ) {
    /*
     * check if quickar already initialized
     */
    if ( !quickbar_init ) {
        log_e("quickbar not initialized");
        return( true );
    }
    
    bool retval = true;
    char time[32] = "";
    
    switch ( event ) {
        case BUTTON_QUICKBAR:
            motor_vibe(3);
            lv_disp_trig_activity( NULL );
            if( lv_obj_get_hidden( quickbar ) ) {
                timesync_get_current_timestring( time, sizeof( time ) );
                lv_label_set_text( quickbar_time_label, time );
                lv_obj_align_to( quickbar_time_label, NULL, LV_ALIGN_TOP_MID, 0, 0);
                quickbar_hide( false );
            }
            else {
                quickbar_hide( true );
            }
            break;
    }
    return( retval );
}

void quickbar_hide( bool hide ) {
    /*
     * check if quickar already initialized
     */
    if ( !quickbar_init ) {
        log_e("quickbar not initialized");
        return;
    }

    if ( hide ) {
        lv_obj_clear_flag( quickbar, LV_OBJ_FLAG_HIDDEN );
        lv_obj_invalidate( lv_scr_act() );

        wf_image_button_fade_out( quickbar_maintile_btn, 300, 0 );
        wf_image_button_fade_out( quickbar_setup_btn, 300, 0 );
        wf_image_button_fade_out( quickbar_screenshot_btn, 300, 0 );
        #if defined( ROUND_DISPLAY )
            wf_image_button_fade_out( quickbar_wifi_btn, 300, 0 );
            wf_image_button_fade_out( quickbar_bluetooth_btn, 300, 0 );
        #endif
    }
    else {
        lv_obj_clear_flag( quickbar, LV_OBJ_FLAG_HIDDEN );
        lv_obj_invalidate( lv_scr_act() );

        wf_image_button_fade_in( quickbar_maintile_btn, 300, 0 );
        wf_image_button_fade_in( quickbar_setup_btn, 300, 0 );
        wf_image_button_fade_in( quickbar_screenshot_btn, 300, 0 );
        #if defined( ROUND_DISPLAY )
            wf_image_button_fade_in( quickbar_wifi_btn, 300, 0 );
            wf_image_button_fade_in( quickbar_bluetooth_btn, 300, 0 );
        #endif
    }
}

static lv_event_cb_t quickbar_maintile_event_cb( lv_obj_t *obj, lv_event_t event ) {
    /*
     * check if quickar already initialized
     */
    if ( !quickbar_init ) {
        log_e("quickbar not initialized");
        return;
    }
    
    switch ( event.code ) {
        case ( LV_EVENT_CLICKED ):
            quickbar_hide( true );
            mainbar_jump_to_maintile( LV_ANIM_OFF );
            break;
    }
}

static lv_event_cb_t quickbar_setup_event_cb( lv_obj_t *obj, lv_event_t event ) {
    /*
     * check if quickar already initialized
     */
    if ( !quickbar_init ) {
        log_e("quickbar not initialized");
        return;
    }
    
    switch ( event.code ) {
        case ( LV_EVENT_CLICKED ):
            quickbar_hide( true );
            mainbar_jump_to_tilenumber( setup_get_tile_num(), LV_ANIM_OFF );
            break;
    }
}

static lv_event_cb_t quickbar_screenshot_event_cb( lv_obj_t *obj, lv_event_t event ) {
    /*
     * check if quickar already initialized
     */
    if ( !quickbar_init ) {
        log_e("quickbar not initialized");
        return;
    }
    
    switch ( event.code ) {
        case ( LV_EVENT_CLICKED ):
            quickbar_hide( true );
            lv_disp_trig_activity( NULL );
            lv_task_handler();
            quickbar_counter = 3;
            quickbar_task = lv_timer_create( quickbar_counter_task, 1000, NULL );
            break;
    }
}

static lv_event_cb_t quickbar_wifi_event_cb( lv_obj_t *obj, lv_event_t event ) {
    switch ( event.code ) {
        case ( LV_EVENT_CLICKED ):
            if( wifistate ) {
                wifictl_off();
                lv_obj_remove_style_all( lv_obj_get_child( obj, NULL ) );
                lv_obj_add_style( lv_obj_get_child( obj, NULL ), &quickbarstyle[ QUICKBAR_STYLE_RED ], LV_PART_MAIN );
            }
            else {
                wifictl_on();
                lv_obj_remove_style_all( lv_obj_get_child( obj, NULL ) );
                lv_obj_add_style( lv_obj_get_child( obj, NULL ), &quickbarstyle[ QUICKBAR_STYLE_GRAY ], LV_PART_MAIN );
            }
            break;
    }
}

static lv_event_cb_t quickbar_bluetooth_event_cb( lv_obj_t *obj, lv_event_t event ) {
    switch ( event.code) {
        case ( LV_EVENT_CLICKED ):
            if( bluetoothstate ) {
                blectl_off();
                lv_obj_remove_style_all( lv_obj_get_child( obj, NULL ) );
                lv_obj_add_style( lv_obj_get_child( obj, NULL ), &quickbarstyle[ QUICKBAR_STYLE_RED ], LV_PART_MAIN );
            }
            else {
                blectl_on();
                lv_obj_remove_style_all( lv_obj_get_child( obj, NULL ) );
                lv_obj_add_style( lv_obj_get_child( obj, NULL ), &quickbarstyle[ QUICKBAR_STYLE_GRAY ], LV_PART_MAIN );
            }
            break;
    }
}

static lv_event_cb_t quickbar_counter_task( lv_timer_t * task ) {
    /*
     * check if quickar already initialized
     */
    if ( !quickbar_init ) {
        log_e("quickbar not initialized");
        return;
    }
    
    quickbar_counter--;
    if ( quickbar_counter == 0 ) {
        screenshot_take();
        screenshot_save();
        lv_timer_del( quickbar_task );
    }
}