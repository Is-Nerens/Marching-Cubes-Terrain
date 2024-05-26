#version 330 core

layout (location = 0) in vec4 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec2 textureCoord;

out vec3 v_Normal; // Output the normal
out vec2 v_TextCoord;
out vec3 FragPosWorld;

uniform mat4 u_MVP;
uniform mat4 u_ModelPositionMatrix;

void main() {
    gl_Position = u_MVP * vertexPosition;
    v_Normal = vertexNormal; // Assign the normal attribute to the output
    v_TextCoord = textureCoord;


    vec4 FragPosWorldVec4 = u_ModelPositionMatrix * vertexPosition;
    FragPosWorld = FragPosWorldVec4.xyz; // position * vew * modelMatrix * position
}
