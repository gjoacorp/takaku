#define RAYGUI_IMPLEMENTATION
#include "../include/raygui.h"
#include "../headers/board.h"
#include "../headers/ai.h"
#include "../headers/data_manager.h"
#include "../headers/game_manager.h"
#include "../headers/rect_button.h"
#include <numbers>
#include <cmath>
#include <iostream>
#include <vector>
#include <string>

void recentre_window(const int& window_width, const int& window_height)
{
  int monitor_width = GetMonitorWidth(GetCurrentMonitor());
  int monitor_height = GetMonitorHeight(GetCurrentMonitor());
  int monitor_centre_x = monitor_width / 2;
  int monitor_centre_y = monitor_height / 2;

  SetWindowPosition(monitor_centre_x - (window_width / 2), monitor_centre_y - (window_height / 2));
}

unsigned int window_width = ~0U;
unsigned int window_height = ~0U;

const Color CYBER_BLUE = (Color) {0, 34, 43, 1};
const Color CYBER_BASE = (Color) {2, 70, 88, 255};
const Color CYBER_LIGHT = (Color) {130, 205, 224, 255};

std::vector<Color> player_colors = { RED, BLUE };
const std::vector<std::pair<int, int>> resolutions = 
{
  {800, 800}, {900, 900}, {1000, 1000}, {1100, 1100}, {1200, 1200}
};

bool change_resolution = false;

const char* VERSION_STR = "takaku v0.08";

int main(void)
{
  int warning_output = false;
  bool updated_win_loss = false;

  int mode_selector_active_item = 0;
  int sr_dd_active_item = -1;
  bool sr_dd_edit = false;

  bool mode_selector_edit = false;

  bool is_game_active = false; // Has the player transitioned out of the main menu and started a game

  //////////////////////////////////////////////////////////////////////////////
  //// DATA LOADING (WIN/LOSS; RESOLUTION OPTIONS)
  //////////////////////////////////////////////////////////////////////////////

  std::pair<int, int> win_loss_data = data_manager::load_win_loss_data(game_manager::selected_board_size);

  data_manager::load_sr_config(window_width, window_height, sr_dd_active_item);

  //////////////////////////////////////////////////////////////////////////////
  //// GAME OBJECT INITIALISATION
  //////////////////////////////////////////////////////////////////////////////

  ai robot;

  //////////////////////////////////////////////////////////////////////////////
  //// WINDOW INITIALISATION
  //////////////////////////////////////////////////////////////////////////////

  Image w_i = LoadImage("icons/window_icon.png");

  SetConfigFlags(FLAG_MSAA_4X_HINT); 
  InitWindow(window_width, window_height, VERSION_STR); 
  SetWindowState(FLAG_WINDOW_RESIZABLE); 
  SetWindowMinSize(800, 800); 
  SetWindowIcon(w_i);
  SetWindowState(FLAG_VSYNC_HINT); 
  Vector2 window_centre = {static_cast<float>(window_width / 2), static_cast<float>(window_height / 2)};

  UnloadImage(w_i);

  // Board set-up
  board b(Vector2(0,0), window_width);
  b.set_color(CYBER_BLUE);
  b.player_colors = player_colors;
  b.default_circle_color = CYBER_BASE;
  b.frozen_circle_color = CYBER_LIGHT;

  //////////////////////////////////////////////////////////////////////////////
  //// RECT BUTTON INITIALISATION
  //////////////////////////////////////////////////////////////////////////////

  Image door = LoadImage("icons/door.png");
  Image arrows = LoadImage("icons/inward_arrows.png");
  if ( IsImageValid (door) && IsImageValid(arrows)) 
    std::cout << "Loaded images successfully." << std::endl;
  const Texture2D door_tex = LoadTextureFromImage(door);
  const Texture2D arrows_tex = LoadTextureFromImage(arrows);
  UnloadImage(door);
  UnloadImage(arrows);

  rect_button home_button ( Vector2(20, 20), Vector2 {128, 128}, CYBER_BLUE, CYBER_LIGHT, door_tex, Vector2(128,128));
  rect_button arrows_button ( Vector2(128, 20), Vector2 {128, 128}, CYBER_BLUE, CYBER_LIGHT, arrows_tex, Vector2(128,128));
  home_button.set_hover_background_color(CYBER_BASE);
  arrows_button.set_hover_background_color(CYBER_BASE);
  arrows_button.set_rounded(false);
  home_button.set_rounded(false);
  arrows_button.set_rect_scale_multiplier(1.5f);
  home_button.set_rect_scale_multiplier(1.5f);
  arrows_button.set_scale(0.5f);
  home_button.set_scale(0.5f);

  //////////////////////////////////////////////////////////////////////////////
  //// FONT AND RAYGUI STYLE INITIALISATION
  //////////////////////////////////////////////////////////////////////////////

  GuiLoadStyle("styles/cyber/style_cyber.rgs"); // Load style that is used for raygui 
  Font rockwell = LoadFontEx("fonts/rockwell.ttf", 48, NULL, 0); 

  if (rockwell.texture.id == 0)
    std::cout << "APPLICATON ERROR: Failed to load font." << std::endl;
  else
    GuiSetFont(rockwell);

  GuiSetStyle(DEFAULT, TEXT_SIZE, 24);
  GuiSetStyle(DEFAULT, TEXT_SPACING, 2);

  //////////////////////////////////////////////////////////////////////////////
  //// MAIN LOOP BEGINS
  //////////////////////////////////////////////////////////////////////////////

  while (!WindowShouldClose())    
  {
    ClearBackground(CYBER_BLUE);
    home_button.update();
    arrows_button.update();
    b.update(GetFrameTime());

    //////////////////////////////////////////////////////////////////////////////
    //// HANDLING MAIN MENU UI
    //////////////////////////////////////////////////////////////////////////////

    if (!is_game_active) 
    {
      // Text labels
      GuiLabel((Rectangle){ 4, static_cast<float>(window_height) - 40.0f, 300, 48}, VERSION_STR);
      GuiLabel((Rectangle){static_cast<float>(window_centre.x) - 300.0f, window_centre.y - 200, 250, 48}, "WINDOW SIZE");
      GuiLabel((Rectangle){static_cast<float>(window_centre.x) - 300.0f, window_centre.y - 100, 250, 48}, "BOARD SIZE");
      GuiLabel((Rectangle){static_cast<float>(window_centre.x) - 300.0f, window_centre.y, 250, 48}, "OPPONENT");

      // Have we been instructed to change the screen resolution
      change_resolution = GuiButton((Rectangle) { window_centre.x + 150, window_centre.y - 200, 100, 48}, "Apply");

      // Play button
      is_game_active = GuiButton((Rectangle) { window_centre.x + 150, window_centre.y, 100, 48}, "Play");

      // Resolution selector
      if ( GuiDropdownBox((Rectangle){ window_centre.x - 100, window_centre.y - 200, 200, 48 }, "800x800;900x900;1000x1000;1100x1100;1200x1200", &sr_dd_active_item, sr_dd_edit) ) 
      {
        sr_dd_edit = !sr_dd_edit;
      }

      if ( !sr_dd_edit ) 
      {
        // Opponent selection
        if ( GuiDropdownBox((Rectangle){ window_centre.x - 100, window_centre.y, 200, 48 }, "Computer;Human", &mode_selector_active_item, mode_selector_edit) )
          mode_selector_edit = !mode_selector_edit;

        // Board size selector
        GuiSpinner((Rectangle){ window_centre.x - 100, window_centre.y - 100, 200, 48 }, "", (int*)&game_manager::selected_board_size, 6, 12, false);

        // If the user has selected a different board size
        if ( game_manager::selected_board_size != game_manager::old_selected_board_size )
        {
          game_manager::old_selected_board_size = game_manager::selected_board_size;
          win_loss_data = data_manager::load_win_loss_data( game_manager::selected_board_size );
        }
      }

      if ( mode_selector_active_item == 0 && !mode_selector_edit )
      {
        // Win/loss
        std::string win_loss = "WIN/LOSS: " + std::to_string(win_loss_data.first) + " - " + std::to_string(win_loss_data.second);
        GuiLabel((Rectangle){window_centre.x - 100, window_centre.y + 100, 400, 48}, win_loss.c_str());
      }

      if (change_resolution) 
      {
        window_width = resolutions[sr_dd_active_item].first;
        window_height = resolutions[sr_dd_active_item].second;
        window_centre = { static_cast<float>(window_width / 2), static_cast<float>(window_height / 2) };
        recentre_window(window_width, window_height);
        SetWindowSize(window_width, window_height);

        data_manager::save_sr_config(window_width, window_height, sr_dd_active_item);
        b.set_size(window_width);
      }
    }

    //////////////////////////////////////////////////////////////////////////////
    //// HANDLING GAME STARTED BEHAVIOUR
    //////////////////////////////////////////////////////////////////////////////

    else
    {
      if (!b.is_initialised())
      {
        // If opponent is computer
        if ( mode_selector_active_item == 0 )
        {
          b.ai_enabled = true;
          b.set_player_idx( rand() % 2 ); // Randomly choose whether the player moves first or second
        }
        else
          b.ai_enabled = false;

        b.init_circles(300.0f, 30.0f, game_manager::selected_board_size);
      }
      else 
      {
        if ( arrows_button.get_activated() )
          b.return_circles_to_initial_positions();

        if ( b.ai_enabled && !b.is_game_over() && b.get_turn_idx() == !b.get_player_idx() )
        {
          float loading_f = b.get_time_since_last_move();
          float think_time = 1.0f;
          GuiProgressBar(Rectangle((static_cast<float>(window_width) / 2.0f) - 150, static_cast<float>(window_height) - 75.0f, 300, 50), "ai is thinking...", "", &loading_f, 0.0d, think_time);
          if (b.get_time_since_last_move() > think_time)
            robot.make_move(b);
        }



        // If SPACE is pressed and the game is not yet over, we mark this game as a loss
        if ( IsKeyPressed(KEY_SPACE) && b.is_game_over() )
        {
          b.reset_board();
          updated_win_loss = false;
          b.set_player_idx( rand() % 2 );
        }

        // Horrible if statement -- essentially handles whether wins/losses should be written to file and updated
        if  ( mode_selector_active_item == 0 && b.get_losing_player() != -1 && !updated_win_loss )
        {
          if ( b.get_losing_player() == !b.get_player_idx() )
          {
            win_loss_data.first++;
            data_manager::save_storage_value(2U * ( game_manager::selected_board_size - 6 ), win_loss_data.first);
          }
          else
          {
            win_loss_data.second++;
            data_manager::save_storage_value( (2U * ( game_manager::selected_board_size - 6 )) + 1U, win_loss_data.second);
          }
          updated_win_loss = true;
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////////
    //// HANDLING RETURN TO MAIN MENU BEHAVIOUR
    //////////////////////////////////////////////////////////////////////////////

    if ( (IsKeyPressed(KEY_M) || home_button.get_activated() ) && is_game_active )
    {
      // If the opponent is a human
      if ( mode_selector_active_item == 1 )
      {
        game_manager::show_warning_box = false;
      }
      // If the opponent is a robot, the player moves first, and they haven't moved yet
      else if ( b.get_player_idx() == 0 && b.get_line_counter() < 1 )
      {
        game_manager::show_warning_box = false;
      }
      // If the opponent is a robot, the player moves second, and they haven't moved yet
      else if ( b.get_player_idx() == 1 && b.get_line_counter() < 2 )
      {
        game_manager::show_warning_box = false;
      }
      // If the game is over
      else if ( b.is_game_over() )
      {
        game_manager::show_warning_box = false;
      }
      else
        game_manager::show_warning_box = true;

      if ( !game_manager::show_warning_box )
      {
        updated_win_loss = false;
        is_game_active = false;
        game_manager::show_warning_box = false;
        b.hard_reset_board();
      }
    }

    // Handle displaying game over text
    if ( b.is_game_over() )
    {
      std::string text = "";
      if ( b.get_player_idx() != b.get_losing_player() ) 
        text = "Victory!";
      else
        text = "Defeat!";
      text += " Press SPACE to play again.";
      GuiTextBox( Rectangle((window_width / 2) - 200, window_height - 100, 400, 48), text.data(), 24, 0 );
    }

    if ( game_manager::show_warning_box )
      warning_output = GuiMessageBox( (Rectangle) {(static_cast<float>(window_width) / 2.0f) - 300.0f, (static_cast<float>(window_height) / 2.0f) - 150, 600, 300}, 
          "", "Returning to the main menu now will forfeit \nthe current game and count as a loss.",
          "Return to main menu;Continue playing");
    else
      warning_output = -1;

    if ( game_manager::show_warning_box && warning_output == 1 )
    {
      if ( !b.is_game_over() && mode_selector_active_item == 0 )
      {
        win_loss_data.second++;
        data_manager::save_storage_value( (2U * ( game_manager::selected_board_size - 6 )) + 1U, win_loss_data.second);
      }
      updated_win_loss = false;
      game_manager::show_warning_box = false;
      is_game_active = false;
      b.hard_reset_board();
    }
    else if ( warning_output == 2 )
      game_manager::show_warning_box = false;

    //////////////////////////////////////////////////////////////////////////////
    //// DRAWING BEGINS
    //////////////////////////////////////////////////////////////////////////////

    BeginDrawing();

    if (b.is_initialised() && !game_manager::show_warning_box)
    {
      b.draw();
      home_button.draw();
      arrows_button.draw();

      DrawTextEx(rockwell, std::to_string(b.get_line_counter()).c_str(), (Vector2){static_cast<float>(window_width) - 60.0f, static_cast<float>(window_height) - 60.0f}, 48, 2.0f, RAYWHITE);

      if ( b.ai_enabled )
      {
        // Who is who bottom-left corner UI
        DrawRectangle(25, window_height - 75, 50, 50, RED);
        DrawRectangle(25, window_height - 135, 50, 50, BLUE);

        if (b.get_player_idx() == 0)
        {
          DrawTextEx(rockwell, "you", (Vector2){30, static_cast<float>(window_height) - 60.0f}, 24, 2.0f, RAYWHITE);
          DrawTextEx(rockwell, "ai", (Vector2){30, static_cast<float>(window_height) - 115.0f}, 24, 2.0f, RAYWHITE);
        }
        else if (b.get_player_idx() == 1)
        {
          DrawTextEx(rockwell, "ai", (Vector2){30, static_cast<float>(window_height) - 60.0f}, 24, 2.0f, RAYWHITE);
          DrawTextEx(rockwell, "you", (Vector2){30, static_cast<float>(window_height) - 115.0f}, 24, 2.0f, RAYWHITE);
        }
      }
    }
    EndDrawing();
  }

  //////////////////////////////////////////////////////////////////////////////
  //// MAIN LOOP ENDS; PERFORM CLEANUP
  //////////////////////////////////////////////////////////////////////////////

  UnloadFont(rockwell);
  UnloadTexture(door_tex);
  UnloadTexture(arrows_tex);
  CloseWindow();
  return 0;
}
