#ifndef CUBE_H
#define CUBE_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <Adafruit_SSD1306.h>

#define WIDTH 128
#define HEIGHT 64
#define TEXTURE_SIZE 32

extern Adafruit_SSD1306 display;

typedef uint8_t Texture[TEXTURE_SIZE][TEXTURE_SIZE];

void rotateCube(float cube[8][3], float angleX, float angleY, float angleZ) {
    float sx = sinf(angleX), cx = cosf(angleX);
    float sy = sinf(angleY), cy = cosf(angleY);
    float sz = sinf(angleZ), cz = cosf(angleZ);

    for (int i = 0; i < 8; i++) {
        float x = cube[i][0], y = cube[i][1], z = cube[i][2];

        // Rotate around X-axis
        float newY = y * cx - z * sx;
        float newZ = y * sx + z * cx;
        y = newY;
        z = newZ;

        // Rotate around Y-axis
        float newX = x * cy + z * sy;
        newZ = -x * sy + z * cy;
        x = newX;
        z = newZ;

        // Rotate around Z-axis
        newX = x * cz - y * sz;
        newY = x * sz + y * cz;

        cube[i][0] = newX;
        cube[i][1] = newY;
        cube[i][2] = newZ;
    }
}

void drawTexturedFace(float face[4][3], Texture texture, float viewerDistance) {
    for (int tx = 0; tx < TEXTURE_SIZE; tx++) {
        for (int ty = 0; ty < TEXTURE_SIZE; ty++) {
            if (texture[tx][ty] == 0) continue;

            // Compute barycentric coordinates
            float u = (float)tx / (TEXTURE_SIZE - 1);
            float v = (float)ty / (TEXTURE_SIZE - 1);

            float x = (1 - u) * (1 - v) * face[0][0] + u * (1 - v) * face[1][0] + u * v * face[2][0] + (1 - u) * v * face[3][0];
            float y = (1 - u) * (1 - v) * face[0][1] + u * (1 - v) * face[1][1] + u * v * face[2][1] + (1 - u) * v * face[3][1];
            float z = (1 - u) * (1 - v) * face[0][2] + u * (1 - v) * face[1][2] + u * v * face[2][2] + (1 - u) * v * face[3][2];

            // Apply perspective projection
            int screenX = (int)(WIDTH / 2 + x * viewerDistance / (viewerDistance + z));
            int screenY = (int)(HEIGHT / 2 - y * viewerDistance / (viewerDistance + z));

            // Draw pixel if within bounds
            if (screenX >= 0 && screenX < WIDTH && screenY >= 0 && screenY < HEIGHT) {
                display.drawPixel(screenX, screenY, 1);
            } else {
                display.drawPixel(screenX, screenY, 0);
            }
        }
    }
}

void drawCubeWithTextures(float cube[8][3], Texture textures[6], float viewerDistance) {
    int faces[6][4] = {
        {0, 1, 3, 2}, // Front face
        {4, 5, 7, 6}, // Back face
        {0, 1, 5, 4}, // Bottom face
        {2, 3, 7, 6}, // Top face
        {0, 2, 6, 4}, // Left face
        {1, 3, 7, 5}  // Right face
    };

    for (int i = 0; i < 6; i++) {
        float face[4][3];
        for (int j = 0; j < 4; j++) {
            int vertex = faces[i][j];
            face[j][0] = cube[vertex][0];
            face[j][1] = cube[vertex][1];
            face[j][2] = cube[vertex][2];
        }

        drawTexturedFace(face, textures[i], viewerDistance);
    }
}

#endif
