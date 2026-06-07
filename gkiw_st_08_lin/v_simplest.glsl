#version 330

//Zmienne jednorodne
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

//Atrybuty
in vec4 vertex; //wspolrzedne wierzcholka w przestrzeni modelu
in vec4 color; //per-vertex color
in vec4 normal; //normal vector in model space
in vec2 texCoord;

//Dane wysylane do fragment shadera
out vec4 vColor;
out vec3 vNormal;
out vec3 vFragPos;
out vec2 iTexCoord0;

void main(void) {
    gl_Position = P*V*M*vertex;
    vColor = color;
    
    // Transform normal to world space
    vNormal = normalize(mat3(transpose(inverse(M))) * normal.xyz);
    
    // Fragment position in world space
    vFragPos = vec3(M * vertex);

    iTexCoord0=texCoord;
}
