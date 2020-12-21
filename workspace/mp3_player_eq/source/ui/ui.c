/*******************************************************************************
  @file     ui.c
  @brief    User Interface Module
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "ui.h"
#include "audio/audio.h"
#include "display/display.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "drivers/MCAL/equaliser/equaliser.h"
#include "drivers/HAL/HD44780_LCD/HD44780_LCD.h"
#include "drivers/HAL/timer/timer.h"
#include "lib/fatfs/ff.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

// UI module general parameters
#define UI_LCD_ROTATION_TIME_MS  	  350
#define UI_LCD_FPS_MS              (200)
#define UI_LCD_LINE_NUMBER       	  1
#define UI_FILE_SYSTEM_ROOT 	      ""
#define UI_BUFFER_SIZE              256
#define UI_EQUALISER_GAIN_COUNT     (19)
#define UI_EQUALISER_BAND_COUNT     (8)

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef enum {
  UI_STRING_FOLDER,
  UI_STRING_FILE,
  UI_STRING_OTHER
} ui_string_type_t;

typedef enum {
  UI_STATE_MENU,                // Displaying the main menu to the user
  UI_STATE_FILE_SYSTEM,         // Navigating the file system
  UI_STATE_EQUALISER            // Configuring the equaliser filter
} ui_state_t;

typedef enum {
  UI_OPTION_FILE_SYSTEM,        // File system menu option
  UI_OPTION_EQUALISER,          // Equaliser menu option

  UI_OPTION_COUNT
} ui_main_menu_options_t;

typedef enum {
  UI_EQUALISER_STATE_MENU,      // Equaliser menu state
  UI_EQUALISER_STATE_CUSTOM     // Custom menu state
} ui_equaliser_state_t;

typedef enum {
  UI_EQUALISER_OPTION_JAZZ,     // Jazz option for the equaliser setting
  UI_EQUALISER_OPTION_ROCK,     // Rock option for the equaliser setting
  UI_EQUALISER_OPTION_CLASSIC,  // Classic option for the equaliser setting
  UI_EQUALISER_OPTION_CUSTOM,   // Custom option for the equaliser setting
  
  UI_EQUALISER_OPTION_COUNT
} ui_equaliser_menu_options_t;

typedef struct {
  ui_main_menu_options_t currentOptionIndex;  // Index of the current menu option
} ui_menu_context_t;

typedef struct {  
  uint32_t  currentFileIndex;                 // Current file index
  char      currentPath[UI_BUFFER_SIZE];      // Path of the current directory
  FILINFO   currentFile;                      // Current file information
  FRESULT   currentError;                     // Error variable for the FatFs handler
  DIR       currentDirectory;                 // Directory of current position in file system
} ui_file_system_context_t;

typedef struct {
  ui_equaliser_state_t        eqState;        // Current equaliser state
  ui_equaliser_menu_options_t eqOption;       // Current equaliser option selected

  bool 		    hasEqBandSelected;                  	// Whether a band is selected or not
  uint16_t	  currentEqBandSelected;              	// Index of the current equaliser band selected
  uint32_t  	eqBandGain[UI_EQUALISER_BAND_COUNT]; 	// Equaliser gains
} ui_equaliser_context_t;

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

/**
 * @brief Callback to be called on FPS event triggered by the timer driver,
 * 		  used to update the LCD.
 */
static void	uiLcdUpdate(void);

/**
 * @brief Open a directory handler for the file system on the current path.
 */
static void uiFileSystemOpenDirectory(void);

/**
 * @brief Update the current string being displayed.
 * @param message   New string to be updated
 * @param type      Type of the string
 */
static void uiSetDisplayString(const char* message, ui_string_type_t type);

/**
 * @brief Update the current state of the UI module.
 * @param state   Next state
 */
static void uiSetState(ui_state_t state);

/**
 * @brief Cycle the UI in the menu state.
 * @param event   Next event
 */
static void uiRunMenu(event_t event);

/**
 * @brief Cycle the UI in the file system state.
 * @param event   Next event
 */
static void uiRunFileSystem(event_t event);
/**
 * @brief Cycle the UI in the Equaliser state.
 * @param event   Next event
 */
static void uiRunEqualiser(event_t event);

/**
 * @brief Initializes the UI in the menu state.
 */
static void uiInitMenu(void);

/**
 * @brief Initializes the UI in the file system state.
 */
static void uiInitFileSystem(void);

/**
 * @brief Initializes the UI in the equaliser state.
 */
static void uiInitEqualiser(void); 

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static const char*  MAIN_MENU_OPTIONS[UI_OPTION_COUNT] = {
  "Sistema de archivos",
  "Ecualizador"
};

static const char* EQUALISER_MENU_OPTIONS[UI_EQUALISER_OPTION_COUNT] = {
  "Jazz",
  "Rock",
  "Classic",
  "Custom"
};

static const double GAIN_VALUES[UI_EQUALISER_GAIN_COUNT] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};

static const double DEFAULT_GAINS[][UI_EQUALISER_GAIN_COUNT] = {
  { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8 }, // Jazz Default Gains
  { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8 }, // Rock Default Gains
  { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8 }  // Classic Default Gains
};

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static bool			    		  messageChanged = false;		    // Internal flag for changing the LCD message
static bool         			alreadyInit = false;          // Internal flag for initialization process
static ui_state_t   			currentState;         		    // Current state of the user interface module
static char               messageBuffer[UI_BUFFER_SIZE];// Buffer for message to print

static ui_menu_context_t        menuContext;            	// Context for the menu state of the UI module
static ui_file_system_context_t fsContext;              	// Context for the file system state of the UI module
static ui_equaliser_context_t 	eqContext;                // Context for the equalisator UI module

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void uiInit(void)
{
  if (!alreadyInit)
  {
    // Raise the already initialized flag, to avoid multiple initialization
    alreadyInit = true;

    // Initialize the LCD
    HD44780LcdInit();

    // Initialization of the timer driver
    timerInit();
    timerStart(timerGetId(), TIMER_MS2TICKS(UI_LCD_FPS_MS), TIM_MODE_PERIODIC, uiLcdUpdate);

    // Initialize the internal state of the UI module
    uiSetState(UI_STATE_MENU);
  }
}

void uiRun(event_t event)
{
  switch (currentState)
  {
    case UI_STATE_MENU:
      uiRunMenu(event);
      break;
      
    case UI_STATE_FILE_SYSTEM:
      uiRunFileSystem(event);
      break;
      
    case UI_STATE_EQUALISER:
      uiRunEqualiser(event);
      break;

    default:
      break;
  }
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static void	uiLcdUpdate(void)
{
  if (HD44780LcdInitReady())
  {
    if (messageChanged)
    {
      messageChanged = false;
      HD44780WriteRotatingString(UI_LCD_LINE_NUMBER, messageBuffer, strlen(messageBuffer), UI_LCD_ROTATION_TIME_MS);
    }
  }
}

static void uiSetDisplayString(const char* message, ui_string_type_t type)
{
  switch (type)
  {
    case UI_STRING_FOLDER:
      messageBuffer[0] = HD44780_CUSTOM_FOLDER;
      sprintf(messageBuffer + 1, " - %s", message);
      break;
    case UI_STRING_FILE:
        messageBuffer[0] = HD44780_CUSTOM_MUSIC;
      sprintf(messageBuffer + 1, " - %s", message);
      break;
    case UI_STRING_OTHER:
    default:
      sprintf(messageBuffer, "%s", message);
      break;
  }
  messageChanged = true;
}

static void uiSetState(ui_state_t state)
{
  currentState = state;

  switch (currentState)
  {
    case UI_STATE_MENU:
      uiInitMenu();
      break;

    case UI_STATE_FILE_SYSTEM:
      uiInitFileSystem();
      break;
      
    case UI_STATE_EQUALISER:
      uiInitEqualiser();
      break;

    default:
      break;
  }
}

static void uiRunMenu(event_t event)
{
  switch (event.id)
  {
    case EVENTS_LEFT:
      if (menuContext.currentOptionIndex)
      {
        menuContext.currentOptionIndex--;
      }
      uiSetDisplayString(MAIN_MENU_OPTIONS[menuContext.currentOptionIndex], UI_STRING_OTHER);
      break;

    case EVENTS_RIGHT:
      if (menuContext.currentOptionIndex + 1 < UI_OPTION_COUNT)
      {
        menuContext.currentOptionIndex++;
      }
      uiSetDisplayString(MAIN_MENU_OPTIONS[menuContext.currentOptionIndex], UI_STRING_OTHER);
      break;

    case EVENTS_ENTER:
      uiSetState(menuContext.currentOptionIndex + UI_STATE_FILE_SYSTEM);
      break;

    default:
      break;
  }
}

static void uiRunFileSystem(event_t event)
{
  switch (event.id)
  {
    case EVENTS_LEFT:
      // Read the previous directory file
      fsContext.currentError = f_rewinddir(&(fsContext.currentDirectory));
      if (fsContext.currentError == FR_OK)
      {
        for (uint32_t i = 0 ; (i < fsContext.currentFileIndex) && (fsContext.currentError == FR_OK) ; i++)
        {
          fsContext.currentError = f_readdir(&(fsContext.currentDirectory), &(fsContext.currentFile));
        }
        if (fsContext.currentError == FR_OK)
        {
          uiSetDisplayString(fsContext.currentFile.fname, fsContext.currentFile.fattrib == AM_DIR ? UI_STRING_FOLDER : UI_STRING_FILE);
          if (fsContext.currentFileIndex)
          {
              fsContext.currentFileIndex--;
          }
        }
        else
        {
          uiSetState(UI_STATE_MENU);
        }      
      }
      else
      {
        uiSetState(UI_STATE_MENU);
      }
      break;

    case EVENTS_RIGHT:
      // Read the next directory file
      fsContext.currentError = f_readdir(&(fsContext.currentDirectory), &(fsContext.currentFile));
      if (fsContext.currentError == FR_OK)
      {
        if (fsContext.currentFile.fname[0])
        {
            uiSetDisplayString(fsContext.currentFile.fname, fsContext.currentFile.fattrib == AM_DIR ? UI_STRING_FOLDER : UI_STRING_FILE);
            fsContext.currentFileIndex++;
        }
      }
      else
      {
        uiSetState(UI_STATE_MENU);
      }
      break;

    case EVENTS_ENTER:
      if (fsContext.currentFile.fattrib == AM_DIR)
      {
        // Appends the path
    	  sprintf(&fsContext.currentPath[strlen(fsContext.currentPath)], strlen(fsContext.currentPath) ? "/%s" : "%s", fsContext.currentFile.fname);

        // Open the directory
        uiFileSystemOpenDirectory();  
      }
      else if (fsContext.currentFile.fattrib == AM_ARC)
      {
        audioSetFolder(fsContext.currentPath, fsContext.currentFile.fname, fsContext.currentFileIndex);
      }
      break;

    case EVENTS_EXIT:
      // Verify, if current directory is the root, then changes the state
      // and sets the UI module in the menu state
      if ((strlen(fsContext.currentPath) == 0) || (strcmp(fsContext.currentPath, UI_FILE_SYSTEM_ROOT) == 0))
      {
        // Go to the menu state
        uiSetState(UI_STATE_MENU);
      }
      else
      {
        // Append the folder name to the current directory path
        for (uint32_t i = 1 ; i < strlen(fsContext.currentPath) ; i++)
        {
          if (fsContext.currentPath[strlen(fsContext.currentPath) - i] == '/')
          {
            fsContext.currentPath[strlen(fsContext.currentPath) - i] = '\0';
            break;
          }
        }

        // Open the directory
        uiFileSystemOpenDirectory(); 
      }
      break;

    default:
      break;
  }
}

static void uiRunEqualiser(event_t event)
{
  if (eqContext.eqState == UI_EQUALISER_STATE_MENU)
  {
    switch (event.id)
    {
      case EVENTS_LEFT:
        if (eqContext.eqOption)
        {
          eqContext.eqOption--;
        }
        uiSetDisplayString(EQUALISER_MENU_OPTIONS[eqContext.eqOption], UI_STRING_OTHER);
        break;

      case EVENTS_RIGHT:
        if ((eqContext.eqOption + 1) < UI_EQUALISER_OPTION_COUNT)
        {
          eqContext.eqOption++;
        }
        uiSetDisplayString(EQUALISER_MENU_OPTIONS[eqContext.eqOption], UI_STRING_OTHER);
        break;

      case EVENTS_EXIT:
      case EVENTS_ENTER:
        if (eqContext.eqOption == UI_EQUALISER_OPTION_CUSTOM)
        {
          eqContext.eqState = UI_EQUALISER_STATE_CUSTOM;
          eqContext.hasEqBandSelected = false;
          eqContext.currentEqBandSelected = 0;
        }
        else
        {
          for (uint8_t i = 0 ; i < UI_EQUALISER_BAND_COUNT ; i++)
          {
            eqSetFilterGains(DEFAULT_GAINS[eqContext.eqOption]);
          }
          displaySelectColumn(DISPLAY_UNSELECT_COLUMN);
          uiSetState(UI_STATE_MENU);
        }
        break;

      default:
        break;
    }
  }
  else if (eqContext.eqState == UI_EQUALISER_STATE_CUSTOM)
  {
    switch (event.id)
    {
      case EVENTS_LEFT:
        if (eqContext.hasEqBandSelected)
        {
          if (eqContext.eqBandGain[eqContext.currentEqBandSelected] > 0)
          {
            eqContext.eqBandGain[eqContext.currentEqBandSelected]--;
          }
        }
        else
        {
          if (eqContext.currentEqBandSelected)
          {
            eqContext.currentEqBandSelected--;
            displaySelectColumn(eqContext.currentEqBandSelected);
          }
        }
        break;

      case EVENTS_RIGHT:
        if (eqContext.hasEqBandSelected)
        {
          if (eqContext.eqBandGain[eqContext.currentEqBandSelected] < UI_EQUALISER_GAIN_COUNT)
          {
            eqContext.eqBandGain[eqContext.currentEqBandSelected]++;
          }
        }
        else
        {
          if ((eqContext.currentEqBandSelected + 1) < UI_EQUALISER_BAND_COUNT)
          {
            eqContext.currentEqBandSelected++;
            displaySelectColumn(eqContext.currentEqBandSelected);
          }
        }
        break;

      case EVENTS_EXIT:
      case EVENTS_ENTER:
        if (eqContext.hasEqBandSelected)
        {
          for (uint8_t i = 0 ; i < UI_EQUALISER_BAND_COUNT ; i++)
          {
            eqSetFilterGain(GAIN_VALUES[eqContext.eqBandGain[i]], i);
          }
          displaySelectColumn(DISPLAY_UNSELECT_COLUMN);
          uiSetState(UI_STATE_MENU);
        }
        else
        {
          eqContext.hasEqBandSelected = true;
        }
        break;

      default:
        break;
    }
  }
}

static void uiInitMenu(void)
{
  // Sets the initial option of the menu state, and changes the
  // corresponding string representing this options in the LCD display.
  menuContext.currentOptionIndex = UI_OPTION_FILE_SYSTEM;
  uiSetDisplayString(MAIN_MENU_OPTIONS[menuContext.currentOptionIndex], UI_STRING_OTHER);
}

static void uiFileSystemOpenDirectory(void)
{
  // Force to close the directory
  f_closedir(&fsContext.currentDirectory);

  // Open the new directory
  fsContext.currentError = f_opendir(&(fsContext.currentDirectory), fsContext.currentPath);
  if (fsContext.currentError == FR_OK)
  {
    fsContext.currentError = f_readdir(&(fsContext.currentDirectory), &(fsContext.currentFile));  
    if (fsContext.currentError == FR_OK)
    {
      uiSetDisplayString(fsContext.currentFile.fname, fsContext.currentFile.fattrib == AM_DIR ? UI_STRING_FOLDER : UI_STRING_FILE);
      fsContext.currentFileIndex = 0;
    }
    else
    {
      uiSetState(UI_STATE_MENU);
    }
  }
  else
  {
    uiSetState(UI_STATE_MENU);
  }
}

static void uiInitFileSystem(void)
{
  // Starts on the root directory
  sprintf(fsContext.currentPath, "%s", UI_FILE_SYSTEM_ROOT);

  // Open the directory
  uiFileSystemOpenDirectory();
}

static void uiInitEqualiser(void)
{
  // Sets the initial option of the equaliser state, and changes the
  // corresponding string representing this options in the LCD display.
  eqContext.eqOption = UI_EQUALISER_OPTION_JAZZ;
  uiSetDisplayString(EQUALISER_MENU_OPTIONS[eqContext.eqOption], UI_STRING_OTHER);
}

/*******************************************************************************
 *******************************************************************************
						INTERRUPT SERVICE ROUTINES
 *******************************************************************************
 ******************************************************************************/

/******************************************************************************/
