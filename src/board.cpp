#define GLSL_VERSION 330

#include "../headers/board.h"
#include <iostream>
#include "../include/raymath.h"

board::board(const Vector2& position, const unsigned int& size, const unsigned int& num_players, const std::vector<Color>& colors ) : entity(position) 
{
  this->size = size;
  num_players_ = num_players;
  player_colors = colors;

  const bool success = init_triangle_shader();
  if ( success )
    std::cout << "triangle shader loaded successfully" << std::endl;
  else
    std::cout << "SHADER ERROR: triangle shader was not loaded successfully" << std::endl;
}

board::board(const Vector2& position, const unsigned int& size) : entity(position) 
{
  this->size = size;

  const bool success = init_triangle_shader();
  if ( success )
    std::cout << "triangle shader loaded successfully" << std::endl;

  else
    std::cout << "SHADER ERROR: triangle shader was not loaded successfully" << std::endl;
}

board::~board()
{
  UnloadShader(triangle_shader_);
}

void board::update(const float& delta) 
{
  if (initialised)
  {
    const float time_elapsed = static_cast<float>(_timer.time_elapsed());
    SetShaderValue(triangle_shader_, time_uniform_loc, &time_elapsed, SHADER_UNIFORM_FLOAT);

    for (int i = 0; i < circles.size(); ++i) 
    {
      circles[i].update(delta);
      if ( circles[i].is_mouse_over() && !game_over && ( turn_idx == player_idx || !ai_enabled) ) 
      {
        hover_circle = &circles[i];
        circles[i].set_target_radius(circles[i].get_mouse_over_growth_mult() * circles[i].get_initial_radius());

        if ( IsKeyPressed(KEY_F) ) 
        {
          circles[i].set_frozen(!circles[i].is_frozen());

          if (draggable_circle == &circles[i])
            draggable_circle = nullptr;
        }

        if ( IsMouseButtonPressed(1) ) 
        {
          if ( !circles[i].is_frozen() ) 
            draggable_circle = &circles[i];
        }
        else if ( IsMouseButtonReleased(1) && draggable_circle != nullptr) 
          draggable_circle = nullptr;

        else if ( IsMouseButtonPressed(0) ) 
        {
          if ( line_source == nullptr ) 
          {
            line_source = &circles[i];
          }
          else 
          {
            if ( line_source != &circles[i] ) 
              line_target = &circles[i];
            else
              line_source = nullptr;
          }

          if ( is_move_valid(line_source, line_target ) )
            make_move(line_source, line_target);
        }
      }
      else
        circles[i].set_target_radius(circles[i].get_initial_radius());

      if ( &circles[i] != line_source ) 
      {
        if ( circles[i].is_frozen() ) 
          circles[i].set_color(frozen_circle_color);
        else
          circles[i].set_color(default_circle_color);
      }
      else 
      {
        circles[i].set_color( player_colors[turn_idx] );
      }


      if (draggable_circle != nullptr) 
      {
        Vector2 mp = GetMousePosition();

        if ( ( mp.x > get_position().x ) && ( mp.x < get_position().x + size ) && ( mp.y > get_position().y ) && ( mp.y < get_position().y + size ) )
        {
          draggable_circle->set_position(GetMousePosition());
          draggable_circle->set_target_position(GetMousePosition());
        }
        else
          draggable_circle = nullptr;
      }
    }
  }

  if (IsKeyPressed(KEY_R)) 
  {
    return_circles_to_initial_positions();
  }
}

bool board::simulate_move(circle*& circ_a, circle*& circ_b) 
{
  lines.push_back( line (circ_a, circ_b, 0.0f, player_colors[turn_idx] ) );
  bool losing_move = std::get<0>(contains_monochromatic_triangle());
  lines.pop_back();
  return losing_move;
}

void board::make_move(circle*& circ_a, circle*& circ_b) 
{
  lines.push_back( line(circ_a, circ_b, player_colors[turn_idx]) );
  line_counter++;

  circ_a = nullptr;
  circ_b = nullptr;

  mono_tri_data = contains_monochromatic_triangle();

  if (std::get<0>(mono_tri_data))
  {
    game_over = true;
    losing_player = turn_idx;
  }
  else
    turn_idx = ( 1 + turn_idx ) % num_players_;

  _timer.start();
}

void board::draw() 
{
  // Drawing the background rectangle of the board
  DrawRectangleV(get_position(), Vector2{ static_cast<float>(size), static_cast<float>(size) }, get_color());

  for (int i = 0; i < circles.size(); ++i) 
  {
    circles[i].draw_shadow();
  }

  if ( !lines.empty() ) 
  {
    for (int i = 0; i < line_counter; ++i) 
    { if ( (lines[i].get_source() != nullptr) && (lines[i].get_target() != nullptr) ) 
      {
        if ( i == lines.size() - 1 )
          lines[i].set_thickness(12.0f);
        else
          lines[i].set_thickness(5.0f);

        lines[i].update(GetFrameTime());
        lines[i].draw();
      }
    }
  }

  if (std::get<0>( mono_tri_data) ) 
  {
    const std::vector<circle*> tri_verts = std::get<1>(mono_tri_data);
    Color tri_color = std::get<2>(mono_tri_data);
    tri_color.a = 200;

    BeginShaderMode(triangle_shader_);

    DrawTriangle(tri_verts[0]->get_position(), tri_verts[1]->get_position(), tri_verts[2]->get_position(), tri_color);
    DrawTriangle(tri_verts[1]->get_position(), tri_verts[0]->get_position(), tri_verts[2]->get_position(), tri_color);

    EndShaderMode();
  }


  for (int i = 0; i < circles.size(); ++i) 
  {
    circles[i].draw();
  }
}

void board::init_circles(const float& poly_radius, const float& circle_radius, const unsigned int& num_circles) 
{
  int centre_x = get_position().x + (size / 2); 
  int centre_y = get_position().y + (size / 2);
  double angle = (2.0f * 3.14159265358979323846) / (double)num_circles;

  for (int i = 0; i < num_circles; ++i) 
  {
    circle c {};

    int x = centre_x + (poly_radius * std::cos(i * angle));
    int y = centre_y + (poly_radius * std::sin(i * angle));

    // We are safe to cast x and y to floats since we do not expect them to be larger than 2^24.
    circle_initial_positions.push_back(Vector2{ static_cast<float>(x), static_cast<float>(y) });
    c.set_target_position(Vector2{ static_cast<float>(x), static_cast<float>(y)});
    c.set_position(Vector2{ static_cast<float>(centre_x), static_cast<float>(centre_y)});
    c.set_initial_radius(circle_radius);
    c.set_mouse_over_growth_mult(circle_growth_mult);
    c.set_color(default_circle_color);
    c.set_frozen(false);

    circles.push_back(c);
  }
  _timer.start();
  initialised = true;
}

void board::move_circles_to(const std::vector<Vector2>& positions)
{
  if ( circles.size() == positions.size() ) {
    for ( int i = 0; i < circles.size(); i++ ) {
      if (!circles[i].is_frozen())
        circles[i].set_target_position(positions[i]);
    }
  }
}

void board::return_circles_to_initial_positions() 
{
  move_circles_to(circle_initial_positions);
}

bool board::are_colors_equal(const Color& col_a, const Color& col_b) const 
{
  if (col_a.r == col_b.r && col_a.g == col_b.g && col_a.b == col_b.b && col_a.a == col_b.a)
    return true;
  return false;
}

std::pair<bool, line*> board::does_line_exist(circle* c_a, circle* c_b) 
{
  if ( c_a == nullptr || c_b == nullptr) 
    return std::make_pair(false, nullptr);

  for (size_t i = 0; i < lines.size(); ++i) {
    if ( ( lines[i].get_source() == c_a && lines[i].get_target() == c_b ) ||
        ( lines[i].get_target() == c_a && lines[i].get_source() == c_b ) ) 
      return std::make_pair(true, &lines[i]);
  }
  return std::make_pair(false, nullptr);
}

std::tuple<bool, std::vector<circle*>, Color> board::contains_monochromatic_triangle() 
{
  for (size_t i = 0; i < circles.size(); ++i) {

    for (size_t j = i + 1; j < circles.size(); ++j) {

      for (size_t k = j + 1; k < circles.size(); ++k) {

        // Obtaining line data
        const std::pair<bool, line*> line_data_ij = does_line_exist(&circles[i], &circles[j]);
        const std::pair<bool, line*> line_data_jk = does_line_exist(&circles[j], &circles[k]);
        const std::pair<bool, line*> line_data_ki = does_line_exist(&circles[k], &circles[i]);

        // Verifying that a triangle exists
        if ( line_data_ij.first && line_data_jk.first && line_data_ki.first)

          if ( are_colors_equal(line_data_ij.second->get_color(), line_data_jk.second->get_color()) && are_colors_equal( line_data_jk.second->get_color(), line_data_ki.second->get_color()) ) {

            const Color tri_color = line_data_ij.second->get_color();
            std::vector<circle*> temp;

            temp.push_back(&circles[i]); temp.push_back(&circles[j]); temp.push_back(&circles[k]); 
            return std::make_tuple(true, temp, tri_color);
          }
      }
    }
  }
  return std::make_tuple(false, std::vector<circle*>{}, BLACK);
}

bool board::is_move_valid(circle*& circ_a, circle*& circ_b) {
  // Checking whether source and target circles exist and are distinct
  if ( ((circ_a != nullptr) && (circ_b != nullptr)) && (circ_a != circ_b) ) 
  {
    // Checking whether the lines [source -> target] and [target -> source] already exist
    if ( !lines.empty() ) 
    {
      for (int i = 0; i < line_counter; ++i) 
      {
        if ( ( (lines[i].get_source() == circ_a) && (lines[i].get_target() == circ_b) ) || ( (lines[i].get_source() == circ_b) && (lines[i].get_target() == circ_a) ) ) 
          return false;
      }
    }
  }
  else
    return false;

  return true;
}

bool board::init_triangle_shader() 
{
  triangle_shader_ = LoadShader(0, TextFormat("shaders/rt.fs", GLSL_VERSION));
  if ( IsShaderValid(triangle_shader_) )
  {
    time_uniform_loc = GetShaderLocation(triangle_shader_, "time");
    return true;
  }
  return false;
}

void board::reset_board() 
{
  turn_idx = 0;
  lines.clear();
  line_source = nullptr;
  line_target = nullptr;
  draggable_circle = nullptr;
  line_counter = 0;
  game_over = false;
  losing_player = -1;
  mono_tri_data = std::make_tuple(false, std::vector<circle*>{}, BLACK);
  _timer.start();

  for (size_t i = 0; i < circles.size(); ++i)
    circles[i].kill_outgoing_lines();
}

void board::hard_reset_board() 
{
  reset_board();
  circles.clear();
  initialised = false;
  circle_initial_positions.clear();
}

void board::thaw_circles() 
{
  for ( auto c : circles )
    c.set_frozen(false);
}

// Getters and setters

unsigned int board::get_line_counter() const { return line_counter; }

std::vector<circle>& board::get_circles() { return circles; }

unsigned int board::get_turn_idx() const { return turn_idx; }

bool board::is_game_over() const { return game_over; }

int board::get_losing_player() const { return losing_player; }

bool board::is_initialised() const { return initialised; }

double board::get_time_since_last_move() const { return _timer.time_elapsed(); }
