# Project structure
This file will present a rough structure of functions and classes created during writing of this project

## constants
All needed constants, such as board size or RGBA color vectors, are defined in constants.h file

## class coordinates
A helper class that holds an object's coordinates
```c++
// Attributes
float x;
float y;
float z;
```

## class baseCube
A helper class, serves as a template and base version of other objects
```c++
// Attributes

// Real coordinates, not logic
coordinates position;
// Base color of a cube, RGBA vector
glm::vec4 color;
// Size of the cube -> 2 = 2 times bigger
float scale;
```

```c++
// Methods

// Basic way to draw a cube, with position, color and scale given in attributes, along with option to use a default texture, defined in initOpenGLProgram
void draw_cube(glm::mat4 baseM, bool useTexture=false)
```

## class Tile -> public baseCube

Extension of baseCube class

The class that handles drawing of a tile in a board and individual tile logic when given appropriete signals through killCube() and flagCube()

```c++
// Attributes

// Logic attributes to help determine the tile's current situation
bool isMine;
bool isFlagged;
bool isRevealed;
// Tells whether the tile death animation is currently playing
bool isEscaping;
// Is cursor above the tile?
bool cursor;
```

```c++
// Methods

// Constructor, sets default values, accepts coordinates as arguments, while defaulting to 0
Tile(float x = 0, float y = 0, float z = 0)

// Handles the cube drawing process, along with playing the death animation if needed, doesn't draw revealed tiles unless they are mines
void draw_tile(glm::mat4 baseM)

// Changes some attributes so that the death animation starts playing and logic proceeds
void killCube()

// Flags the cube so that it can't be interacted in any other way than unflagging
void flagCube()
```


## class Number -> public baseCube

Extension of baseCube class

A helper class made to draw 3D numbers on the board.\
Every number is drawn as cubes in a 5x5 grid, with (0,0) being the center
```c++
// Attributes

// Value of the number to draw
int number;
// Coordinates of the centerpiece of the number
coordinates center;
```

```c++
// Methods

// Constructor, sets default values, accepts coordinates of the whole number as arguments, while defaulting to 0
Number(float x = 0, float y = 0, float z = 0)

// Extension of draw_cube(), additionaly calculates the coordinate offset based on logical offset
void draw_number_cube(int x_offset, int z_offset, glm::mat4 baseM, float spacing = 10)

// A collection of ways to draw numbers 1-8, decides what to draw through number attribute
void draw_number(glm::mat4 baseM)
```

## class Board

The most important class that uses all the previous ones.\
Serves as the game logic brain and communication tool with main program loop.

```c++
// Attributes

// Used to center the board, calculated in constructor
float boardOffset;

// All needed custom classes
Tile tiles[boardSize][boardSize];
Tile cursorCube;
Number numbers[boardSize][boardSize];

// Position of the cursor
int x_position;
int z_position;

// Score related counters
int totalRevealed;
int score;

// How many mines should be generated
int mineCount;

// Has the game ended
bool gameOver;
```

```c++
// Methods

// Constructor, sets default values, accepts mine count as argument, while defaulting to 10
Board(int minecount = 10)

// Helper function used to determine the logical position of mines on the board
void randomizeMines(int mineCount)

// Helper function used to calculate the number of adjacent mines around the given tile
int countAdjacentMines(int x, int z)

// Initialize or reset current board along with cursor, possible to do only once per second, uses randomizeMines() and countAdjacentMines
void initializeCubes()

// Updates the cursor position and some logic attributes based on offsets x and z
void update_cursor(int x, int z)

// A proxy function to flag or kill the cube chosen with cursor. If flag is true then it attempts to use flagCube() function, otherwise uses killCube() and calculates the score
void check_cube(bool flag)

// Helper function to reset the current board, uses function initializeCubes() along with directly changing some attributes
void reset_board()

// The main drawing loop that uses every previous draw_ function, also determines when the end of the game happens
void draw_board(glm::mat4 baseM)
```

## Other functions

```c++
// Handles keyboard input, see README.md for controls
void keyCallback(GLFWwindow* window,int key,int scancode,int action,int mods)

// Handles textures
GLuint readTexture(const char* filename)

// OpenGL functions
void windowResizeCallback(GLFWwindow* window,int width,int height)
void initOpenGLProgram(GLFWwindow* window)
void freeOpenGLProgram(GLFWwindow* window)

// Initializes the most important and basic OpenGL attributes and shaders
void drawScene(GLFWwindow* window,float angle_x,float angle_y)

// Main loop, camera angle calculation and window title management
int main(void)
```