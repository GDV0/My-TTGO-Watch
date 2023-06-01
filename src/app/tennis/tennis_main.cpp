#include "config.h"

#include "tennis.h"
#include "tennis_main.h"
#include "tennis_loop.h"

#include "gui/mainbar/app_tile/app_tile.h"
#include "gui/mainbar/main_tile/main_tile.h"
#include "gui/mainbar/mainbar.h"
#include "gui/statusbar.h"
#include "gui/app.h"
#include "gui/widget_factory.h"
#include "gui/widget_styles.h"

#include "hardware/display.h"
#include "hardware/touch.h"

#ifdef NATIVE_64BIT
    #include <string>
    #include "utils/logging.h"
    #include "utils/millis.h"

    using namespace std;
    #define String string
#else
    #include <Arduino.h>
#endif

static volatile bool tennis_app_active = false;

lv_task_t * _tennis_app_task = NULL;

void tennis_activate_cb( void );
void tennis_hibernate_cb( void );
void tennis_time_activate_cb( void );
void tennis_time_hibernate_cb( void );
void tennis_add_widget( void );
void tennis_remove_widget( void );
void tennis_app_task( lv_task_t * task );

/**************************************************************************//**
* Initialisation application tennis tile
******************************************************************************/
void tennis_main_setup( uint32_t tile_num ) 
{
    // Creation de la callback activation de l'application
    mainbar_add_tile_activate_cb( tile_num, tennis_activate_cb );

    // Creation de la callback ahibernation de l'application ()
    mainbar_add_tile_hibernate_cb( tile_num, tennis_hibernate_cb );
}

/**************************************************************************//**
* Initialisation result tennis tile
******************************************************************************/
void tennis_result_setup( uint32_t tile_num ) 
{
    // Creation de la callback activation de l'application
    mainbar_add_tile_activate_cb( tile_num, tennis_time_activate_cb );

    // Creation de la callback ahibernation de l'application ()
    mainbar_add_tile_hibernate_cb( tile_num, tennis_time_hibernate_cb );
}

/**************************************************************************//**
* Callback appelée à l'activation de l'application
******************************************************************************/
void tennis_activate_cb( void ) 
{
    TENNIS_INFO_LOG("enter tennis app");
    /**
     * set tennis app active on enter tile
     */
        tennis_app_active = true;
    /**
     * Ensure status bar
     */
    // Masque la barre d'état haute
    statusbar_hide( true );

    // Creation du widget tennis
    tennis_add_widget();

    // Execute l'initialisation de la boucle principale de l'application tennis
    tennis_loop( tennis_get_app_main_tile_num(), 0 );

    // Creation de la tache cyclique (200ms) d'appel de la boucle de l'application tennis
    if( !_tennis_app_task )
        _tennis_app_task = lv_task_create( tennis_app_task, 200, LV_TASK_PRIO_MID, NULL );
}

/**************************************************************************//**
* Callback appelée à l'hibernation de l'application
******************************************************************************/
void tennis_hibernate_cb( void ) 
{
    TENNIS_INFO_LOG("exit tennis app");
    /**
     * set tennis app inactive on exit tile
     */
    tennis_app_active = false;
    
    // Destruction de la tache cyclique si elle existe
    if( _tennis_app_task )
        lv_task_del( _tennis_app_task );
    _tennis_app_task = NULL;
}

/**************************************************************************//**
* Callback appelée à l'activation de l'ecran temps de match
******************************************************************************/
void tennis_time_activate_cb( void ) 
{
    TENNIS_INFO_LOG("enter tennis time");

    // Execute l'initialisation de la boucle principale de l'application tennis
    tennis_time_display( tennis_get_app_result_tile_num());
}

/**************************************************************************//**
* Callback appelée à l' hibernation de l'écran temps de match
******************************************************************************/
void tennis_time_hibernate_cb( void ) 
{
    TENNIS_INFO_LOG("exit tennis time");
}

/**************************************************************************//**
*  Tache cyclique de l'application
******************************************************************************/
void tennis_app_task( lv_task_t * task ) 
{
    uint8_t appOnGoing = 0;

    // Force l'option "block maintile" à true
    display_set_block_return_maintile( true); 
    
    // Execute une boucle de l'application
    appOnGoing = tennis_loop( tennis_get_app_main_tile_num(), 1 );
    
    // Test si demande =de fermeture de l'application
    if (appOnGoing == 1)
    {
        // Detruit le widget tennis
        tennis_remove_widget();

        // Quitte l'application tennis
        mainbar_jump_back();
    }
}