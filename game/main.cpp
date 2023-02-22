#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_Events.h>
#include <SDL2/SDL_image.h>
#include <math.h>
#include <vector>
#include <stdlib.h>
#include <windows.h>
#include <cmath>

#include <bits/stdc++.h>
using namespace std;

float curSign, prvSign = -1;
int WINDOW_WIDTH = 1280;
int WINDOW_HEIGHT = 720;

float groundY = 700;
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Surface* screenSurface = NULL;

bool prvDir = 0;
void ClearScreen()
{
COORD cursorPosition;	cursorPosition.X = 0;	cursorPosition.Y = 0;	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cursorPosition);
}

bool initEverything() {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0){
		std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return 0;
	}

    window = SDL_CreateWindow("Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (window == nullptr){
        std::cerr<< "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 0;
    }
    // SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        SDL_DestroyWindow(window);
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 0;
    }
    return 1;
}

SDL_Surface* load_and_optimize_surface(std::string path) {
    SDL_Surface* optimizedSurface = NULL;
    SDL_Surface* loadedSurface = IMG_Load(path.c_str());

    if (loadedSurface == NULL) {
        std::cerr << "Unable to load image: " << path << ", SDL_image error: " << IMG_GetError() << "\n";
    }
    else {
        optimizedSurface = SDL_ConvertSurface(loadedSurface, screenSurface->format, 0);
        if (optimizedSurface == NULL) {
            std::cerr << "Unable to optimize image " << path << ", error: " << SDL_GetError() << "\n";
        }
        SDL_FreeSurface(loadedSurface);
    }
    return optimizedSurface;
}

SDL_Surface* mediaSurface(std::string path) {
    SDL_Surface* surface = SDL_LoadBMP(path.c_str());
    if (surface == NULL) {
        std::cerr << "Unable to load image: " << path << ", error: " << SDL_GetError() << "\n";
    }
    return surface;
}

SDL_Texture* loadTexture(const char* path) {
    SDL_Texture* newTexture = NULL;
    SDL_Surface* loadedSurface = IMG_Load(path);
    if (loadedSurface == NULL) {
        std::cerr << "Unable to load image: " << path << ", error: " << SDL_GetError() << "\n";
    }
    else {
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
        newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
        if (newTexture == NULL) {
            std::cerr << "Unable to create texture from " << path << ", error: " << SDL_GetError() << "\n";
        }
        SDL_FreeSurface(loadedSurface);
    }
    return newTexture;
}

void showSurface(SDL_Surface* surface) {
    if (surface == NULL) {
        std::cerr << "Failed to show surface" << "\n";
    }
    else {
        SDL_BlitSurface(surface, NULL, screenSurface, NULL);
        SDL_UpdateWindowSurface(window);
    }
}

void closeEverything() {
//    SDL_DestroyTexture(carB);

    SDL_DestroyRenderer(renderer);
    renderer = NULL;

    SDL_DestroyWindow(window);
    window = NULL;

    IMG_Quit();
    SDL_Quit();
}

bool quit = false;
SDL_Texture* carB = NULL;
SDL_Rect carRect;

const float FRICTION = 0.1f;
const float VELOCITY_X_FACTOR = 4;
const float ROTATE_FACTOR = 2; // 3
const float deltaTicks = 0.025;

const float GRAVITY_ACCELERATION = 5.0f;

const float JUMP_VELOCITY = 7;
const float JUMP_DRAG_ACCELERATION = 100;


const float MAX_BOOST_VELOCITY = 8;
const float BOOST_ACCELERATION = 6;

struct Point {
    float x, y;
};

std::vector<Point> findPerpendicularVectors(const Point& a, const Point& b, const float V) {
    // Calculate vector u going from point a to point b
    Point u = { b.x - a.x, b.y - a.y };

    // Calculate the magnitude of u
    float mag_u = std::sqrt(u.x * u.x + u.y * u.y);

    // Calculate the unit vector of u
    Point u_unit = { u.x / mag_u, u.y / mag_u };

    // Calculate v1 and v2, which are perpendicular to u
    Point v1 = { u_unit.y, -u_unit.x };
    Point v2 = { -u_unit.y, u_unit.x };

    // Calculate the x and y components of v1 and v2 with magnitude V
    Point actual_v1 = { v1.x * V, v1.y * V };
    Point actual_v2 = { v2.x * V, v2.y * V };

    // Project actual_v1 and actual_v2 onto the x and y axes
    Point projected_v1 = { -actual_v1.x, -actual_v1.y };
    Point projected_v2 = { -actual_v2.x, -actual_v2.y };

    // Return the x and y components of projected_v1 and projected_v2
    std::vector<Point> projected = { {projected_v1.x, projected_v1.y}, {projected_v2.x, projected_v2.y} };
    return projected;
}

Point findParallelVector(const Point& a, const Point& b, const float V) {
    // Calculate vector u going from point a to point b
    Point u = { b.x - a.x, b.y - a.y };

    // Calculate the magnitude of u
    float mag_u = std::sqrt(u.x * u.x + u.y * u.y);

    // Calculate the unit vector of u
    Point u_unit = { u.x / mag_u, u.y / mag_u };

    // Calculate the x and y components of v with magnitude V
    Point actual_v = { u_unit.x * V, u_unit.y * V };

    Point projected_v = { actual_v.x, actual_v.y };
    // Return the x and y components of projected_v
    return projected_v;
}

std::vector<Point> getCoords(SDL_FRect rect, float angle_degrees) {
    float angle_radians = angle_degrees * M_PI / 180.0f;
    float cos_angle = cos(angle_radians);
    float sin_angle = sin(angle_radians);

    // Compute the center of the rectangle
    float cx = rect.x + rect.w / 2;
    float cy = rect.y + rect.h / 2;

    // Compute the coordinates of the corners of the unrotated rectangle
    float x1 = rect.x;
    float y1 = rect.y;
    float x2 = rect.x + rect.w;
    float y2 = rect.y;
    float x3 = rect.x + rect.w;
    float y3 = rect.y + rect.h;
    float x4 = rect.x;
    float y4 = rect.y + rect.h;

    // Translate the coordinates so that the center of the rectangle is at the origin
    x1 -= cx;
    y1 -= cy;
    x2 -= cx;
    y2 -= cy;
    x3 -= cx;
    y3 -= cy;
    x4 -= cx;
    y4 -= cy;

    // Rotate the coordinates of the corners
    float rotated_x1 = cos_angle * x1 - sin_angle * y1;
    float rotated_y1 = sin_angle * x1 + cos_angle * y1;
    float rotated_x2 = cos_angle * x2 - sin_angle * y2;
    float rotated_y2 = sin_angle * x2 + cos_angle * y2;
    float rotated_x3 = cos_angle * x3 - sin_angle * y3;
    float rotated_y3 = sin_angle * x3 + cos_angle * y3;
    float rotated_x4 = cos_angle * x4 - sin_angle * y4;
    float rotated_y4 = sin_angle * x4 + cos_angle * y4;

    // Translate the coordinates back to their original position
    rotated_x1 += cx;
    rotated_y1 += cy;
    rotated_x2 += cx;
    rotated_y2 += cy;
    rotated_x3 += cx;
    rotated_y3 += cy;
    rotated_x4 += cx;
    rotated_y4 += cy;

    // Pack the coordinates into a vector of Points
    std::vector<Point> coords = {
        {rotated_x1, rotated_y1},
        {rotated_x2, rotated_y2},
        {rotated_x3, rotated_y3},
        {rotated_x4, rotated_y4}
    };

    return coords;
}

float getAngle(const std::vector<Point>& points) {
    // Compute the center of the rectangle
    float cx = 0.0f;
    float cy = 0.0f;
    for (const Point& p : points) {
        cx += p.x;
        cy += p.y;
    }
    cx /= 4.0f;
    cy /= 4.0f;

    // Compute the angle of rotation
    float dx = points[1].x - points[0].x;
    float dy = points[1].y - points[0].y;
    float angle_radians = atan2(dy, dx);
    float angle_degrees = angle_radians * 180.0f / M_PI;


    if (!(angle_degrees >= 0 && angle_degrees <= 359)) {
        if (angle_degrees >= 0) {
            angle_degrees = fmod(angle_degrees, 360);
        }
        else {
            angle_degrees = 360 - fmod(-angle_degrees, 360);
        }
        angle_degrees = fmod(angle_degrees, 360);
    }

    return angle_degrees;
}


class Car {
public:
    float weight;
    float mass;
    float velocityX;
    float velocityY;
    float accelerationX;
    float accelerationY;
    float xPos;
    float yPos;
    bool onGround;
    bool jumping;
    float angle;
    bool dir;
    SDL_RendererFlip flip;
    bool clockWise;
    bool pointing;
    float width;
    float height;
    bool canJump = 1;
    float goingVelocityX = 0;
    float gravityVelocityY;
    float jumpVelocityX = 0;
    float jumpVelocityY = 0;
    float initialJumpDragAccelerationX = 0;
    float initialJumpDragAccelerationY = 0;
    float initialGravityAccelerationY = 0;
    float boostVelocityX = 0;
    float boostVelocityY = 0;
    float boostVelocity = 0;
    float initialBoostAccelerationX = 0;
    float initialBoostAccelerationY = 0;
//    vector<vector<Point>> corner{2, vector<Point>(2)};

    Car(float weight, float mass, float velocityX, float velocityY, float accelerationX, float accelerationY, float xPos, float yPos, bool onGround, bool jumping, float angle, bool dir, SDL_RendererFlip flip, bool clockWise, bool pointing, float width) :
        weight(weight), mass(mass), velocityX(velocityX), velocityY(velocityY), accelerationX(accelerationX), accelerationY(accelerationY), xPos(xPos), yPos(yPos), onGround(onGround), jumping(jumping), angle(angle), dir(dir), flip(flip), clockWise(clockWise), pointing(pointing), width(width) {}


    bool hasCollision() {
        SDL_FRect carRect = {xPos, yPos, width, height};
        vector<Point> tmp = getCoords(carRect, angle);
        float x1 = tmp[0].x, y1 = tmp[0].y;
        float x2 = tmp[1].x, y2 = tmp[1].y;
        float x3 = tmp[2].x, y3 = tmp[2].y;
        float x4 = tmp[3].x, y4 = tmp[3].y;

        float minX = min({x1, x2, x3, x4});
        float minY = min({y1, y2, y3, y4});
        float maxX = max({x1, x2, x3, x4});
        float maxY = max({y1, y2, y3, y4});

        std::cerr << maxX << ", " << "WINDOW_WIDTH = " << WINDOW_WIDTH << "\n";
        if (maxX >= WINDOW_WIDTH) {
            std::cerr << "Hit right wall!" << "maxX = " << maxX << ", " << "WINDOW_WIDTH = " << WINDOW_WIDTH << "\n";
            return 1;
        }
        if (minX <= 0) {
            std::cerr << "Hit left wall!" << "\n";
            return 1;
        }
        if (minY <= 0) {
            std::cerr << "Hit ceiling!" << "\n";
            return 1;
        }
        if (maxY >= groundY) {
            std::cerr << "Hit ground" << "\n";
            return 1;
        }
        return 0;
    }

    float binS(float sign) { // find biggest so that angle +- ans errorless
        float _angle = angle;
        float l = 0, r = ROTATE_FACTOR, ans = l, m;
        for (int iter = 1; iter <= 5; ++iter) {
            m = l + (r - l)*0.5;
            angle += sign*m;
            if (hasCollision() == 0) {
                ans = m;
                l = m;
            }
            else {
                r = m;
            }
        }
        angle = _angle;
        return ans;
    }
    void tiltDown() {
        if (clockWise == 0) {
            angle += binS(1.0);
//            angle += ROTATE_FACTOR;
//            if (!hasCollision())
//            for (int i = 1; i <= ROTATE_FACTOR; ++i) {
//                angle += 1;
//                if (hasCollision()) {
//                    angle -= 1;
//                    break;
//                }
//            }
//            angle += ROTATE_FACTOR;
        }
        else {
            angle -= binS(-1.0);
//            angle -= ROTATE_FACTOR;
//            if (!hasCollision())
//            for (int i = 1; i <= ROTATE_FACTOR; ++i) {
//                angle -= 1;
//                if (hasCollision()) {
//                    angle += 1;
//                    break;
//                }
//            }
//            angle -= ROTATE_FACTOR;
        }
        if (angle >= 360) {
            angle = angle - 360;
        }
        if (angle < 0) {
            angle = 360 + angle;
        }
        if (angle == 360) {
            angle = 0;
        }
    }

    void tiltUp() {
        if (clockWise == 0) {
            angle -= binS(-1.0);
//            angle -= ROTATE_FACTOR;
//            if (!hasCollision())
//            for (int i = 1; i <= ROTATE_FACTOR; ++i) {
//                angle -= 1;
//                if (hasCollision()) {
//                    angle += 1;
//                    break;
//                }
//            }
//            angle -= ROTATE_FACTOR;
        }
        else {
            angle += binS(1.0);
//            angle += ROTATE_FACTOR;
//            if (!hasCollision())
//            for (int i = 1; i <= ROTATE_FACTOR; ++i) {
//                angle += 1;
//                if (hasCollision()) {
//                    angle -= 1;
//                    break;
//                }
//            }
//            angle += ROTATE_FACTOR;
        }
        if (angle >= 360) {
            angle = angle - 360;
        }
        if (angle < 0) {
            angle = 360 + angle;
        }
        if (angle == 360) {
            angle = 0;
        }
    }

    void verticalFlip() {
        if (angle <= 180) {
            angle = 90 - angle + 90;
        }
        else {
            angle = 270 - angle + 270;
        }
    }

    void moveRight() {
//        accelerationX = weight*0 / mass;
//        velocityX += accelerationX * dt;
//        xPos += velocityX * dt;
//        xPos += velocityX;
//        if (velocityX < 0) {
//            velocityX = 0;
//        }
//        else {
//            velocityX = abs(VX);
//        }
        clockWise = 1;
//        xPos += VELOCITY_X_FACTOR;
//        velocityX += VELOCITY_X_FACTOR;
        goingVelocityX = VELOCITY_X_FACTOR;
    }

    void moveLeft() {
//        accelerationX = weight*0 / mass;
//        velocityX += accelerationX * dt;
//        xPos -= velocityX * dt;
//        if (velocityX > 0) {
//            velocityX = 0;
//        }
//        else {
//            velocityX = -abs(VX);
//        }
        clockWise = 0;
//        xPos -= VELOCITY_X_FACTOR;
        goingVelocityX = -VELOCITY_X_FACTOR;
//        velocityX -= VELOCITY_X_FACTOR;
    }

    void correctPosition() {
        SDL_FRect carRect = {xPos, yPos, width, height};
        vector<Point> tmp = getCoords(carRect, angle);
        float x1 = tmp[0].x, y1 = tmp[0].y;
        float x2 = tmp[1].x, y2 = tmp[1].y;
        float x3 = tmp[2].x, y3 = tmp[2].y;
        float x4 = tmp[3].x, y4 = tmp[3].y;

        float minX = min({x1, x2, x3, x4});
        float minY = min({y1, y2, y3, y4});
        float maxX = max({x1, x2, x3, x4});
        float maxY = max({y1, y2, y3, y4});

        if (maxY > groundY) {
            float delta = maxY - groundY;
            y1 -= delta;
            y2 -= delta;
            y3 -= delta;
            y4 -= delta;
            yPos -= delta;
            minX = min({x1, x2, x3, x4});
            minY = min({y1, y2, y3, y4});
            maxX = max({x1, x2, x3, x4});
            maxY = max({y1, y2, y3, y4});
        }

        if (minY < 0) {
            float delta = -minY;
            y1 += delta;
            y2 += delta;
            y3 += delta;
            y4 += delta;
            yPos += delta;
            minX = min({x1, x2, x3, x4});
            minY = min({y1, y2, y3, y4});
            maxX = max({x1, x2, x3, x4});
            maxY = max({y1, y2, y3, y4});
        }

        if (maxX > WINDOW_WIDTH) {
            float delta = maxX - WINDOW_WIDTH;
            x1 -= delta;
            x2 -= delta;
            x3 -= delta;
            x4 -= delta;
            xPos -= delta;
            minX = min({x1, x2, x3, x4});
            minY = min({y1, y2, y3, y4});
            maxX = max({x1, x2, x3, x4});
            maxY = max({y1, y2, y3, y4});
        }

        if (minX < 0) {
            float delta = -minX;
            x1 += delta;
            x2 += delta;
            x3 += delta;
            x4 += delta;
            xPos += delta;
            minX = min({x1, x2, x3, x4});
            minY = min({y1, y2, y3, y4});
            maxX = max({x1, x2, x3, x4});
            maxY = max({y1, y2, y3, y4});
        }
    }

    void applyGravity() {
        if (onGround == 0) {
            initialGravityAccelerationY = GRAVITY_ACCELERATION;
            std::cerr << "not on ground anymore, aY = gravity" << "\n";
//            ;
        }
    }


    void boost(float sign) {
        SDL_FRect carRect = {xPos, yPos, width, height};
        vector<Point> tmp = getCoords(carRect, angle);
        float x1 = tmp[0].x, y1 = tmp[0].y;
        float x2 = tmp[1].x, y2 = tmp[1].y;
        float x3 = tmp[2].x, y3 = tmp[2].y;
        float x4 = tmp[3].x, y4 = tmp[3].y;

        float minX = min({x1, x2, x3, x4});
        float minY = min({y1, y2, y3, y4});
        float maxX = max({x1, x2, x3, x4});
        float maxY = max({y1, y2, y3, y4});

        if (sign == 1) {
            if (prvSign == -1) {
                gravityVelocityY = GRAVITY_ACCELERATION;
            }
            else {
                gravityVelocityY = GRAVITY_ACCELERATION/5;
            }

        }
        if (dir != prvDir) {
            boostVelocity /= 5;
        }
//        gravityVelocityY = 0;
        boostVelocity += sign*BOOST_ACCELERATION * deltaTicks;
        boostVelocity = min(boostVelocity, MAX_BOOST_VELOCITY);
        boostVelocity = max(boostVelocity, 0.0f);

        Point projected = findParallelVector(tmp[1], tmp[0], boostVelocity);
        boostVelocityX = projected.x;
        boostVelocityY = projected.y;
    }

    void moveCar() {
        SDL_FRect carRect = {xPos, yPos, width, height};
        vector<Point> tmp = getCoords(carRect, angle);
        float x1 = tmp[0].x, y1 = tmp[0].y;
        float x2 = tmp[1].x, y2 = tmp[1].y;
        float x3 = tmp[2].x, y3 = tmp[2].y;
        float x4 = tmp[3].x, y4 = tmp[3].y;

        float minX = min({x1, x2, x3, x4});
        float minY = min({y1, y2, y3, y4});
        float maxX = max({x1, x2, x3, x4});
        float maxY = max({y1, y2, y3, y4});


//        vector<Point> projected = findPerpendicularVectors(tmp[1], tmp[0], JUMP_VELOCITY);
//        //clockwise - counterclockwise
//
//        Point projected_v1 = projected[0]; // clockwise
//        Point projected_v2 = projected[1]; // counterclockwise
//
//        float deltaVx, deltaVy;
//        if (flip == SDL_FLIP_NONE) {
//            deltaVx = projected_v1.x;
//            deltaVy = projected_v1.y;
//        }
//        else {
//            deltaVx = projected_v2.x;
//            deltaVy = projected_v2.y;
//        }
//
//        deltaVx *= -1;
//        deltaVy *= -1;




//        gravityVelocityY = GRAVITY;
        if (maxY >= groundY) {
            float delta = maxY - groundY;
            y1 -= delta;
            y2 -= delta;
            y3 -= delta;
            y4 -= delta;
            yPos -= delta;
            maxY = max({y1, y2, y3, y4});
        }
        if (maxY >= groundY && jumping == 0) {
            velocityY = 0;
            gravityVelocityY = 0;
            initialGravityAccelerationY = 0;
            onGround = 1;
        }
        else {
            if (jumping) {
                if (velocityY >= 0) {
                    jumping = 0;
                }
            }
        }

//        if (onGround) {
//            velocityY = 0;
//        }
//        if (jumping && velocityY == 0 && !onGround) {
//            jumping = 0;
//        }
//        if (jumping) {
//            if (velocityY >= 0) {
//                jumping = 0;
//            }
//        }
//        else {
//            if (maxY >= groundY) {
//                onGround = 1;
//                velocityY = 0;
////                yPos = groundY - height + 1;
//                float delta = maxY - groundY;
//                yPos -= delta;
//                gravityVelocityY = 0;
//            }
//        }

//        xPos += velocityX;
//        yPos += velocityY;
//        velocityX += accelerationX + goingVelocity;
//        velocityY += gravityVelocityY;

        if (boostVelocity != 0) {
            goingVelocityX = 0;
        }

//        boostVelocity = min(boostVelocity, MAX_BOOST_VELOCITY);
//        boostVelocity = max(boostVelocity, 0.0f);

        /// ///////////////////////////////////////////////////////
        velocityX = goingVelocityX + jumpVelocityX + boostVelocityX;
        velocityY =  gravityVelocityY + jumpVelocityY + boostVelocityY;

        accelerationX = initialJumpDragAccelerationX;
        accelerationY = initialGravityAccelerationY + initialJumpDragAccelerationY;

        xPos += (velocityX + accelerationX * deltaTicks);
        yPos += (velocityY + accelerationY * deltaTicks);
        /// ///////////////////////////////////////////////////////
//
//        if (curSign == -1) {
//            std::cerr << "stopping boost" << "\n";
//            if (abs(initialBoostAccelerationX) * deltaTicks > abs(boostVelocityX)) {
//                boostVelocityX = 0;
//                initialBoostAccelerationX = 0;
//            }
//            else {
//                boostVelocityX += initialBoostAccelerationX * deltaTicks;
//            }
//
//            if (abs(initialBoostAccelerationY) * deltaTicks > abs(boostVelocityY)) {
//                boostVelocityY = 0;
//                initialBoostAccelerationY = 0;
//            }
//            else {
//                boostVelocityY += initialBoostAccelerationY * deltaTicks;
//            }
//        }
//        else {
//            boostVelocityX += initialBoostAccelerationX * deltaTicks;
//            boostVelocityY += initialBoostAccelerationY * deltaTicks;
//        }
        std::cerr << "boostX = " << boostVelocityX << "\n";
        std::cerr << "boostY = " << boostVelocityY << "\n";
        std::cerr << "jump velocity X = " << jumpVelocityX << ", " << "dragX = " << initialJumpDragAccelerationX << "\n";
        std::cerr << "jump velocity Y = " << jumpVelocityY << ", " << "dragX = " << initialJumpDragAccelerationY << "\n";
        if (jumpVelocityY != 0) {
            std::cerr << "JUMP VELOCITYX not 0++++++++" << "\n";
        }
        if (abs(initialJumpDragAccelerationX) * deltaTicks > abs(jumpVelocityX)) {
            std::cerr << "resetting jump and drag accer X to 0 ================" << " " << initialJumpDragAccelerationX << " " << jumpVelocityX << "\n";
            std::cerr << "resetting jump and drag accer X to 0 ================" << " " << initialJumpDragAccelerationX << " " << jumpVelocityX << "\n";
            std::cerr << "resetting jump and drag accer X to 0 ================" << " " << initialJumpDragAccelerationX << " " << jumpVelocityX << "\n";
            std::cerr << "resetting jump and drag accer X to 0 ================" << " " << initialJumpDragAccelerationX << " " << jumpVelocityX << "\n";

            jumpVelocityX = 0;
            initialJumpDragAccelerationX = 0;
        }
        else {
            jumpVelocityX += initialJumpDragAccelerationX * deltaTicks;
        }

        if (abs(initialJumpDragAccelerationY) * deltaTicks > abs(jumpVelocityY)) {
            std::cerr << "resetting jump and drag accer Y to 0 ================" << " " << initialJumpDragAccelerationY << " " << jumpVelocityY << "\n";
            std::cerr << "resetting jump and drag accer Y to 0 ================" << " " << initialJumpDragAccelerationY << " " << jumpVelocityY << "\n";
            std::cerr << "resetting jump and drag accer Y to 0 ================" << " " << initialJumpDragAccelerationY << " " << jumpVelocityY << "\n";
            std::cerr << "resetting jump and drag accer Y to 0 ================" << " " << initialJumpDragAccelerationY << " " << jumpVelocityY << "\n";
            jumpVelocityY = 0;
            initialJumpDragAccelerationY = 0;
        }
        else {
            jumpVelocityY += initialJumpDragAccelerationY * deltaTicks;
        }



//        if (abs(initialGravityAccelerationY) * deltaTicks > abs(gravityVelocityY)) {
//            gravityVelocityY = 0;
//            initialGravityAccelerationY = 0;
//        }
//        else {
//            gravityVelocityY +=
//        }

        gravityVelocityY += GRAVITY_ACCELERATION * deltaTicks;

        carRect = {xPos, yPos, width, height};
        tmp = getCoords(carRect, angle);
        x1 = tmp[0].x, y1 = tmp[0].y;
        x2 = tmp[1].x, y2 = tmp[1].y;
        x3 = tmp[2].x, y3 = tmp[2].y;
        x4 = tmp[3].x, y4 = tmp[3].y;

        minX = min({x1, x2, x3, x4});
        minY = min({y1, y2, y3, y4});
        maxX = max({x1, x2, x3, x4});
        maxY = max({y1, y2, y3, y4});

        if (maxY >= groundY) {
            float delta = maxY - groundY;
            y1 -= delta;
            y2 -= delta;
            y3 -= delta;
            y4 -= delta;
            yPos -= delta;
            maxY = max({y1, y2, y3, y4});
        }
    }


    void correctAngle() {
//        if ((angle >= 0 && angle < 90) || (angle >= 270 && angle <= 359)) {
////            flip = SDL_FLIP_NONE;
////            dir = 1;
//            pointing = 0;
////            clockWise = 1;
//        }
//        else {
////            flip = SDL_FLIP_VERTICAL;
////            dir = 0;
////            clockWise = 0;
//            pointing = 1;
//        }
//        if (pointing == 1) {
//            if (dir == 1) {
//                flip = SDL_FLIP_VERTICAL;
//                clockWise = 1;
//            }
//        }
//        else {
//            if (dir == 0) {
//                flip = SDL_FLIP_NONE;
//                clockWise = 0;
//            }
//        }

        if (!(angle >= 0 && angle <= 359)) {
            if (angle >= 0) {
                angle = fmod(angle, 360);
            }
            else {
                angle = 360 - fmod(-angle, 360);
            }
            angle = fmod(angle, 360);
        }
        if ((angle >= 0 && angle <= 90) || (angle >= 270 && angle <= 359)) {
//            flip = SDL_FLIP_NONE;
//            dir = 1;
            pointing = 0;
//            clockWise = 1;
        }
        else {
//            flip = SDL_FLIP_VERTICAL;
//            dir = 0;
//            clockWise = 0;
            pointing = 1;
        }
        if (angle == 90) {
            if (dir == 0) {
                pointing = 0;
            }
            else {
                pointing = 1;
            }
        }
        if (angle == 270) {
            if (dir == 0) {
                pointing = 0;
            }
            else {
                pointing = 1;
            }
        }
        if (pointing == 1) {
            if (dir == 1) {
                flip = SDL_FLIP_VERTICAL;
                clockWise = 1;
            }
        }
        else {
            if (dir == 0) {
                flip = SDL_FLIP_NONE;
                clockWise = 0;
            }
        }
    }

    void initSize() {
        int w, h;
        SDL_QueryTexture(carB, NULL, NULL, &w, &h);
        float scale = width / float(w);
        height = float(h) * scale;
    }

    void draw(SDL_Renderer* renderer) {
        correctAngle();
        correctPosition();

        SDL_FRect carRect = {xPos, yPos, width, height};
        vector<Point> tmp = getCoords(carRect, angle);
        float x1 = tmp[0].x, y1 = tmp[0].y;
        float x2 = tmp[1].x, y2 = tmp[1].y;
        float x3 = tmp[2].x, y3 = tmp[2].y;
        float x4 = tmp[3].x, y4 = tmp[3].y;

        float minX = min({x1, x2, x3, x4});
        float minY = min({y1, y2, y3, y4});
        float maxX = max({x1, x2, x3, x4});
        float maxY = max({y1, y2, y3, y4});

        vector<Point> projected = findPerpendicularVectors(tmp[1], tmp[0], JUMP_VELOCITY);
        //clockwise - counterclockwise

        Point projected_v1 = projected[0]; // clockwise
        Point projected_v2 = projected[1]; // counterclockwise

        std::cerr << "jump velocity x = " << jumpVelocityX << "\n";
        std::cerr << "jump velocity y = " << jumpVelocityY << "\n";
        float deltaVx, deltaVy;
        if (flip == SDL_FLIP_NONE) {
            deltaVx = projected_v1.x;
            deltaVy = projected_v1.y;
        }
        else {
            deltaVx = projected_v2.x;
            deltaVy = projected_v2.y;
        }
        std::cerr << "gravity velocity y = " << gravityVelocityY << "\n";
        std::cerr << "A(" << tmp[1].x << ", " << tmp[1].y << ")" << " ";
        std::cerr << "B(" << tmp[0].x << ", " << tmp[0].y << ")" << "\n";
        std::cerr << "jump VX = " << deltaVx << "\n";
        std::cerr << "jump VY = " << deltaVy << "\n";
//        int w, h;
//        SDL_QueryTexture(carB, NULL, NULL, &w, &h);
//
//        float scale = 100.0f / w;
//        w *= scale;
//        h *= scale;

        std::cerr << "\n";
        std::cerr << "car width = " << width << ", " << "car height = " << height << "\n";
//        std::cerr << "======" << "\n";
        std::cerr << "xPos = " << xPos << ", " << "yPos = " << yPos << "\n";
//        SDL_FPoint center = {width/2, height/2};
        SDL_FPoint center = { width/2, height/2 };
        vector<Point> perpen = findPerpendicularVectors(tmp[1], tmp[0], JUMP_VELOCITY);
//        std::cerr <<

//        float cosAngle = cos(angle * M_PI / 180.0f);
//        float sinAngle = sin(angle * M_PI / 180.0f);
//        carRect.x += center.x - (center.x * cosAngle - center.y * sinAngle);
//        carRect.y += center.y - (center.y * cosAngle + center.x * sinAngle);

//        SDL_FPoint carRectCenter = { xPos + width/2, yPos + height/2 };
//        for (int i = 0; i < 2; ++i) {
//            for (int j = 0; j < 2; ++j) {
////                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
////                SDL_RenderDrawPoint(renderer, tmp[i][j].x, tmp[i][j].y);
//                std::cerr << "[" << tmp[i][j].x << ",  " << tmp[i][j].y << "]" << "\n";
//            }
//            cout << endl;
//        }
//        std::cerr << "----------------" << "\n";
//        std::cerr << angle << "\n";
        std::cerr << "correct angle = " << angle << "\n";
        bool testThis = 0;
        if (testThis) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            for (int i = 0; i < 4; ++i) {
    //            std::cerr << "[" << tmp[i].x << ",  " << tmp[i].y << "]" << "\n";
                std::cerr << "<" << i+1 << ">";
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);

                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawPoint(renderer, tmp[i].x, tmp[i].y);
                SDL_RenderCopyExF(renderer, carB, NULL, &carRect, angle, &center, SDL_FLIP_VERTICAL);
                SDL_RenderPresent(renderer);
                SDL_Delay(1000);
            }
            std::cerr << "\n";
        }

//        std::cerr << getAngle(tmp) << "\n";
//        std::cerr << "----------------" << "\n";
//        std::cerr << "\n";
//        for (int i = 0; i < 4; ++i) {
//            for (int j = 0; j < 2; ++j) {
//                std::cerr << "[" << tmp[i][j].x << ",  " << tmp[i][j].y << "]" << "\n";
//            }
//        }
//        SDL_RenderClear(renderer);
//        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
//        std::cerr << (clockWise ? "clockwise" : "notCLockwise") << " ";
//        std::cerr << (dir ? "going right" : "going left") << " ";
//        std::cerr << (pointing ? "pointing right" : "pointing left");
//        std::cerr << "\n";

//        std::cerr << xPos << " " << yPos << "\n";
        std::cerr << "******************" << "\n";
        for (int i = 0; i < 4; ++i) {
            std::cerr << "[" << tmp[i].x << ",  " << tmp[i].y << "]" << "\n";
        }
        std::cerr << "******************" << "\n";
        std::cerr << "angle = " << angle << "\n";
        std::cerr << (clockWise ? "clockwise      " : "not CLockwise") << "\n";
        std::cerr << "dir = " << dir << ", " << (dir ? "going right" : "going left  ") << "\n";
        std::cerr << "pointing = " << pointing << ", " << (pointing ? "pointing right" : "pointing left  ") << "\n";
        std::cerr << (flip == SDL_FLIP_VERTICAL ? "vertical flipped" : "                      ") << "\n";
        std::cerr << "\n";
        SDL_RenderCopyExF(renderer, carB, NULL, &carRect, angle, &center, flip);
    }

    void jump() {
//        if (onGround) {
//            std::cerr << "jumping now, was on ground" << "\n";
//            velocityY = -9;
//            onGround = false;
//            jumping = true;
//        }
        if (canJump) {
//            Uint32 startJumpTime = SDL_GetTicks();

            correctPosition();
            std::cerr << "jumping!" << "\n";
//            velocityY -= 9;
            onGround = false;
            jumping = true;

            SDL_FRect carRect = {xPos, yPos, width, height};
            vector<Point> tmp = getCoords(carRect, angle);
            float x1 = tmp[0].x, y1 = tmp[0].y;
            float x2 = tmp[1].x, y2 = tmp[1].y;
            float x3 = tmp[2].x, y3 = tmp[2].y;
            float x4 = tmp[3].x, y4 = tmp[3].y;

            float minX = min({x1, x2, x3, x4});
            float minY = min({y1, y2, y3, y4});
            float maxX = max({x1, x2, x3, x4});
            float maxY = max({y1, y2, y3, y4});

            vector<Point> projected = findPerpendicularVectors(tmp[1], tmp[0], JUMP_VELOCITY);
            //clockwise - counterclockwise

            Point projected_v1 = projected[0]; // clockwise
            Point projected_v2 = projected[1]; // counterclockwise

            float initialJumpVelocityX, initialJumpVelocityY;
            if (flip == SDL_FLIP_NONE) {
                initialJumpVelocityX = projected_v1.x;
                initialJumpVelocityY = projected_v1.y;
            }
            else {
                initialJumpVelocityX = projected_v2.x;
                initialJumpVelocityY = projected_v2.y;
            }

            std::cerr << "jump delta vx: " << initialJumpVelocityX << "\n";
            std::cerr << "jump delta vy: " << initialJumpVelocityY << "\n";

            jumpVelocityX = initialJumpVelocityX;
            jumpVelocityY = initialJumpVelocityY;

            projected = findPerpendicularVectors(tmp[1], tmp[0], JUMP_DRAG_ACCELERATION);

//            float initialJumpDragAccelerationX initialJumpDragAccelerationY;
            if (flip == SDL_FLIP_NONE) {
                initialJumpDragAccelerationX = projected_v1.x;
                initialJumpDragAccelerationY = projected_v1.y;
            }
            else {
                initialJumpDragAccelerationX = projected_v2.x;
                initialJumpDragAccelerationY = projected_v2.y;
            }

            initialJumpDragAccelerationX *= -1;
            initialJumpDragAccelerationY *= -1;


            gravityVelocityY = 0;
            initialGravityAccelerationY = GRAVITY_ACCELERATION;
//            jumpDragVelocityX += initialJumpDragAccelerationY * deltaTicks;
//            jumpDragVelocityY = 0;
//            velocityX += initialJumpVelocityX;
//            velocityY += deltaVy;
        }

    }

    void setDir(bool newDir) {
//        dir = newDir;
        if (newDir != dir) {
            if (pointing != dir) {

            }
            else {
                verticalFlip();
            }
//            std::cerr << "changed dir" << "\n";
//            verticalFlip();
            dir = newDir;
        }
    }

    void handleGroundCollision() {
//        int width = 100, height = 50;
//        SDL_QueryTexture(carB, NULL, NULL, &width, &height);
//
//        float scale = 100.0f / w;
//        w *= scale;
//        height *= scale;

//        SDL_FPoint center = {w/2, height/2};
//        SDL_FPoint center = { w/2, height/2 };



//        correctAngle();
        SDL_FRect carRect = {xPos, yPos, width, height};
        vector<Point> tmp = getCoords(carRect, angle);
        float x1 = tmp[0].x, y1 = tmp[0].y;
        float x2 = tmp[1].x, y2 = tmp[1].y;
        float x3 = tmp[2].x, y3 = tmp[2].y;
        float x4 = tmp[3].x, y4 = tmp[3].y;

        float minX = min({x1, x2, x3, x4});
        float minY = min({y1, y2, y3, y4});
        float maxX = max({x1, x2, x3, x4});
        float maxY = max({y1, y2, y3, y4});

        bool collideWithGround = 0;
        if (maxY >= groundY) {
            collideWithGround = 1;
            float delta = maxY - groundY;
            std::cerr << "Collapsed with ground, delta=" << delta << "\n";
            y1 -= delta;
            y2 -= delta;
            y3 -= delta;
            y4 -= delta;
            maxY = max({y1, y2, y3, y4});
            yPos -= delta;
        }


        float deltaRotate = 20;
        if (collideWithGround) {
            if (angle > 0 && angle < 90) {
                std::cerr << "<11111111111111111>" << "\n";
                float diff = angle;
                if (deltaRotate > diff) {
                    deltaRotate = diff;
                }
                angle -= deltaRotate;
            }
            else if (angle > 90 && angle < 180) {
                std::cerr << "<22222222222222222>" << "\n";
                float diff = 180 - angle;
                if (deltaRotate > diff) {
                    deltaRotate = diff;
                }
                angle += deltaRotate;
            }
            else if (angle > 180 && angle < 270) {
                std::cerr << "<33333333333333333>" << "\n";
                float diff = angle - 180;
                if (deltaRotate > diff) {
                    deltaRotate = diff;
                }
                angle -= deltaRotate;
            }
            else if (angle > 270 && angle < 360) {
                std::cerr << "<444444444444444444>" << "\n";
                float diff = 360 - diff;
                if (deltaRotate > diff) {
                    deltaRotate = diff;
                }
                angle += deltaRotate;
            }
        }
        carRect = {xPos, yPos, width, height};
        tmp = getCoords(carRect, angle);
        x1 = tmp[0].x, y1 = tmp[0].y;
        x2 = tmp[1].x, y2 = tmp[1].y;
        x3 = tmp[2].x, y3 = tmp[2].y;
        x4 = tmp[3].x, y4 = tmp[3].y;

        minX = min({x1, x2, x3, x4});
        minY = min({y1, y2, y3, y4});
        maxX = max({x1, x2, x3, x4});
        maxY = max({y1, y2, y3, y4});

        if (maxY >= groundY) {
            float delta = maxY - groundY;
            std::cerr << "Collapsed with ground, delta = " << delta << "\n";
            y1 -= delta;
            y2 -= delta;
            y3 -= delta;
            y4 -= delta;
            maxY = max({y1, y2, y3, y4});
            yPos -= delta;
        }
    }

    void handleWallCollision() {
// Assume that the car has the following properties:
// - x1, y1, x2, y2, x3, y3, x4, y4 are the coordinates of the 4 corners of the rectangle
// - angle is the current angle of the car (in degrees)
// - weight, mass, velocityX, velocityY, accelerationX, accelerationY are the properties of the car
// - screenWidth and screenHeight are the dimensions of the screen
// - wallAngle is the angle of the wall with respect to the x-axis

// Check if any corner of the car lies outside the screen
//        int width = 100, height = 50;
//        SDL_QueryTexture(carB, NULL, NULL, &width, &height);
//
//        float scale = 100.0f / width;
//        width *= scale;
//        height *= scale;
//        SDL_FPoint center = {width/2, height/2};
//        SDL_FPoint center = { width/2, height/2 };


        SDL_FRect carRect = {xPos, yPos, width, height};
        vector<Point> tmp = getCoords(carRect, angle);
        float x1 = tmp[0].x, y1 = tmp[0].y;
        float x2 = tmp[1].x, y2 = tmp[1].y;
        float x3 = tmp[2].x, y3 = tmp[2].y;
        float x4 = tmp[3].x, y4 = tmp[3].y;

        float minX = min({x1, x2, x3, x4});
        float minY = min({y1, y2, y3, y4});
        float maxX = max({x1, x2, x3, x4});
        float maxY = max({y1, y2, y3, y4});

        bool isOutsideScreen = false;
        if (x1 <= 0 || x2 <= 0 || x3 <= 0 || x4 <= 0 ||
            x1 >= WINDOW_WIDTH || x2 >= WINDOW_WIDTH || x3 >= WINDOW_WIDTH || x4 >= WINDOW_WIDTH ||
            y1 <= 0 || y2 <= 0 || y3 <= 0 || y4 <= 0 ||
            y1 >= WINDOW_HEIGHT || y2 >= WINDOW_HEIGHT || y3 >= WINDOW_HEIGHT || y4 >= WINDOW_HEIGHT) {
            isOutsideScreen = true;
        }

        float wallAngle;
        if (minX <= 0) {
            wallAngle = -M_PI/2;
        }
        if (maxX >= WINDOW_WIDTH) {
            wallAngle = M_PI/2;
        }
        if (minY <= 0) {
            wallAngle = M_PI;
        }
        if (maxY >= groundY) {
            wallAngle = 0;
        }


        if (isOutsideScreen) {
            std::cerr << "OUTSIDEEEEEEEEEEEEEEEEEE" << " " << velocityY << " " << velocityX << "\n";
            // Calculate the angle between the velocity vector of the car and the normal vector of the wall
            double angleBetween = atan2(velocityY, velocityX) - wallAngle;
            if (angleBetween < -M_PI) {
                angleBetween += 2*M_PI;
            } else if (angleBetween > M_PI) {
                angleBetween -= 2*M_PI;
            }
            std::cerr << "angle between: " << angleBetween << "\n";
            if (angleBetween < 0) {
                // Apply a force to slow down the car and eventually stop it
                double force = mass * accelerationX;
                velocityX += force * cos(wallAngle);
                velocityY += force * sin(wallAngle);

                // Calculate the angle of rotation required to make the car parallel to the wall
                double requiredAngle = wallAngle + 90;

                // Apply a torque to rotate the car towards the required angle
                double torque = weight * accelerationX * (requiredAngle - angle);
                angle += torque;

                // Calculate the penetration depth of the car into the wall
                double penetrationDepth = 0;
                if (wallAngle == -M_PI/2) {
                    penetrationDepth = x1;
                } else if (wallAngle == M_PI/2) {
                    penetrationDepth = WINDOW_WIDTH - x1;
                } else if (wallAngle == 0) {
                    penetrationDepth = WINDOW_HEIGHT - y1;
                } else if (wallAngle == M_PI) {
                    penetrationDepth = y1;
                }
                penetrationDepth /= cos(angle - wallAngle);

                // Move the car back by the penetration depth
                x1 -= penetrationDepth * cos(angle);
                y1 -= penetrationDepth * sin(angle);
                x2 -= penetrationDepth * cos(angle);
                y2 -= penetrationDepth * sin(angle);
                x3 -= penetrationDepth * cos(angle);
                y3 -= penetrationDepth * sin(angle);
                x4 -= penetrationDepth * cos(angle);
                y4 -= penetrationDepth * sin(angle);

                x1 += velocityX;
                y1 += velocityY;
                x2 += velocityX;
                y2 += velocityY;
                x3 += velocityX;
                y3 += velocityY;
                x4 += velocityX;
                y4 += velocityY;

                vector<Point> manh = {{x1, y1}, {x2, y2}, {x3, y3}, {x4, y4}};
                std::cerr << "                                        current angle: " << angle << "\n";
                angle = getAngle(manh);
                std::cerr << "                                        changing angleeeee to: " << angle << "\n";
            }
        }
    }
};


class Ball {
    float weight;
    float mass;
    float velocityX;
    float velocityY;
    float accelerationX;
    float accelerationY;
    float xPos;
    float yPos;

    Ball(float weight, float mass, float velocityX, float velocityY, float accelerationX, float accelerationY, float xPos, float yPos) :
            weight(weight), mass(mass), velocityX(velocityX), velocityY(velocityY), accelerationX(accelerationX), accelerationY(accelerationY), xPos(xPos), yPos(yPos) {}

};

void testRender() {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

  // Clear the entire screen with black color
  SDL_RenderClear(renderer);

  // Set the rendering color to green
  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

  // Create a SDL_Rect structure with the coordinates for the rectangle
  SDL_Rect rect;
  rect.x = 0;
  rect.y = 380;
  rect.w = 640;
  rect.h = 100;

  // Render the filled rectangle on the screen
  SDL_RenderFillRect(renderer, &rect);

  // Update the screen
  SDL_RenderPresent(renderer);

  // Wait for a few seconds before quitting
  SDL_Delay(3000);

  // Destroy the renderer and window
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}

void renderGround() {
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_FRect ground;
    ground.x = 0;
    ground.y = groundY;
    ground.w = WINDOW_WIDTH;
    ground.h = WINDOW_HEIGHT - groundY;

//    ground.x = 0;
//    ground.y = 380;
//    ground.w = 640;
//    ground.h = 100;
    SDL_RenderFillRectF(renderer, &ground);
}

void initCar() {

}


int main(int argc, char* argv[]) {
    if (!initEverything()) {
        return 1;
    }
//    return 0;
//
    std::cerr << "groundY = " << groundY << "\n";
//    SDL_FRect tmp_rect = {WINDOW_WIDTH/2, WINDOW_HEIGHT/2, 50, 25};
//    vector<Point> tmp_coord = getCoords(tmp_rect, 0);
//        for (int i = 0; i < 4; ++i) {
//            std::cerr << "[" << tmp_coord[i].x << ",  " << tmp_coord[i].y << "]" << "\n";
//        }


    carB = IMG_LoadTexture(renderer, "C:/Users/Phong Vu/Desktop/spr_casualcar_0.png");
//    Car car(100, 1000, 0, 0, 0, 0, WINDOW_WIDTH / 2, groundY, 1, 0, 0, 0, SDL_FLIP_NONE, 0, 0);
    vector<vector<Point>> tmp(2, vector<Point>(2));
    Car car(100, 1000, 0, 0, 0, 0, WINDOW_WIDTH / 2 - 100 / 2, groundY - 32, 1, 0, 0, 0, SDL_FLIP_NONE, 0, 0, 100);
    car.initSize();
    car.yPos = groundY - car.height;

    bool isRunning = true;

    std::string prvLeftRight = "";

    while (isRunning) {
        SDL_Event event;
        const Uint8* state = SDL_GetKeyboardState(NULL);

        std::string stateLeft = (state[SDL_SCANCODE_LEFT] ? "1" : "0");
        std::string stateRight = (state[SDL_SCANCODE_RIGHT] ? "1" : "0");
        std::string curLeftRight = stateLeft + stateRight;
        if (state[SDL_SCANCODE_LEFT] && state[SDL_SCANCODE_RIGHT]) {
            if (prvLeftRight == "10") {
                car.moveRight();
                car.setDir(1);
            }
            else {
                car.moveLeft();
                car.setDir(0);
            }
        }
        else {
            if (state[SDL_SCANCODE_LEFT]) {
                car.moveLeft();
                car.setDir(0);
            }
            else if (state[SDL_SCANCODE_RIGHT]) {
                car.moveRight();
                car.setDir(1);
            }
            else {
//                if (car.dir == 1) {
//                    car.velocityX -= VELOCITY_X_FACTOR;
//                }
//                else {
//                    car.velocityX += VELOCITY_X_FACTOR;
//                }
                car.goingVelocityX = 0;
            }
        }

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    isRunning = false;
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_BACKSLASH) {
                        std::cerr << "HIT JUMPPPPPPP" << "\n";
                        car.jump();
                        car.canJump = 0;
                    }
                    // Handle other key down events here
                    break;
                case SDL_KEYUP:
                    if (event.key.keysym.sym == SDLK_BACKSLASH) {
                        car.canJump = 1;
                    }
                // Handle other events here
            }
        }
//        std::cerr << "\n\n\n\n\n";
//        std::string stateLeft = (state[SDL_SCANCODE_LEFT] ? "1" : "0");
//        std::string stateRight = (state[SDL_SCANCODE_RIGHT] ? "1" : "0");
//        std::string curLeftRight = stateLeft + stateRight;
//        if (state[SDL_SCANCODE_LEFT] && state[SDL_SCANCODE_RIGHT]) {
//            if (prvLeftRight == "10") {
//                car.moveRight();
//                car.setDir(1);
//            }
//            else {
//                car.moveLeft();
//                car.setDir(0);
//            }
//        }
//        else {
//            if (state[SDL_SCANCODE_LEFT]) {
//                car.moveLeft();
//                car.setDir(0);
//            }
//            else if (state[SDL_SCANCODE_RIGHT]) {
//                car.moveRight();
//                car.setDir(1);
//            }
//            else {
//                car.velocityX = 0;
//            }
//        }
//        std::cerr << car.xPos << " " << car.yPos << "\n";

        if (curLeftRight == "10" || curLeftRight == "01") {
            prvLeftRight = curLeftRight;
        }

//        if (state[SDL_SCANCODE_RSHIFT]) {
//            car.jump();
//        }
        if (state[SDL_SCANCODE_UP]) {
            car.tiltUp();
//            car.flip = SDL_FLIP_HORIZONTAL;
        }
        else if (state[SDL_SCANCODE_DOWN]) {
            car.tiltDown();
//            car.flip = SDL_FLIP_HORIZONTAL;
        }

        if (state[SDL_SCANCODE_RIGHTBRACKET]) {
            curSign = 1.0;
            car.boost(1.0);
        }
        else {
//            car.boostVelocityX = 0;
//            car.boostVelocityY = 0;
//            car.boostVelocity = 0;
            curSign = -1.0;
            car.boost(-1.0);
//            car.initialBoostAccelerationX = ;

//            car.boost(-1.0);
        }

//        if (state[])
        SDL_SetRenderDrawColor(renderer, 221, 160, 221, 255);
        SDL_RenderClear(renderer);

        renderGround();

//        SDL_RenderClear(renderer);
        std::cerr << car.angle << "\n";
        car.applyGravity();

//        SDL_RenderClear(renderer);

        car.handleGroundCollision();
        car.moveCar();
//
        car.draw(renderer);
        SDL_RenderPresent(renderer);
//        SDL_Delay(200);
//        ClearScreen();
//        bool gg = 1;
//        for (int i = 1; gg&&i <= 50; ++i) {
//            for (int j = 1; j <= 50; ++j) {
//                std::cerr << " ";
//            }
//            std::cerr << "\n";
//        }
//        ClearScreen();
        prvSign = curSign;
        prvDir = car.dir;

    }
    closeEverything();
    return 0;
}
