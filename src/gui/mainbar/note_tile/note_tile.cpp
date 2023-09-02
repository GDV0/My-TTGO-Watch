/****************************************************************************
 *   Tu May 22 21:23:51 2020
 *   Copyright  2020  Dirk Brosswick
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
#include "gui/mainbar/mainbar.h"
#include "gui/widget_styles.h"

#include "gui/mainbar/app_tile/app_tile.h"
#include "gui/mainbar/main_tile/main_tile.h"
#include "gui/mainbar/note_tile/note_tile.h"
#include "gui/mainbar/setup_tile/setup_tile.h"

#ifdef NATIVE_64BIT
    #include "utils/logging.h"
#else
    #include <Arduino.h>
    #ifdef M5PAPER
    #elif defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V2 ) || defined( LILYGO_WATCH_2020_V3 )
    #endif
#endif

static bool notetile_init = false;

static lv_obj_t *note_cont = NULL;
static lv_obj_t *notelabel = NULL;
uint32_t note_tile_num;

static lv_style_t *style;
static lv_style_t notestyle;

LV_FONT_DECLARE(Ubuntu_72px);
LV_FONT_DECLARE(Ubuntu_16px);

static bool note_tile_button_event_cb( EventBits_t event, void *arg );

void note_tile_setup( void ) {

    if ( notetile_init ) {
        log_e("note tile already init");
        return;
    }

    #if defined( M5PAPER )
        note_tile_num = mainbar_add_tile( 0, 3, "note tile", ws_get_mainbar_style() );
        note_cont = mainbar_get_tile_obj( note_tile_num );
    #elif defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V2 ) || defined( LILYGO_WATCH_2020_V3 ) || defined( M5CORE2 )
        note_tile_num = mainbar_add_tile( 0, 1, "note tile", ws_get_mainbar_style() );
        note_cont = mainbar_get_tile_obj( note_tile_num );
    #elif defined( LILYGO_WATCH_2021 )
        note_tile_num = mainbar_add_tile( 0, 1, "note tile", ws_get_mainbar_style() );
        note_cont = mainbar_get_tile_obj( note_tile_num );
    #else
        note_tile_num = mainbar_add_tile( 0, 1, "note tile", ws_get_mainbar_style() );
        note_cont = mainbar_get_tile_obj( note_tile_num );
        #warning "no note tiles setup"  
    #endif 
    style = ws_get_mainbar_style();

    lv_style_init( &notestyle);
    lv_style_set_text_opa( &notestyle, LV_OPA_30);
    lv_style_set_text_font( &notestyle, &Ubuntu_72px);

    notelabel = lv_label_create( note_cont);
    lv_label_set_text( notelabel, "note");
    lv_obj_remove_style_all( notelabel );
    lv_obj_add_style( notelabel, style, LV_PART_MAIN );
    lv_obj_add_style( notelabel, &notestyle, LV_PART_MAIN );
    lv_obj_align_to( notelabel, NULL, LV_ALIGN_CENTER, 0, 0);

    mainbar_add_tile_button_cb( note_tile_num, note_tile_button_event_cb );

    notetile_init = true;
}

static bool note_tile_button_event_cb( EventBits_t event, void *arg ) {
    switch( event ) {
        case BUTTON_LEFT:   mainbar_jump_to_tilenumber( setup_get_tile_num(), LV_ANIM_OFF );
                            mainbar_clear_history();
                            break;
    }
    return( true );
}

uint32_t note_tile_get_tile_num( void ) {
    /*
     * check if maintile alread initialized
     */
    if ( !notetile_init ) {
        log_e("maintile not initialized");
        while( true );
    }

    return( note_tile_num );
}