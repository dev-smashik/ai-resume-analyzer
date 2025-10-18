#include <GL/glut.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
#include <algorithm>

using namespace std;

// Window dimensions
const int WINDOW_WIDTH = 900;
const int WINDOW_HEIGHT = 700;

// Game states
enum GameState { MENU, HELP, PLAYING, PAUSED, GAME_OVER };
GameState currentState = MENU;

// Game variables
int score = 0;
int highScore = 0;
float gameTime = 60.0f;
float timeRemaining = gameTime;
float lastTime = 0;
int comboCount = 0;
int maxCombo = 0;

// Basket properties
float basketX = 450;
float basketY = 50;
float basketWidth = 80;
float basketHeight = 60;
float basketSpeed = 15.0f;

// Multiple chicken properties
struct Chicken {
    float x, y;
    float speed;
    int direction;
    float stickY;
    float stickLeft, stickRight;
    bool active;
};

vector<Chicken> chickens;

// Egg/Item types
enum ItemType { 
    NORMAL_EGG, BLUE_EGG, GOLDEN_EGG, POOP, 
    PERK_LARGER_BASKET, PERK_SLOW_TIME, PERK_EXTRA_TIME,
    PERK_SHIELD, PERK_MAGNET, PERK_SPEED_BOOST,
    BOMB // New damage item
};

struct FallingItem {
    float x, y;
    float vx, vy; // Velocity for airflow
    ItemType type;
    float speed;
    bool active;
    float rotation;
    int sourceChicken; // Which chicken dropped it
};

vector<FallingItem> items;
float spawnTimer = 0;
float spawnInterval = 1.2f;

// Airflow system
struct Airflow {
    float startTime;
    float duration;
    float strength;
    int direction; // -1 left, 1 right
    bool active;
};

Airflow currentAirflow;
float airflowTimer = 0;
float airflowInterval = 8.0f;

// Particle system for effects
struct Particle {
    float x, y;
    float vx, vy;
    float life;
    float r, g, b;
};

vector<Particle> particles;

// Perk effects
float perkTimer = 0;
bool largerBasketActive = false;
bool slowTimeActive = false;
bool shieldActive = false;
bool magnetActive = false;
bool speedBoostActive = false;
float perkDuration = 5.0f;
int shieldHits = 0;

// Input
bool keys[256];
int mouseX = 0;

// Animation
float animTime = 0;

// Function declarations
void init();
void display();
void reshape(int w, int h);
void timer(int value);
void keyboard(unsigned char key, int x, int y);
void keyboardUp(unsigned char key, int x, int y);
void mouse(int button, int state, int x, int y);
void mouseMotion(int x, int y);
void updateGame(float deltaTime);
void spawnItem(int chickenIndex);
void drawText(float x, float y, const char* text, void* font = GLUT_BITMAP_HELVETICA_18);
void drawMenu();
void drawHelp();
void drawGame();
void drawPaused();
void drawGameOver();
void drawBasket();
void drawChicken(Chicken& chicken);
void drawItem(FallingItem& item);
void drawStick(Chicken& chicken);
void drawAirflowIndicator();
void drawParticles();
bool checkCollision(FallingItem& item);
void resetGame();
void initChickens();
void createParticleExplosion(float x, float y, float r, float g, float b);
void playSound(int frequency); // Simple sound
void drawCircle(float cx, float cy, float r, int segments = 30);

// Initialize OpenGL
void init() {
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    srand(time(0));
    
    // Initialize chickens
    initChickens();
    
    // Initialize airflow
    currentAirflow.active = false;
}

// Initialize multiple chickens
void initChickens() {
    chickens.clear();
    
    // Chicken 1 - Top left
    Chicken c1;
    c1.x = 200;
    c1.y = 600;
    c1.speed = 2.0f;
    c1.direction = 1;
    c1.stickY = 580;
    c1.stickLeft = 50;
    c1.stickRight = 400;
    c1.active = true;
    chickens.push_back(c1);
    
    // Chicken 2 - Top right
    Chicken c2;
    c2.x = 700;
    c2.y = 600;
    c2.speed = 2.5f;
    c2.direction = -1;
    c2.stickY = 580;
    c2.stickLeft = 500;
    c2.stickRight = 850;
    c2.active = true;
    chickens.push_back(c2);
    
    // Chicken 3 - Middle (appears after 20 seconds)
    Chicken c3;
    c3.x = 450;
    c3.y = 500;
    c3.speed = 3.0f;
    c3.direction = 1;
    c3.stickY = 480;
    c3.stickLeft = 200;
    c3.stickRight = 700;
    c3.active = false; // Activates later
    chickens.push_back(c3);
}

// Simple sound effect (beep)
void playSound(int frequency) {
    // On Windows, this uses the console beep
    // On Linux/Mac, this might not work - it's just for demonstration
    #ifdef _WIN32
    Beep(frequency, 50);
    #else
    // On Unix systems, write to /dev/console (requires permissions)
    // Or use external command: system("beep -f frequency -l 50");
    cout << "\a"; // Terminal bell as fallback
    #endif
}

// Draw text on screen
void drawText(float x, float y, const char* text, void* font) {
    glRasterPos2f(x, y);
    while (*text) {
        glutBitmapCharacter(font, *text);
        text++;
    }
}

// Draw filled circle
void drawCircle(float cx, float cy, float r, int segments) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; i++) {
        float theta = 2.0f * 3.1415926f * i / segments;
        glVertex2f(cx + r * cos(theta), cy + r * sin(theta));
    }
    glEnd();
}

// Create particle explosion
void createParticleExplosion(float x, float y, float r, float g, float b) {
    for (int i = 0; i < 15; i++) {
        Particle p;
        p.x = x;
        p.y = y;
        float angle = (rand() % 360) * 3.14159f / 180.0f;
        float speed = 2.0f + (rand() % 3);
        p.vx = cos(angle) * speed;
        p.vy = sin(angle) * speed;
        p.life = 1.0f;
        p.r = r;
        p.g = g;
        p.b = b;
        particles.push_back(p);
    }
}

// Draw particles
void drawParticles() {
    for (auto& p : particles) {
        glColor4f(p.r, p.g, p.b, p.life);
        drawCircle(p.x, p.y, 3);
    }
}

// Draw basket with effects
void drawBasket() {
    float w = basketWidth;
    float h = basketHeight;
    
    // Shield effect
    if (shieldActive) {
        glColor4f(0.0f, 0.8f, 1.0f, 0.3f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        drawCircle(basketX, basketY + h/2, w/2 + 20 + sin(animTime * 5) * 5);
        glDisable(GL_BLEND);
    }
    
    // Magnet effect
    if (magnetActive) {
        glColor4f(1.0f, 0.2f, 0.2f, 0.3f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        drawCircle(basketX, basketY + h/2, w/2 + 30);
        glDisable(GL_BLEND);
    }
    
    // Speed boost trail
    if (speedBoostActive) {
        for (int i = 0; i < 3; i++) {
            glColor4f(1.0f, 1.0f, 0.0f, 0.3f - i * 0.1f);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBegin(GL_QUADS);
            glVertex2f(basketX - w/2 - i*10, basketY);
            glVertex2f(basketX + w/2 + i*10, basketY);
            glVertex2f(basketX + w/2 - 10 + i*10, basketY + h);
            glVertex2f(basketX - w/2 + 10 - i*10, basketY + h);
            glEnd();
            glDisable(GL_BLEND);
        }
    }
    
    // Basket body
    glColor3f(0.6f, 0.4f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(basketX - w/2, basketY);
    glVertex2f(basketX + w/2, basketY);
    glVertex2f(basketX + w/2 - 10, basketY + h);
    glVertex2f(basketX - w/2 + 10, basketY + h);
    glEnd();
    
    // Basket rim
    glColor3f(0.4f, 0.3f, 0.15f);
    glLineWidth(3);
    glBegin(GL_LINE_LOOP);
    glVertex2f(basketX - w/2, basketY);
    glVertex2f(basketX + w/2, basketY);
    glVertex2f(basketX + w/2 - 10, basketY + h);
    glVertex2f(basketX - w/2 + 10, basketY + h);
    glEnd();
    
    // Basket pattern
    glBegin(GL_LINES);
    for (int i = 1; i < 5; i++) {
        float xOffset = (w / 5) * i - w/2;
        glVertex2f(basketX + xOffset, basketY);
        glVertex2f(basketX + xOffset - 2, basketY + h);
    }
    glEnd();
}

// Draw chicken with animation
void drawChicken(Chicken& chicken) {
    if (!chicken.active) return;
    
    float bobbing = sin(animTime * 5) * 2;
    float cx = chicken.x;
    float cy = chicken.y + bobbing;
    
    // Body (white)
    glColor3f(1.0f, 1.0f, 1.0f);
    drawCircle(cx, cy, 20);
    
    // Head
    drawCircle(cx, cy + 20, 12);
    
    // Beak (yellow)
    glColor3f(1.0f, 0.8f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx + (chicken.direction * 12), cy + 20);
    glVertex2f(cx + (chicken.direction * 20), cy + 18);
    glVertex2f(cx + (chicken.direction * 12), cy + 16);
    glEnd();
    
    // Eye (black)
    glColor3f(0.0f, 0.0f, 0.0f);
    drawCircle(cx + (chicken.direction * 4), cy + 23, 2);
    
    // Comb (red) - animated
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx - 3, cy + 28);
    glVertex2f(cx, cy + 35 + bobbing);
    glVertex2f(cx + 3, cy + 28);
    glEnd();
    
    // Wings (animated flapping)
    glColor3f(0.9f, 0.9f, 0.9f);
    float wingFlap = sin(animTime * 8) * 5;
    glBegin(GL_TRIANGLES);
    // Left wing
    glVertex2f(cx - 15, cy);
    glVertex2f(cx - 25 - wingFlap, cy + 5);
    glVertex2f(cx - 15, cy + 10);
    // Right wing
    glVertex2f(cx + 15, cy);
    glVertex2f(cx + 25 + wingFlap, cy + 5);
    glVertex2f(cx + 15, cy + 10);
    glEnd();
    
    // Legs
    glColor3f(1.0f, 0.8f, 0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glVertex2f(cx - 8, cy - 20);
    glVertex2f(cx - 8, cy - 5);
    glVertex2f(cx + 8, cy - 20);
    glVertex2f(cx + 8, cy - 5);
    glEnd();
}

// Draw bamboo stick
void drawStick(Chicken& chicken) {
    if (!chicken.active) return;
    
    // Main stick
    glColor3f(0.4f, 0.6f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(chicken.stickLeft, chicken.stickY);
    glVertex2f(chicken.stickRight, chicken.stickY);
    glVertex2f(chicken.stickRight, chicken.stickY + 10);
    glVertex2f(chicken.stickLeft, chicken.stickY + 10);
    glEnd();
    
    // Stick segments
    glColor3f(0.3f, 0.5f, 0.2f);
    glLineWidth(2);
    int segments = (chicken.stickRight - chicken.stickLeft) / 40;
    for (int i = 0; i < segments; i++) {
        float x = chicken.stickLeft + (i * 40);
        glBegin(GL_LINES);
        glVertex2f(x, chicken.stickY);
        glVertex2f(x, chicken.stickY + 10);
        glEnd();
    }
}

// Draw airflow indicator
void drawAirflowIndicator() {
    if (!currentAirflow.active) return;
    
    glColor4f(1.0f, 1.0f, 1.0f, 0.6f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    int numLines = 8;
    for (int i = 0; i < numLines; i++) {
        float y = 100 + (i * 60);
        float offset = fmod(animTime * 100 * currentAirflow.direction, 50);
        
        for (int j = 0; j < 20; j++) {
            float x = j * 50 + offset;
            glBegin(GL_LINES);
            glVertex2f(x, y);
            glVertex2f(x + 30 * currentAirflow.direction, y);
            glEnd();
        }
    }
    
    glDisable(GL_BLEND);
    
    // Text indicator
    glColor3f(1.0f, 1.0f, 1.0f);
    const char* text = currentAirflow.direction > 0 ? "WIND >>>" : "<<< WIND";
    drawText(WINDOW_WIDTH/2 - 40, 600, text, GLUT_BITMAP_HELVETICA_18);
}

// Draw falling item
void drawItem(FallingItem& item) {
    glPushMatrix();
    glTranslatef(item.x, item.y, 0);
    glRotatef(item.rotation, 0, 0, 1);
    
    switch (item.type) {
        case NORMAL_EGG:
            glColor3f(0.95f, 0.95f, 0.9f);
            drawCircle(0, 0, 15);
            drawCircle(0, 5, 12);
            break;
            
        case BLUE_EGG:
            glColor3f(0.3f, 0.5f, 0.9f);
            drawCircle(0, 0, 15);
            drawCircle(0, 5, 12);
            glColor3f(0.2f, 0.4f, 0.8f);
            drawCircle(-5, 2, 3);
            drawCircle(4, 6, 2);
            break;
            
        case GOLDEN_EGG:
            glColor3f(1.0f, 0.84f, 0.0f);
            drawCircle(0, 0, 15);
            drawCircle(0, 5, 12);
            glColor3f(1.0f, 1.0f, 0.5f);
            drawCircle(-3, 8, 4 + sin(animTime * 10) * 2);
            break;
            
        case POOP:
            glColor3f(0.4f, 0.3f, 0.2f);
            drawCircle(0, 0, 12);
            drawCircle(-5, 5, 8);
            drawCircle(5, 5, 8);
            drawCircle(0, 10, 6);
            break;
            
        case BOMB:
            // Black bomb with fuse
            glColor3f(0.1f, 0.1f, 0.1f);
            drawCircle(0, 0, 15);
            // Fuse
            glColor3f(0.6f, 0.3f, 0.0f);
            glLineWidth(3);
            glBegin(GL_LINES);
            glVertex2f(0, 12);
            glVertex2f(-5, 20);
            glEnd();
            // Spark
            glColor3f(1.0f, 0.5f, 0.0f);
            drawCircle(-5, 20, 3 + sin(animTime * 20) * 2);
            break;
            
        case PERK_LARGER_BASKET:
            glColor3f(0.2f, 0.8f, 0.3f);
            glBegin(GL_QUADS);
            glVertex2f(-12, -12);
            glVertex2f(12, -12);
            glVertex2f(12, 12);
            glVertex2f(-12, 12);
            glEnd();
            glColor3f(1.0f, 1.0f, 1.0f);
            drawText(-6, -5, "B", GLUT_BITMAP_HELVETICA_18);
            break;
            
        case PERK_SLOW_TIME:
            glColor3f(0.3f, 0.3f, 0.9f);
            glBegin(GL_QUADS);
            glVertex2f(-12, -12);
            glVertex2f(12, -12);
            glVertex2f(12, 12);
            glVertex2f(-12, 12);
            glEnd();
            glColor3f(1.0f, 1.0f, 1.0f);
            drawCircle(0, 0, 8);
            glColor3f(0.3f, 0.3f, 0.9f);
            glBegin(GL_LINES);
            glVertex2f(0, 0);
            glVertex2f(0, 6);
            glVertex2f(0, 0);
            glVertex2f(4, 0);
            glEnd();
            break;
            
        case PERK_EXTRA_TIME:
            glColor3f(0.9f, 0.8f, 0.2f);
            glBegin(GL_QUADS);
            glVertex2f(-12, -12);
            glVertex2f(12, -12);
            glVertex2f(12, 12);
            glVertex2f(-12, 12);
            glEnd();
            glColor3f(1.0f, 1.0f, 1.0f);
            glLineWidth(3);
            glBegin(GL_LINES);
            glVertex2f(-6, 0);
            glVertex2f(6, 0);
            glVertex2f(0, -6);
            glVertex2f(0, 6);
            glEnd();
            break;
            
        case PERK_SHIELD:
            glColor3f(0.2f, 0.8f, 0.9f);
            glBegin(GL_QUADS);
            glVertex2f(-12, -12);
            glVertex2f(12, -12);
            glVertex2f(12, 12);
            glVertex2f(-12, 12);
            glEnd();
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(0, -8);
            glVertex2f(6, -2);
            glVertex2f(6, 6);
            glVertex2f(0, 10);
            glVertex2f(-6, 6);
            glVertex2f(-6, -2);
            glEnd();
            break;
            
        case PERK_MAGNET:
            glColor3f(0.9f, 0.2f, 0.2f);
            glBegin(GL_QUADS);
            glVertex2f(-12, -12);
            glVertex2f(12, -12);
            glVertex2f(12, 12);
            glVertex2f(-12, 12);
            glEnd();
            glColor3f(1.0f, 1.0f, 1.0f);
            glLineWidth(4);
            glBegin(GL_LINE_STRIP);
            glVertex2f(-6, -8);
            glVertex2f(-6, 4);
            glVertex2f(0, 8);
            glVertex2f(6, 4);
            glVertex2f(6, -8);
            glEnd();
            break;
            
        case PERK_SPEED_BOOST:
            glColor3f(1.0f, 0.6f, 0.0f);
            glBegin(GL_QUADS);
            glVertex2f(-12, -12);
            glVertex2f(12, -12);
            glVertex2f(12, 12);
            glVertex2f(-12, 12);
            glEnd();
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_TRIANGLES);
            glVertex2f(-8, 6);
            glVertex2f(8, 0);
            glVertex2f(-8, -6);
            glEnd();
            break;
    }
    
    glPopMatrix();
}

// Check collision with magnet effect
bool checkCollision(FallingItem& item) {
    float itemBottom = item.y - 15;
    float basketTop = basketY + basketHeight;
    float basketLeft = basketX - basketWidth/2;
    float basketRight = basketX + basketWidth/2;
    
    // Magnet effect - attract eggs
    if (magnetActive && (item.type == NORMAL_EGG || item.type == BLUE_EGG || item.type == GOLDEN_EGG)) {
        float dx = basketX - item.x;
        float dy = basketY + basketHeight/2 - item.y;
        float dist = sqrt(dx*dx + dy*dy);
        
        if (dist < 150) {
            item.vx += dx * 0.002f;
            item.x += item.vx;
        }
    }
    
    return (itemBottom <= basketTop && itemBottom >= basketY &&
            item.x >= basketLeft && item.x <= basketRight);
}

// Spawn item from chicken
void spawnItem(int chickenIndex) {
    if (chickenIndex >= chickens.size() || !chickens[chickenIndex].active) return;
    
    FallingItem newItem;
    newItem.x = chickens[chickenIndex].x;
    newItem.y = chickens[chickenIndex].y - 30;
    newItem.vx = 0;
    newItem.vy = 0;
    newItem.active = true;
    newItem.rotation = 0;
    newItem.sourceChicken = chickenIndex;
    
    int random = rand() % 100;
    if (random < 5) {
        newItem.type = GOLDEN_EGG;
        newItem.speed = 2.5f;
    } else if (random < 20) {
        newItem.type = BLUE_EGG;
        newItem.speed = 3.0f;
    } else if (random < 28) {
        newItem.type = POOP;
        newItem.speed = 2.0f;
    } else if (random < 31) {
        newItem.type = BOMB;
        newItem.speed = 2.5f;
    } else if (random < 35) {
        newItem.type = PERK_LARGER_BASKET;
        newItem.speed = 2.5f;
    } else if (random < 39) {
        newItem.type = PERK_SLOW_TIME;
        newItem.speed = 2.5f;
    } else if (random < 43) {
        newItem.type = PERK_EXTRA_TIME;
        newItem.speed = 2.5f;
    } else if (random < 46) {
        newItem.type = PERK_SHIELD;
        newItem.speed = 2.5f;
    } else if (random < 49) {
        newItem.type = PERK_MAGNET;
        newItem.speed = 2.5f;
    } else if (random < 52) {
        newItem.type = PERK_SPEED_BOOST;
        newItem.speed = 2.5f;
    } else {
        newItem.type = NORMAL_EGG;
        newItem.speed = 3.5f;
    }
    
    if (slowTimeActive) {
        newItem.speed *= 0.5f;
    }
    
    items.push_back(newItem);
}

// Update game logic
void updateGame(float deltaTime) {
    if (currentState != PLAYING) return;
    
    animTime += deltaTime;
    
    // Update time
    timeRemaining -= deltaTime;
    if (timeRemaining <= 0) {
        timeRemaining = 0;
        currentState = GAME_OVER;
        if (score > highScore) {
            highScore = score;
        }
        return;
    }
    
    // Activate third chicken after 20 seconds
    if (gameTime - timeRemaining > 20 && chickens.size() > 2) {
        chickens[2].active = true;
    }
    
    // Update perk timer
    if (perkTimer > 0) {
        perkTimer -= deltaTime;
        if (perkTimer <= 0) {
            largerBasketActive = false;
            slowTimeActive = false;
            shieldActive = false;
            magnetActive = false;
            speedBoostActive = false;
            basketWidth = 80;
            basketSpeed = 15.0f;
        }
    }
    
    // Update airflow
    airflowTimer += deltaTime;
    if (airflowTimer > airflowInterval) {
        currentAirflow.active = true;
        currentAirflow.duration = 5.0f;
        currentAirflow.strength = 2.0f + (rand() % 3);
        currentAirflow.direction = (rand() % 2) * 2 - 1;
        currentAirflow.startTime = animTime;
        airflowTimer = 0;
        playSound(400);
    }
    
    if (currentAirflow.active) {
        if (animTime - currentAirflow.startTime > currentAirflow.duration) {
            currentAirflow.active = false;
        }
    }
    
    // Update chickens
    for (auto& chicken : chickens) {
        if (!chicken.active) continue;
        
        chicken.x += chicken.speed * chicken.direction;
        if (chicken.x > chicken.stickRight - 30 || chicken.x < chicken.stickLeft + 30) {
            chicken.direction *= -1;
        }
    }
    
    // Move basket
    float currentSpeed = speedBoostActive ? basketSpeed * 2 : basketSpeed;
    if (keys['a'] || keys['A']) {
        basketX -= currentSpeed;
    }
    if (keys['d'] || keys['D']) {
        basketX += currentSpeed;
    }
    
    if (basketX < basketWidth/2) basketX = basketWidth/2;
    if (basketX > WINDOW_WIDTH - basketWidth/2) basketX = WINDOW_WIDTH - basketWidth/2;
    
    // Spawn items
    spawnTimer += deltaTime;
    if (spawnTimer >= spawnInterval) {
        for (int i = 0; i < chickens.size(); i++) {
            if (chickens[i].active && (rand() % 2)) {
                spawnItem(i);
            }
        }
        spawnTimer = 0;
    }
    
    // Update items
    for (auto it = items.begin(); it != items.end();) {
        it->y -= it->speed;
        it->rotation += 2.0f;
        
        // Apply airflow
        if (currentAirflow.active) {
            it->vx += currentAirflow.strength * currentAirflow.direction * 0.1f;
        }
        
        // Apply velocity with damping
        it->x += it->vx;
        it->vx *= 0.95f;
        
        // Check collision
        if (checkCollision(*it)) {
            bool caught = true;
            
            switch (it->type) {
                case NORMAL_EGG:
                    score += 1 + comboCount;
                    comboCount++;
                    createParticleExplosion(it->x, it->y, 1.0f, 1.0f, 1.0f);
                    playSound(600);
                    break;
                case BLUE_EGG:
                    score += 5 + comboCount * 2;
                    comboCount++;
                    createParticleExplosion(it->x, it->y, 0.3f, 0.5f, 0.9f);
                    playSound(800);
                    break;
                case GOLDEN_EGG:
                    score += 10 + comboCount * 3;
                    comboCount++;
                    createParticleExplosion(it->x, it->y, 1.0f, 0.84f, 0.0f);
                    playSound(1000);
                    break;
                case POOP:
                    if (shieldActive) {
                        shieldHits++;
                        if (shieldHits >= 3) {
                            shieldActive = false;
                            perkTimer = 0;
                        }
                    } else {
                        score -= 10;
                        if (score < 0) score = 0;
                        comboCount = 0;
                    }
                    createParticleExplosion(it->x, it->y, 0.4f, 0.3f, 0.2f);
                    playSound(200);
                    break;
                case BOMB:
                    if (shieldActive) {
                        shieldHits++;
                        if (shieldHits >= 3) {
                            shieldActive = false;
                            perkTimer = 0;
                        }
                    } else {
                        score -= 20;
                        if (score < 0) score = 0;
                        comboCount = 0;
                        // Explosion effect
                        for (int i = 0; i < 30; i++) {
                            createParticleExplosion(it->x, it->y, 1.0f, 0.3f, 0.0f);
                        }
                    }
                    playSound(150);
                    break;
                case PERK_LARGER_BASKET:
                    largerBasketActive = true;
                    basketWidth = 120;
                    perkTimer = perkDuration;
                    playSound(900);
                    break;
                case PERK_SLOW_TIME:
                    slowTimeActive = true;
                    perkTimer = perkDuration;
                    playSound(900);
                    break;
                case PERK_EXTRA_TIME:
                    timeRemaining += 10.0f;
                    playSound(900);
                    break;
                case PERK_SHIELD:
                    shieldActive = true;
                    shieldHits = 0;
                    perkTimer = perkDuration;
                    playSound(900);
                    break;
                case PERK_MAGNET:
                    magnetActive = true;
                    perkTimer = perkDuration;
                    playSound(900);
                    break;
                case PERK_SPEED_BOOST:
                    speedBoostActive = true;
                    basketSpeed = 25.0f;
                    perkTimer = perkDuration;
                    playSound(900);
                    break;
            }
            
            if (comboCount > maxCombo) {
                maxCombo = comboCount;
            }
            
            it = items.erase(it);
        } else if (it->y < -30 || it->x < -50 || it->x > WINDOW_WIDTH + 50) {
            if (it->type == NORMAL_EGG || it->type == BLUE_EGG || it->type == GOLDEN_EGG) {
                comboCount = 0;
            }
            it = items.erase(it);
        } else {
            ++it;
        }
    }
    
    // Update particles
    for (auto it = particles.begin(); it != particles.end();) {
        it->x += it->vx;
        it->y += it->vy;
        it->vy -= 0.2f; // Gravity
        it->life -= deltaTime * 2;
        
        if (it->life <= 0) {
            it = particles.erase(it);
        } else {
            ++it;
        }
    }
}

// Draw menu screen
void drawMenu() {
    glColor3f(0.2f, 0.2f, 0.2f);
    drawText(350, 550, "CATCH THE EGGS", GLUT_BITMAP_TIMES_ROMAN_24);
    drawText(350, 510, "Team Name: Spooky_Pixels", GLUT_BITMAP_HELVETICA_18);

    drawText(350, 480, "Member 1: Soma Das - 21201111", GLUT_BITMAP_HELVETICA_18);
    drawText(350, 460, "Member 2: Sheikh Muhammad Ashik - 21201118", GLUT_BITMAP_HELVETICA_18);
    
    glColor3f(0.0f, 0.0f, 0.0f);
    drawText(350, 420, "Press ENTER to Start");
    drawText(350, 390, "Press H for Help");
    drawText(350, 360, "Press Q to Quit");
    
    char scoreText[50];
    sprintf(scoreText, "High Score: %d", highScore);
    drawText(350, 280, scoreText);
    
    // Feature highlights
    glColor3f(0.0f, 0.5f, 0.0f);
    drawText(350, 200, "NEW FEATURES:");
    glColor3f(0.0f, 0.0f, 0.0f);
    drawText(350, 170, "* Multiple Chickens & Sticks", GLUT_BITMAP_HELVETICA_12);
    drawText(350, 150, "* Wind System Affects Falling", GLUT_BITMAP_HELVETICA_12);
    drawText(350, 130, "* Shield, Magnet & Speed Perks", GLUT_BITMAP_HELVETICA_12);
    drawText(350, 110, "* Combo System for High Scores", GLUT_BITMAP_HELVETICA_12);
    // drawText(350, 90, "* Particle Effects & Sounds", GLUT_BITMAP_HELVETICA_12);
}

// Draw help screen
void drawHelp() {
    glColor3f(0.2f, 0.2f, 0.2f);
    drawText(380, 650, "HELP MENU", GLUT_BITMAP_TIMES_ROMAN_24);
    
    glColor3f(0.0f, 0.0f, 0.0f);
    
    // Controls
    drawText(50, 600, "CONTROLS:", GLUT_BITMAP_HELVETICA_18);
    drawText(50, 570, "A/D or Arrow Keys - Move basket left/right", GLUT_BITMAP_HELVETICA_12);
    drawText(50, 550, "Mouse - Move basket with mouse", GLUT_BITMAP_HELVETICA_12);
    drawText(50, 530, "P - Pause/Resume game", GLUT_BITMAP_HELVETICA_12);
    drawText(50, 510, "ESC - Return to menu", GLUT_BITMAP_HELVETICA_12);
    
    // Items
    drawText(50, 470, "ITEMS:", GLUT_BITMAP_HELVETICA_18);
    drawText(50, 440, "White Egg: 1 point (+combo bonus)", GLUT_BITMAP_HELVETICA_12);
    drawText(50, 420, "Blue Egg: 5 points (+combo bonus)", GLUT_BITMAP_HELVETICA_12);
    drawText(50, 400, "Golden Egg: 10 points (+combo bonus)", GLUT_BITMAP_HELVETICA_12);
    drawText(50, 380, "Poop (brown): -10 points (breaks combo)", GLUT_BITMAP_HELVETICA_12);
    drawText(50, 360, "Bomb (black): -20 points (breaks combo)", GLUT_BITMAP_HELVETICA_12);
    
    // Perks
    drawText(450, 470, "POWER-UPS:", GLUT_BITMAP_HELVETICA_18);
    drawText(450, 440, "Green B: Larger Basket (5s)", GLUT_BITMAP_HELVETICA_12);
    drawText(450, 420, "Blue Clock: Slow Time (5s)", GLUT_BITMAP_HELVETICA_12);
    drawText(450, 400, "Yellow +: Extra Time (+10s)", GLUT_BITMAP_HELVETICA_12);
    drawText(450, 380, "Cyan Shield: Block 3 damages", GLUT_BITMAP_HELVETICA_12);
    drawText(450, 360, "Red Magnet: Attract eggs", GLUT_BITMAP_HELVETICA_12);
    drawText(450, 340, "Orange Arrow: Speed Boost", GLUT_BITMAP_HELVETICA_12);
    
    // Special features
    drawText(50, 290, "SPECIAL FEATURES:", GLUT_BITMAP_HELVETICA_18);
    drawText(50, 260, "* Multiple chickens on different sticks", GLUT_BITMAP_HELVETICA_12);
    drawText(50, 240, "* Wind appears periodically, pushing items left/right", GLUT_BITMAP_HELVETICA_12);
    drawText(50, 220, "* Combo system: Catch eggs consecutively for bonus points", GLUT_BITMAP_HELVETICA_12);
    drawText(50, 200, "* Third chicken appears after 20 seconds!", GLUT_BITMAP_HELVETICA_12);
    drawText(50, 180, "* Particle effects and sound feedback", GLUT_BITMAP_HELVETICA_12);
    
    // Tips
    glColor3f(0.0f, 0.5f, 0.0f);
    drawText(50, 140, "TIPS:", GLUT_BITMAP_HELVETICA_18);
    glColor3f(0.0f, 0.0f, 0.0f);
    drawText(50, 110, "* Prioritize golden eggs for maximum points", GLUT_BITMAP_HELVETICA_12);
    drawText(50, 90, "* Use shield before catching poop if needed", GLUT_BITMAP_HELVETICA_12);
    drawText(50, 70, "* Watch for wind indicators - plan your position!", GLUT_BITMAP_HELVETICA_12);
    drawText(50, 50, "* Build combos by catching only eggs", GLUT_BITMAP_HELVETICA_12);
    
    glColor3f(1.0f, 0.0f, 0.0f);
    drawText(320, 20, "Press ESC to return to menu", GLUT_BITMAP_HELVETICA_18);
}

// Draw game screen
void drawGame() {
    // Draw ground
    glColor3f(0.4f, 0.7f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(WINDOW_WIDTH, 0);
    glVertex2f(WINDOW_WIDTH, 30);
    glVertex2f(0, 30);
    glEnd();
    
    // Draw sticks and chickens
    for (auto& chicken : chickens) {
        drawStick(chicken);
        drawChicken(chicken);
    }
    
    // Draw airflow indicator
    drawAirflowIndicator();
    
    // Draw items
    for (auto& item : items) {
        drawItem(item);
    }
    
    // Draw particles
    drawParticles();
    
    // Draw basket
    drawBasket();
    
    // Draw UI - Top bar
    glColor3f(0.0f, 0.0f, 0.0f);
    char scoreText[50];
    sprintf(scoreText, "Score: %d", score);
    drawText(10, WINDOW_HEIGHT - 30, scoreText);
    
    char timeText[50];
    sprintf(timeText, "Time: %.1f", timeRemaining);
    drawText(WINDOW_WIDTH - 120, WINDOW_HEIGHT - 30, timeText);
    
    // Combo indicator
    if (comboCount > 0) {
        glColor3f(1.0f, 0.5f, 0.0f);
        char comboText[50];
        sprintf(comboText, "COMBO x%d", comboCount);
        drawText(WINDOW_WIDTH/2 - 40, WINDOW_HEIGHT - 30, comboText, GLUT_BITMAP_HELVETICA_18);
    }
    
    // Active perk indicators
    int perkY = WINDOW_HEIGHT - 60;
    if (perkTimer > 0) {
        char perkText[100];
        if (largerBasketActive) {
            glColor3f(0.0f, 0.8f, 0.0f);
            sprintf(perkText, "Large Basket: %.1fs", perkTimer);
            drawText(10, perkY, perkText, GLUT_BITMAP_HELVETICA_12);
            perkY -= 20;
        }
        if (slowTimeActive) {
            glColor3f(0.0f, 0.0f, 0.8f);
            sprintf(perkText, "Slow Time: %.1fs", perkTimer);
            drawText(10, perkY, perkText, GLUT_BITMAP_HELVETICA_12);
            perkY -= 20;
        }
        if (shieldActive) {
            glColor3f(0.0f, 0.8f, 1.0f);
            sprintf(perkText, "Shield: %d hits left", 3 - shieldHits);
            drawText(10, perkY, perkText, GLUT_BITMAP_HELVETICA_12);
            perkY -= 20;
        }
        if (magnetActive) {
            glColor3f(1.0f, 0.0f, 0.0f);
            sprintf(perkText, "Magnet: %.1fs", perkTimer);
            drawText(10, perkY, perkText, GLUT_BITMAP_HELVETICA_12);
            perkY -= 20;
        }
        if (speedBoostActive) {
            glColor3f(1.0f, 0.5f, 0.0f);
            sprintf(perkText, "Speed Boost: %.1fs", perkTimer);
            drawText(10, perkY, perkText, GLUT_BITMAP_HELVETICA_12);
            perkY -= 20;
        }
    }
    
    // Controls hint
    glColor3f(0.3f, 0.3f, 0.3f);
    drawText(WINDOW_WIDTH/2 - 150, 10, "A/D or Mouse | P: Pause | ESC: Menu", GLUT_BITMAP_HELVETICA_12);
}

// Draw paused screen
void drawPaused() {
    drawGame();
    
    // Semi-transparent overlay
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(WINDOW_WIDTH, 0);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);
    glVertex2f(0, WINDOW_HEIGHT);
    glEnd();
    glDisable(GL_BLEND);
    
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(400, 400, "PAUSED", GLUT_BITMAP_TIMES_ROMAN_24);
    drawText(360, 340, "Press P to Resume");
    drawText(360, 310, "Press ESC for Menu");
}

// Draw game over screen
void drawGameOver() {
    glColor3f(0.2f, 0.2f, 0.2f);
    drawText(360, 500, "GAME OVER!", GLUT_BITMAP_TIMES_ROMAN_24);
    
    glColor3f(0.0f, 0.0f, 0.0f);
    char scoreText[50];
    sprintf(scoreText, "Final Score: %d", score);
    drawText(370, 420, scoreText, GLUT_BITMAP_HELVETICA_18);
    
    sprintf(scoreText, "High Score: %d", highScore);
    drawText(370, 390, scoreText, GLUT_BITMAP_HELVETICA_18);
    
    sprintf(scoreText, "Max Combo: x%d", maxCombo);
    drawText(370, 360, scoreText, GLUT_BITMAP_HELVETICA_18);
    
    // Performance rating
    glColor3f(0.0f, 0.5f, 0.0f);
    const char* rating;
    if (score >= 500) rating = "LEGENDARY!";
    else if (score >= 300) rating = "AMAZING!";
    else if (score >= 200) rating = "GREAT!";
    else if (score >= 100) rating = "GOOD!";
    else rating = "KEEP TRYING!";
    drawText(380, 310, rating, GLUT_BITMAP_HELVETICA_18);
    
    glColor3f(0.0f, 0.0f, 0.0f);
    drawText(320, 230, "Press ENTER to Play Again");
    drawText(350, 200, "Press ESC for Menu");
}

// Reset game
void resetGame() {
    score = 0;
    timeRemaining = gameTime;
    items.clear();
    particles.clear();
    basketX = 450;
    basketWidth = 80;
    basketSpeed = 15.0f;
    spawnTimer = 0;
    perkTimer = 0;
    airflowTimer = 0;
    comboCount = 0;
    maxCombo = 0;
    largerBasketActive = false;
    slowTimeActive = false;
    shieldActive = false;
    magnetActive = false;
    speedBoostActive = false;
    currentAirflow.active = false;
    animTime = 0;
    initChickens();
}

// Display callback
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    
    switch (currentState) {
        case MENU:
            drawMenu();
            break;
        case HELP:
            drawHelp();
            break;
        case PLAYING:
            drawGame();
            break;
        case PAUSED:
            drawPaused();
            break;
        case GAME_OVER:
            drawGameOver();
            break;
    }
    
    glutSwapBuffers();
}

// Reshape callback
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
}

// Timer callback
void timer(int value) {
    float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    
    if (deltaTime > 0.1f) deltaTime = 0.016f; // Cap at 60 FPS
    
    updateGame(deltaTime);
    
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

// Keyboard callback
void keyboard(unsigned char key, int x, int y) {
    keys[key] = true;
    
    switch (currentState) {
        case MENU:
            if (key == 13) { // Enter
                resetGame();
                currentState = PLAYING;
                lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
            } else if (key == 'h' || key == 'H') {
                currentState = HELP;
            } else if (key == 'q' || key == 'Q') {
                exit(0);
            }
            break;
            
        case HELP:
            if (key == 27) { // ESC
                currentState = MENU;
            }
            break;
            
        case PLAYING:
            if (key == 'p' || key == 'P') {
                currentState = PAUSED;
            } else if (key == 27) { // ESC
                currentState = MENU;
            }
            break;
            
        case PAUSED:
            if (key == 'p' || key == 'P') {
                currentState = PLAYING;
                lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
            } else if (key == 27) { // ESC
                currentState = MENU;
            }
            break;
            
        case GAME_OVER:
            if (key == 13) { // Enter
                resetGame();
                currentState = PLAYING;
                lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
            } else if (key == 27) { // ESC
                currentState = MENU;
            }
            break;
    }
}

// Keyboard up callback
void keyboardUp(unsigned char key, int x, int y) {
    keys[key] = false;
}

// Mouse callback
void mouse(int button, int state, int x, int y) {
    if (currentState == PLAYING && button == GLUT_LEFT_BUTTON) {
        mouseX = x;
        basketX = x;
    }
}

// Mouse motion callback
void mouseMotion(int x, int y) {
    if (currentState == PLAYING) {
        basketX = x;
        if (basketX < basketWidth/2) basketX = basketWidth/2;
        if (basketX > WINDOW_WIDTH - basketWidth/2) basketX = WINDOW_WIDTH - basketWidth/2;
    }
}

// Main function
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("Catch The Eggs - Enhanced Edition");
    
    init();
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutMouseFunc(mouse);
    glutPassiveMotionFunc(mouseMotion);
    glutTimerFunc(0, timer, 0);
    
    cout << "\n";
    cout << "========================================\n";
    cout << "   CATCH THE EGGS  \n";
    cout << "========================================\n\n";
    cout << "FEATURES:\n";
    cout << "- Multiple chickens on different sticks\n";
    cout << "- Wind system affects falling items\n";
    cout << "- Shield, Magnet, Speed Boost perks\n";
    cout << "- Combo system for bonus points\n";
    cout << "- Particle effects and sound\n";
    cout << "- Bombs for extra challenge\n\n";
    cout << "Press H in menu for detailed help!\n";
    cout << "========================================\n\n";
    
    glutMainLoop();
    return 0;
}