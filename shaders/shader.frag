#version 330 core

// FROM VERTEX SHADER
in vec3 v_Normal;
in vec2 v_TextCoord;
in vec3 FragPosWorld; 

// FROM CPU
uniform vec3 u_CameraPos;
uniform vec4 u_Colour;
uniform sampler2D u_albedo_texture;
uniform sampler2D u_normal_texture;
uniform sampler2D u_height_texture;
uniform sampler2D u_roughness_texture;

// PIXEL OUTPUT COLOUR
layout (location = 0) out vec4 FragColour;

void main() {
    vec3 lightDirection = normalize(vec3(-1.0, -1.0, -1.0)); 
    vec3 skyColor = vec3(0.53, 0.81, 0.92);
    vec3 fogColour = vec3(0.53, 0.81, 0.92);
    float fogDensity = 0.007; 
    float textureScale = 8.0;
    float ambientStrength = 0.2;
    float normalMapStrength = 0.6;

    // FINAL TEXTURE COLOUR
    vec4 texColorX = texture(u_albedo_texture, FragPosWorld.yz / textureScale);
    vec4 texColorY = texture(u_albedo_texture, FragPosWorld.xz / textureScale);
    vec4 texColorZ = texture(u_albedo_texture, FragPosWorld.xy / textureScale);

    // TRIPLANAR MAPPING - BLENDING BETWEEN A TEXTURE FROM 3 SIDES BASED ON THE FRAGMENT NORMAL
    vec3 blendWeights = abs(v_Normal);
    blendWeights = blendWeights / (blendWeights.x + blendWeights.y + blendWeights.z); // Normalize weights
    vec3 textureColor = texColorX.rgb * blendWeights.x + texColorY.rgb * blendWeights.y + texColorZ.rgb * blendWeights.z;

    // APPLY NORMAL MAP
    vec2 uvX = FragPosWorld.yz / textureScale; // x facing plane
    vec2 uvY = FragPosWorld.xz / textureScale; // y facing plane
    vec2 uvZ = FragPosWorld.xy / textureScale; // z facing plane

    vec3 normalMapX = texture(u_normal_texture, uvX).rgb * 2.0 - 1.0;
    vec3 normalMapY = texture(u_normal_texture, uvY).rgb * 2.0 - 1.0;
    vec3 normalMapZ = texture(u_normal_texture, uvZ).rgb * 2.0 - 1.0;

    // Get the sign (-1 or 1) of the surface normal
    vec3 axisSign = sign(v_Normal);

    // Flip tangent normal z to account for surface normal facing
    normalMapX.z *= axisSign.x;
    normalMapY.z *= axisSign.y;
    normalMapZ.z *= axisSign.z;

    // Swizzle tangent normals to match world orientation and triblend
    vec3 blendedNormal = normalize(
        normalMapX.zyx * blendWeights.x +
        normalMapY.xzy * blendWeights.y +
        normalMapZ.xyz * blendWeights.z
    );

    // Interpolate between the original normal and the blended normal map
    vec3 SurfaceNormal = normalize(mix(v_Normal, blendedNormal, normalMapStrength));

    // The cos rule - the more the normal faces the sun, the brighter the sun intensity
    float directIntensity = max(dot(SurfaceNormal, lightDirection), 0.0);

    // Colour of the mesh material (the actual model's material colour)
    vec3 shadedColor = textureColor * directIntensity + (textureColor + skyColor) * ambientStrength;

    // Distance fog - the further away the fragment is, the more fog
    float distance = length(FragPosWorld - u_CameraPos); 
    float fogFactor = 1.0 - exp(-fogDensity * distance);

    // Output the shaded color with fog
    FragColour = vec4(mix(shadedColor, fogColour, fogFactor), 1.0);
}
