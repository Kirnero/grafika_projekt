/*
Niniejszy program jest wolnym oprogramowaniem; możesz go
rozprowadzać dalej i / lub modyfikować na warunkach Powszechnej
Licencji Publicznej GNU, wydanej przez Fundację Wolnego
Oprogramowania - według wersji 2 tej Licencji lub(według twojego
wyboru) którejś z późniejszych wersji.

Niniejszy program rozpowszechniany jest z nadzieją, iż będzie on
użyteczny - jednak BEZ JAKIEJKOLWIEK GWARANCJI, nawet domyślnej
gwarancji PRZYDATNOŚCI HANDLOWEJ albo PRZYDATNOŚCI DO OKREŚLONYCH
ZASTOSOWAŃ.W celu uzyskania bliższych informacji sięgnij do
Powszechnej Licencji Publicznej GNU.

Z pewnością wraz z niniejszym programem otrzymałeś też egzemplarz
Powszechnej Licencji Publicznej GNU(GNU General Public License);
jeśli nie - napisz do Free Software Foundation, Inc., 59 Temple
Place, Fifth Floor, Boston, MA  02110 - 1301  USA
*/

#define GLM_FORCE_RADIANS
#define GLM_FORCE_SWIZZLE



#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include "constants.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include "myCube.h"
#include "myTeapot.h"

float speed_x=0;
float speed_y=0;
float aspectRatio=1;
int camera_distance = 15;
bool scoreTitle = false;

GLuint tex0;

ShaderProgram *sp;

GLuint readTexture(const char* filename) {
 GLuint tex;
 glActiveTexture(GL_TEXTURE0);
 //Wczytanie do pamięci komputera
 std::vector<unsigned char> image; //Alokuj wektor do wczytania obrazka
 unsigned width, height; //Zmienne do których wczytamy wymiary obrazka
 //Wczytaj obrazek
 unsigned error = lodepng::decode(image, width, height, filename);
 if (error) {
     std::cerr << "Failed to load texture '" << filename << "': " << lodepng_error_text(error) << "\n";
     return 0;
 }
 //Import do pamięci karty graficznej
 glGenTextures(1,&tex); //Zainicjuj jeden uchwyt
 glBindTexture(GL_TEXTURE_2D, tex); //Uaktywnij uchwyt
 //Wczytaj obrazek do pamięci KG skojarzonej z uchwytem
 glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
 GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*) image.data());
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
 return tex;
}

//Odkomentuj, żeby rysować kostkę
float* vertices = myCubeVertices;
float* normals = myCubeNormals;
float* texCoords = myCubeTexCoords;
float* colors = myCubeColors;
int vertexCount = myCubeVertexCount;



//Odkomentuj, żeby rysować czajnik
//float* vertices = myTeapotVertices;
//float* normals = myTeapotNormals;
//float* texCoords = myTeapotTexCoords;
//float* colors = myTeapotColors;
//int vertexCount = myTeapotVertexCount;

class coordinates{
	public:
	float x;
	float y;
	float z;
};

// Helper function to draw a single small cube (for number rendering)
void draw_small_cube(glm::vec3 position, float scale, glm::vec4 color, glm::mat4 baseM) {
	glm::mat4 M = baseM;
	M = glm::translate(M, position);
	M = glm::scale(M, glm::vec3(scale, scale, scale));
	
	glUniformMatrix4fv(sp->u("M"), 1, false, glm::value_ptr(M));
	glUniform4fv(sp->u("uColor"), 1, glm::value_ptr(color));
	glUniform1i(sp->u("uUseTexture"), 0);
	
	glDrawArrays(GL_TRIANGLES, 0, vertexCount);
}


class baseCube{
	public:
	coordinates position;
	glm::vec4 color;
	float scale;

	void draw_cube(glm::mat4 baseM, bool useTexture=false){
		glm::mat4 M = baseM;
		M = glm::translate(M, glm::vec3(position.x, position.y, position.z));
		M = glm::scale(M, glm::vec3(scale, scale, scale));
		
		glUniformMatrix4fv(sp->u("M"), 1, false, glm::value_ptr(M));
		glUniform4fv(sp->u("uColor"), 1, glm::value_ptr(color));
		glUniform1i(sp->u("uUseTexture"), useTexture ? 1 : 0);
		
		glDrawArrays(GL_TRIANGLES, 0, vertexCount);
	}
};

class Tile : public baseCube{
	public:
		bool isMine;
		bool isFlagged;
		bool isRevealed;
		bool isEscaping;
		bool cursor;
	
	Tile(float x = 0, float y = 0, float z = 0){
		position = {x, y, z};
		isMine = false;
		isFlagged = false;
		isRevealed = false;
		isEscaping = false;
		cursor = false;
		color = GREY;
		scale = 0.4f;
	}

	void draw_tile(glm::mat4 baseM){
		if(isRevealed && !isMine) return; // don't draw if revealed
		if(isEscaping) {
			scale += isMine ? 0.002f : -0.002f; // mines grow, safe cubes shrink
			if(scale < 0.01f || scale > 1.0f) {isRevealed = true; isEscaping = false;} 
		}
		if(isRevealed && scale > 0.4f) scale = 0.4f;

		draw_cube(baseM, isEscaping && isMine);
	}
	

	void killCube(){
		scoreTitle = true;
		if(isFlagged) return;
		isEscaping = true;
		if(isMine) color = RED;
		else color = GREEN;
	}

	void flagCube(){
		if(isRevealed || isEscaping) return;
		if(!isFlagged){
			isFlagged = true;
			std::cout << "Flag placed at (" << position.x << ", " << position.z << ")" << std::endl;
			color = YELLOW;
		}
		else{
			isFlagged = false;
			std::cout << "Flag removed at (" << position.x << ", " << position.z << ")" << std::endl;
			color = GREY;
		}
	}
};

class Number : public baseCube{
	public:
		int number;
		coordinates center;
	
	Number(float x = 0, float y = 0, float z = 0){
		center = {x, y, z};
		position = {x, y, z};
		number = 0;
		color = GREY;
		scale = scale/5;
	}

	// 0,0 is in the center
	void draw_number_cube(int x_offset, int z_offset, glm::mat4 baseM, float spacing = 10){
		float x = x_offset * blockSpacing / spacing;
		float z = z_offset * blockSpacing / spacing;
		position = {center.x + x, center.y, center.z + z};
		draw_cube(baseM);
	}

	void draw_number(glm::mat4 baseM){
		switch(number) {
			case 0: return; // No number for 0
			case 1:
				color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
				draw_number_cube(0, 2, baseM);
				draw_number_cube(0, 1, baseM);
				draw_number_cube(0, 0, baseM);
				draw_number_cube(0, -1, baseM);
				draw_number_cube(0, -2, baseM);
				break;  // Blue
			case 2:
				color = glm::vec4(0.0f, 0.5f, 0.0f, 1.0f);
				draw_number_cube(0, 0, baseM);
				draw_number_cube(0, 2, baseM);
				draw_number_cube(0, -2, baseM);
				draw_number_cube(1, 0, baseM);
				draw_number_cube(1, 2, baseM);
				draw_number_cube(1, -2, baseM);
				draw_number_cube(1, -1, baseM);
				draw_number_cube(-1, 0, baseM);
				draw_number_cube(-1, 2, baseM);
				draw_number_cube(-1, -2, baseM);
				draw_number_cube(-1, 1, baseM);
				break;  // Dark Green
			case 3:
				color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
				draw_number_cube(0, 0, baseM);
				draw_number_cube(-1, 0, baseM);
				draw_number_cube(-1, -1, baseM);
				draw_number_cube(-1, -2, baseM);
				draw_number_cube(-1, 1, baseM);
				draw_number_cube(-1, 2, baseM);
				draw_number_cube(0, 2, baseM);
				draw_number_cube(0, -2, baseM);
				draw_number_cube(1, 2, baseM);
				draw_number_cube(1, -2, baseM);
				break;  // Red
			case 4:
				color = glm::vec4(0.0f, 0.0f, 0.5f, 1.0f);
				draw_number_cube(1, 2, baseM);
				draw_number_cube(1, 1, baseM);
				draw_number_cube(1, 0, baseM);
				draw_number_cube(0, 0, baseM);
				draw_number_cube(-1, 2, baseM);
				draw_number_cube(-1, 1, baseM);
				draw_number_cube(-1, 0, baseM);
				draw_number_cube(-1, -1, baseM);
				draw_number_cube(-1, -2, baseM);
				break;  // Dark Blue
			case 5:
				color = glm::vec4(0.5f, 0.0f, 0.0f, 1.0f);
				draw_number_cube(0, 0, baseM);
				draw_number_cube(0, 2, baseM);
				draw_number_cube(0, -2, baseM);
				draw_number_cube(1, 0, baseM);
				draw_number_cube(1, 2, baseM);
				draw_number_cube(1, -2, baseM);
				draw_number_cube(1, 1, baseM);
				draw_number_cube(-1, 0, baseM);
				draw_number_cube(-1, 2, baseM);
				draw_number_cube(-1, -2, baseM);
				draw_number_cube(-1, -1, baseM);
				break;  // Dark Red
			case 6:
				color = glm::vec4(0.0f, 0.5f, 0.5f, 1.0f);
				draw_number_cube(0, 2, baseM);
				draw_number_cube(1, 2, baseM);
				draw_number_cube(1, 1, baseM);
				draw_number_cube(1, 0, baseM);
				draw_number_cube(1, -1, baseM);
				draw_number_cube(1, -2, baseM);
				draw_number_cube(0, -2, baseM);
				draw_number_cube(-1, -2, baseM);
				draw_number_cube(-1, -1, baseM);
				draw_number_cube(-1, 0, baseM);
				draw_number_cube(0, 0, baseM);
				break;  // Cyan
			case 7:
				color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
				draw_number_cube(1, 2, baseM);
				draw_number_cube(0, 2, baseM);
				draw_number_cube(-1, 2, baseM);
				draw_number_cube(-1, 1, baseM);
				draw_number_cube(-1, 0, baseM);
				draw_number_cube(-1, -1, baseM);
				draw_number_cube(-1, -2, baseM);
				break;  // Black
			case 8:
				color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
				draw_number_cube(1, 2, baseM);
				draw_number_cube(1, 1, baseM);
				draw_number_cube(1, 0, baseM);
				draw_number_cube(1, -1, baseM);
				draw_number_cube(1, -2, baseM);
				draw_number_cube(0, 2, baseM);
				draw_number_cube(0, 0, baseM);
				draw_number_cube(0, -2, baseM);
				draw_number_cube(-1, 2, baseM);
				draw_number_cube(-1, 1, baseM);
				draw_number_cube(-1, 0, baseM);
				draw_number_cube(-1, -1, baseM);
				draw_number_cube(-1, -2, baseM);
				break;  // Gray
			default: return;
		}
	}
};

class Board{
	public:
	float boardOffset;
	Tile tiles[boardSize][boardSize];
	Tile cursorCube;
	Number numbers[boardSize][boardSize];
	int x_position;
	int z_position;
	int totalRevealed;
	int score;
	int mineCount;
	bool gameOver;

	Board(int minecount = 10){
		boardOffset = (boardSize - 1) * blockSpacing / 2.0f;
		x_position = 0;
		z_position = 0;
		score = 0;
		mineCount = minecount;
		totalRevealed = 0;
		gameOver = false;
	}


	void randomizeMines(int mineCount){
		if(mineCount > boardSize * boardSize) mineCount = boardSize * boardSize - 1;
		int x, z;
		for(int i = 0; i < mineCount; i++){
			do{
				x = rand() % boardSize;
				z = rand() % boardSize;
			}
			while(tiles[x][z].isMine);

			tiles[x][z].isMine = true;
		}
	}

	int countAdjacentMines(int x, int z){
		int count = 0;
		for(int i = -1; i <= 1; i++){
			for(int j = -1; j <= 1; j++){
				if(x + i >= 0 && x + i < boardSize && z + j >= 0 && z + j < boardSize && !(i == 0 && j==0)){
					if(tiles[x + i][z + j].isMine) count++;
				}
			}
		}
		return count;
	}

	void initializeCubes(){

		srand(time(0));
		
		for(int i = 0; i < boardSize; i++){
			for(int j = 0; j < boardSize; j++){
				tiles[i][j] = Tile(i * blockSpacing - boardOffset, 0, j * blockSpacing - boardOffset);
				numbers[i][j] = Number(i * blockSpacing - boardOffset, 0, j * blockSpacing - boardOffset);
			}
		}

		randomizeMines(mineCount);
		
		// Count adjacent mines after placing mines
		for(int i = 0; i < boardSize; i++){
			for(int j = 0; j < boardSize; j++){
				numbers[i][j].number = countAdjacentMines(i, j);
			}
		}

		cursorCube.position = {x_position * blockSpacing - boardOffset, 0.5f, z_position * blockSpacing - boardOffset};
		tiles[x_position][z_position].cursor = true;
		cursorCube.color = BLUE;
		cursorCube.scale = 0.2f;

		// to change
		//for(int i = 0; i < boardSize; i++){for(int j = 0; j < boardSize; j++){if(tiles[i][j].isMine) tiles[i][j].color = RED;}}
	}

	void draw_cursor(int x, int z){
		tiles[x_position][z_position].cursor = false;
		x_position += x;
		z_position += z;
		cursorCube.position = {x_position * blockSpacing - boardOffset, 0.5f, z_position * blockSpacing - boardOffset};
		tiles[x_position][z_position].cursor = true;
	}

	void check_cube(bool flag){
		if(flag) {
			tiles[x_position][z_position].flagCube();
		}
		else {
			tiles[x_position][z_position].killCube();
			if(tiles[x_position][z_position].isMine && !tiles[x_position][z_position].isFlagged) score++;
			totalRevealed++;
		}
	}

	void draw_board(glm::mat4 baseM){
		for(int x = 0; x < boardSize; x++){
			for(int z = 0; z < boardSize; z++){
				tiles[x][z].draw_tile(baseM);
				if(tiles[x][z].isRevealed){
					numbers[x][z].draw_number(baseM);
				}
			}
		}
		cursorCube.draw_cube(baseM);
		if(totalRevealed == boardSize * boardSize - mineCount + score) gameOver = true;
	}

};

Board board = Board(15);


void keyCallback(GLFWwindow* window,int key,int scancode,int action,int mods) {
    if (action==GLFW_PRESS) {
        if (key==GLFW_KEY_A) speed_x=-PI/2;
        if (key==GLFW_KEY_D) speed_x=PI/2;
        if (key==GLFW_KEY_W) speed_y=PI/2;
        if (key==GLFW_KEY_S) speed_y=-PI/2;
		if (key==GLFW_KEY_O && camera_distance < 30) camera_distance++;
		if (key==GLFW_KEY_P && camera_distance > 1) camera_distance--;
		if (key==GLFW_KEY_RIGHT && board.x_position > 0) board.draw_cursor(-1, 0);
		if (key==GLFW_KEY_LEFT && board.x_position < boardSize - 1) board.draw_cursor(1, 0);
		if (key==GLFW_KEY_DOWN && board.z_position > 0) board.draw_cursor(0, -1);
		if (key==GLFW_KEY_UP && board.z_position < boardSize - 1) board.draw_cursor(0, 1);
        if (key==GLFW_KEY_SPACE) board.check_cube(false);
        if (key==GLFW_KEY_E) board.check_cube(true);
		if (key==GLFW_KEY_R) board.initializeCubes();
		if (key==GLFW_KEY_ESCAPE) {
			glfwSetWindowShouldClose(window, true);
		}
    }
    if (action==GLFW_RELEASE) {
        if (key==GLFW_KEY_A) speed_x=0;
        if (key==GLFW_KEY_D) speed_x=0;
        if (key==GLFW_KEY_W) speed_y=0;
        if (key==GLFW_KEY_S) speed_y=0;
    }
}


//Procedura obsługi błędów
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}




void windowResizeCallback(GLFWwindow* window,int width,int height) {
    if (height==0) return;
    aspectRatio=(float)width/(float)height;
    glViewport(0,0,width,height);
}

//Procedura inicjująca
void initOpenGLProgram(GLFWwindow* window) {
	//************Tutaj umieszczaj kod, który należy wykonać raz, na początku programu************
	glClearColor(0,0,0,1);
	glEnable(GL_DEPTH_TEST);
	glfwSetWindowSizeCallback(window,windowResizeCallback);
	glfwSetKeyCallback(window,keyCallback);
	tex0=readTexture("tester.png");
	sp=new ShaderProgram("v_simplest.glsl",NULL,"f_simplest.glsl");
}


//Zwolnienie zasobów zajętych przez program
void freeOpenGLProgram(GLFWwindow* window) {
    //************Tutaj umieszczaj kod, który należy wykonać po zakończeniu pętli głównej************
	glDeleteTextures(1,&tex0);
    delete sp;
}



//Procedura rysująca zawartość sceny
void drawScene(GLFWwindow* window,float angle_x,float angle_y) {
	//************Tutaj umieszczaj kod rysujący obraz******************l
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 V=glm::lookAt(
         glm::vec3(0, 3, -camera_distance),
         glm::vec3(0, 0, 0),
         glm::vec3(0.0f,1.0f,0.0f)); //Wylicz macierz widoku

    glm::mat4 P=glm::perspective(50.0f*PI/180.0f, aspectRatio, 0.01f, 50.0f); //Wylicz macierz rzutowania

    glm::mat4 M=glm::mat4(1.0f);
	M=glm::rotate(M,angle_y,glm::vec3(1.0f,0.0f,0.0f)); //Wylicz macierz modelu
	M=glm::rotate(M,angle_x,glm::vec3(0.0f,1.0f,0.0f)); //Wylicz macierz modelu

    sp->use();//Aktywacja programu cieniującego
    //Przeslij parametry programu cieniującego do karty graficznej
    glUniformMatrix4fv(sp->u("P"),1,false,glm::value_ptr(P));
    glUniformMatrix4fv(sp->u("V"),1,false,glm::value_ptr(V));

    // Pass light position (point light above the board)
    glm::vec3 lightPos = glm::vec3(5.0f, 8.0f, 5.0f);
    glUniform3fv(sp->u("uLightPos"), 1, glm::value_ptr(lightPos));
    
    // Pass camera position
    glm::vec3 cameraPos = glm::vec3(0, 3, -camera_distance);
    glUniform3fv(sp->u("uViewPos"), 1, glm::value_ptr(cameraPos));

    glEnableVertexAttribArray(sp->a("vertex"));  //Włącz przesyłanie danych do atrybutu vertex
    glVertexAttribPointer(sp->a("vertex"),4,GL_FLOAT,false,0,vertices); //Wskaż tablicę z danymi dla atrybutu vertex

	// Enable per-vertex colors (from myCubeColors)
	glEnableVertexAttribArray(sp->a("color"));
	glVertexAttribPointer(sp->a("color"),4,GL_FLOAT,false,0,colors);
	
	// Enable normals
	glEnableVertexAttribArray(sp->a("normal"));
	glVertexAttribPointer(sp->a("normal"),4,GL_FLOAT,false,0,normals);

	// Enable texture coordinates and bind the texture once for board rendering
	glEnableVertexAttribArray(sp->a("texCoord"));
	glVertexAttribPointer(sp->a("texCoord"),2,GL_FLOAT,false,0,texCoords);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex0);
	glUniform1i(sp->u("textureMap0"), 0);
    
    board.draw_board(M);

	glDisableVertexAttribArray(sp->a("vertex"));  //Wyłącz przesyłanie danych do atrybutu vertex
	glDisableVertexAttribArray(sp->a("color"));
	glDisableVertexAttribArray(sp->a("normal"));
	glDisableVertexAttribArray(sp->a("texCoord"));

    glfwSwapBuffers(window); //Przerzuć tylny bufor na przedni

	//std::cout << "angle_x: " << angle_x << "\tangle_y: " << angle_y << std::endl;
}



int main(void)
{
	GLFWwindow* window; //Wskaźnik na obiekt reprezentujący okno

	glfwSetErrorCallback(error_callback);//Zarejestruj procedurę obsługi błędów

	if (!glfwInit()) { //Zainicjuj bibliotekę GLFW
		fprintf(stderr, "Nie można zainicjować GLFW.\n");
		exit(EXIT_FAILURE);
	}

	std::string title = "Minesweeper - " + std::to_string(boardSize) + "x" + std::to_string(boardSize) + " with " + std::to_string(board.mineCount) + " mines";

	window = glfwCreateWindow(500, 500, title.c_str(), NULL, NULL);  //Utwórz okno 500x500 o tytule "Minesweeper" i kontekst OpenGL.

	if (!window) //Jeżeli okna nie udało się utworzyć, to zamknij program
	{
		fprintf(stderr, "Nie można utworzyć okna.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window); //Od tego momentu kontekst okna staje się aktywny i polecenia OpenGL będą dotyczyć właśnie jego.
	glfwSwapInterval(1); //Czekaj na 1 powrót plamki przed pokazaniem ukrytego bufora

	if (glewInit() != GLEW_OK) { //Zainicjuj bibliotekę GLEW
		fprintf(stderr, "Nie można zainicjować GLEW.\n");
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window); //Operacje inicjujące
	board.initializeCubes();

	//Główna pętla
	float angle_x=0; //Aktualny kąt obrotu obiektu
	float angle_y=-0.5; //Aktualny kąt obrotu obiektu
	glfwSetTime(0); //Zeruj timer
	while (!glfwWindowShouldClose(window)) //Tak długo jak okno nie powinno zostać zamknięte
	{
		if(scoreTitle){
			std::string title = "Minesweeper - Score: " + std::to_string(board.mineCount - board.score) + " / " + std::to_string(board.mineCount);
    		glfwSetWindowTitle(window, title.c_str());
		}
		float y_increment = speed_y*glfwGetTime();
        angle_x+=speed_x*glfwGetTime(); //Zwiększ/zmniejsz kąt obrotu na podstawie prędkości i czasu jaki upłynał od poprzedniej klatki
        if(angle_y + y_increment > -1 && angle_y + y_increment < 0.2) angle_y+=speed_y*glfwGetTime(); //Zwiększ/zmniejsz kąt obrotu na podstawie prędkości i czasu jaki upłynał od poprzedniej klatki
        glfwSetTime(0); //Zeruj timer
		drawScene(window,angle_x,angle_y); //Wykonaj procedurę rysującą
		glfwPollEvents(); //Wykonaj procedury callback w zalezności od zdarzeń jakie zaszły.
	}

	freeOpenGLProgram(window);

	glfwDestroyWindow(window); //Usuń kontekst OpenGL i okno
	glfwTerminate(); //Zwolnij zasoby zajęte przez GLFW
	exit(EXIT_SUCCESS);
}
