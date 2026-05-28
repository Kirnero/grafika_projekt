#version 330


out vec4 pixelColor; //Zmienna wyjsciowa fragment shadera. Zapisuje sie do niej ostateczny (prawie) kolor piksela

in vec4 vColor;
uniform vec4 uColor; // per-cube color

void main(void) {
	pixelColor = uColor;
}
