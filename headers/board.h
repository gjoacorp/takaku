#ifndef BOARD

#define BOARD
#include "line.h"
#include "../headers/timer.h"
#include <vector>
#include <cmath>
#include <utility>
#include <tuple>

class board : public entity {
  private:
    unsigned int num_players_ = 2; 
    unsigned int turn_idx = 0; // Keeps track of which player is making the current move
    unsigned int line_counter = 0;

    int losing_player = -1;
    int time_uniform_loc;

    timer _timer {false};

    float circle_growth_mult = 1.2f;

    circle* line_source = nullptr;
    circle* line_target = nullptr;
    circle* draggable_circle = nullptr;
    circle* hover_circle = nullptr;

    std::tuple<bool, std::vector<circle*>, Color> mono_tri_data;

    bool game_over = false;
    bool initialised = false;
    bool player_idx;

    Shader triangle_shader_;

    std::vector<circle> circles;

    std::pair<bool, line*> does_line_exist(circle* circ_a, circle* circ_b);
    bool are_colors_equal(const Color& col_a, const Color& col_b) const;

    bool init_triangle_shader();

  public:
    unsigned int size;
    bool ai_enabled = false;
    
    Color default_circle_color = BLACK;
    Color frozen_circle_color = PURPLE;
    Color source_circle_color = PINK;

    std::vector<Color> player_colors;

    std::vector<line> lines;
    std::vector<Vector2> circle_initial_positions;

    board(const Vector2& position, const unsigned int& size, const unsigned int& num_players, const std::vector<Color>& colors_v); 
    board(const Vector2& position, const unsigned int& size);
    ~board();

    void update(const float& delta);
    void draw();

    void init_circles(const float& poly_radius, const float& circle_radius, const unsigned int& num_circles);
    void move_circles_to(const std::vector<Vector2>& positions);
    void return_circles_to_initial_positions();
    void thaw_circles();

    void make_move(circle*& circ_a, circle*& circ_b);
    bool is_move_valid(circle*& circ_a, circle*& circ_b);
    bool simulate_move(circle*& circ_a, circle*& circ_b);

    std::tuple<bool, std::vector<circle*>, Color> contains_monochromatic_triangle();

    // Getters and setters

    double get_time_since_last_move() const;

    std::vector<circle>& get_circles();

    unsigned int get_turn_idx() const;
    unsigned int get_line_counter() const;
    int get_losing_player() const;

    bool is_game_over() const;

    bool is_initialised() const;

    void reset_board();
    void hard_reset_board();

    void set_player_idx(const bool& b);
    bool get_player_idx() const;
};
#endif
