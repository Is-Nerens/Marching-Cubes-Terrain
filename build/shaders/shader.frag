#version 330 core

// FROM VERTEX SHADER
in vec3 v_Normal;
in vec2 v_TextCoord;
in vec3 FragPosWorld; 

// FROM CPU
uniform vec3 u_CameraPos;
uniform vec4 u_Colour;
uniform sampler2D u_rock_albedo_texture;
uniform sampler2D u_rock_normal_texture;
uniform sampler2D u_grass_albedo_texture;
uniform sampler2D u_grass_normal_texture;


// PIXEL OUTPUT COLOUR
layout (location = 0) out vec4 FragColour;

void main() {
    vec3 lightDirection = normalize(vec3(-1.0, -1.0, -1.0)); 
    vec3 skyColor = vec3(0.53, 0.81, 0.92);
    vec3 fogColour = vec3(0.53, 0.81, 0.92);
    float fogDensity = 0.007; 
    float stoneTextureScale = 0.125;
    float grassTextureScale = 0.16;
    float ambientStrength = 0.2;
    float normalMapStrength = 0.4;



    // TRIPLANAR MAPPING - BLENDING BETWEEN A TEXTURE FROM 3 SIDES BASED ON THE FRAGMENT NORMAL
    vec2 uvX = FragPosWorld.yz; // x facing plane
    vec2 uvY = FragPosWorld.xz; // y facing plane
    vec2 uvZ = FragPosWorld.xy; // z facing plane
    vec3 blendWeights = abs(v_Normal);
    blendWeights = blendWeights / (blendWeights.x + blendWeights.y + blendWeights.z); // Normalize weights



    // DETERMINE SURFACE TEXTURE COLOUR /////////////////////////////////
    float cosTheta = dot(normalize(v_Normal), vec3(0.0, 1.0, 0.0));
    float theta = acos(cosTheta); // Angle in radians
    float degrees = degrees(theta); // Convert to degrees

    // Define the angle threshold and blend distance
    float angleThreshold = 140; 
    float blendDistance = 25; 

    // Calculate the blend factor based on the angle
    float steepnessBlendFactor = smoothstep(angleThreshold - blendDistance, angleThreshold, degrees);

    vec4 rockColorX = texture(u_rock_albedo_texture, uvX * stoneTextureScale);
    vec4 rockColorY = texture(u_rock_albedo_texture, uvY * stoneTextureScale);
    vec4 rockColorZ = texture(u_rock_albedo_texture, uvZ * stoneTextureScale);

    vec4 grassColorX = texture(u_grass_albedo_texture, uvX * grassTextureScale);
    vec4 grassColorY = texture(u_grass_albedo_texture, uvY * grassTextureScale);
    vec4 grassColorZ = texture(u_grass_albedo_texture, uvZ * grassTextureScale);

    // FINAL TEXTURE COLOUR
    vec4 texColorX = mix(rockColorX, grassColorX, steepnessBlendFactor);
    vec4 texColorY = mix(rockColorY, grassColorY, steepnessBlendFactor);
    vec4 texColorZ = mix(rockColorZ, grassColorZ, steepnessBlendFactor);
    vec3 textureColor = texColorX.rgb * blendWeights.x + texColorY.rgb * blendWeights.y + texColorZ.rgb * blendWeights.z;






    // APPLY NORMAL MAP
    vec3 rockNormalMapX = texture(u_rock_normal_texture, uvX * stoneTextureScale).rgb * 2.0 - 1.0;
    vec3 rockNormalMapY = texture(u_rock_normal_texture, uvY * stoneTextureScale).rgb * 2.0 - 1.0;
    vec3 rockNormalMapZ = texture(u_rock_normal_texture, uvZ * stoneTextureScale).rgb * 2.0 - 1.0;

    vec3 grassNormalMapX = texture(u_grass_normal_texture, uvX * grassTextureScale).rgb * 2.0 - 1.0;
    vec3 grassNormalMapY = texture(u_grass_normal_texture, uvY * grassTextureScale).rgb * 2.0 - 1.0;
    vec3 grassNormalMapZ = texture(u_grass_normal_texture, uvZ * grassTextureScale).rgb * 2.0 - 1.0;

    vec3 normalMapX = mix(rockNormalMapX, grassNormalMapX, steepnessBlendFactor);
    vec3 normalMapY = mix(rockNormalMapY, grassNormalMapY, steepnessBlendFactor);
    vec3 normalMapZ = mix(rockNormalMapZ, grassNormalMapZ, steepnessBlendFactor);

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
