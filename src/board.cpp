#define GLSL_VERSION 330

#include "../headers/board.h"
#include <iostream>
#include "../include/raymath.h"

board::board(const Vector2& position, const unsigned int& board_size, const unsigned int& num_players, const std::vector<Color>& colors_v ) : entity(position) 
{
  this->board_size = board_size;
  this->num_players_ = num_players;
  this->p_colors_v_ = colors_v;

  const bool success = this->init_triangle_shader();
  if ( success )
    std::cout << "triangle_shader loaded successfully" << std::endl;
  else
    std::cout << "SHADER ERROR: triangle_shader was not loaded successfully" << std::endl;
}

board::board(const Vector2& position, const unsigned int& board_size) : entity(position) 
{
  this->board_size = board_size;

  const bool success = this->init_triangle_shader();
  if ( success )
    std::cout << "triangle_shader loaded successfully" << std::endl;

  else
    std::cout << "SHADER ERROR: triangle_shader was not loaded successfully" << std::endl;
}

board::~board()
{
  UnloadShader(this->triangle_shader);
}

void board::update(const float& delta) 
{
  if (this->initialised)
  {
    const float time_elapsed = static_cast<float>(this->_timer.time_elapsed());
    SetShaderValue(this->triangle_shader, this->time_uniform_loc, &time_elapsed, SHADER_UNIFORM_FLOAT);

    for (int i = 0; i < this->circles.size(); ++i) 
    {
      circles[i].update(delta);
      if ( this->circles[i].is_mouse_over() && !this->game_over && ( this->turn_idx == this->player_idx || !this->ai_enabled) ) 
      {
        this->hover_circle = &circles[i];
        this->circles[i].set_target_radius(this->circles[i].get_mouse_over_growth_mult() * this->circles[i].get_initial_radius());

        if ( IsKeyPressed(KEY_F) ) 
        {
          this->circles[i].set_frozen(!this->circles[i].is_frozen());

          if (this->draggable_circle == &this->circles[i])
            this->draggable_circle = nullptr;
        }

        if ( IsMouseButtonPressed(1) ) 
        {
          if ( !this->circles[i].is_frozen() ) 
            this->draggable_circle = &this->circles[i];
        }
        else if ( IsMouseButtonReleased(1) && this->draggable_circle != nullptr) 
          draggable_circle = nullptr;

        else if ( IsMouseButtonPressed(0) ) 
        {
          if ( this->line_source == nullptr ) 
          {
            this->line_source = &this->circles[i];
          }
          else 
          {
            if ( this->line_source != &this->circles[i] ) 
              this->line_target = &this->circles[i];
            else
              this->line_source = nullptr;
          }

          if ( is_move_valid(this->line_source, this->line_target ) )
            make_move(this->line_source, this->line_target);
        }
      }
      else
        this->circles[i].set_target_radius(this->circles[i].get_initial_radius());

      if ( &this->circles[i] != this->line_source ) 
      {
        if ( this->circles[i].is_frozen() ) 
          this->circles[i].set_color(this->frozen_circle_color);
        else
          this->circles[i].set_color(this->default_circle_color);
      }
      else 
      {
        this->circles[i].set_color( this->p_colors_v_[turn_idx] );
      }


      if (this->draggable_circle != nullptr) 
      {
        Vector2 mp = GetMousePosition();

        if ( ( mp.x > this->get_position().x ) && ( mp.x < this->get_position().x + this->board_size ) && ( mp.y > this->get_position().y ) && ( mp.y < this->get_position().y + this->board_size ) )
        {
          this->draggable_circle->set_position(GetMousePosition());
          this->draggable_circle->set_target_position(GetMousePosition());
        }
        else
          this->draggable_circle = nullptr;
      }
    }
  }

  if (IsKeyPressed(KEY_R)) 
  {
    this->return_circles_to_initial_positions();
  }
}

bool board::simulate_move(circle*& circ_a, circle*& circ_b) 
{
  this->lines.push_back( line (circ_a, circ_b, 0.0f, this->p_colors_v_[this->turn_idx] ) );
  bool losing_move = std::get<0>(contains_monochromatic_triangle());
  this->lines.pop_back();
  return losing_move;
}

void board::make_move(circle*& circ_a, circle*& circ_b) 
{
  this->lines.push_back( line(circ_a, circ_b, this->p_colors_v_[this->turn_idx]) );
  this->line_counter++;

  circ_a = nullptr;
  circ_b = nullptr;

  this->mono_tri_data = contains_monochromatic_triangle();

  if (std::get<0>(this->mono_tri_data))
  {
    this->game_over = true;
    this->losing_player = this->turn_idx;
  }
  else
    this->turn_idx = ( 1 + this->turn_idx ) % this->num_players_;

  this->_timer.start();
}

void board::draw() 
{
  // Drawing the background rectangle of the board
  DrawRectangleV(this->get_position(), Vector2(this->board_size, this->board_size), this->get_color());

  for (int i = 0; i < this->circles.size(); ++i) 
  {
    circles[i].draw_shadow();
  }

  if ( !this->lines.empty() ) 
  {
    for (int i = 0; i < this->line_counter; ++i) 
    {
      if ( (this->lines[i].get_source() != nullptr) && (this->lines[i].get_target() != nullptr) ) 
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

  if (std::get<0>( this->mono_tri_data) ) 
  {
    const std::vector<circle*> tri_verts = std::get<1>(this->mono_tri_data);
    Color tri_color = std::get<2>(this->mono_tri_data);
    tri_color.a = 200;

    BeginShaderMode(this->triangle_shader);

    DrawTriangle(tri_verts[0]->get_position(), tri_verts[1]->get_position(), tri_verts[2]->get_position(), tri_color);
    DrawTriangle(tri_verts[1]->get_position(), tri_verts[0]->get_position(), tri_verts[2]->get_position(), tri_color);

    EndShaderMode();
  }


  for (int i = 0; i < this->circles.size(); ++i) 
  {
    circles[i].draw();
  }
}

void board::init_circles(const float& poly_radius, const float& circle_radius, const unsigned int& num_circles) 
{
  int centre_x = this->get_position().x + (this->board_size / 2); 
  int centre_y = this->get_position().y + (this->board_size / 2);
  double angle = (2.0f * 3.14159265358979323846) / (double)num_circles;

  for (int i = 0; i < num_circles; ++i) 
  {
    circle c {};

    int x = centre_x + (poly_radius * std::cos(i * angle));
    int y = centre_y + (poly_radius * std::sin(i * angle));

    // We are safe to cast x and y to floats since we do not expect them to be larger than 2^24.
    this->circle_initial_positions.push_back(Vector2{ static_cast<float>(x), static_cast<float>(y) });
    c.set_target_position(Vector2{ static_cast<float>(x), static_cast<float>(y)});
    c.set_position(Vector2{ static_cast<float>(centre_x), static_cast<float>(centre_y)});
    c.set_initial_radius(circle_radius);
    c.set_mouse_over_growth_mult(this->circle_growth_mult);
    c.set_color(this->default_circle_color);
    c.set_frozen(false);

    this->circles.push_back(c);
  }
  this->_timer.start();
  this->initialised = true;
}

void board::move_circles_to(const std::vector<Vector2>& positions)
{
  if ( this->circles.size() == positions.size() ) {
    for ( int i = 0; i < this->circles.size(); i++ ) {
      if (!this->circles[i].is_frozen())
        this->circles[i].set_target_position(positions[i]);
    }
  }
}

void board::return_circles_to_initial_positions() 
{
  this->move_circles_to(this->circle_initial_positions);
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

  for (size_t i = 0; i < this->lines.size(); ++i) {
    if ( ( this->lines[i].get_source() == c_a && this->lines[i].get_target() == c_b ) ||
        ( this->lines[i].get_target() == c_a && this->lines[i].get_source() == c_b ) ) 
      return std::make_pair(true, &this->lines[i]);
  }
  return std::make_pair(false, nullptr);
}

std::tuple<bool, std::vector<circle*>, Color> board::contains_monochromatic_triangle() 
{
  for (size_t i = 0; i < this->circles.size(); ++i) {

    for (size_t j = i + 1; j < this->circles.size(); ++j) {

      for (size_t k = j + 1; k < this->circles.size(); ++k) {

        // Obtaining line data
        const std::pair<bool, line*> line_data_ij = this->does_line_exist(&this->circles[i], &this->circles[j]);
        const std::pair<bool, line*> line_data_jk = this->does_line_exist(&this->circles[j], &this->circles[k]);
        const std::pair<bool, line*> line_data_ki = this->does_line_exist(&this->circles[k], &this->circles[i]);

        // Verifying that a triangle exists
        if ( line_data_ij.first && line_data_jk.first && line_data_ki.first)

          if ( are_colors_equal(line_data_ij.second->get_color(), line_data_jk.second->get_color()) && are_colors_equal( line_data_jk.second->get_color(), line_data_ki.second->get_color()) ) {

            const Color tri_color = line_data_ij.second->get_color();
            std::vector<circle*> temp;

            temp.push_back(&this->circles[i]); temp.push_back(&this->circles[j]); temp.push_back(&this->circles[k]); 
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
    if ( !this->lines.empty() ) 
    {
      for (int i = 0; i < this->line_counter; ++i) 
      {
        if ( ( (this->lines[i].get_source() == circ_a) && (this->lines[i].get_target() == circ_b) ) || ( (this->lines[i].get_source() == circ_b) && (this->lines[i].get_target() == circ_a) ) ) 
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
  this->triangle_shader = LoadShader(0, TextFormat("shaders/rt.fs", GLSL_VERSION));
  if ( IsShaderValid(this->triangle_shader) )
  {
    this->time_uniform_loc = GetShaderLocation(this->triangle_shader, "time");
    return true;
  }
  return false;
}

void board::reset_board() 
{
  this->turn_idx = 0;
  this->lines.clear();
  this->line_source = nullptr;
  this->line_target = nullptr;
  this->draggable_circle = nullptr;
  this->line_counter = 0;
  this->game_over = false;
  this->losing_player = -1;
  this->mono_tri_data = std::make_tuple(false, std::vector<circle*>{}, BLACK);
  this->_timer.start();

  for (size_t i = 0; i < this->circles.size(); ++i)
    this->circles[i].kill_outgoing_lines();
}

void board::hard_reset_board() 
{
  this->reset_board();
  this->circles.clear();
  this->initialised = false;
  this->circle_initial_positions.clear();
}

void board::thaw_circles() 
{
  for ( auto c : this->circles )
    c.set_frozen(false);
}

// Getters and setters

unsigned int board::get_size() const { return this->board_size; }

void board::set_size(const unsigned int& size) { this->board_size = size; }

void board::set_player_colors(const std::vector<Color>& colors_v) { this->p_colors_v_ = colors_v; }

std::vector<Color> board::get_player_colors() const { return this->p_colors_v_; }

unsigned int board::get_line_counter() const { return this->line_counter; }

std::vector<circle>& board::get_circles() { return this->circles; }

unsigned int board::get_turn_idx() const { return this->turn_idx; }

bool board::is_game_over() const { return this->game_over; }

int board::get_losing_player() const { return this->losing_player; }

bool board::is_initialised() const { return this->initialised; }

double board::get_time_since_last_move() const { return this->_timer.time_elapsed(); }

void board::set_player_idx(const bool& b) { this->player_idx = b; }

bool board::get_player_idx() const { return this->player_idx; }
