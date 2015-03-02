#include <iostream>
#include "Particle.h"
#include <stdlib.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "Sphere.cpp"

using namespace glm;
//const float dt = 1.f;
bool first = true;
const float dt = 0.0004f;
const float particleSize = 3.5f;

// Constructor for a particle.
void Particle::CreateParticle()
{
	pos = vec3(0, 0, 0);
	vel = vec3(0, 0, 0);

    pressure = 0;
    density = 0;
    
    viscousity_force = glm::vec3(0);
    pressure_force = glm::vec3(0);
    gravity_force = glm::vec3(0);
}

//Evolves the particle parameters over time.
//This method changes the vertical and horizontal position of the particle
void Particle::EvolveParticle()
{
    glm::vec3 newPos = pos + dt*vel + dt*dt*((gravity_force + pressure_force + viscousity_force)/density)/2.f;
	glm::vec3 newVel = (newPos - pos) / dt;

    pos = newPos;
    vel = newVel;
    
    if(pos.x < 1){        
        vel.x = -0.8*vel.x;
        pos.x = 1;
    }
    
    else if(pos.x > 256){

        vel.x = -0.8*vel.x;
        pos.x = 256;
    }
    
    if(pos.y < 1){
        
        vel.y = -0.8*vel.y;
        pos.y = 1;
    }
    
    else if(pos.y > 256){
        
        vel.y = -0.8*vel.y;
        pos.y = 256;
    }

	if (pos.z < 1){
		vel.z = -0.8*vel.z;
		pos.z = 1;
	}
    
	else if(pos.z > 256){
		vel.z = -0.8*vel.z;
		pos.z = 256;
	}

    
}

//Draw all the particles
void Particle::DrawObjects() {
    /*
    glColor3f(0.2,0.2,1);
    Sphere sphere(16/3, 6, 12);
    sphere.draw(pos[0], pos[1], pos[2]);*/
    
    glBegin(GL_QUADS);                // Begin drawing the color cube with 6 quads
    // Top face (y = 1.0f)
    // Define vertices in counter-clockwise (CCW) order with normal pointing out
    glColor3f( 0.0f, 1.0f, 0.0f);     // Green
    glVertex3f(pos.x + 1.0f*particleSize, pos.y + 1.0f*particleSize, pos.z + -1.0f*particleSize);
    glVertex3f(pos.x + -1.0f*particleSize, pos.y + 1.0f*particleSize, pos.z - 1.0f*particleSize);
    glVertex3f(pos.x + -1.0f*particleSize, pos.y + 1.0f*particleSize, pos.z + 1.0f*particleSize);
    glVertex3f(pos.x + 1.0f*particleSize, pos.y + 1.0f*particleSize, pos.z + 1.0f*particleSize);
    
    // Bottom face (y = -1.0f*particleSize)
    glVertex3f(pos.x + 1.0f*particleSize, pos.y + -1.0f*particleSize, pos.z + 1.0f*particleSize);
    glVertex3f(pos.x + -1.0f*particleSize, pos.y + -1.0f*particleSize, pos.z + 1.0f*particleSize);
    glVertex3f(pos.x + -1.0f*particleSize, pos.y + -1.0f*particleSize, pos.z + -1.0f*particleSize);
    glVertex3f(pos.x + 1.0f*particleSize, pos.y + -1.0f*particleSize, pos.z + -1.0f*particleSize);
    
    // Front face  (z = 1.0f*particleSize)
    glVertex3f(pos.x + 1.0f*particleSize, pos.y + 1.0f*particleSize, pos.z + 1.0f*particleSize);
    glVertex3f(pos.x + -1.0f*particleSize, pos.y + 1.0f*particleSize, pos.z + 1.0f*particleSize);
    glVertex3f(pos.x + -1.0f*particleSize, pos.y + -1.0f*particleSize, pos.z + 1.0f*particleSize);
    glVertex3f(pos.x + 1.0f*particleSize, pos.y + -1.0f*particleSize, pos.z + 1.0f*particleSize);
    
    // Back face (z = -1.0f*particleSize)
    glVertex3f(pos.x + 1.0f*particleSize, pos.y + -1.0f*particleSize, pos.z + -1.0f*particleSize);
    glVertex3f(pos.x + -1.0f*particleSize, pos.y + -1.0f*particleSize, pos.z + -1.0f*particleSize);
    glVertex3f(pos.x + -1.0f*particleSize, pos.y + 1.0f*particleSize, pos.z + -1.0f*particleSize);
    glVertex3f(pos.x + 1.0f*particleSize, pos.y + 1.0f*particleSize, pos.z + -1.0f*particleSize);
    
    // Left face (x = -1.0f*particleSize)
    glVertex3f(pos.x + -1.0f*particleSize, pos.y + 1.0f*particleSize, pos.z + 1.0f*particleSize);
    glVertex3f(pos.x + -1.0f*particleSize, pos.y + 1.0f*particleSize, pos.z + -1.0f*particleSize);
    glVertex3f(pos.x + -1.0f*particleSize, pos.y + -1.0f*particleSize, pos.z + -1.0f*particleSize);
    glVertex3f(pos.x + -1.0f*particleSize, pos.y + -1.0f*particleSize, pos.z + 1.0f*particleSize);
    
    // Right face (x = 1.0f*particleSize)
    glVertex3f(pos.x + 1.0f*particleSize, pos.y + 1.0f*particleSize, pos.z + -1.0f*particleSize);
    glVertex3f(pos.x + 1.0f*particleSize, pos.y + 1.0f*particleSize,  pos.z + 1.0f*particleSize);
    glVertex3f(pos.x + 1.0f*particleSize, pos.y + -1.0f*particleSize, pos.z + 1.0f*particleSize);
    glVertex3f(pos.x + 1.0f*particleSize, pos.y + -1.0f*particleSize, pos.z + -1.0f*particleSize);
    glEnd();  // End of drawing color-cube
    
}

const glm::vec3 Particle::getPos() const {
    
	return pos;
    
}

const glm::vec3 Particle::getVelocity(){
    
    return vel;
    
}

int Particle::getCellIndex() {
	glm::vec3 cell = glm::vec3(glm::floor(pos[0] / 512.f * (512 / 32)), glm::floor(pos[1] / 512.f * (512 / 32)), glm::floor(pos[2] / 512.f * (512 / 32)));
    int _cellIndex = (int)cell.x % 32 + (int)cell.y % 32 + (int)cell.z * 32;
	
    //std::cout << _cellIndex << std::endl;
    
	return _cellIndex;
}

void Particle::setPos(glm::vec3 p){
    pos = p;
}

void Particle::setVel(glm::vec3 v){

    vel = v;
}
void Particle::setPressure(float p){
    pressure = p;
}

void Particle::setDensity(float d){
    density = d;
}

float Particle::getDensity(){
    return density;
}

float Particle::getPressure(){
    return pressure;
}


void Particle::setPressureForce(glm::vec3 f){
    pressure_force = f;
}

void Particle::setViscousityForce(glm::vec3 f){
    
    viscousity_force = f;
    
}

void Particle::setGravityForce(glm::vec3 f){
    
    gravity_force = f;
    
}
