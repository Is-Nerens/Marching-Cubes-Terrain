import math

global maxProbeDist
maxProbeDistance = 0


vertexMap = [-1] * 4410
vertexMapKeys = [0.0] * 132300

def Hash(x, y, z):
    # Convert each float to an integer representation
    x_int = int(x * 73856093)
    y_int = int(y * 19349663)
    z_int = int(z * 8349271)
    combined_hash = x_int ^ y_int ^ z_int
    index = combined_hash % 4410
    return index

def Get(x, y, z):
    keyIndex = Hash(x, y, z)
    arraySize = 4410

    global maxProbeDistance

    for i in range(maxProbeDistance+1):
        index = (keyIndex + i) % arraySize
        print(f"[Get] probing: {index}")
        if vertexMapKeys[index * 3] == x and vertexMapKeys[index * 3 + 1] == y and vertexMapKeys[index * 3 + 2] == z:
            return vertexMap[index]
        
    # key not found
    return -2

def Set(x, y, z, value):
    keyIndex = Hash(x, y, z)
    arraySize = 4410

    global maxProbeDistance

    for i in range(maxProbeDistance+1):
        index = (keyIndex + i) % arraySize
        if vertexMapKeys[index * 3] == x and vertexMapKeys[index * 3 + 1] == y and vertexMapKeys[index * 3 + 2] == z:
            vertexMap[index] = value
            print("[Set] key found in location")
            return

    stepCount = 0
    while (stepCount < arraySize):
        index = (keyIndex + stepCount) % arraySize
        if vertexMap[index] == -1:
            vertexMapKeys[index * 3]     = x
            vertexMapKeys[index * 3 + 1] = y
            vertexMapKeys[index * 3 + 2] = z
            vertexMap[index] = value
            print("[Set] found new empty location")
            return

        print(f"[Set] probing for empty location {vertexMap[index]}")
        
        stepCount += 1
        if maxProbeDistance < stepCount: maxProbeDistance = stepCount;

    

Set(0, 0, 0, 100)
Set(1, 0, 0, 123)
Set(0, 2, 0, 14)
Set(0, 0, 0, 10430)
Set(0, 0, 10, 1)
Set(0, 2, 0, 50)
#print(Get(12, 1, 1))


