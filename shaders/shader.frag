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
// uniform sampler2D u_grass_texture;
// uniform sampler2D u_height_texture;
// uniform sampler2D u_roughness_texture;

// PIXEL OUTPUT COLOUR
layout (location = 0) out vec4 FragColour;

void main() {
    vec3 lightDirection = normalize(vec3(-1.0, -1.0, -1.0)); 
    vec3 skyColor = vec3(0.53, 0.81, 0.92);
    vec3 fogColour = vec3(0.53, 0.81, 0.92);
    float fogDensity = 0.007; 
    float stoneTextureScale = 8.0;
    float grassTextureScale = 6.0;
    float ambientStrength = 0.2;
    float normalMapStrength = 0.4;



    // TRIPLANAR MAPPING - BLENDING BETWEEN A TEXTURE FROM 3 SIDES BASED ON THE FRAGMENT NORMAL
    vec2 uvX = FragPosWorld.yz / stoneTextureScale; // x facing plane
    vec2 uvY = FragPosWorld.xz / stoneTextureScale; // y facing plane
    vec2 uvZ = FragPosWorld.xy / stoneTextureScale; // z facing plane
    vec3 blendWeights = abs(v_Normal);
    blendWeights = blendWeights / (blendWeights.x + blendWeights.y + blendWeights.z); // Normalize weights



    // DETERMINE SURFACE TEXTURE COLOUR /////////////////////////////////
    // Determine steepness based on the dot product with the up vector
    float steepness = dot(normalize(v_Normal), vec3(0.0, 1.0, 0.0));
    float steepnessThreshold = 0.1; // Adjust this value as needed
    float blendFactor = smoothstep(steepnessThreshold - 0.2, steepnessThreshold, steepness);
    if (FragPosWorld.y < 70) { blendFactor = 1; }

    vec4 albedoColorX = texture(u_albedo_texture, uvX);
    vec4 albedoColorY = texture(u_albedo_texture, uvY);
    vec4 albedoColorZ = texture(u_albedo_texture, uvZ);

    // vec4 grassColorX = texture(u_normal_texture, uvX);
    // vec4 grassColorY = texture(u_normal_texture, uvY);
    // vec4 grassColorZ = texture(u_normal_texture, uvZ);
    vec4 grassColorX = vec4(0.4, 0.7, 0.2, 1.0);
    vec4 grassColorY = vec4(0.4, 0.7, 0.2, 1.0);
    vec4 grassColorZ = vec4(0.4, 0.7, 0.2, 1.0);

    // FINAL TEXTURE COLOUR
    vec4 texColorX = mix(grassColorX, albedoColorX, blendFactor);
    vec4 texColorY = mix(grassColorY, albedoColorY, blendFactor);
    vec4 texColorZ = mix(grassColorZ, albedoColorZ, blendFactor);
    vec3 textureColor = texColorX.rgb * blendWeights.x + texColorY.rgb * blendWeights.y + texColorZ.rgb * blendWeights.z;






    // APPLY NORMAL MAP
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
