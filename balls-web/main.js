let ui;
let font;

function preload() {
    font = loadFont('arial.ttf');
}

let balls, diam, amount, matrix, segs;

function resize(val) {
    balls = Number(ui.getValue('Balls'));
    diam = balls * 2 - 1;
    amount = 1;
    for (let i = 0; i < balls; i++) amount += 6 * i;

    // ============= matrix =============
    {
        let xx = diam - balls;
        let curX = balls;
        let dir = 1, i = 0;
        matrix = Array.from(new Array(diam), () => new Array(diam * 2).fill(-1));

        for (let y = 0; y < diam; y++) {
            for (let x = 0; x < curX; x++) {
                matrix[y][xx] = i++;
                xx += 2 * dir;
            }
            let d = y < Math.floor(diam / 2);
            if (!d) dir = -dir;
            xx += dir;
            if (d) dir = -dir;
            curX += d ? 1 : -1;
            xx += 2 * dir;
        }
    }

    // ============= segs =============
    {
        segs = [];
        let xx = diam - 1, yy = balls - 1;
        for (let i = 0; i < balls; i++) {
            let seg = [matrix[yy][xx]];

            const incr = [
                [0, 2],
                [1, 1],
                [1, -1],
                [0, -2],
                [-1, -1],
                [-1, 1],
            ];

            for (let n = 0; n < 6; n++) {
                let ii = i;
                while (ii--) seg.push(matrix[yy += incr[n][0]][xx += incr[n][1]]);
            }

            if (i) {
                seg.pop();
                n = i - 1;
                while (n--) seg.unshift(seg.pop()); // spiral
            }

            xx--;
            yy--;
            segs.push(seg);
        }
    }

    console.log('side:', balls);
    console.log('diam:', diam);
    console.log('amount:', amount);
    console.log('matrix:', matrix);
    console.log('segs:', segs);

//     let res = `#pragma once
// #include <Arduino.h>

// const uint8_t matrix[][${matrix[0].length}] PROGMEM = {`;
//     for (let row of matrix) {
//         res += `\r\n\t{${row.map(v => v + 1).join(', ')}},`;
//     }
//     res += '\r\n};';

//     res += `\r\n`

//     ui.setValue('hex_config', res);
}

function setup() {
    // https://github.com/bit101/quicksettings?tab=readme-ov-file#adding-controls
    ui = QuickSettings.create(0, 0)
        .addRange("Balls", 1, 10, 6, 1, resize)
        .addBoolean("Show num", 1)
        .addDropDown("Mode", ['spiral', 'pyramid', 'sin', 'sin2', 'noise'])
        .addRange("Offset", 40, 80, 40, 1)
        .addRange("Step", 0, 200, 50, 5)
        .addRange("Noise", 0, 100, 0, 1)
        // .addTextArea('hex_config')
        .setWidth(220)
        .setDraggable(false)
        .setCollapsible(false);
    createCanvas(1000, 1000, WEBGL);
    textFont(font);
    textSize(20);
    textAlign(CENTER, CENTER);
    frameRate(30);

    resize();
}

function draw() {
    orbitControl();
    background(220);
    let mode = ui.getValue('Mode').value;
    let step = Number(ui.getValue('Step'));
    let noisev = Number(ui.getValue('Noise'));

    switch (mode) {
        case 'spiral': {
            let heights = new Array(amount).fill(0);

            let i = 1;

            for (let h1 of segs) {
                for (let h2 of h1) {
                    i += step / h1.length;
                    heights[h2] = i;
                    if (noisev) heights[h2] += noisev * noise(i / 10 + frameCount / 100) - noisev / 2;
                }
            }

            translate(0, -balls * step / 2, -50 * balls);
            drawBalls(heights);
        } break;

        case 'pyramid': {
            let heights = new Array(amount).fill(0);

            let i = 1, j = 1;

            for (let h1 of segs) {
                for (let h2 of h1) {
                    heights[h2] = j;
                    if (noisev) heights[h2] += noisev * noise(i / 10 + frameCount / 100) - noisev / 2;
                    i++;
                }
                j += step;
            }

            translate(0, -balls * step / 2*0-300, -50 * balls);
            drawBalls(heights);
        } break;

        case 'sin': {
            let heights = new Array(amount).fill(0);
            let i = 0;

            for (let h1 of segs) {
                let val = step * sin(frameCount / 30 + i / (segs.length) * 3.14);
                for (let h2 of h1) {
                    heights[h2] = val;
                }
                i++;
            }

            translate(0, 0, -50 * balls);
            drawBalls(heights);
        } break;

        case 'sin2': {
            let heights = new Array(amount).fill(0);
            let i = 0;
            let odd = 0;

            for (let h1 of segs) {
                let val = step * sin(frameCount / 30 + i / (segs.length / 2) * 3.14);
                for (let h2 of h1) {
                    heights[h2] = val;
                    if (odd) heights[h2] = step * 2 - heights[h2];
                }

                odd = !odd;
                if (odd) i++;
            }

            translate(0, 0, -50 * balls);
            drawBalls(heights);
        } break;

        case 'noise': {
            let heights = new Array(amount).fill(0);
            for (let y = 0; y < matrix.length; y++) {
                for (let x = 0; x < matrix[0].length; x++) {
                    let n = matrix[y][x];
                    if (n == -1) continue;
                    heights[n] = (step * 5) * noise(x / 30, y / 30, frameCount / 200);
                    heights[n] -= (step * 5) / 2;
                }
            }
            translate(0, 0, -50 * balls);
            drawBalls(heights);
        } break;

        // case 'spiral': { } break;
    }
}

function drawBalls(heights) {
    const ball_d = ui.getValue('Offset');
    const ballr = 20;
    const sq3 = Math.sqrt(3);

    let curX = balls;
    let dir = 1;
    let i = 0;
    let show = ui.getValue('Show num');

    if (!show) {
        ambientLight(128, 0, 0);
        directionalLight(128, 128, 128, 0, 0, -1);
    }

    translate(-diam / 2 * ball_d / 2, 0, 0);
    for (let y = 0; y < diam; y++) {
        for (let x = 0; x < curX; x++) {
            translate(ball_d * dir, 0, 0);
            translate(0, heights[i], 0);
            if (show) {
                noFill();
                stroke('#0001');
                sphere(ballr, 10, 10);
                fill('red');
                text(i, 0, 0);
            } else {
                noStroke();
                sphere(ballr, 10, 10);
            }
            translate(0, -heights[i], 0);
            i++;
        }
        let d = y < Math.floor(diam / 2);
        translate(ball_d * dir, 0, 0);
        if (!d) dir = -dir;
        translate(dir * ball_d / 2, 0, ball_d / 2 * sq3);
        if (d) dir = -dir;
        curX += d ? 1 : -1;
    }
}