#include <bits/stdc++.h>
#include <math.h>
#include <array>

#include "picosystem.hpp"

using namespace picosystem;

// Estados del Juego
enum state_t
{
  PLAYING,
  GAME_OVER,
  MENU,
  SETTINGS
};
state_t state = MENU;

struct vec_t
{
  int32_t x, y;
};

// Fronteras y escala del mapa
constexpr vec_t bounds{.x = 18, .y = 16};
constexpr int scale = 6;

// Selector de Juego
bool play_selected = true;
// Verificar si el juego está listo
bool is_game_set = false;

// Selectores de Dificultad en Menu
bool easy_selected = true;
bool medium_selected = false;
bool hard_selected = false;
// Velocidad de aparición de enemigos
int32_t difficulty = 300;

// Arreglo de Sprites
std::array<int, 6> fruit_sprites = {{
    APPLE,
    CHERRY,
    BANANA,
    ORANGE,
    EGGPLANT,
    STRAWBERRY,
}};

// Estructura de Jugador
struct
{
  vec_t dir;
  vec_t pos;
  int32_t score;
  int32_t lives;

  vec_t next()
  {
    return {.x = pos.x + dir.x, .y = pos.y + dir.y};
  }

  void move()
  {
    double L = sqrt(dir.x * dir.x + dir.y * dir.y);
    dir.x /= L;
    dir.y /= L;
    pos = next();
  }
} player;

class Fruit
{
public:
  vec_t pos;
  vec_t dir;
  Fruit(int x, int y)
  {
    pos = {.x = x, .y = y};
    dir = {.x = -1, .y = 0};
  }
  vec_t next()
  {
    return {.x = pos.x + dir.x, .y = pos.y + dir.y};
  }

  void move()
  {
    pos = next();
  }
};

class Bullet
{
public:
  vec_t pos;
  vec_t dir;
  Bullet(int x, int y)
  {
    pos = {.x = x, .y = y};
    dir = {.x = 1, .y = 0};
  }
  vec_t next()
  {
    return {.x = pos.x + dir.x, .y = pos.y + dir.y};
  }

  void move()
  {
    pos = next();
  }
};

/*
Funciones de la mecánica de Juego
*/

// Sonidos de selector y aceptar
voice_t blip, enter;
// Fruta aleatoria del arreglo fruit_sprites
int fruit_selected;

// Deques de los elementos del juego
std::deque<Fruit> fruits;
std::deque<Fruit> rotten_fruits;

Fruit place_fruit()
{
  return Fruit(bounds.x - 1, std::rand() % bounds.y);
}

void change_state(state_t _state)
{
  state = _state;
}

// Transformar posiciones del mapa de acuerdo a la escala
vec_t transform(vec_t v)
{
  return {.x = (v.x * scale) + 6, .y = (v.y * scale) + 18};
}

/*
Revisión de colisión en paredes
*/

bool in_bounds_bottom(vec_t next)
{
  return next.y <= bounds.y - 1;
}

bool in_bounds_top(vec_t next)
{
  return next.y >= 1;
}

bool wall_hit_check_x(vec_t next)
{
  return next.x < 1 || next.x >= bounds.x - 3;
}

bool wall_hit_check_y(vec_t next)
{
  return next.y < 3 || next.y >= bounds.y - 3;
}

bool wall_hit_check(vec_t next)
{
  return next.x < 0 || next.x >= bounds.x || next.y < 1 || next.y >= bounds.y;
}

// Armar las condiciones de juego
void set_game(int32_t _lives)
{

  // state = PLAYING;

  player.lives = _lives;

  player.pos = {.x = 0, .y = 15};
  player.dir = {.x = 1, .y = 0};

  fruits.push_back(place_fruit());

  fruit_selected = fruit_sprites[std::rand() % 5];

  is_game_set = true;
}

void init()
{

  play_selected = true;
  easy_selected = true;
  medium_selected = false;
  hard_selected = false;
  blip = voice(50, 50, 50, 50, 10, 2);
  enter = voice(100, 50, 100, 50, 20, 2);

  if (state == PLAYING)
  {
    fruits.clear();
    rotten_fruits.clear();
    player.score = 0;
    set_game(7);
  }
}

void update(uint32_t tick)
{

  if (state == MENU)
  {

    if (button(A))
    {
      change_state(PLAYING);
      play(enter, 2500, 50, 100);
      sleep(200);
    }
  } else if (state == PLAYING)
  {

    if (is_game_set)
    {

      // play_song(tick);

      // Colocar frutas y enemigos

      if (tick % difficulty == 0)
      {
        rotten_fruits.push_back(place_fruit());
      }

      if (tick % (difficulty * 2) == 0)
      {
        fruits.push_back(place_fruit());
      }

      if (tick % 30 == 0)
      {
        if (state != GAME_OVER)
        {
          for (auto &fruit : fruits)
          {
            fruit.move();
          }
          for (auto &fruit : rotten_fruits)
          {
            fruit.move();
          }
        }
      }

      if (tick % 10 == 0)
      {

        vec_t next = player.next();
        int sprite_size = 1;

        if (button(UP) && in_bounds_top(next))
        {
          player.dir.x = 0;
          player.dir.y = -1;
          player.move();
        }
        if (button(DOWN) && in_bounds_bottom(next))
        {
          player.dir.y = 1;
          player.dir.x = 0;
          player.move();
        }
        if (button(B))
        {
          is_game_set = false;
          change_state(MENU);
        }

        // Revisar colisiones de frutas con el jugador
        for (auto &fruit : fruits)
        {
          if (intersects(next.x, next.y, sprite_size, sprite_size, fruit.next().x, fruit.next().y, sprite_size, sprite_size))
          {
            player.lives = std::min(player.lives + 3, 7l); // Aumentar la vida en 3 puntos hasta un máximo de 7
            play(blip, 1000, 30, 100);
            fruit = Fruit(std::rand() % bounds.x, 100);
          }
          if (wall_hit_check(fruit.pos) || fruit.pos.y >= 18)
          {

            fruits.pop_front();
          }
        }
        
        
        // Revisar si un enemigo pasó la barrera inferior
        for (auto &rotten_fruit : rotten_fruits)
        {
          if (intersects(next.x, next.y, sprite_size, sprite_size, rotten_fruit.next().x, rotten_fruit.next().y, sprite_size, sprite_size))
          {
            player.lives -= 2; // Disminuir la vida en 2 puntos
            play(blip, 500, 30, 100);
            rotten_fruit = Fruit(std::rand() % bounds.x, 100);
          }

          if (wall_hit_check(rotten_fruit.pos) || rotten_fruit.pos.y >= 100)
          {
            rotten_fruits.pop_front();
            if (player.lives <= 0)
            {
              state = GAME_OVER;
            }
            else
            {
              player.lives -= 1;
              player.score += 1; // Aumentar el puntaje si la fruta malévola llega al límite
              play(blip, 1000, 30, 100);
            }
          }
        }
      }
    }
    else
    {
      init();
    }
  }
  else if (state == GAME_OVER)
  {
    if (pressed(B))
    {
      is_game_set = false;
      state = MENU;
    }
  }
}

/*
Funciones de dibujado
*/

// Etiqueta de Puntaje y Vidas

void label(std::string s)
{

  blend(ALPHA);
  pen(1, 2, 3, 8);
  frect(0, 11, SCREEN->w, 15);
  pen(0, 0, 0);
  text("Score: " + s, 3, 16);
  pen(15, 15, 15);
  text("Score: " + s, 2, 15);

  for (int i = player.lives; i > 0; i--)
  {
    text("\\spr064", SCREEN->w - 10 * i, 15);
  }
}

// Estilos de texto

auto split_text = [](std::string m, color_t c1, color_t c2, int32_t x, int32_t y)
{
  blend(MASK);
  pen(c1);
  clip(0, y, 120, 4);
  text(m, x, y);
  pen(c2);
  clip(0, y + 4, 120, 4);
  text(m, x, y);
  clip();
};

auto glow_text = [](std::string m, color_t c, int32_t x, int32_t y)
{
  blend(PEN);
  pen(c);
  text(m, x - 1, y - 1);
  text(m, x, y - 1);
  text(m, x + 1, y - 1);
  text(m, x - 1, y);
  text(m, x, y);
  text(m, x + 1, y);
  text(m, x - 1, y + 1);
  text(m, x, y + 1);
  text(m, x + 1, y + 1);
  blend(PEN);
  pen(0, 0, 0);
  text(m, x, y);
};

auto glitch_text = [](std::string m, color_t c, int32_t x, int32_t y)
{
  blend(MASK);
  pen(c);
  for (uint8_t i = 0; i < 8; i++)
  {
    clip(0, y + i, 120, 1);
    int8_t o = 0;
    if (std::rand() % 8 == 0)
    {
      o = (std::rand() % 9) - 4;
    }
    text(m, x + o, y);
  }
  clip();
};

// Constantes de dibujado
float hue = 0.0f, hue_step = 1.0f / 7.0f;
// Colores
color_t c1, c2, c3, c4;

void draw(uint32_t tick)
{

  int menu_options_pos_x = 55;
  int menu_options_pos_y = 68;
  int menu_options_pos_main_y = 60;

  hue += 0.01f;

  // Cálculo de colores
  float hue2 = hue + 0.5;
  if (hue2 > 1.0f)
    hue2 -= 1.0f;
  c1 = hsv(hue + (hue_step * 4), 0.8f, 1.0f);
  c2 = hsv(hue + (hue_step * 4) + 0.1f, 0.4f, 0.6f);
  int32_t g = 1 + ((sin(time() / 250.0f) + 1.0f) * 2.0f);
  c3 = hsv(hue + (hue_step * 5), 0.8f, 1.0f, 0.2f);
  c4 = hsv(hue + (hue_step * 6), 0.8f, 1.0f);

  if (state == MENU)
  {
    pen(0, 0, 0);
    clear();

    split_text("Car Runner \\spr150", c1, c2, 27.5, 30);
    glitch_text("Play", (255, 255, 255), menu_options_pos_x, menu_options_pos_main_y);
    glow_text("Made by Jose and Hugo",(13,25,252), 3, 90);

    if (play_selected)
    {
      pen(0, 0, 0);
      frect(menu_options_pos_x - 17, menu_options_pos_main_y + 21, 5, 5);
      pen((255, 255, 255));
      frect(menu_options_pos_x - 10, menu_options_pos_main_y + 1, 5, 5);
    }
  }else{

    // Background
    pen(20, 20, 54);
    clear();

    pen(255, 255, 255);

    pen(255,255,0);
    frect(5, 72.5, 30, 5);
    frect(45, 72.5, 30, 5);
    frect(85, 72.5, 30, 5);

    pen(0, 0, 0, 4);

    // Dibujo de etiqueta superior
    pen(15, 15, 15);
    frect(0, 0, SCREEN->w, 11);
    pen(0, 0, 0);
    text("Survive!", 2, 2);

    label(str(player.score));

    // Dibujo de límites
    rect(2, 27, 116, 90);
    rect(2, 29, 116, 90);

    // Dibujo de frutas
    for (auto fruit : fruits)
    {
      sprite(POTION_RED, transform(fruit.pos).x + 3, transform(fruit.pos).y + 3);
    }
    // Dibujo de enemigos
    for (auto fruit : rotten_fruits)
    {
      sprite(BLOCK_STACK_STONE, transform(fruit.pos).x + 3, transform(fruit.pos).y + 3);
    }

    // Dibujo de jugador (Parpadeo si hay GAME_OVER)
    bool flash = ((time() / 250) % 2) == 0;
    if (state == PLAYING || (state == GAME_OVER && flash))
    {
      vec_t p = transform(player.pos);
      sprite(PILOT_GOGGLES, p.x, p.y);
    }

    // Dibujo de titulo Game Over
    if (state == GAME_OVER)
    {
      glow_text("GAME OVER", c3, 33, menu_options_pos_y - 10);
      glow_text("Press B to Return", c3, 18, menu_options_pos_y + 10);
    }
  }
}