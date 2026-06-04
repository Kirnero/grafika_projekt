#version 330

out vec4 pixelColor;

in vec4 vColor;
in vec3 vNormal;
in vec3 vFragPos;

uniform vec4 uColor; // per-cube color
uniform vec3 uLightPos; // light position in world space
uniform vec3 uViewPos; // camera position in world space

void main(void) {
    vec3 baseColor = uColor.rgb;

    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * baseColor;
    
    // Diffuse
    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(uLightPos - vFragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * baseColor;
    
    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(uViewPos - vFragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);
    
    vec3 result = ambient + diffuse + specular;
    pixelColor = vec4(result, 1.0);
}
