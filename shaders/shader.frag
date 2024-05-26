#version 330 core

// from vertex shader
in vec3 v_Normal;
in vec2 v_TextCoord;
in vec3 FragPosWorld; 

// from cpu
uniform vec3 u_CameraPos;
uniform vec4 u_Colour;
uniform sampler2D u_TextureAlbedo;
//uniform sampler2D u_TextureNormal;

// output colour for pixel
layout (location = 0) out vec4 FragColour;

void main() {
    // shading variables and settings
    vec3 lightDirection = normalize(vec3(-1.0, -1.0, -1.0)); 
    vec3 skyColor = vec3(0.53f, 0.81f, 0.92f);
    vec3 fogColour = vec3(0.53f, 0.81f, 0.92f);
    float fogDensity = 0.004; 
    float textureScale = 10;
    float ambientStrength = 0.2f;
    float normalMapStrength = 0.1f;




    // The cos rule - the more the normal faces the sun, the brighter the sun intensity
    // Calculate the cosine of the angle between the normal and light direction
    float directIntensity = max(dot(normalize(v_Normal), lightDirection), 0.0);




    // Triplanar mapping - blending between a texture from 3 sides based on the fragment normal
    vec3 blendWeights = abs(v_Normal);
    blendWeights = blendWeights / (blendWeights.x + blendWeights.y + blendWeights.z); // Normalize weights
    vec4 texColorX = texture(u_TextureAlbedo, FragPosWorld.yz / textureScale);
    vec4 texColorY = texture(u_TextureAlbedo, FragPosWorld.xz / textureScale);
    vec4 texColorZ = texture(u_TextureAlbedo, FragPosWorld.xy / textureScale); // correct
    vec3 textureColor = texColorX.rgb * blendWeights.x + texColorY.rgb * blendWeights.y + texColorZ.rgb * blendWeights.z;




    // apply normal map
    //vec3 normalMapRGB = texture(u_NormalMapTexture, FragPosWorld).rgb;
    //vec3 normalMapNormal = normalize(normalMapRGB * 2.0 - 1.0);

    // blend v_normal and normalMap



    
    // Colour of the mesh material (the actual model's material colour)
    vec3 shadedColor = textureColor * directIntensity + (textureColor + skyColor) * ambientStrength;




    // Distance fog - the further away the fragment is, the more fog
    float distance = length(FragPosWorld - u_CameraPos); 
    float fogFactor = 1.0 - exp(-fogDensity * distance);






    // Output the shaded color with fog
    FragColour = vec4(mix(shadedColor, fogColour, fogFactor), 1.0);
}


