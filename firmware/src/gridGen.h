#pragma once
#include "DoubleArray.h"

static void gridGen(DoubleArray<uint8_t>& matrix, DoubleArray<uint8_t>& segs, uint8_t side) {
    uint8_t diam = side * 2 - 1;
    uint16_t amount = 1;
    for (uint8_t i = 0; i < side; i++) amount += 6 * i;
    // matrix
    {
        uint8_t xx = diam - side;
        uint8_t curX = side;
        int8_t dir = 1;
        uint16_t i = 0;

        matrix.init(diam);
        for (uint8_t i = 0; i < diam; i++) matrix.initRow(i, diam * 2);

        for (uint8_t y = 0; y < diam; y++) {
            for (uint8_t x = 0; x < curX; x++) {
                matrix[y][xx] = ++i;
                xx += 2 * dir;
            }
            bool d = y < floor(diam / 2);
            if (!d) dir = -dir;
            xx += dir;
            if (d) dir = -dir;
            curX += d ? 1 : -1;
            xx += 2 * dir;
        }
    }
    // segs
    {
        const int8_t incr[][2] = {
            {0, 2},
            {1, 1},
            {1, -1},
            {0, -2},
            {-1, -1},
            {-1, 1},
        };

        segs.init(side);

        uint8_t xx = diam - 1, yy = side - 1;
        for (uint8_t i = 0; i < side; i++) {
            uint16_t len = i ? (i * 6) : 1;
            segs.initRow(i, len);
            uint8_t seg[len + 1];
            seg[0] = matrix[yy][xx] - 1;

            uint16_t ii = 0;
            for (uint8_t n = 0; n < 6; n++) {
                uint8_t k = i;
                while (k--) seg[ii++] = matrix[yy += incr[n][0]][xx += incr[n][1]] - 1;
            }

            if (i) {
                memcpy(segs[i], seg + len - i, i);
                memcpy(segs[i] + i, seg, len - i);
            } else {
                segs[i][0] = seg[0];
            }

            xx--;
            yy--;
        }
    }
}