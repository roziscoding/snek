#include <stdio.h>
#include <chrono>
#include <iostream>
#include <format>
#include <unistd.h>
#include <curses.h>
#include <locale.h>
#include <stdlib.h>
#include <time.h>
#include <vector>

using namespace std;

const string CLEAR = "\033[H\033[J";
const int HEADER_SIZE = 3;
int const w(WEXITSTATUS(std::system("exit `tput cols`")));
int const h(WEXITSTATUS(std::system("exit `tput lines`")) - HEADER_SIZE);

struct Position {
  int x;
  int y;
};

class Snake {
  int x;
  int y;
  int xSpeed;
  int ySpeed;
  vector<Position> segments;

public:
  Snake()
    {
      x = 0;
      y = 0;
      xSpeed = 1;
      ySpeed = 0;
      segments.push_back({x, y});
    };

    void setXSpeed (int newSpeed) {
      if (xSpeed != 0) return;
      xSpeed = newSpeed;
      ySpeed = 0;
    };

    void setYSpeed (int newSpeed) {
      if (ySpeed != 0) return;
      xSpeed = 0;
      ySpeed = newSpeed;
    };

    void tick () {
      x += xSpeed;
      y += ySpeed;

      if (x < 0) {
        x = w - 1;
      } else if (x >= w) {
        x = 0;
      }

      if (y < 0) {
        y = h - 1;
      } else if (y >= h) {
        y = 0;
      }

      if (segments.size() == 1) {
        segments[0] = Position{x, y};
        return;
      }

      segments.emplace(segments.begin(), Position{x, y});
      segments.pop_back();
    };

    int getX () {
      return x;
    };

    int getY () {
      return y;
    };

    void draw () {
      int i = 0;
      attron(COLOR_PAIR(1));

      for (Position segment : segments) {
        move(segment.y + HEADER_SIZE, segment.x);
        printw(i == 0 ? "\u25A0" : "\u25A1");
        i++;
      }

      attroff(COLOR_PAIR(1));
    }

    void grow() {
      segments.push_back(Position{x, y});
    }

    bool hasCollided () {
      int i = 0;
      for (Position segment : segments) {
        if (i == 0) {
          i++;
          continue;
        }

        if (segment.x == x && segment.y == y) {
          return true;
        }
      }

      return false;
    }
};

void drawFood (int x, int y) {
  move(y + HEADER_SIZE, x);
  attron(COLOR_PAIR(2));
  printw("\u25A0");
  attroff(COLOR_PAIR(2));
}

void drawSnakeCoords (Snake snek) {
  move(0, 0);
  attron(COLOR_PAIR(1));
  printw("x: %d y: %d", snek.getX(), snek.getY());
  attroff(COLOR_PAIR(1));
  refresh();
}

void drawFoodCoords (int x, int y) {
  move(1, 0);
  attron(COLOR_PAIR(2));
  printw("x: %d y: %d", x, y);
  attroff(COLOR_PAIR(2));
  refresh();
}

void drawScore (int score) {
  move(2, 0);
  attron(COLOR_PAIR(3));
  printw("Score: %d", score);
  attroff(COLOR_PAIR(3));
  refresh();
}

void drawGameOver (int score) {
  string gameOverStr = "GAME OVER";
  string scoreStr = "Score: " + to_string(score);
  string pressSpaceStr = "Press space to play again";
  
  move(h / 2, (w / 2) - (gameOverStr.length() / 2));
  attron(COLOR_PAIR(2));
  attron(A_BOLD);
  addstr(gameOverStr.c_str());
  attroff(A_BOLD);
  attroff(COLOR_PAIR(2));
  attron(COLOR_PAIR(3));
  move((h / 2) + 1, (w / 2) - (scoreStr.length() / 2));
  addstr(scoreStr.c_str());
  attroff(COLOR_PAIR(3));
  attron(COLOR_PAIR(1));
  move((h / 2) + 2, (w / 2) - (pressSpaceStr.length() / 2));
  addstr(pressSpaceStr.c_str());
  attroff(COLOR_PAIR(1));
  refresh();
}

void drawPaused (bool isReset = false) {
  string pausedStr = isReset ? "GAME RESET" : "GAME PAUSED";
  string action = isReset ? "play" : "resume";
  string pressSpaceStr = "Press space to " + action + " or q to quit";

  attron(COLOR_PAIR(3));
  move(h / 2, (w / 2) - (pausedStr.length() / 2));
  attron(A_BOLD);
  addstr(pausedStr.c_str());
  attroff(A_BOLD);
  attroff(COLOR_PAIR(3));

  attron(COLOR_PAIR(1));
  move((h / 2) + 1, (w / 2) - (pressSpaceStr.length() / 2));
  addstr(pressSpaceStr.c_str());
  attroff(COLOR_PAIR(1));
  refresh();
}

template <
    class result_t   = std::chrono::milliseconds,
    class clock_t    = std::chrono::steady_clock,
    class duration_t = std::chrono::milliseconds
>
auto since(std::chrono::time_point<clock_t, duration_t> const& start)
{
    return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}

Snake snek = Snake();
struct {
  int x;
  int y;
} foodPos = {w, h};

bool running = true;
bool paused = false;
bool gameOver = false;
int score = 0;

void reset () {
  snek = Snake();
  foodPos.x = rand() % w;
  foodPos.y = rand() % h;
  score = 0;
  gameOver = false;
  clear();
}

int main() {
  setlocale(LC_ALL, "");
  srand(time(NULL));

  WINDOW *win;

  win = initscr();
  nodelay(win, true);
  keypad(win, true);
  noecho();

  start_color();
  init_pair(1, COLOR_GREEN, COLOR_BLACK);
  init_pair(2, COLOR_RED, COLOR_BLACK);
  init_pair(3, COLOR_CYAN, COLOR_BLACK);

  snek.draw();

  auto lastTickTime = std::chrono::steady_clock::now();

  reset();

  do {
    switch(getch()) {
      case KEY_UP:
      case 'w':
        snek.setYSpeed(-1);
        break;
      case KEY_DOWN:
      case 's':
        snek.setYSpeed(1);
        break;
      case KEY_LEFT:
      case 'a':
        snek.setXSpeed(-1);
        break;
      case KEY_RIGHT:
      case 'd':
        snek.setXSpeed(1);
        break;
      case 'q':
        running = false;
        break;
      case 'r':
        reset();
        drawPaused(true);
        paused = true;
        break;
      case ' ':
        if (gameOver) {
          reset();
          break;
        }
        paused = !paused;
        drawPaused();
    }

    if (gameOver || paused){
      continue;
    }

    auto timeSinceLastTick = since(lastTickTime);
    
    if (timeSinceLastTick < chrono::milliseconds(100)) {
      continue;
    }

    lastTickTime = std::chrono::steady_clock::now();

    if (snek.getX() == foodPos.x && snek.getY() == foodPos.y) {
      foodPos.x = rand() % w;
      foodPos.y = rand() % h;
      score++;
      snek.grow();
    }

    snek.tick();

    if (snek.hasCollided()) {
      gameOver = true;
      drawGameOver(score);
      continue;
    }

    clear();
    snek.draw();
    drawSnakeCoords(snek);
    drawFoodCoords(foodPos.x, foodPos.y);
    drawScore(score);
    drawFood(foodPos.x, foodPos.y);
    refresh();
  } while (running);

  endwin();

  return 0;
}