/*******************************************************************
 ** This code is part of Breakout.
 **
 ** Breakout is free software: you can redistribute it and/or modify
 ** it under the terms of the CC BY 4.0 license as published by
 ** Creative Commons, either version 4 of the License, or (at your
 ** option) any later version.
 ******************************************************************/
#include "game.h"
#include "resource.h"
#include "spriteReader.h"
#include "gameObject.h"
#include "ballObject.h"
#include "particleGenerator.h"

// Game-related State data
SpriteRenderer      *Renderer;
GameObject          *Player;
BallObject          *Ball;
ParticleGenerator   *Particles;

// 初始化球的速度
const glm::vec2 INITIAL_BALL_VELOCITY(100.0f, -350.0f);
// 球的半径
const GLfloat BALL_RADIUS = 12.5f;

typedef std::tuple<GLboolean, Direction, glm::vec2> Collision;

Game::Game(GLuint width, GLuint height)
: State(GAME_ACTIVE), Keys(), Width(width), Height(height)
{
    
}

Game::~Game()
{
    delete Renderer;
    delete Player;
    delete Ball;
    delete Particles;
}

void Game::Init()
{
    // Load shaders
    ResourceManager::LoadShader("sprite.vert", "sprite.frag", nullptr, "sprite");
    ResourceManager::LoadShader("particle.vert", "particle.frag", nullptr, "particle");
    
    // Configure shaders
    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(this->Width), static_cast<GLfloat>(this->Height), 0.0f, -1.0f, 1.0f);
    ResourceManager::GetShader("sprite").Use().SetInteger("sprite", 0);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
    ResourceManager::GetShader("particle").Use().SetInteger("sprite", 0);
    ResourceManager::GetShader("particle").SetMatrix4("projection", projection);
    
    // Load textures
    ResourceManager::LoadTexture("texture/background.jpg", GL_FALSE, "background");
    ResourceManager::LoadTexture("texture/awesomeface.png", GL_TRUE, "face");
    ResourceManager::LoadTexture("texture/block.png", GL_FALSE, "block");
    ResourceManager::LoadTexture("texture/block_solid.png", GL_FALSE, "block_solid");
    ResourceManager::LoadTexture("texture/paddle.png", true, "paddle");
    ResourceManager::LoadTexture("texture/particle.png", GL_TRUE, "particle");
    
    // Set render-specific controls
    Shader myShader;
    myShader = ResourceManager::GetShader("sprite");
    Renderer = new SpriteRenderer(myShader);
    Particles = new ParticleGenerator(ResourceManager::GetShader("particle"), ResourceManager::GetTexture("particle"), 500);
    
    // Load levels
    GameLevel one; one.Load("level/one.lvl", this->Width, this->Height * 0.5);
    GameLevel two; two.Load("level/two.lvl", this->Width, this->Height * 0.5);
    GameLevel three; three.Load("level/three.lvl", this->Width, this->Height * 0.5);
    GameLevel four; four.Load("level/four.lvl", this->Width, this->Height * 0.5);
    this->Levels.push_back(one);
    this->Levels.push_back(two);
    this->Levels.push_back(three);
    this->Levels.push_back(four);
    this->Level = 0;
    // Configure game objects
    glm::vec2 playerPos = glm::vec2(this->Width / 2 - PLAYER_SIZE.x / 2, this->Height - PLAYER_SIZE.y);
    Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("paddle"));
    
    glm::vec2 ballPos = playerPos + glm::vec2(PLAYER_SIZE.x / 2 - BALL_RADIUS, -BALL_RADIUS * 2);
    Ball = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY,
                          ResourceManager::GetTexture("face"));
}

void Game::Update(GLfloat dt)
{
    Ball->Move(dt, this->Width);
    this->DoCollisions();
    // Update particles
    Particles->Update(dt, *Ball, 2, glm::vec2(Ball->Radius / 2));
    // Check loss condition
    if(Ball->Position.y>=this->Height){// 球是否接触底部边界？
        this->ResetLevel();
        this->ResetPlayer();
    }
}

void Game::ResetLevel()
{
    if (this->Level == 0)this->Levels[0].Load("level/one.lvl", this->Width, this->Height * 0.5f);
    else if (this->Level == 1)
        this->Levels[1].Load("level/two.lvl", this->Width, this->Height * 0.5f);
    else if (this->Level == 2)
        this->Levels[2].Load("level/three.lvl", this->Width, this->Height * 0.5f);
    else if (this->Level == 3)
        this->Levels[3].Load("level/four.lvl", this->Width, this->Height * 0.5f);
}

void Game::ResetPlayer()
{
    // Reset player/ball stats
    Player->Size = PLAYER_SIZE;
    Player->Position = glm::vec2(this->Width / 2 - PLAYER_SIZE.x / 2, this->Height - PLAYER_SIZE.y);
    Ball->Reset(Player->Position + glm::vec2(PLAYER_SIZE.x / 2 - BALL_RADIUS, -(BALL_RADIUS * 2)), INITIAL_BALL_VELOCITY);
}

void Game::ProcessInput(GLfloat dt)
{
    if (this->State == GAME_ACTIVE)
    {
        GLfloat velocity = PLAYER_VELOCITY * dt;
        // 移动玩家挡板
        if (this->Keys[GLFW_KEY_A])
        {
            if (Player->Position.x >= 0)
            {
                Player->Position.x -= velocity;
                if (Ball->Stuck)
                    Ball->Position.x -= velocity;
            }
        }
        if (this->Keys[GLFW_KEY_D])
        {
            if (Player->Position.x <= this->Width - Player->Size.x)
            {
                Player->Position.x += velocity;
                if (Ball->Stuck)
                    Ball->Position.x += velocity;
            }
        }
        if (this->Keys[GLFW_KEY_SPACE])
            Ball->Stuck = false;
    }
}
void Game::Render()
{
    if (this->State == GAME_ACTIVE)
    {
        Texture2D myTexture;
        myTexture = ResourceManager::GetTexture("background");
        // Draw background
        Renderer->DrawSprite(myTexture, glm::vec2(0, 0), glm::vec2(this->Width, this->Height), 0.0f);
        // Draw level
        this->Levels[this->Level].Draw(*Renderer);
        // Draw player
        Player->Draw(*Renderer);
        // Draw particles
        Particles->Draw();
        // Draw ball
        Ball->Draw(*Renderer);
    }
}

void Game::DoCollisions()
{
    for (GameObject &box : this->Levels[this->Level].Bricks)
    {
        if (!box.Destroyed)
        {
            Collision collision = CheckCollision(*Ball, box);
            if (std::get<0>(collision)) // 如果collision 是 true
            {
                // 如果砖块不是实心就销毁砖块
                if (!box.IsSolid)
                    box.Destroyed = GL_TRUE;
                // 碰撞处理
                Direction dir = std::get<1>(collision);
                glm::vec2 diff_vector = std::get<2>(collision);
                if (dir == LEFT || dir == RIGHT) // 水平方向碰撞
                {
                    Ball->Velocity.x = -Ball->Velocity.x; // 反转水平速度
                    // 重定位
                    GLfloat penetration = Ball->Radius - std::abs(diff_vector.x);
                    if (dir == LEFT)
                        Ball->Position.x += penetration; // 将球右移
                    else
                        Ball->Position.x -= penetration; // 将球左移
                }
                else // 垂直方向碰撞
                {
                    Ball->Velocity.y = -Ball->Velocity.y; // 反转垂直速度
                    // 重定位
                    GLfloat penetration = Ball->Radius - std::abs(diff_vector.y);
                    if (dir == UP)
                        Ball->Position.y -= penetration; // 将球上移
                    else
                        Ball->Position.y += penetration; // 将球下移
                }
            }
            Collision result = CheckCollision(*Ball, *Player);
            if (!Ball -> Stuck && std::get<0>(result)) {
                // 检查碰到了挡板的哪个位置，并根据碰到哪个位置来改变速度
                GLfloat centerBoard = Player-> Position.x + Player->Size.x/2;
                GLfloat distance = (Ball->Position.x+Ball->Radius) - centerBoard;
                GLfloat percentage = distance / (Player->Size.x/2);
                
                // 依据结果移动
                GLfloat strength = 2.0f;
                glm::vec2 oldVelocity = Ball -> Velocity;
                Ball->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
                // Ball->Velocity.y = -Ball->Velocity.y;
                Ball->Velocity.y = -1 * abs(Ball->Velocity.y);
                Ball->Velocity=glm::normalize(Ball->Velocity) * glm::length(oldVelocity);
            }
        }
    }
}

GLboolean Game::CheckCollisionAABB(BallObject &one, GameObject &two)
{
     // x轴方向碰撞？
    bool collisionX = one.Position.x + one.Size.x >= two.Position.x &&
                        two.Position.x + two.Size.x >= one.Position.x;
      // y轴方向碰撞？
    bool collisionY = one.Position.y + one.Size.y >= two.Position.y &&
                        two.Position.y + two.Size.y >= one.Position.y;
    return collisionX && collisionY;
}

Collision Game::CheckCollision(BallObject &one, GameObject &two)
{
    // Get center point circle first
    glm::vec2 center(one.Position + one.Radius);
    // Calculate AABB info (center, half-extents)
    glm::vec2 aabbHalfExtents(two.Size.x / 2, two.Size.y / 2);
    glm::vec2 aabbCenter(two.Position + aabbHalfExtents);
    glm::vec2 difference = center - aabbCenter;
    glm::vec2 clamped = glm::clamp(difference, -aabbHalfExtents, aabbHalfExtents);
    // Add clamped value to AABB_center and we get the value of box closest to circle
    glm::vec2 closest = aabbCenter + clamped;
    difference = closest - center;
    if (glm::length(difference) <= one.Radius) {
        return std::make_tuple(GL_TRUE, VectorDirection(difference), difference);
    } else {
        return std::make_tuple(GL_FALSE, UP, glm::vec2(0, 0));
    }
}

Direction Game::VectorDirection(glm::vec2 target)
{
    glm::vec2 compass[] = {
        glm::vec2(0.0f, 1.0f),    // up
        glm::vec2(1.0f, 0.0f),    // right
        glm::vec2(0.0f, -1.0f),    // down
        glm::vec2(-1.0f, 0.0f)    // left
    };
    GLfloat max = 0.0f;
    GLuint best_match = -1;
    for (GLuint i = 0; i < 4; i++)
    {
        GLfloat dot_product = glm::dot(glm::normalize(target), compass[i]);
        if (dot_product > max)
        {
            max = dot_product;
            best_match = i;
        }
    }
    return (Direction)best_match;
}
