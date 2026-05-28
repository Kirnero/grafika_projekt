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

const int boardSize = 8;
float blockSpacing = 1.4f;

ShaderProgram *sp;


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

class Cube{
	public:
		coordinates position;
		bool isMine = false;
		bool isFlagged = false;
		bool isRevealed = false;
		bool isEscaping = false;
		bool cursor = false;
		glm::vec4 color = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f); // default gray
		float scale = 0.4f;


	void draw_cube(glm::mat4 baseM){
		if(isRevealed) return; // don't draw if revealed
		if(isEscaping) {
			scale += isMine ? 0.002f : -0.002f; // mines grow, safe cubes shrink
			if(scale < 0.01f || scale > 1.0f) isRevealed = true; 
		}

		glm::mat4 M = baseM;
		M = glm::translate(M, glm::vec3(position.x, position.y, position.z));
		M = glm::scale(M, glm::vec3(scale, scale, scale));
		
		glUniformMatrix4fv(sp->u("M"), 1, false, glm::value_ptr(M));
		glUniform4fv(sp->u("uColor"), 1, glm::value_ptr(color));
		

		glDrawArrays(GL_TRIANGLES, 0, vertexCount);
	}

	void killCube(){
		if(isFlagged) return;
		isEscaping = true;
		if(isMine) color = RED;
		else color = GREEN;
	}

	void flagCube(){
		
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

class Board{
	public:
	float boardOffset = (boardSize - 1) * blockSpacing / 2.0f;  //Wycentruj planszę
	Cube cubes[boardSize][boardSize];
	Cube cursorCube;
	int x_position = 0;
	int z_position = 0;


	void randomizeMines(int mineCount){
		if(mineCount > boardSize * boardSize) mineCount = boardSize * boardSize - 1;
		int x, z;
		for(int i = 0; i < mineCount; i++){
			do{
				x = rand() % boardSize;
				z = rand() % boardSize;
			}
			while(cubes[x][z].isMine);

			cubes[x][z].isMine = true;
		}
	}

	void initializeCubes(){

		srand(time(0));
		
		for(int i = 0; i < boardSize; i++){
			for(int j = 0; j < boardSize; j++){
				cubes[i][j].position = {i * blockSpacing - boardOffset, 0, j * blockSpacing - boardOffset};
				cubes[i][j].isMine = false;
				cubes[i][j].isFlagged = false;
				cubes[i][j].isRevealed = false;
				cubes[i][j].isEscaping = false;
				cubes[i][j].cursor = false;
				cubes[i][j].color = GREY;
				cubes[i][j].scale = 0.4f;
			}
		}
		randomizeMines(10);

		cursorCube.position = {x_position * blockSpacing - boardOffset, 0.5f, z_position * blockSpacing - boardOffset};
		cubes[x_position][z_position].cursor = true;
		cursorCube.color = BLUE;
		cursorCube.scale = 0.2f;

		// to change
		for(int i = 0; i < boardSize; i++){
			for(int j = 0; j < boardSize; j++){
				if(cubes[i][j].isMine) cubes[i][j].color = RED;
			}
		}
	}

	void draw_cursor(int x, int z){
		cubes[x_position][z_position].cursor = false;
		x_position += x;
		z_position += z;
		cursorCube.position = {x_position * blockSpacing - boardOffset, 0.5f, z_position * blockSpacing - boardOffset};
		cubes[x_position][z_position].cursor = true;
	}

	void check_cube(bool flag){
		if(flag) {
			cubes[x_position][z_position].flagCube();
		}
		else {
			cubes[x_position][z_position].killCube();
		}
	}

	void draw_board(glm::mat4 baseM){
		for(int x = 0; x < boardSize; x++){
			for(int z = 0; z < boardSize; z++){
				cubes[x][z].draw_cube(baseM);
			}
		}
		cursorCube.draw_cube(baseM);
	}

} board;


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


	sp=new ShaderProgram("v_simplest.glsl",NULL,"f_simplest.glsl");
}


//Zwolnienie zasobów zajętych przez program
void freeOpenGLProgram(GLFWwindow* window) {
    //************Tutaj umieszczaj kod, który należy wykonać po zakończeniu pętli głównej************

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

    glEnableVertexAttribArray(sp->a("vertex"));  //Włącz przesyłanie danych do atrybutu vertex
    glVertexAttribPointer(sp->a("vertex"),4,GL_FLOAT,false,0,vertices); //Wskaż tablicę z danymi dla atrybutu vertex

	// Enable per-vertex colors (from myCubeColors)
	glEnableVertexAttribArray(sp->a("color"));
	glVertexAttribPointer(sp->a("color"),4,GL_FLOAT,false,0,colors);
    
    board.draw_board(M);

	glDisableVertexAttribArray(sp->a("vertex"));  //Wyłącz przesyłanie danych do atrybutu vertex
	glDisableVertexAttribArray(sp->a("color"));

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

	window = glfwCreateWindow(500, 500, "Minesweeper", NULL, NULL);  //Utwórz okno 500x500 o tytule "Minesweeper" i kontekst OpenGL.

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
	float angle_y=-0.2; //Aktualny kąt obrotu obiektu
	glfwSetTime(0); //Zeruj timer
	while (!glfwWindowShouldClose(window)) //Tak długo jak okno nie powinno zostać zamknięte
	{
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
