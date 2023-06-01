#include "config.h"

#include "tennis.h"
#include "tennis_loop.h"

#include "gui/mainbar/app_tile/app_tile.h"
#include "gui/mainbar/main_tile/main_tile.h"
#include "gui/mainbar/mainbar.h"
#include "gui/mainbar/setup_tile/watchface/watchface_tile.h"
#include "gui/statusbar.h"
#include "gui/app.h"
#include "gui/widget_factory.h"
#include "gui/widget_styles.h"

#include "hardware/display.h"
#include "hardware/pmu.h"
#include "hardware/wifictl.h"
#include "hardware/blectl.h"

#include "lvgl.h"

//#define TIMERBATT

void ringMenu( uint32_t tile_num, int itemMax, char** itemName, char** itemHelp, int state );
void initScore( void );
void copieScore( void );
void gestionJeu( int G, int P );
void gestionJeuAdd( int G, int P, int max, int diff );
void gestionJeuNoAdd( int G, int P, int max, int diff) ;
void gestionSuperTBreak( int G, int P, int max, int diff) ;
void gestionJeuTMC( int G, int P, int max, int diff );
void gestionSet( int G, int P, int max, int diff );
void gestionSetTMC( int G, int P, int max, int diff );
void gestionMatch( void );
lv_obj_t* displayScore( uint32_t tile_num, uint8_t state );
lv_obj_t* displayScoreFinal ( uint32_t tile_num );

static void enter_tennis_time_event_cb( lv_obj_t * obj, lv_event_t event );
static void exit_tennis_time_event_cb( lv_obj_t * obj, lv_event_t event );

static bool blockMaintile;
static bool blockWatchface;
static uint8_t scrTimeout;

const int maxFormat = 9;                                                                                                                                                                                                                                             // number of match formats
char* FormatMenu[maxFormat] = { "1", "2", "3", "4", "5", "6", "7", "8", "Q" };                                                                                                                                                                                       // Tennis format names
char* FormatHelp[maxFormat] = { "Format  1\n\n3 sets 6 jeux\navec JD 6-6\n", 
                                "Format  2\n\n2 sets 6 jeux\navec JD 6-6\nSet 3: SJD 10pts", 
                                "Format  3\n\n2 sets 4 jeux NoAdd\navec JD 4-4\nSet 3: SJD 10pts", 
                                "Format  4\n\n2 sets 6 jeux NoAdd\navec JD 6-6\nSet 3: SJD 10pts", 
                                "Format  5\n\n2 sets 3 jeux NoAdd\navec JD 2-2\nSet 3: SJD 10pts", 
                                "Format  6\n\n2 sets 4 jeux NoAdd\navec JD 3-3\nSet 3: SJD 10pts",  
                                "Format  7\n\n2 sets 5 jeux NoAdd\navec JD 4-4\nSet 3: SJD 10pts", 
                                "Format  8\n\n3 sets Super JD\n", 
                                "Quitte" };  // Tennis format help

const int maxService = 3;                                             // Number of item in Service menu
char* ServiceMenu[maxService] = { "S", "R", "Q" };                    // Service item names
char* ServiceHelp[maxService] = { "Serveur", "Receveur", "Quitte" };  // Service item help texts

uint16_t x, y;
uint16_t indexScore, currentScore;
char* ScoreJeu[51] = { " O", "15", "3O", "4O", "Ad", "-", " ", " ", " ", " ", " O", " 1", " 2", " 3", " 4", " 5", " 6", " 7", " 8", " 9", " 1O", " 11", " 12", " 13", " 14", " 15", " 16", " 17", " 18", " 19", " 2O", " 21", " 22", " 23", " 24", " 25", " 26", " 27", " 28", " 29", " 3O", " 31", " 32", " 33", " 34", " 35", " 36", " 37", " 38", " 39", " 4O" };
char* ScoreSet[8] = { "O", "1", "2", "3", "4", "5", "6", "7" };

int maxSets = 3;
int maxJeu, diffJeu;
bool IServe, memIServe;
bool exitScore = false;
int matchFormat = 0;
int matchSetType[3];
bool changement = false;
int appStep = 0;
bool menuSelect = false;
uint8_t appExit = 0;
uint8_t gestDir = -1;
int backlightTime;

static lv_style_t cont_style;
static lv_style_t help_st;
static lv_obj_t *view;
static lv_obj_t *help_lab;
static lv_obj_t *item1_lab;
static lv_obj_t *item2_lab;
static lv_obj_t *item3_lab;
static lv_obj_t *item4_lab;
static lv_obj_t *item5_lab;

static lv_style_t Batt_style;
static lv_obj_t *Batt1;
static lv_style_t line_style;
static lv_obj_t *line1;
static lv_obj_t *imgBall;
static lv_obj_t *imgChg;
static lv_style_t Jeu_style;
static lv_style_t NoAdd_style;
static lv_obj_t *Score1;
static lv_obj_t *Score2;
static lv_style_t set_style;
static lv_obj_t *Set1;
static lv_obj_t *Set2;

LV_FONT_DECLARE(liquidCrystal_nor_24);
LV_FONT_DECLARE(liquidCrystal_nor_32);
LV_FONT_DECLARE(liquidCrystal_nor_64);
LV_FONT_DECLARE(TwentySeven_80);
LV_IMG_DECLARE(ball_25);
LV_IMG_DECLARE(redball_25);
LV_IMG_DECLARE(fleche1_40);

// Structure historique match
typedef struct score_t {
  int format;
  bool server;
  bool tiebreak;
  bool noAdd;
  bool chgtCote;
  int set;
  uint32_t time;
  int sets[2][3];
  int tbScore[2][3];
  int gainSets[2];
  int jeu[2];
  int score[2];
} SCORESTRUCT;

SCORESTRUCT Result[300];

int menuItem = 0;

/**************************************************************************//**
* Callback gesture
******************************************************************************/
void scr_event_cb( lv_obj_t * obj, lv_event_t e )
{
  TENNIS_INFO_LOG("Scr event = %d", e);
  changement = false;
  static bool gestAccept = true;

  if ( e == LV_EVENT_CLICKED  || e == LV_EVENT_SHORT_CLICKED )
  {
    backlightTime = 25;
    gestAccept = false;
  }

  if ( e == LV_EVENT_RELEASED ) 
  {
    TTGOClass *ttgo = TTGOClass::getWatch();

    if (gestAccept && ttgo->bl->isOn())
    {
      lv_gesture_dir_t dir = lv_indev_get_gesture_dir( lv_indev_get_act() );
      TENNIS_INFO_LOG("Gesture = %d", dir);
      backlightTime = 25;
      gestDir = -1;
      changement = true;
      gestDir = dir;
    }
    else
      gestAccept = true;
  }
}

/**************************************************************************//**
* Boucle principale de l'application tennis
******************************************************************************/
uint8_t tennis_loop( uint32_t tile_num, uint8_t init  )
{
  if (init == 0)
  {
    menuItem = 0;
    appExit = 0;
    menuSelect = false;
//    wifictl_off();
#ifndef NO_BLUETOOTH    
//    blectl_off();
#endif
  } 
//TENNIS_INFO_LOG("appStep = %d ", appStep);
  
  // Gestion du timer de sauvegarde batterie
  #ifdef TIMERBATT
  TTGOClass *ttgo = TTGOClass::getWatch();
  // Test si temps écoulé
  if (backlightTime > 0)
  {
    // Affichage: décrémentation du timer, backlight on et augmentation fréquence CPU 
    backlightTime --;
    ttgo->displayWakeup();
    ttgo->bl->on();
    setCpuFrequencyMhz(240);
  }
  else
  {
    // temps écoulé: backlight off et reduction fréquence CPU
    ttgo->bl->off();
    ttgo->displaySleep();
    setCpuFrequencyMhz(80);
  }
  #endif //TIMERBATT

//TENNIS_INFO_LOG("Freq: %d, cons.: %f mA Heap size: %d", getCpuFrequencyMhz(),  ttgo->power->getVbusCurrent(), ESP.getFreeHeap());

  switch (appStep)
  {
    case 0: // Affichage choix format de match
            
            TENNIS_INFO_LOG("Step 0: Start AppTennis, choix du format de match");
            // 5 secondes pour une action avant extinction écran 
            backlightTime = 25;
            
            // Affichage menu
            ringMenu(tile_num, maxFormat, FormatMenu, FormatHelp, 0 );

            changement = true;
            appStep = 1;

    case 1: // Attente validation choix format ou quitte
            // 5 secondes pour une action avant extinction écran
            backlightTime = 25;
            
            // Gestion menu
            ringMenu(tile_num, maxFormat, FormatMenu, FormatHelp, 1 );
                          
            if (menuSelect)
            {
            TENNIS_INFO_LOG("Format MenuItem = %d ", menuItem);

              if (menuItem == maxFormat - 1)
              {
                appStep = 0;
                appExit = 1;
              }
              else
              {
                matchFormat = menuItem + 1;
                appStep = 2;
                TENNIS_INFO_LOG("Match format = %d", matchFormat);
              }
              menuItem = 0;
              menuSelect = false;
            }
            break;
    case 2: // Affichage choix serveur

            lv_obj_clean( view );

            // Affichage menu
            ringMenu(tile_num, maxService, ServiceMenu, ServiceHelp, 0 );

            changement = true;
            appStep = 3;

    case 3: // Attente validation choix serveur
            // Gestion menu
            ringMenu(tile_num, maxService, ServiceMenu, ServiceHelp, 1 );

            if (menuSelect)
            {
              TENNIS_INFO_LOG("Service MenuItem = %d ", menuItem);
              if (menuItem == maxService - 1)
              {
                appStep = 0;
                appExit = 1;
              }
              else
              {
                IServe = (menuItem == 0);
                TENNIS_INFO_LOG("Service  = %d ", IServe);
                appStep = 4;
              }
              menuItem = 0;
              menuSelect = false;
            }
            break;
    case 4: // Initialisation écran match
            // Memorisation état block maintile et force le blocage pour l'application
            blockMaintile = display_get_block_return_maintile();
            display_set_block_return_maintile(true);

            // Mémorisation état watchface initial  et force lre non affichage de watchface sur wakeup
            blockWatchface = watchface_get_enable_tile_after_wakeup();
            watchface_enable_tile_after_wakeup( false );

            // Memorisation valeur timeout écran
            scrTimeout = display_get_timeout();
#ifdef TIMERBATT
            display_set_timeout (300);
#else
            display_set_timeout (3);
#endif            
            // Initialisation et affichage ecran match
            gestionMatch();
            displayScore( tile_num, 0);
            appStep = 5;
            break;
    case 5: // Gestion score pendant le match
        if (!exitScore)
        {
          switch (gestDir)
          {
            case LV_GESTURE_DIR_TOP:
                    // Point gagné par le score haut
                    currentScore ++;
                    indexScore = currentScore;
                    copieScore();
                    gestionJeu(0, 1);

                    break;
            case LV_GESTURE_DIR_BOTTOM:
                    // Point gagné par le score bas
                    currentScore ++;
                    indexScore = currentScore;
                    copieScore();
                    gestionJeu(1, 0);
                    break;
            case LV_GESTURE_DIR_LEFT:
                    // Retour sur le score précédent
                    if (currentScore > 0) currentScore --;
                    break;
            case LV_GESTURE_DIR_RIGHT:
                    // Avance sur le score suivant s'il existe
                    if (currentScore < indexScore) currentScore ++;
                    break;
          }
          gestDir = -1;
          if (!exitScore)
            displayScore( tile_num, 1 );              
        }
        else
        {
          appStep = 6;
          exitScore = false;
        }
        break;
    case 6: // Affichage score final
        appExit = false;
        #ifdef TIMERBATT
        ttgo->bl->on();
        #endif
        displayScoreFinal(tile_num); 
        appStep = 7;
        break;
    case 7: // Gestion sortie de l'ecran score final
        static uint8_t Quit = 0;
        switch (gestDir)
        {
          case LV_GESTURE_DIR_TOP:
          case LV_GESTURE_DIR_RIGHT:
                  Quit = 0;
                  break;
          case LV_GESTURE_DIR_BOTTOM:
                  Quit += 1;
                  break;
          case LV_GESTURE_DIR_LEFT:
                  if (Quit == 1)
                  {
                    // Restaure état initial de l'option "block maintile"
                    display_set_block_return_maintile(blockMaintile);

                    // Restaure état initial de l'option "watchface"
                    watchface_enable_tile_after_wakeup( blockWatchface );

                    // Restaure valeur du timeout ecran
                    display_set_timeout (scrTimeout);

                    appExit = true;
                    appStep = 0;
                  }
                  else
                  {
                    Quit = 0;
                  }
                  break;
        }
        gestDir = -1;
        break;
  }  
  return appExit;
}

/**************************************************************************//**
* Affichage menu
******************************************************************************/
void ringMenu( uint32_t tile_num, int itemMax, char** itemName, char** itemHelp, int state )
{
  if (state == 0)
  {
    if ( view != NULL )
      lv_obj_clean( view );
    else
    {
      lv_style_init(&cont_style);
      lv_style_set_radius(&cont_style, LV_OBJ_PART_MAIN, 0);
      lv_style_set_bg_color(&cont_style, LV_OBJ_PART_MAIN, LV_COLOR_BLACK);
      lv_style_set_bg_opa(&cont_style, LV_OBJ_PART_MAIN, LV_OPA_COVER);
      lv_style_set_border_width(&cont_style, LV_OBJ_PART_MAIN, 0);

      view = mainbar_get_tile_obj( tile_num );
      lv_obj_add_style(view, LV_OBJ_PART_MAIN, &cont_style);
      lv_obj_set_event_cb( view, scr_event_cb);
    }  

    static lv_style_t bubble1;
    lv_style_init(&bubble1);
    lv_style_set_text_color(&bubble1, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
    lv_style_set_text_font(&bubble1, LV_STATE_DEFAULT, &lv_font_montserrat_32);
  
    static lv_style_t bubble2;
    lv_style_init(&bubble2);
    lv_style_set_text_color(&bubble2, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_text_font(&bubble2, LV_STATE_DEFAULT, &lv_font_montserrat_22);

    static lv_style_t bubble3;
    lv_style_init(&bubble3);
    lv_style_set_text_color(&bubble3, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_text_font(&bubble3, LV_STATE_DEFAULT, &lv_font_montserrat_14);

    item1_lab = lv_label_create(view, NULL);
    lv_obj_set_size(item1_lab, 15, 15);
    lv_obj_add_style(item1_lab, LV_OBJ_PART_MAIN, &bubble3);
    lv_obj_align(item1_lab, NULL, LV_ALIGN_CENTER, -86, -50);       

    item2_lab = lv_label_create(view, NULL);
    lv_obj_set_size(item2_lab, 20, 20);
    lv_obj_add_style(item2_lab, LV_OBJ_PART_MAIN, &bubble2);
    lv_obj_align(item2_lab, NULL, LV_ALIGN_CENTER, -50, -86);       
  
    item3_lab = lv_label_create(view, NULL);
    lv_obj_set_size(item3_lab, 30, 30);
    lv_obj_align(item3_lab, NULL, LV_ALIGN_CENTER, 0, -100);       
    lv_obj_add_style(item3_lab, LV_OBJ_PART_MAIN, &bubble1);
  
    item4_lab = lv_label_create(view, NULL);
    lv_obj_set_size(item4_lab, 20, 20);
    lv_obj_align(item4_lab, NULL, LV_ALIGN_CENTER, 50, -86);       
    lv_obj_add_style(item4_lab, LV_OBJ_PART_MAIN, &bubble2);

    item5_lab = lv_label_create(view, NULL);
    lv_obj_set_size(item5_lab, 15, 15);
    lv_obj_align(item5_lab, NULL, LV_ALIGN_CENTER, 86, -50);       
    lv_obj_add_style(item5_lab, LV_OBJ_PART_MAIN, &bubble3);

    lv_style_init(&help_st);
    lv_style_set_bg_color(&help_st, LV_LABEL_PART_MAIN, LV_COLOR_RED);
    lv_style_set_text_color(&help_st, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_text_font(&help_st, LV_STATE_DEFAULT, &lv_font_montserrat_22);
    
    help_lab = lv_label_create( view, NULL );
    lv_obj_set_width( help_lab, 240 );
    lv_obj_add_style( help_lab, LV_OBJ_PART_MAIN, &help_st);
    lv_obj_align( help_lab, view, LV_ALIGN_CENTER, 0, 30 );
    lv_obj_set_auto_realign( help_lab, true );
    lv_label_set_align( help_lab, LV_LABEL_ALIGN_CENTER);      

    changement = true;
  }
  else
  {
    if (changement)
    {
      changement = false;
      switch (gestDir)
      {
        case LV_GESTURE_DIR_TOP:
        case LV_GESTURE_DIR_BOTTOM:
                // Validation de l'item menu
                menuSelect = true;
                break;
        case LV_GESTURE_DIR_LEFT:
                // Pointe sur l'tem menu suivant
                menuItem ++;
                if (menuItem > itemMax-1) menuItem = 0;
                break;
        case LV_GESTURE_DIR_RIGHT:
                // Pointe sur l'item menu précédent
                menuItem --;
                if (menuItem < 0) menuItem = itemMax-1;
                break;
      }
      gestDir = -1;

      TENNIS_INFO_LOG("menu item = %d", menuItem);
      if (menuItem <= itemMax-1)
      {
        lv_label_set_text_fmt(item1_lab, "%s", itemName[(itemMax + menuItem - 2) % itemMax]);
        lv_label_set_text_fmt(item2_lab, "%s",itemName[(itemMax + menuItem - 1) % itemMax]);
        lv_label_set_text_fmt(item3_lab, "%s", itemName[menuItem]);
        lv_label_set_text_fmt(item4_lab, "%s", itemName[(itemMax + menuItem + 1) % itemMax]);
        lv_label_set_text_fmt(item5_lab, "%s", itemName[(itemMax + menuItem + 2)  % itemMax]);
        lv_label_set_text_fmt(help_lab, "%s", itemHelp[menuItem]);        
      }
    }
  }
}

/**************************************************************************//**
* Initialisation de l'historique du match
******************************************************************************/
void initScore( void )
{
  indexScore = 0;
  currentScore = 0;
  Result[currentScore].format = matchFormat;
  Result[currentScore].server = IServe;
  Result[currentScore].tiebreak = false;
  Result[currentScore].noAdd = false;
  Result[currentScore].chgtCote = false;
  Result[currentScore].set = 0;
  Result[currentScore].time = millis();

  for (int i = 0; i < maxSets; i++)
  {
    Result[currentScore].sets[0][i] = 0;
    Result[currentScore].sets[1][i] = 0;
    Result[currentScore].tbScore[0][i] = 0;
    Result[currentScore].tbScore[1][i] = 0;
  }

  Result[currentScore].gainSets[0] = 0;
  Result[currentScore].gainSets[1] = 0;
  Result[currentScore].jeu[0] = 0;
  Result[currentScore].jeu[1] = 0;
  Result[currentScore].score[0] = 0;
  Result[currentScore].score[1] = 0;  
}

/**************************************************************************//**
* Préparation item suivant de l'historique
******************************************************************************/
void copieScore( void )
{
  if (currentScore > 0)
  {
    Result[currentScore].format = Result[currentScore - 1].format;
    Result[currentScore].server = Result[currentScore - 1].server;
    Result[currentScore].tiebreak = Result[currentScore - 1].tiebreak;
    Result[currentScore].noAdd = Result[currentScore - 1].noAdd;
    Result[currentScore].chgtCote = Result[currentScore - 1].chgtCote;
    Result[currentScore].set = Result[currentScore - 1].set;
    Result[currentScore].time = millis();
 
    for (int i = 0; i < maxSets; i++)
    {
      Result[currentScore].sets[0][i] = Result[currentScore - 1].sets[0][i];
      Result[currentScore].sets[1][i] = Result[currentScore - 1].sets[1][i];
      Result[currentScore].tbScore[0][i] = Result[currentScore - 1].tbScore[0][i];
      Result[currentScore].tbScore[1][i] = Result[currentScore - 1].tbScore[1][i];
    }

    Result[currentScore].gainSets[0] =   Result[currentScore - 1].gainSets[0];
    Result[currentScore].gainSets[1] =   Result[currentScore - 1].gainSets[1];
    Result[currentScore].jeu[0] = Result[currentScore - 1].jeu[0];
    Result[currentScore].jeu[1] = Result[currentScore - 1].jeu[1];
    Result[currentScore].score[0] = Result[currentScore - 1].score[0];
    Result[currentScore].score[1] = Result[currentScore - 1].score[1];
  }
}

/**************************************************************************//**
* Gestion d'un jeu
******************************************************************************/
void gestionJeu( int G, int P )
{

TENNIS_INFO_LOG("match set = %d ", matchSetType[Result[currentScore].set]);
  switch (matchSetType[Result[currentScore].set])
  {
    case 0:
        Serial.print("Set ");
        Serial.print(matchSetType[Result[currentScore].set]);
        Serial.print(" - SetType ");
        Serial.println(matchSetType[Result[currentScore].set]);
        exitScore = true;
        break;
    case 1:
        maxJeu = 6;
        diffJeu = 2;        
        gestionJeuAdd (G, P, maxJeu, diffJeu);
        break;
    case 2:
        maxJeu = 6;
        diffJeu = 2;        
        gestionJeuNoAdd (G, P, maxJeu, diffJeu);
        break;
    case 3:
        gestionSuperTBreak (G, P, maxJeu, diffJeu);
        break;
    case 4:
        maxJeu = 4;
        diffJeu = 2;        
        gestionJeuNoAdd (G, P, maxJeu, diffJeu);
        break;
    case 5:
        maxJeu = 3;
        diffJeu = 2;        
        gestionJeuTMC (G, P, maxJeu, diffJeu);
        break;
    case 6:
        maxJeu = 4;
        diffJeu = 2;        
        gestionJeuTMC (G, P, maxJeu, diffJeu);
        break;
    case 7:
        maxJeu = 5;
        diffJeu = 2;        
        gestionJeuTMC (G, P, maxJeu, diffJeu);
        break;
  }
}

/**************************************************************************//**
* Gestion d'un jeu sans point décisif
******************************************************************************/
void gestionJeuAdd( int G, int P, int max, int diff )
{
  Result[currentScore].score[G] += 1;
  Result[currentScore].chgtCote = false;
  if (!Result[currentScore].tiebreak)
  {
    // Gestion du jeu normal
    // Gain du jeu apres avantage service
    if (Result[currentScore].score[G] == 5)
    {
      Result[currentScore].server = !Result[currentScore].server;
      gestionSet(G, P, max, diff);      
    }    
    // Retour à égalité apres avantage retour
    if (Result[currentScore].score[G] == 6)
    {
      Result[currentScore].score[G] = Result[currentScore].score[P] = 3;     
    }   
    // Gain du jeu
    if  ((Result[currentScore].score[G] == 4) && (Result[currentScore].score[P] < 3))
    {
      Result[currentScore].server = !Result[currentScore].server;
      gestionSet(G, P, max, diff);      
    }
    // Gestion de l'avantage
    if ((Result[currentScore].score[G] == 4) && (Result[currentScore].score[P] == 3))
    {
      Result[currentScore].score[P] = 5; 
    }
  }
  else
  { 
    //Gestion du tiebreak
    // Changement de terrain tous les 6 points
    if (((Result[currentScore].score[G] + Result[currentScore].score[P] - 20) % 6) == 0)
    {
      Result[currentScore].chgtCote = true;
    }
    // Changement de service pour somme de points impair
    if ((Result[currentScore].score[G] + Result[currentScore].score[P])%2 == 1)
      Result[currentScore].server = !Result[currentScore].server;
    
    // Fin du tiebreak si score gagnant >= 7 points avec 2 points d'écart 
    Result[currentScore].tbScore[G][Result[currentScore].set] = Result[currentScore].score[G];
    Result[currentScore].tbScore[P][Result[currentScore].set] = Result[currentScore].score[P];
    if ((Result[currentScore].score[G] >= 17) && ((Result[currentScore].score[G] - Result[currentScore].score[P]) >= 2))
    {
      gestionSet(G, P, max, diff);
    }   
  } 
}

/**************************************************************************//**
* Gestion d'un jeu avec point décisif
******************************************************************************/
void gestionJeuNoAdd( int G, int P, int max, int diff )
{
  Result[currentScore].score[G] += 1;
  Result[currentScore].chgtCote = false;
  if (!Result[currentScore].tiebreak)
  {
    if ((Result[currentScore].score[G] == 3) && (Result[currentScore].score[P] == 3))
    {
      Result[currentScore].noAdd = true;
    }

    // Gain du jeu avec 2 points d'ecart ou avec un point en No Add
    if ((Result[currentScore].score[G] == 4) && (Result[currentScore].score[P] <= 3))
    {
      Result[currentScore].server = !Result[currentScore].server;
      Result[currentScore].noAdd = false;
      gestionSet(G, P, max, diff);      
    }
  }
  else 
  { // Tiebreak
    // Changement de terrain tous les 6 points
    if (((Result[currentScore].score[G] + Result[currentScore].score[P] - 20) % 6) == 0)
    {
      Result[currentScore].chgtCote = true;
    }
    // Changement de service pour somme de points impair
    if ((Result[currentScore].score[G] + Result[currentScore].score[P])%2 == 1)
      Result[currentScore].server = !Result[currentScore].server;

    Result[currentScore].tbScore[G][Result[currentScore].set] = Result[currentScore].score[G];
    Result[currentScore].tbScore[P][Result[currentScore].set] = Result[currentScore].score[P];
    if ((Result[currentScore].score[G] >= 17) && ((Result[currentScore].score[G] - Result[currentScore].score[P])>=2))
    {
      gestionSet(G, P, max, diff);
    }   
  }
}

/**************************************************************************//**
* Gestion d'un super tiebreak en 10 points
******************************************************************************/
void gestionSuperTBreak( int G, int P, int max, int diff )
{
  if (Result[currentScore].score[G] == 0) Result[currentScore].score[G] = 10;
  if (Result[currentScore].score[P] == 0) Result[currentScore].score[P] = 10;
  
  Result[currentScore].score[G] += 1;  
  Result[currentScore].chgtCote = false;
  Result[currentScore].tbScore[G][Result[currentScore].set] = Result[currentScore].score[G];
  Result[currentScore].tbScore[P][Result[currentScore].set] = Result[currentScore].score[P];
  
  // Changement de service pour somme de points impair
  if ((Result[currentScore].score[G] + Result[currentScore].score[P])%2 == 1)
  {
    Result[currentScore].server = !Result[currentScore].server;
  }
  // Changement de terrain tous les 6 points
  if (((Result[currentScore].score[G] + Result[currentScore].score[P] - 20) % 6) == 0) 
  {
      Result[currentScore].chgtCote = true;
  }
  
  if ((Result[currentScore].score[G] >= 20) && ((Result[currentScore].score[G] - Result[currentScore].score[P]) >= 2))
  {
    Result[currentScore].score[G] = Result[currentScore].score[P] = 0;
    Result[currentScore].sets[G][Result[currentScore].set] += 1;
    Result[currentScore].set +=1;
    Result[currentScore].gainSets[G] +=1;
    if (Result[currentScore].gainSets[G] > maxSets/2) 
    {
      exitScore = true;
      matchSetType[Result[currentScore].set] = 0;
      Result[currentScore].set -= 1;
    }
  }   
}

/**************************************************************************//**
* Gestion d'un jeu type TMC
******************************************************************************/
void gestionJeuTMC( int G, int P, int max, int diff )
{
  Result[currentScore].score[G] += 1;
  Result[currentScore].chgtCote = false;
  if (!Result[currentScore].tiebreak)
  {
    if ((Result[currentScore].score[G] == 3) && (Result[currentScore].score[P] == 3))
    {
      Result[currentScore].noAdd = true;
    }

    if ((Result[currentScore].score[G] == 4) && (Result[currentScore].score[P] <= 3))
    {
      Result[currentScore].server = !Result[currentScore].server;
      Result[currentScore].noAdd = false;
      gestionSetTMC(G, P, max, diff);      
    }
  }
  else
  {
    if (((Result[currentScore].score[G] + Result[currentScore].score[P] - 20) % 6) == 0)
      Result[currentScore].chgtCote = true;

    // Changement de service pour somme de points impair
    if ((Result[currentScore].score[G] + Result[currentScore].score[P])%2 == 1)
      Result[currentScore].server = !Result[currentScore].server;

    Result[currentScore].tbScore[G][Result[currentScore].set] = Result[currentScore].score[G];
    Result[currentScore].tbScore[P][Result[currentScore].set] = Result[currentScore].score[P];
    if ((Result[currentScore].score[G] >= 17) && ((Result[currentScore].score[G] - Result[currentScore].score[P]) >= 2 ))
    {
      gestionSetTMC(G, P, max, diff);
    }   
  }
}

/**************************************************************************//**
* Gestion d'un changement de set
******************************************************************************/
void gestionSet( int G, int P, int max, int diff )
{
  Result[currentScore].score[G] = Result[currentScore].score[P] = 0;
  Result[currentScore].sets[G][Result[currentScore].set] += 1;
  Result[currentScore].chgtCote = false;

  if ((Result[currentScore].sets[G][Result[currentScore].set] + Result[currentScore].sets[P][Result[currentScore].set]) % 2 == 1)
    Result[currentScore].chgtCote = true;
  
  // on part en Tie break;
  if ((Result[currentScore].sets[G][Result[currentScore].set] == max) and Result[currentScore].sets[P][Result[currentScore].set] == max)
  {
    Result[currentScore].tiebreak = true;
    Result[currentScore].score[0] = Result[currentScore].score[1] = 10;
    memIServe = !Result[currentScore].server;
  }
  
  // Fins de set
  // Fin normal 6 jeux et 2 jeux d'ecart
  if ((Result[currentScore].sets[G][Result[currentScore].set] == max) and Result[currentScore].sets[P][Result[currentScore].set] <= (max-diff))
  {
    Result[currentScore].gainSets[G] +=1;
    if (Result[currentScore].gainSets[G] > maxSets/2) 
    {
      exitScore = true;
      matchSetType[Result[currentScore].set] = 0;
    }
    else
      Result[currentScore].set += 1;

  }
  // Fin 7 jeux à 5
  if ((Result[currentScore].sets[G][Result[currentScore].set] == (max + 1)) and Result[currentScore].sets[P][Result[currentScore].set] == (max - 1))
   {
    Result[currentScore].gainSets[G] +=1;
    if (Result[currentScore].gainSets[G] > maxSets/2) 
    {
      exitScore = true;
      matchSetType[Result[currentScore].set] = 0;
    }
    else
    {
      Result[currentScore].set +=1;
      // Colorise le score en cas de Super Tiebreak
      if (matchSetType[Result[currentScore].set] == 3)
        Result[currentScore].tiebreak = true;
    }
      Result[currentScore].set +=1;

  }

  if ((Result[currentScore].sets[G][Result[currentScore].set] == (max + 1)) and Result[currentScore].sets[P][Result[currentScore].set] == max)
  {
    Result[currentScore].gainSets[G] +=1;
    if (Result[currentScore].gainSets[G] > maxSets/2) 
    {
      exitScore = true;
      matchSetType[Result[currentScore].set] = 0;
    }
    else
    {
    Result[currentScore].set +=1;
    Result[currentScore].server = memIServe;
    Result[currentScore].tiebreak = false;
    }
  }
}

/**************************************************************************//**
* Gestion d'un changement de set
******************************************************************************/
void gestionSetTMC( int G, int P, int max, int diff )
{
  Result[currentScore].score[G] = Result[currentScore].score[P] = 0;
  Result[currentScore].sets[G][Result[currentScore].set] += 1;
  Result[currentScore].chgtCote = false;

  if ((Result[currentScore].sets[G][Result[currentScore].set] + Result[currentScore].sets[P][Result[currentScore].set]) % 2 == 1)
    Result[currentScore].chgtCote = true;
  
  // on part en Tie break si les 2 scores sont egaux à max-1;
  if ((Result[currentScore].sets[G][Result[currentScore].set] == max - 1) and (Result[currentScore].sets[P][Result[currentScore].set] == (max - 1)))
  {
    Result[currentScore].tiebreak = true;
    Result[currentScore].score[0] = Result[currentScore].score[1] = 10;
    memIServe = !Result[currentScore].server;
  }
  
  // Fins du set
  // Fin normal max jeux avec 2 jeux d'ecart
  if ((Result[currentScore].sets[G][Result[currentScore].set] == max) and (Result[currentScore].sets[P][Result[currentScore].set] <= (max-diff)))
  {
    
    Result[currentScore].gainSets[G] +=1;
    if (Result[currentScore].gainSets[G] > maxSets/2) 
    {
      exitScore = true;
      matchSetType[Result[currentScore].set] = 0;
    }
    else
      Result[currentScore].set += 1;
  }
  // Fin 7 jeux à 5
  if ((Result[currentScore].sets[G][Result[currentScore].set] == (max + 1)) and (Result[currentScore].sets[P][Result[currentScore].set] == (max - 1)))
   {

    Result[currentScore].gainSets[G] +=1;
    if (Result[currentScore].gainSets[G] > maxSets/2) 
    {
      exitScore = true;
      matchSetType[Result[currentScore].set] = 0;
    }
    else
      Result[currentScore].set +=1;
  }

  if ((Result[currentScore].sets[G][Result[currentScore].set] == (max)) and (Result[currentScore].sets[P][Result[currentScore].set] == (max-1)))
  {
    Result[currentScore].gainSets[G] +=1;
    if (Result[currentScore].gainSets[G] > maxSets/2) 
    {
      exitScore = true;
      matchSetType[Result[currentScore].set] = 0;
    }
    else
        Result[currentScore].set +=1;

    Result[currentScore].server = memIServe;
    Result[currentScore].tiebreak = false;
  }
}

/**************************************************************************//**
* Initialisation du match
******************************************************************************/
void gestionMatch( void )
{
  initScore();

  switch(Result[currentScore].format)  
  {
    case 1:
        maxSets = 3;
        matchSetType[0] = 1;
        matchSetType[1] = 1;
        matchSetType[2] = 1;
        break;
    case 2:
        maxSets = 3;
        matchSetType[0] = 1;
        matchSetType[1] = 1;
        matchSetType[2] = 3;
        break;
    case 3:
        maxSets = 3;
        matchSetType[0] = 4;
        matchSetType[1] = 4;
        matchSetType[2] = 3;
        break;
    case 4:
        maxSets = 3;
        matchSetType[0] = 2;
        matchSetType[1] = 2;
        matchSetType[2] = 3;
        break;
    case 5:
        maxSets = 3;
        matchSetType[0] = 5;
        matchSetType[1] = 5;
        matchSetType[2] = 3;
        break;
    case 6:
        maxSets = 3;
        matchSetType[0] = 6;
        matchSetType[1] = 6;
        matchSetType[2] = 3;
        break;
    case 7:
        maxSets = 3;
        matchSetType[0] = 7;
        matchSetType[1] = 7;
        matchSetType[2] = 3;
        break;
    case 8:
        maxSets = 3;
        matchSetType[0] = 3;
        matchSetType[1] = 3;
        matchSetType[2] = 3;
        break;
  }
}

/**************************************************************************//**
* Affichage score match en cours
******************************************************************************/
lv_obj_t* displayScore( uint32_t tile_num, uint8_t state )
{
  if (state == 0)
  {
    lv_style_init( &cont_style );
    lv_style_set_radius( &cont_style, LV_OBJ_PART_MAIN, 0 );
    lv_style_set_bg_color( &cont_style, LV_OBJ_PART_MAIN, LV_COLOR_BLACK );
    lv_style_set_bg_opa( &cont_style, LV_OBJ_PART_MAIN, LV_OPA_COVER );
    lv_style_set_border_width( &cont_style, LV_OBJ_PART_MAIN, 0 );

    view = mainbar_get_tile_obj( tile_num );
    lv_obj_clean( view );

    lv_obj_add_style( view, LV_OBJ_PART_MAIN, &cont_style );
    //lv_event_send_func(scr_event_cb, view, LV_EVENT_GESTURE, NULL);
    lv_obj_set_event_cb( view, scr_event_cb );

    //Affichage etat batterie
    lv_style_init(&Batt_style);
    lv_style_set_text_color(&Batt_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_text_font(&Batt_style, LV_STATE_DEFAULT, &liquidCrystal_nor_24);
    
    Batt1 = lv_label_create(view, nullptr);
    lv_obj_add_style(Batt1, LV_OBJ_PART_MAIN, &Batt_style);
    lv_label_set_text_fmt(Batt1, "%d", pmu_get_battery_percent( ));
    lv_obj_align(Batt1, NULL, LV_ALIGN_CENTER, 80, -100);

    // Ligne centrale
    lv_style_init(&line_style);
    lv_style_set_line_color(&line_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_line_width(&line_style, LV_STATE_DEFAULT, 1);
    lv_style_set_line_rounded(&line_style, LV_STATE_DEFAULT, 1);
    static lv_point_t line_points[] = { {0, 0}, {240, 0} };

    imgBall = lv_img_create(view, NULL);
    lv_obj_align( imgBall, view, LV_ALIGN_IN_LEFT_MID, 40, -80 );
    lv_obj_set_hidden(imgBall, true);

    imgChg = lv_img_create(view, NULL);
    lv_img_set_src(imgChg, &fleche1_40);
    lv_obj_align(imgChg, view, LV_ALIGN_IN_RIGHT_MID, -20, 0);
    lv_obj_set_hidden(imgChg, true);
 
    line1 = lv_line_create(view, NULL);
    lv_line_set_points(line1, line_points, 2);     /*Set the points*/
    lv_obj_add_style(line1, LV_OBJ_PART_MAIN, &line_style);
    lv_obj_align(line1, NULL, LV_ALIGN_CENTER, 0, 0);

    lv_style_init(&Jeu_style);
    lv_style_set_text_font(&Jeu_style, LV_STATE_DEFAULT, &TwentySeven_80);
    
    lv_style_init(&NoAdd_style);
    lv_style_set_text_font(&NoAdd_style, LV_STATE_DEFAULT, &TwentySeven_80);
    lv_style_set_text_color(&NoAdd_style, LV_STATE_DEFAULT, LV_COLOR_ORANGE);

    //Jeu Score Moi/Nous
    Score1 = lv_label_create(view, nullptr);
    lv_obj_add_style(Score1, LV_OBJ_PART_MAIN, &Jeu_style);
    lv_label_set_text(Score1, ScoreJeu[Result[currentScore].score[0]]);
    lv_obj_align(Score1, view, LV_ALIGN_CENTER, 10, -70);

    //Jeu Score Lui-Elle/Eux
    Score2 = lv_label_create(view, nullptr);
    lv_obj_add_style(Score2, LV_OBJ_PART_MAIN, &Jeu_style);
    lv_label_set_text(Score2, ScoreJeu[Result[currentScore].score[1]]);
    lv_obj_align(Score2, view, LV_ALIGN_CENTER, 10, 70);

    //Affichage Score des sets
    lv_style_init(&set_style);
    lv_style_set_text_color(&set_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_text_font(&set_style, LV_STATE_DEFAULT, &liquidCrystal_nor_32);
  
    // Score Sets Moi/Nous
    Set1 = lv_label_create(view, nullptr);
    lv_obj_add_style(Set1, LV_OBJ_PART_MAIN, &set_style);
    lv_obj_align(Set1, view, LV_ALIGN_IN_LEFT_MID, 10, -20);

    // Score Sets Lui-Elle/Eux
    Set2 = lv_label_create(view, nullptr);
    lv_obj_add_style(Set2, LV_OBJ_PART_MAIN, &set_style);
    lv_obj_align(Set2, view, LV_ALIGN_IN_LEFT_MID, 10, 20);
  }
  else
  {
    //Affichage etat batterie
    lv_label_set_text_fmt(Batt1, "%d", pmu_get_battery_percent( ));

    // Affichage Service
    // Choix de la couleur de balle
    if (currentScore == indexScore)
      lv_img_set_src( imgBall, &ball_25 );
    else    
      lv_img_set_src( imgBall, &redball_25 );

    // Positionnement de la balle
    if (Result[currentScore].server)
    {
      lv_obj_align( imgBall, view, LV_ALIGN_IN_LEFT_MID, 40, -80 );
    }
    else
    {
      lv_obj_align( imgBall, view, LV_ALIGN_IN_LEFT_MID, 40, 80 );
    }
    lv_obj_set_hidden(imgBall, false);

    // Affichage changement terrain
    if (Result[currentScore].chgtCote)
    {
      lv_obj_set_hidden(imgChg, false);
    }
    else
      lv_obj_set_hidden(imgChg, true);


    // Coloration des scores si tiebreak
    if (Result[currentScore].tiebreak)
      lv_style_set_text_color(&Jeu_style, LV_STATE_DEFAULT, LV_COLOR_YELLOW);  
    else
      lv_style_set_text_color(&Jeu_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);

    //Affichage du score jeu en cours
    //Jeu Score Moi/Nous
    if (Result[currentScore].noAdd && !Result[currentScore].server )
      lv_obj_add_style(Score1, LV_OBJ_PART_MAIN, &NoAdd_style);
    else
      lv_obj_add_style(Score1, LV_OBJ_PART_MAIN, &Jeu_style);
    lv_label_set_text(Score1, ScoreJeu[Result[currentScore].score[0]]);
 
    //Jeu Score Lui-Elle/Eux
    if (Result[currentScore].noAdd && Result[currentScore].server)
      lv_obj_add_style(Score2, LV_OBJ_PART_MAIN, &NoAdd_style);
    else
      lv_obj_add_style(Score2, LV_OBJ_PART_MAIN, &Jeu_style);
    lv_label_set_text(Score2, ScoreJeu[Result[currentScore].score[1]]);

    //Affichage Score des sets
    // Score Sets Moi/Nous
    if (Result[currentScore].set == 0)
      lv_label_set_text_fmt(Set1, "%s",ScoreSet[Result[currentScore].sets[0][0]]);
    if (Result[currentScore].set == 1)
      lv_label_set_text_fmt(Set1, "%s  %s",ScoreSet[Result[currentScore].sets[0][0]],ScoreSet[Result[currentScore].sets[0][1]]);
    if (Result[currentScore].set == 2)
      lv_label_set_text_fmt(Set1, "%s  %s  %s",ScoreSet[Result[currentScore].sets[0][0]],ScoreSet[Result[currentScore].sets[0][1]],ScoreSet[Result[currentScore].sets[0][2]]);

    // Score Sets Lui-Elle/Eux
    if (Result[currentScore].set == 0)
      lv_label_set_text_fmt(Set2, "%s",ScoreSet[Result[currentScore].sets[1][0]]);
    if (Result[currentScore].set == 1)
      lv_label_set_text_fmt(Set2, "%s  %s",ScoreSet[Result[currentScore].sets[1][0]],ScoreSet[Result[currentScore].sets[1][1]]);
    if (Result[currentScore].set == 2)
      lv_label_set_text_fmt(Set2, "%s  %s  %s",ScoreSet[Result[currentScore].sets[1][0]],ScoreSet[Result[currentScore].sets[1][1]],ScoreSet[Result[currentScore].sets[1][2]]);
  }

  gestDir = -1;
  return view;
}

/**************************************************************************//**
* Affichage score final
******************************************************************************/
lv_obj_t* displayScoreFinal ( uint32_t tile_num )
{
  if (view != NULL)
    lv_obj_clean( view );
  else
  {
    lv_style_init( &cont_style );
    lv_style_set_radius( &cont_style, LV_OBJ_PART_MAIN, 0 );
    lv_style_set_bg_color( &cont_style, LV_OBJ_PART_MAIN, LV_COLOR_BLACK );
    lv_style_set_bg_opa( &cont_style, LV_OBJ_PART_MAIN, LV_OPA_COVER );
    lv_style_set_border_width( &cont_style, LV_OBJ_PART_MAIN, 0 );

    view = mainbar_get_tile_obj( tile_num );
    lv_obj_add_style( view, LV_OBJ_PART_MAIN, &cont_style );
    lv_obj_set_event_cb( view, scr_event_cb );
    }

  // Ligne centrale
  static lv_style_t line_style;
  lv_style_init(&line_style);
  lv_style_set_line_color(&line_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_line_width(&line_style, LV_STATE_DEFAULT, 1);
  lv_style_set_line_rounded(&line_style, LV_STATE_DEFAULT, 1);
  static lv_point_t line_points[] = { {0, 0}, {240, 0} };

  lv_obj_t *line1;
  line1 = lv_line_create(view, NULL);
  lv_line_set_points(line1, line_points, 2);     /*Set the points*/
  lv_obj_add_style(line1, LV_OBJ_PART_MAIN, &line_style);
  lv_obj_align(line1, NULL, LV_ALIGN_CENTER, 0, 0);

  //Affichage du score final
  static lv_style_t HJeu_style;
  lv_style_init(&HJeu_style);
  lv_style_set_text_color(&HJeu_style, LV_STATE_DEFAULT, LV_COLOR_SILVER);
  lv_style_set_text_font(&HJeu_style, LV_STATE_DEFAULT, &TwentySeven_80);

  static lv_style_t BJeu_style;
  lv_style_init(&BJeu_style);
  lv_style_set_text_color(&BJeu_style, LV_STATE_DEFAULT, LV_COLOR_SILVER);
  lv_style_set_text_font(&BJeu_style, LV_STATE_DEFAULT, &TwentySeven_80);

  if (Result[currentScore].gainSets[0] > Result[currentScore].gainSets[1])
  {
    lv_style_set_text_color(&HJeu_style, LV_STATE_DEFAULT, LV_COLOR_LIME);
  }
  else
  {
    lv_style_set_text_color(&BJeu_style, LV_STATE_DEFAULT, LV_COLOR_LIME);
  }

  //Jeu Score Haut
  lv_obj_t *Score1_1 = lv_label_create(view, nullptr);
  lv_obj_add_style(Score1_1, LV_OBJ_PART_MAIN, &HJeu_style);
  lv_label_set_text_fmt(Score1_1, "%s", ScoreSet[Result[currentScore].sets[0][0]]);
  lv_obj_align(Score1_1, view, LV_ALIGN_CENTER, -80, -80);
  lv_obj_t *Score1_2 = lv_label_create(view, nullptr);
  lv_obj_add_style(Score1_2, LV_OBJ_PART_MAIN, &HJeu_style);
  lv_label_set_text_fmt(Score1_2, "%s", ScoreSet[Result[currentScore].sets[0][1]]);
  lv_obj_align(Score1_2, view, LV_ALIGN_CENTER, -20, -80);
  
  if (Result[currentScore].set > 1)
  {
    lv_obj_t *Score1_3 = lv_label_create(view, nullptr);
    lv_obj_add_style(Score1_3, LV_OBJ_PART_MAIN, &HJeu_style);
    lv_label_set_text_fmt(Score1_3, "%s", ScoreSet[Result[currentScore].sets[0][2]]);
    lv_obj_align(Score1_3, view, LV_ALIGN_CENTER, 40, -80);
  }  

  //Jeu Score Bas
  lv_obj_t *Score2_1 = lv_label_create(view, nullptr);
  lv_obj_add_style(Score2_1, LV_OBJ_PART_MAIN, &BJeu_style);
  lv_label_set_text_fmt(Score2_1, "%s", ScoreSet[Result[currentScore].sets[1][0]]);
  lv_obj_align(Score2_1, view, LV_ALIGN_CENTER, -80, 80);
  lv_obj_t *Score2_2 = lv_label_create(view, nullptr);
  lv_obj_add_style(Score2_2, LV_OBJ_PART_MAIN, &BJeu_style);
  lv_label_set_text_fmt(Score2_2, "%s", ScoreSet[Result[currentScore].sets[1][1]]);
  lv_obj_align(Score2_2, view, LV_ALIGN_CENTER, -20, 80);
  
  if (Result[currentScore].set > 1)
  {
    lv_obj_t *Score2_3 = lv_label_create(view, nullptr);
    lv_obj_add_style(Score2_3, LV_OBJ_PART_MAIN, &BJeu_style);
    lv_label_set_text_fmt(Score2_3, "%s", ScoreSet[Result[currentScore].sets[1][2]]);
    lv_obj_align(Score2_3, view, LV_ALIGN_CENTER, 40, 80);
  }  

  // Affichage resultats tiebreaks
  static lv_style_t tiebreak_style;
  lv_style_init(&tiebreak_style);
  lv_style_set_text_color(&tiebreak_style, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
  lv_style_set_text_font(&tiebreak_style, LV_STATE_DEFAULT, &liquidCrystal_nor_32);
  
  // Score Tiebreak
  // Set 1
  if (Result[currentScore].tbScore[0][0] != Result[currentScore].tbScore[1][0])
  {
   // Score Tiebreak Haut
    lv_obj_t *Tb0_1 = lv_label_create(view, nullptr);
    lv_obj_add_style(Tb0_1, LV_OBJ_PART_MAIN, &tiebreak_style);
    lv_label_set_text_fmt(Tb0_1, "%s", ScoreJeu[Result[currentScore].tbScore[0][0]]);  
    lv_obj_align(Tb0_1, view, LV_ALIGN_CENTER, -80, -20);
    // Score Tiebreak Bas
    lv_obj_t *Tb1_1 = lv_label_create(view, nullptr);
    lv_obj_add_style(Tb1_1, LV_OBJ_PART_MAIN, &tiebreak_style);
    lv_label_set_text_fmt(Tb1_1, "%s", ScoreJeu[Result[currentScore].tbScore[1][0]]);  
    lv_obj_align(Tb1_1, view, LV_ALIGN_CENTER, -80, 20);
  }

  // Set 2
  if (Result[currentScore].tbScore[0][1] != Result[currentScore].tbScore[1][1])
  {
   // Score Tiebreak Haut
    lv_obj_t *Tb0_2 = lv_label_create(view, nullptr);
    lv_obj_add_style(Tb0_2, LV_OBJ_PART_MAIN, &tiebreak_style);
    lv_label_set_text_fmt(Tb0_2, "%s", ScoreJeu[Result[currentScore].tbScore[0][1]]);  
    lv_obj_align(Tb0_2, view, LV_ALIGN_CENTER, -20, -20);
    // Score Tiebreak Bas
    lv_obj_t *Tb1_2 = lv_label_create(view, nullptr);
    lv_obj_add_style(Tb1_2, LV_OBJ_PART_MAIN, &tiebreak_style);
    lv_label_set_text_fmt(Tb1_2, "%s", ScoreJeu[Result[currentScore].tbScore[1][1]]);  
    lv_obj_align(Tb1_2, view, LV_ALIGN_CENTER, -20, 20);
  }

  // Set 3
  if (Result[currentScore].tbScore[0][2] != Result[currentScore].tbScore[1][2])
  {
   // Score Tiebreak Haut
    lv_obj_t *Tb0_3 = lv_label_create(view, nullptr);
    lv_obj_add_style(Tb0_3, LV_OBJ_PART_MAIN, &tiebreak_style);
    lv_label_set_text_fmt(Tb0_3, "%s", ScoreJeu[Result[currentScore].tbScore[0][2]]);  
    lv_obj_align(Tb0_3, view, LV_ALIGN_CENTER, 40, -20);
    // Score Tiebreak Bas
    lv_obj_t *Tb1_3 = lv_label_create(view, nullptr);
    lv_obj_add_style(Tb1_3, LV_OBJ_PART_MAIN, &tiebreak_style);
    lv_label_set_text_fmt(Tb1_3, "%s", ScoreJeu[Result[currentScore].tbScore[1][2]]);  
    lv_obj_align(Tb1_3, view, LV_ALIGN_CENTER, 40, 20);
  }

  lv_obj_t *time_btn = wf_add_right_button( view, enter_tennis_time_event_cb );
  lv_obj_align( time_btn, view, LV_ALIGN_IN_BOTTOM_RIGHT, -THEME_PADDING, -THEME_PADDING );

  return view;
}

/**************************************************************************//**
* Affichage temps match
******************************************************************************/
void tennis_time_display( uint32_t tile_num)
{
  lv_obj_clean( mainbar_get_tile_obj( tile_num ));

  // Calcul temps de match
  uint32_t tempsTotal = (Result[currentScore].time - Result[0].time) / 1000;
  uint32_t tempsH = tempsTotal / 3600;
  uint32_t tempsM = (tempsTotal - tempsH * 3600 ) / 60;
  uint32_t tempsS = (tempsTotal - tempsH * 3600 - tempsM * 60 );
  TENNIS_INFO_LOG("%02d:%02d;%02d", tempsH, tempsM, tempsS);

  // Affichage du temps de match
  static lv_style_t time_style;
  lv_style_init(&time_style);
  lv_style_set_text_color(&time_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_text_font(&time_style, LV_STATE_DEFAULT, &liquidCrystal_nor_64);

    lv_obj_t *matchTime = lv_label_create(mainbar_get_tile_obj( tile_num ), nullptr);
  lv_obj_add_style(matchTime, LV_OBJ_PART_MAIN, &time_style);
  lv_label_set_text_fmt(matchTime, "%2d:%02d:%02d", tempsH, tempsM, tempsS);
  lv_obj_align(matchTime, mainbar_get_tile_obj( tile_num ), LV_ALIGN_CENTER, 0, 0);


// Bouton de sortie vers le score final
  lv_obj_t *time_exit_btn = wf_add_left_button( mainbar_get_tile_obj( tile_num ), exit_tennis_time_event_cb );
  lv_obj_align( time_exit_btn, mainbar_get_tile_obj( tile_num ), LV_ALIGN_IN_BOTTOM_LEFT, THEME_PADDING, -THEME_PADDING );

}

/**************************************************************************//**
* Callback entrée tuile temps match
******************************************************************************/
static void enter_tennis_time_event_cb( lv_obj_t * obj, lv_event_t event )
{
    switch( event )
    {
        case( LV_EVENT_CLICKED ):       mainbar_jump_to_tilenumber( tennis_get_app_result_tile_num(), LV_ANIM_ON, true );
                                        break;
    }
}

/**************************************************************************//**
* Callback sortie tuile temps match
******************************************************************************/
static void exit_tennis_time_event_cb( lv_obj_t * obj, lv_event_t event )
{
    switch( event )
    {
        case( LV_EVENT_CLICKED ):       
              mainbar_jump_back();
              break;
    }
}

