/***************************************************************************** 
 * Copyright (c) 2013-2014 Intel Corporation
 * All rights reserved.
 *
 * WARRANTY DISCLAIMER
 *
 * THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
 * MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Intel Corporation is the author of the Materials, and requests that all
 * problem reports or change requests be submitted to it directly
 *****************************************************************************/

#define GLEW_STATIC
#define _USE_MATH_DEFINES

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <memory.h>
#include <vector>

//Foldername modified in CUDA library for OpenCL
#include "CL_nvidia\cl.h"
#include "utils.h"

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <iostream>
#include <GLFW/glfw3.h>
#include <GL/glut.h>;
//for perf. counters
#include <Windows.h>
#include "Sphere.cpp"


// Macros for OpenCL versions
#define OPENCL_VERSION_1_2  1.1f
#define OPENCL_VERSION_2_0  1.1f

/* This function helps to create informative messages in
 * case when OpenCL errors occur. It returns a string
 * representation for an OpenCL error code.
 * (E.g. "CL_DEVICE_NOT_FOUND" instead of just -1.)
 */

//From main projekt
#include <GL/glew.h>

#include <iostream>
#include "Particle.h"
#include "Box.h"
//#include "Cell.h"
#include <GLFW/glfw3.h>
#include <thread>
#include <sstream>

typedef struct{
	int neighbours[30];
	int particleIDs[2000];
	int nrOfParticles;
	int nrOfNeighbours;
} Cell;



using namespace glm;

int frameCounterSpecialName = 0;

const int W = 32, H = 32, L = 32;
const long long int NUM_CELLS = W * H * L;
const cl_uint MAX_ADDED_PARTICLES = 100;
const cl_uint BEGIN_PARTICLES = 3000;
const cl_uint NUM_PARTICLES = BEGIN_PARTICLES + MAX_ADDED_PARTICLES;
//Global variable for squirting particles
cl_uint ADDED_PARTICLES = 0;

const int KERNEL_LIMIT = 35;

const float GRAVITY_CONST = 80000 * 9.82f;

vec4* particle_pos;
vec4* particle_vel;
vec4* particle_visc_f;
vec4* particle_press_f;
vec4* particle_grav_f;
cl_float* particle_density;
cl_float* particle_pressure;

Cell *cells;





//BETA BALLS
const int TEMPSIZE = 32;
float squares[TEMPSIZE * TEMPSIZE]; // hard coded values for now, with marching squares

//For 3D version
cl_float gravity_dir[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
cl_float down[4] = { 0.0f, -1.0f, 0.0f, 1.0f };
cl_float model[16];
// Rotating vars
double newTime, currTime = 0, deltaTime = 0, phi = 0, theta = 0;

//3D Cubes
const float particleSize = 3.0f;



// FPS specific vars
double lastTime;
int frames = 0;

// References and pointer needed globally
GLFWwindow* window;

//Some function declarations
void initParticles();
int generateNewParticles();
void setNeighbours(int index);





// Neat way of displaying FPS
void handleFps() {
	frames++;
	double currentTime = glfwGetTime() - lastTime;
	double fps = (double)frames / currentTime;

	if (currentTime > 1.0) {
		lastTime = glfwGetTime();
		frames = 0;

		std::ostringstream stream;
		stream << fps;
		std::string fpsString = "Betafluid 0.0.4 | FPS: " + stream.str();

		// Convert the title to a c_str and set it
		const char* pszConstString = fpsString.c_str();

		glfwSetWindowTitle(window, pszConstString);
	}
}



void handleInputs(){

	glMatrixMode(GL_MODELVIEW);
	newTime = glfwGetTime();
	deltaTime = newTime - currTime;
	currTime = newTime;

	//GENERAL INPUT
	if (glfwGetKey(window, GLFW_KEY_R)) {
		initParticles();
	}
	if (glfwGetKey(window, GLFW_KEY_G)) {
		generateNewParticles();
	}
	

	
	//CAMERA CONTROLS
	if (glfwGetKey(window, GLFW_KEY_D)) {
		phi -= deltaTime*M_PI / 2.0; // Rotate 90 degrees per second (pi/2)
		phi = fmod(phi, M_PI*2.0); // Wrap around at 360 degrees (2*pi)
		if (phi < 0.0) phi += M_PI*2.0; // If phi<0, then fmod(phi,2*pi)<0
		glRotatef(-phi, 0, 1, 0);
	}
	if (glfwGetKey(window, GLFW_KEY_A))
	{
		phi += deltaTime*M_PI / 2.0; // Rotate 90 degrees per second (pi/2)
		phi = fmod(phi, M_PI*2.0);
		glRotatef(phi, 0, 1, 0);
	}

	if (glfwGetKey(window, GLFW_KEY_S)) {
		theta -= deltaTime*M_PI / 2.0; // Rotate 90 degrees per second (pi/2)
		theta = fmod(theta, M_PI*2.0); // Wrap around at 360 degrees (2*pi)
		if (theta < 0.0) theta += M_PI*2.0; // If phi<0, then fmod(phi,2*pi)<0
		glRotatef(-theta, 1, 0, 0);
	}
	if (glfwGetKey(window, GLFW_KEY_W))
	{
		theta += deltaTime*M_PI / 2.0; // Rotate 90 degrees per second (pi/2)
		theta = fmod(theta, M_PI*2.0);
		glRotatef(theta, 1, 0, 0);
	}


	if (glfwGetKey(window, GLFW_KEY_LEFT)){
		glTranslatef(10.f, 0.0f, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_RIGHT)){
		glTranslatef(-10.f, 0.0f, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_UP)){
		glTranslatef(0.f, -10.f, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_DOWN)){
		glTranslatef(0.f, 10.f, 0.0f);
	}

}
void drawParticlesContainer(){
	
	//For transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	float containerSize = 256.f / 2;

	glPushMatrix();
	//Move cube to right coordinates
	glTranslatef(containerSize, containerSize, containerSize);
	
	//The Borders
	glPushMatrix();
	glBegin(GL_LINES);
	glColor3f(1.f, 1.f, 1.f);
	//Top
	glVertex3f(1.0f*containerSize, 1.0f*containerSize, -1.0f*containerSize);
	glVertex3f(-1.0f*containerSize, 1.0f*containerSize, -1.0f*containerSize);

	glVertex3f(-1.0f*containerSize, 1.0f*containerSize, 1.0f*containerSize);
	glVertex3f(1.0f*containerSize, 1.0f*containerSize, 1.0f*containerSize);
	//Bottom
	glVertex3f(1.0f*containerSize, -1.0f*containerSize, 1.0f*containerSize);
	glVertex3f(-1.0f*containerSize, -1.0f*containerSize, 1.0f*containerSize);

	glVertex3f(-1.0f*containerSize, -1.0f*containerSize, -1.0f*containerSize);
	glVertex3f(1.0f*containerSize, -1.0f*containerSize, -1.0f*containerSize);
	
	//Front
	glVertex3f(1.0f*containerSize, 1.0f*containerSize, 1.0f*containerSize);
	glVertex3f(1.0f*containerSize, -1.0f*containerSize, 1.0f*containerSize);

	glVertex3f(-1.0f*containerSize, -1.0f*containerSize, 1.0f*containerSize);
	glVertex3f(-1.0f*containerSize, 1.0f*containerSize, 1.0f*containerSize); 

	//BACK
	glVertex3f(1.0f*containerSize, -1.0f*containerSize, -1.0f*containerSize);
	glVertex3f(1.0f*containerSize, 1.0f*containerSize, -1.0f*containerSize);

	glVertex3f(-1.0f*containerSize, 1.0f*containerSize, -1.0f*containerSize);
	glVertex3f(-1.0f*containerSize, -1.0f*containerSize, -1.0f*containerSize);

	// Left  
	glVertex3f(-1.0f*containerSize, 1.0f*containerSize, 1.0f*containerSize);
	glVertex3f(-1.0f*containerSize, 1.0f*containerSize, -1.0f*containerSize);

	glVertex3f(-1.0f*containerSize, -1.0f*containerSize, -1.0f*containerSize);
	glVertex3f(-1.0f*containerSize, -1.0f*containerSize, 1.0f*containerSize);

	// Right 
	glVertex3f(1.0f*containerSize, 1.0f*containerSize, -1.0f*containerSize);
	glVertex3f(1.0f*containerSize, 1.0f*containerSize, 1.0f*containerSize);
	glVertex3f(1.0f*containerSize, -1.0f*containerSize, 1.0f*containerSize);
	glVertex3f(1.0f*containerSize, -1.0f*containerSize, -1.0f*containerSize);

	glEnd();
	glPopMatrix();

	GLfloat white[] = { 0.7, 0.7, 0.7, 1.0 };
	glMaterialfv(GL_FRONT, GL_AMBIENT, white);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, white);
	glMaterialfv(GL_FRONT, GL_SPECULAR, white);
	glMaterialf(GL_FRONT, GL_SHININESS, 30.0);

	glDisable(GL_LIGHTING);


	//The Walls
	glBegin(GL_QUADS);                // Begin drawing the color cube with 6 quads
	// Top face (y = 1.0f)
	// Define vertices in counter-clockwise (CCW) order with normal pointing out
	glColor4f(0.2f, .2f, .2f, 0.2f);     // Green
	glVertex3f(1.0f*containerSize, 1.0f*containerSize, -1.0f*containerSize);
	glVertex3f(-1.0f*containerSize, 1.0f*containerSize, -1.0f*containerSize);
	glVertex3f(-1.0f*containerSize, 1.0f*containerSize, 1.0f*containerSize);
	glVertex3f(1.0f*containerSize, 1.0f*containerSize, 1.0f*containerSize); 
	
	glDisable(GL_BLEND);
	// Bottom face (y = -1.0f*containerSize)
	glColor4f(1.0f, 0.0f, 0.f,1.f);
	glVertex3f(1.0f*containerSize, -1.0f*containerSize, 1.0f*containerSize);
	glVertex3f(-1.0f*containerSize, -1.0f*containerSize, 1.0f*containerSize);
	glVertex3f(-1.0f*containerSize, -1.0f*containerSize, -1.0f*containerSize);
	glVertex3f(1.0f*containerSize, -1.0f*containerSize, -1.0f*containerSize);
	glColor4f(0.2f, .2f, .2f, 0.2f);
	// Front face  (z = 1.0f*containerSize)
	glVertex3f(1.0f*containerSize, 1.0f*containerSize, 1.0f*containerSize);
	glVertex3f(-1.0f*containerSize, 1.0f*containerSize, 1.0f*containerSize);
	glVertex3f(-1.0f*containerSize, -1.0f*containerSize, 1.0f*containerSize);
	glVertex3f(1.0f*containerSize, -1.0f*containerSize, 1.0f*containerSize);

	// Back face (z = -1.0f*containerSize)
	glVertex3f(1.0f*containerSize, -1.0f*containerSize, -1.0f*containerSize);
	glVertex3f(-1.0f*containerSize, -1.0f*containerSize, -1.0f*containerSize);
	glVertex3f(-1.0f*containerSize, 1.0f*containerSize, -1.0f*containerSize);
	glVertex3f(1.0f*containerSize, 1.0f*containerSize, -1.0f*containerSize);

	// Left face (x = -1.0f*containerSize)
	glVertex3f(-1.0f*containerSize, 1.0f*containerSize, 1.0f*containerSize);
	glVertex3f(-1.0f*containerSize, 1.0f*containerSize, -1.0f*containerSize);
	glVertex3f(-1.0f*containerSize, -1.0f*containerSize, -1.0f*containerSize);
	glVertex3f(-1.0f*containerSize, -1.0f*containerSize, 1.0f*containerSize);

	// Right face (x = 1.0f*containerSize)
	glVertex3f(1.0f*containerSize, 1.0f*containerSize, -1.0f*containerSize);
	glVertex3f(1.0f*containerSize, 1.0f*containerSize, 1.0f*containerSize);
	glVertex3f(1.0f*containerSize, -1.0f*containerSize, 1.0f*containerSize);
	glVertex3f(1.0f*containerSize, -1.0f*containerSize, -1.0f*containerSize);
	glEnd();  // End of drawing color-cube
	glPopMatrix();

	glEnable(GL_LIGHTING);
}
void drawCoordinateAxes(){

	glPushMatrix();
	glBegin(GL_LINES);

	glColor3f(1.f, 1.f, 1.f);

	glVertex3f(0.f, 0.f, 0.f);
	glVertex3f(256.f, 0.f, 0.f);

	glVertex3f(0.f, 0.f, 0.f);
	glVertex3f(0.f, 256.f, 0.f);

	glVertex3f(0.f, 0.f, 0.f);
	glVertex3f(0.f, 0.f, 256.f);

	glEnd();
	glPopMatrix();


}


//Calculates the gravity vector based on the current position of the box
void calculateNewGravityVec(){
	gravity_dir[0] = model[0] * down[0] + model[1] * down[1] + model[2] * down[2] + model[3] * down[3];
	gravity_dir[1] = model[4] * down[0] + model[5] * down[1] + model[6] * down[2] + model[7] * down[3];
	gravity_dir[2] = model[8] * down[0] + model[9] * down[1] + model[10] * down[2] + model[11] * down[3];
}





void drawParticles(){
	handleFps();

	for (int i = 0; i < NUM_PARTICLES; i++){

		//Draw Particle
		glColor3f(0.2, 0.2, 1);
		glBegin(GL_LINE_LOOP); //GL_TRIANGLE_FAN for filled Circles
		for (int ii = 0; ii < 15; ii++)
		{
			float theta = 2.0f * 3.1415926f * float(ii) / float(15);//get the current angle

			float x = 16 / 3 * cosf(theta);//calculate the x component
			float y = 16 / 3 * sinf(theta);//calculate the y component

			glVertex2f(x + particle_pos[i][0], y + particle_pos[i][1]);//output vertex
		}
		

		glEnd();


	}

}
//Draw all the particles
void drawSphereParticles() {
	handleFps();
	for (int i = 0; i < NUM_PARTICLES; i++){
		glColor3f(0.2, 0.2, 1);
		Sphere sphere(16 / 3, 6, 12);
		sphere.draw(particle_pos[i].x, particle_pos[i].y, particle_pos[i].z);
		
	}
}
//Draw particles as Cubes
void drawCubeParticles(){
	handleFps();

	for (int i = 0; i < NUM_PARTICLES; i++){

		GLfloat blue[] = { 0.2, 0.6, 1.0, 1.0 };
		GLfloat white[] = { 1.0, 1.0, 1.0, 1.0 };
		glMaterialfv(GL_FRONT, GL_AMBIENT, blue);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, blue);
		glMaterialfv(GL_FRONT, GL_SPECULAR, white);
		glMaterialf(GL_FRONT, GL_SHININESS, 60.0);

		//printf("Particle pos Y %4.8f \n ", particle_pos[i].y);
		glBegin(GL_QUADS);                // Begin drawing the color cube with 6 quads
		// Top face (y = 1.0f)
		// Define vertices in counter-clockwise (CCW) order with normal pointing out
		//glColor3f( 0.0f, 1.0f, 0.0f);     // Green
		glNormal3f(0.f, 1.f, 0.f);
		glVertex3f(particle_pos[i].x + 1.0f*particleSize, particle_pos[i].y + 1.0f*particleSize, particle_pos[i].z + -1.0f*particleSize);
		glVertex3f(particle_pos[i].x + -1.0f*particleSize, particle_pos[i].y + 1.0f*particleSize, particle_pos[i].z - 1.0f*particleSize);
		glVertex3f(particle_pos[i].x + -1.0f*particleSize, particle_pos[i].y + 1.0f*particleSize, particle_pos[i].z + 1.0f*particleSize);
		glVertex3f(particle_pos[i].x + 1.0f*particleSize, particle_pos[i].y + 1.0f*particleSize, particle_pos[i].z + 1.0f*particleSize);

		// Bottom face (y = -1.0f)
		glNormal3f(0.f, -1.f, 0.f);
		glVertex3f(particle_pos[i].x + 1.0f*particleSize, particle_pos[i].y + -1.0f*particleSize, particle_pos[i].z + 1.0f*particleSize);
		glVertex3f(particle_pos[i].x + -1.0f*particleSize, particle_pos[i].y + -1.0f*particleSize, particle_pos[i].z + 1.0f*particleSize);
		glVertex3f(particle_pos[i].x + -1.0f*particleSize, particle_pos[i].y + -1.0f*particleSize, particle_pos[i].z + -1.0f*particleSize);
		glVertex3f(particle_pos[i].x + 1.0f*particleSize, particle_pos[i].y + -1.0f*particleSize, particle_pos[i].z + -1.0f*particleSize);

		// Front face  (z = 1.0f)
		glNormal3f(0.f, 0.f, 1.f);
		glVertex3f(particle_pos[i].x + 1.0f*particleSize, particle_pos[i].y + 1.0f*particleSize, particle_pos[i].z + 1.0f*particleSize);
		glVertex3f(particle_pos[i].x + -1.0f*particleSize, particle_pos[i].y + 1.0f*particleSize, particle_pos[i].z + 1.0f*particleSize);
		glVertex3f(particle_pos[i].x + -1.0f*particleSize, particle_pos[i].y + -1.0f*particleSize, particle_pos[i].z + 1.0f*particleSize);
		glVertex3f(particle_pos[i].x + 1.0f*particleSize, particle_pos[i].y + -1.0f*particleSize, particle_pos[i].z + 1.0f*particleSize);

		// Back face (z = -1.0f)
		glNormal3f(0.f, 0.f, -1.f);
		glVertex3f(particle_pos[i].x + 1.0f*particleSize, particle_pos[i].y + -1.0f*particleSize, particle_pos[i].z + -1.0f*particleSize);
		glVertex3f(particle_pos[i].x + -1.0f*particleSize, particle_pos[i].y + -1.0f*particleSize, particle_pos[i].z + -1.0f*particleSize);
		glVertex3f(particle_pos[i].x + -1.0f*particleSize, particle_pos[i].y + 1.0f*particleSize, particle_pos[i].z + -1.0f*particleSize);
		glVertex3f(particle_pos[i].x + 1.0f*particleSize, particle_pos[i].y + 1.0f*particleSize, particle_pos[i].z + -1.0f*particleSize);

		// Left face (x = -1.0f)
		glNormal3f(-1.f, 0.f, 0.f);
		glVertex3f(particle_pos[i].x + -1.0f*particleSize, particle_pos[i].y + 1.0f*particleSize, particle_pos[i].z + 1.0f*particleSize);
		glVertex3f(particle_pos[i].x + -1.0f*particleSize, particle_pos[i].y + 1.0f*particleSize, particle_pos[i].z + -1.0f*particleSize);
		glVertex3f(particle_pos[i].x + -1.0f*particleSize, particle_pos[i].y + -1.0f*particleSize, particle_pos[i].z + -1.0f*particleSize);
		glVertex3f(particle_pos[i].x + -1.0f*particleSize, particle_pos[i].y + -1.0f*particleSize, particle_pos[i].z + 1.0f*particleSize);

		// Right face (x = 1.0f)
		glNormal3f(1.f, 0.f, 0.f);
		glVertex3f(particle_pos[i].x + 1.0f*particleSize, particle_pos[i].y + 1.0f*particleSize, particle_pos[i].z + -1.0f*particleSize);
		glVertex3f(particle_pos[i].x + 1.0f*particleSize, particle_pos[i].y + 1.0f*particleSize, particle_pos[i].z + 1.0f*particleSize);
		glVertex3f(particle_pos[i].x + 1.0f*particleSize, particle_pos[i].y + -1.0f*particleSize, particle_pos[i].z + 1.0f*particleSize);
		glVertex3f(particle_pos[i].x + 1.0f*particleSize, particle_pos[i].y + -1.0f*particleSize, particle_pos[i].z + -1.0f*particleSize);
		glEnd();  	

	}
}

void drawBetaBalls(){

	handleFps();
	// Beta balls begins

	// Marching cubes algorithm -
	// bitwise tiling (Jonsson, 2015)
	// Using ..
	// f(x,y) = sum(i = 1 -> n) ri^2 / ((x - xi)^2 + (y - yi)^2)
	//

	// Needed for scaling
	int kSize = 256 / TEMPSIZE;

	// Iterate through all cells
	for (int k = 0; k < TEMPSIZE * TEMPSIZE; k++) {
		int x = kSize * (k % TEMPSIZE);
		int y = (k / (TEMPSIZE)) * kSize;

		// Brute force
		// for each cell, check
		for (int i = 0; i < NUM_PARTICLES; i++) {
			// calculate height map, sum

			squares[k] += (4.f*4.0f) / ((x - particle_pos[i].x) * (x - particle_pos[i].x) + (y - particle_pos[i].y) * (y - particle_pos[i].y));
		}
	}

	// draw stuff
	for (int i = 0; i < TEMPSIZE - 1; i++) {
		for (int j = 0; j < TEMPSIZE - 1; j++) {
			int bitwiseSum = 0;

			//  ***** USED FOR MARCHING SQUARES *****
			//
			//       CASE EXAMPLE:  bitwiseSum = 4
			//    (only point d is within a metaball):
			//
			//
			//             |            |
			//             | a          | b
			//       ------+------------+------
			//             |            |
			//             |            *  (Qx, Qy)
			//             |        _ ^ |
			//             | c   _ ^    | d
			//       ------+-- * -------+------
			//             | (Px, Py)   |
			//             |            |
			//
			//t


			// calculate bitwise sum
			// sum increments by 2^n for each corner, going clockwise, starting in top right

			float a, b, c, d;
			a = squares[i + (j * TEMPSIZE + TEMPSIZE)]; // upper left
			b = squares[i + 1 + (j * TEMPSIZE + TEMPSIZE)]; // upper right
			c = squares[i + (j * TEMPSIZE)]; // lower left
			d = squares[i + 1 + (j * TEMPSIZE)]; // lower right

			if (a > 1.f)   { bitwiseSum += 1; } // upper left corner
			if (b > 1.f)   { bitwiseSum += 2; } // upper right corner
			if (d > 1.f)   { bitwiseSum += 4; } // lower right corner
			if (c > 1.f)   { bitwiseSum += 8; } // lower left corner

			// Depending on the bitwiseSum, draw triangles.

			if (bitwiseSum >= 1.f) { // bitwiseSum cannot assume negative values and 0 �is an empty square.

				// These are used for interpolation process, increasing the possible angles from n * PI/4 to basically infinity.
				float qx, qy, px, py;

				// all points, position
				float ax, ay, bx, by, cx, cy, dx, dy;

				// topleft
				ax = i * kSize + kSize / 2;
				ay = (j + 1) * kSize + kSize / 2;

				// topright
				bx = (i + 1) * kSize + kSize / 2;
				by = (j + 1) * kSize + kSize / 2;

				// botleft
				cx = i * kSize + kSize / 2;
				cy = j * kSize + kSize / 2;

				// botright
				dx = (i + 1) * kSize + kSize / 2;
				dy = j * kSize + kSize / 2;

				glBegin(GL_TRIANGLE_STRIP);

				glColor4f(0.5f, 0.7f, 1.f, 0.0f);

				switch (bitwiseSum) {
				case 1:
					// Points of intersection of square bounadires from metaballs
					// left intersection
					qx = ax;
					qy = ay + (cy - ay) * ((1 - a) / (c - a));

					// upper intersection
					px = bx + (ax - bx) * ((1 - b) / (a - b));
					py = ay;

					// top left triangle
					glTexCoord2f(0.0, 1.0); glVertex3f(ax, ay, 0);  // top         left
					glTexCoord2f(0.0, 1.0); glVertex3f(px, py, 0);  // top         left+half
					glTexCoord2f(0.0, 1.0); glVertex3f(qx, qy, 0);  // bot+half    left
					break;
				case 2:
					// Points of intersection of square boundaries from metaballs
					// right intersection
					qx = bx;
					qy = by + (dy - by) * ((1 - b) / (d - b));

					// upper intersection
					px = bx + (ax - bx) * ((1 - b) / (a - b));
					py = ay;

					// top right triangle
					glTexCoord2f(0.0, 1.0); glVertex3f(bx, by, 0);  // top          right
					glTexCoord2f(0.0, 1.0); glVertex3f(px, py, 0);  // top          left+half
					glTexCoord2f(0.0, 1.0); glVertex3f(qx, qy, 0);  // bot+half     right

					break;
				case 3:
					// Points of intersection of square boundaries from metaballs
					// right intersection
					qx = bx;
					qy = by + (dy - by) * ((1 - b) / (d - b));

					// left intersection
					px = ax;
					py = ay + (cy - ay) * ((1 - a) / (c - a));


					// top rectangle
					glTexCoord2f(0.0, 1.0); glVertex3f(bx, by, 0);  // top      right
					glTexCoord2f(0.0, 1.0); glVertex3f(ax, ay, 0);  // top      left
					glTexCoord2f(0.0, 1.0); glVertex3f(qx, qy, 0);  // bot+half right
					glTexCoord2f(0.0, 1.0); glVertex3f(px, py, 0);  // bot+half left

					break;
				case 4:
					// Points of intersection of square boundaries from metaballs
					// right intersection
					qx = bx;
					qy = by + (dy - by) * ((1 - b) / (d - b));

					// lower intersection
					px = dx + (cx - dx) * ((1 - d) / (c - d));
					py = cy;

					// bot right triangle
					glTexCoord2f(0.0, 1.0); glVertex3f(dx, dy, 0);   // bot      right
					glTexCoord2f(0.0, 1.0); glVertex3f(px, py, 0);   // bot      left+half
					glTexCoord2f(0.0, 1.0); glVertex3f(qx, qy, 0);   // bot+half right

					break;
				case 5:
					//std::cout << "SPECIAL CASE 5" << endl;
					break;
				case 6:
					// Points of intersection of square boundaries from metaballs
					// upper intersection
					qx = bx + (ax - bx) * ((1 - b) / (a - b));
					qy = ay;

					// lower intersection
					px = dx + (cx - dx) * ((1 - d) / (c - d));
					py = cy;

					// right rectangle
					glTexCoord2f(0.0, 1.0); glVertex3f(bx, by, 0);    // top    right
					glTexCoord2f(0.0, 1.0); glVertex3f(qx, qy, 0);    // top    left+half
					glTexCoord2f(0.0, 1.0); glVertex3f(dx, dy, 0);    // bottom right
					glTexCoord2f(0.0, 1.0); glVertex3f(px, py, 0);    // bottom left+half

					break;
				case 7:
					// Points of intersection of square boundaries from metaballs
					// left intersection
					qx = ax;
					qy = ay + (cy - ay) * ((1 - a) / (c - a));

					// lower intersection
					px = dx + (cx - dx) * ((1 - d) / (c - d));
					py = cy;

					// missing bot left triangle
					glTexCoord2f(0.0, 1.0); glVertex3f(ax, ay, 0);    // top        left
					glTexCoord2f(0.0, 1.0); glVertex3f(bx, by, 0);    // top        right
					glTexCoord2f(0.0, 1.0); glVertex3f(qx, qy, 0);    // bot+half   left
					glTexCoord2f(0.0, 1.0); glVertex3f(px, py, 0);    // bot        left+half
					glTexCoord2f(0.0, 1.0); glVertex3f(bx, by, 0);    // top        right
					glTexCoord2f(0.0, 1.0); glVertex3f(dx, dy, 0);    // bot        right
					break;
				case 8:
					// Points of intersection of square boundaries from metaballs
					// left intersection
					qx = ax;
					qy = ay + (cy - ay) * ((1 - a) / (c - a));

					// lower intersection
					px = dx + (cx - dx) * ((1 - d) / (c - d));
					py = cy;

					// bot left triangle
					glTexCoord2f(0.0, 1.0); glVertex3f(cx, cy, 0);   // bottom      left
					glTexCoord2f(0.0, 1.0); glVertex3f(qx, qy, 0);   // bottom+half left
					glTexCoord2f(0.0, 1.0); glVertex3f(px, py, 0);   // bottom      left+half
					break;
				case 9:
					// Points of intersection of square boundaries from metaballs
					// upper intersection
					qx = bx + (ax - bx) * ((1 - b) / (a - b));
					qy = ay;

					// lower intersection
					px = dx + (cx - dx) * ((1 - d) / (c - d));
					py = cy;

					// left rectangle
					glTexCoord2f(0.0, 1.0); glVertex3f(ax, ay, 0);   // top    left
					glTexCoord2f(0.0, 1.0); glVertex3f(cx, cy, 0);   // bottom left
					glTexCoord2f(0.0, 1.0); glVertex3f(qx, qy, 0);   // top    left+half
					glTexCoord2f(0.0, 1.0); glVertex3f(px, py, 0);   // bottom left+half

					break;
				case 10:
					//std::cout << "SPECIAL CASE 10" << endl;
					break;
				case 11:
					// Points of intersection of square boundaries from metaballs
					// right intersection
					qx = bx;
					qy = by + (dy - by) * ((1 - b) / (d - b));

					// lower intersection
					px = dx + (cx - dx) * ((1 - d) / (c - d));
					py = cy;

					// missing bot right triangle
					glTexCoord2f(0.0, 1.0); glVertex3f(ax, ay, 0);    // top    left
					glTexCoord2f(0.0, 1.0); glVertex3f(bx, by, 0);    // top    right
					glTexCoord2f(0.0, 1.0); glVertex3f(cx, cy, 0);    // bottom left
					glTexCoord2f(0.0, 1.0); glVertex3f(qx, qy, 0);    // bot+half right
					glTexCoord2f(0.0, 1.0); glVertex3f(px, py, 0);    // bottom left+half
					break;
				case 12:
					// Points of intersection of square boundaries from metaballs
					qx = bx;
					qy = by + (dy - by) * ((1 - b) / (d - b));

					// left intersection
					px = ax;
					py = ay + (cy - ay) * ((1 - a) / (c - a));


					// bot rectangle
					glTexCoord2f(0.0, 1.0); glVertex3f(dx, dy, 0);    // bottom right
					glTexCoord2f(0.0, 1.0); glVertex3f(cx, cy, 0);    // bottom left
					glTexCoord2f(0.0, 1.0); glVertex3f(qx, qy, 0);    // bot+half right
					glTexCoord2f(0.0, 1.0); glVertex3f(px, py, 0);    // bot+half left

					break;
				case 13:
					// Points of intersection of square boundaries from metaballs
					// right intersection
					qx = bx;
					qy = by + (dy - by) * ((1 - b) / (d - b));

					// upper intersection
					px = bx + (ax - bx) * ((1 - b) / (a - b));
					py = ay;

					// missing top right triangle
					glTexCoord2f(0.0, 1.0); glVertex3f(ax, ay, 0);    // top    left
					glTexCoord2f(0.0, 1.0); glVertex3f(px, py, 0);    // top    left+half
					glTexCoord2f(0.0, 1.0); glVertex3f(cx, cy, 0);    // bottom left
					glTexCoord2f(0.0, 1.0); glVertex3f(qx, qy, 0);    // bot+half right
					glTexCoord2f(0.0, 1.0); glVertex3f(dx, dy, 0);    // bottom right

					break;
				case 14:


					// Points of intersection of square bounadires from metaballs
					// left intersection
					qx = ax;
					qy = ay + (cy - ay) * ((1 - a) / (c - a));

					// upper intersection
					px = bx + (ax - bx) * ((1 - b) / (a - b));
					py = ay;

					// missing top left triangle
					glTexCoord2f(0.0, 1.0); glVertex3f(qx, qy, 0);    // bot+half left
					glTexCoord2f(0.0, 1.0); glVertex3f(px, py, 0);    // top    left+half
					glTexCoord2f(0.0, 1.0); glVertex3f(cx, cy, 0);    // bottom left
					glTexCoord2f(0.0, 1.0); glVertex3f(bx, by, 0);    // top    right
					glTexCoord2f(0.0, 1.0); glVertex3f(dx, dy, 0);    // bottom right

					break;
				case 15:
					// draw full square
					glTexCoord2f(0.0, 1.0); glVertex3f(ax, ay, 0);    // top    left
					glTexCoord2f(0.0, 1.0); glVertex3f(bx, by, 0);    // top    right
					glTexCoord2f(0.0, 1.0); glVertex3f(cx, cy, 0);    // bottom left
					glTexCoord2f(0.0, 1.0); glVertex3f(dx, dy, 0);    // bottom right

					break;
				default:
					std::cout << "THIS IS FUCKED" << std::endl;
					break;
				}

				glEnd();
			}
		}
	}

	// clear squares
	for (int i = 0; i < TEMPSIZE * TEMPSIZE; ++i) {
		squares[i] = 0.f;
	}

	// Beta balls ends

	/*for(int i = 0; i < NUM_PARTICLES; i++){

	particles[i].DrawObjects();

	}*/


}

void reshape_window(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void idle()
{
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
	

		float dt = 0.0004f;

		vec4 newPos = particle_pos[i] + dt * particle_vel[i] + dt * dt* ((particle_grav_f[i] + particle_press_f[i] + particle_visc_f[i]) / particle_density[i]) / 2.f;
		vec4 newVel = (newPos - particle_pos[i]) / dt;

		particle_pos[i] = newPos;
		particle_vel[i] = newVel;

		if (particle_pos[i].x < 1){
			particle_vel[i].x = -0.8*particle_vel[i].x;
			particle_pos[i].x = 1;
		}

		else if (particle_pos[i].x > 512){

			particle_vel[i].x = -0.8*particle_vel[i].x;
			particle_pos[i].x = 512;
		}

		if (particle_pos[i].y < 1){

			particle_vel[i].y = -0.8*particle_vel[i].y;
			particle_pos[i].y = 1;

		}


		else if (particle_pos[i].y > 512){

			particle_vel[i].y = -0.8*particle_vel[i].y;
			particle_pos[i].y = 512;

		}

		
	}

}
void initNeighbours(){

	for (long long int i = 0; i < NUM_CELLS; i++){

		//Fill array of particle IDS with unvalid default value -1
		std::fill(cells[i].particleIDs, cells[i].particleIDs + 2000, -1);

		cells[i].nrOfParticles = 0;
		setNeighbours(i);
	}
}
void resetValues(){


}
void setNeighbours(int index) {

	cells[index].neighbours[0] = (index);
	
	int x = index % W;
	int y = (int)((index / W)) % H;
	int z = floor(index / (H * W));

	switch (index % W) {
	case 0: //Left side
		if (index == 0) { //Top left front (3*3*3) (0)
			cells[index].neighbours[1] = (1);          // 1
			cells[index].neighbours[2] = (W);          // 3
			cells[index].neighbours[3] = (W + 1);        // 4
			cells[index].neighbours[4] = (W*H);          // 9
			cells[index].neighbours[5] = (W*H + 1);        // 10
			cells[index].neighbours[6] = (W*H + W);        // 12
			cells[index].neighbours[7] = (W*H + W + 1);      // 13
			cells[index].nrOfNeighbours = 8;
		}
		else if (index == H * (W - 1)) { // Down left front (6)
			cells[index].neighbours[1] = (index + 1);      // 7
			cells[index].neighbours[2] = (index - W);      // 3
			cells[index].neighbours[3] = (index - W + 1);    // 4
			cells[index].neighbours[4] = (index + W*H);      // 15
			cells[index].neighbours[5] = (index + W*H + 1);    // 16
			cells[index].neighbours[6] = (index + W*H - W);    // 12
			cells[index].neighbours[7] = (index + W*H - W + 1);  // 13
			cells[index].nrOfNeighbours = 8;
		}
		else if (index == L * H * (W - 1)) { // Top left back (18)
			cells[index].neighbours[1] = (index + 1);      // 19
			cells[index].neighbours[2] = (index + W);      // 21
			cells[index].neighbours[3] = (index + W + 1);    // 22 
			cells[index].neighbours[4] = (index - W*H);      // 9
			cells[index].neighbours[5] = (index - W*H + 1);    // 10
			cells[index].neighbours[6] = (index - W*H + W);    // 12
			cells[index].neighbours[7] = (index - W*H + W + 1);  // 13
			cells[index].nrOfNeighbours = 8;
		}
		else if (index == H * L * W - W){// Down left back (24)
			cells[index].neighbours[1] = (index + 1);      // 25
			cells[index].neighbours[2] = (index - W);      // 21
			cells[index].neighbours[3] = (index - W + 1);    // 22
			cells[index].neighbours[4] = (index - W*H);      // 15
			cells[index].neighbours[5] = (index - W*H + 1);    // 16
			cells[index].neighbours[6] = (index - W*H - W);    // 12
			cells[index].neighbours[7] = (index - W*H - W + 1);  // 13
			cells[index].nrOfNeighbours = 8;
		}
		else{       //4*4*4     

			if (y == H - 1 && z < H - 1 && z > 0){  // (28)

				cells[index].neighbours[1] = (index + 1);    // 29
				cells[index].neighbours[2] = (index - W);    // 24
				cells[index].neighbours[3] = (index - W + 1);  // 25
				cells[index].neighbours[4] = (index + W*H);    // 44
				cells[index].neighbours[5] = (index + W*H + 1);  // 45
				cells[index].neighbours[6] = (index + W*H - W);  // 40
				cells[index].neighbours[7] = (index + W*H - W + 1);//41
				cells[index].neighbours[8] = (index - W*H);    //12
				cells[index].neighbours[9] = (index - W*H + 1);  //13
				cells[index].neighbours[10] = (index - W*H - W);  //8
				cells[index].neighbours[11] = (index - W*H - W + 1); //9
				cells[index].nrOfNeighbours = 12;
			}

			else if (y == 0 && z < H - 1 && z > 0){//(16)
				cells[index].neighbours[1] = (index + 1);    //17
				cells[index].neighbours[2] = (index + W);    //20
				cells[index].neighbours[3] = (index + W + 1);  //21
				cells[index].neighbours[4] = (index + W*H);    //32
				cells[index].neighbours[5] = (index + W*H + 1);  //33
				cells[index].neighbours[6] = (index + W*H + W);  //36
				cells[index].neighbours[7] = (index + W*H + W + 1); // 37
				cells[index].neighbours[8] = (index - W*H);    //0
				cells[index].neighbours[9] = (index - W*H + 1);  //1
				cells[index].neighbours[10] = (index - W*H + W);  //4
				cells[index].neighbours[11] = (index - W*H + W + 1); //5
				cells[index].nrOfNeighbours = 12;
			}
			else if (z == 0 && y > 0 && y < H - 1){// (4)
				cells[index].neighbours[1] = (index + 1);    //5
				cells[index].neighbours[2] = (index + W);    //8
				cells[index].neighbours[3] = (index + W + 1);  //9
				cells[index].neighbours[4] = (index - W);    //0
				cells[index].neighbours[5] = (index - W + 1);  //1
				cells[index].neighbours[6] = (index + W*H);    //20
				cells[index].neighbours[7] = (index + W*H + 1);  //21
				cells[index].neighbours[8] = (index + W*H + W);  //24
				cells[index].neighbours[9] = (index + W*H + W + 1);//25
				cells[index].neighbours[10] = (index + W*H - W);  //16
				cells[index].neighbours[11] = (index + W*H - W + 1);//17
				cells[index].nrOfNeighbours = 12;
			}

			else if (z == L - 1 && x == 0 && y < H - 1 && y > 0){ // (52)
				cells[index].neighbours[1] = (index + 1);      //53
				cells[index].neighbours[2] = (index + W);      //56
				cells[index].neighbours[3] = (index + W + 1);    //57
				cells[index].neighbours[4] = (index - W);      //48
				cells[index].neighbours[5] = (index - W + 1);    //49
				cells[index].neighbours[6] = (index - W*H);      //36
				cells[index].neighbours[7] = (index - W*H + 1);    //37
				cells[index].neighbours[8] = (index - W*H - W);    //32
				cells[index].neighbours[9] = (index - W*H - W + 1);  //33
				cells[index].neighbours[10] = (index - W*H + W);    //40
				cells[index].neighbours[11] = (index - W*H + W + 1);  //41
				cells[index].nrOfNeighbours = 12;
			}

			else if (y != 0 && y != H - 1 && z != 0 && z != L - 1){ // (20) 
				cells[index].neighbours[1] = (index + 1);        // 21 
				cells[index].neighbours[2] = (index + W);        // 24
				cells[index].neighbours[3] = (index + W + 1);      // 25
				cells[index].neighbours[4] = (index - W);        // 16
				cells[index].neighbours[5] = (index - W + 1);      // 17
				cells[index].neighbours[6] = (index + W * H);      // 36
				cells[index].neighbours[7] = (index + W * H + 1);    // 37
				cells[index].neighbours[8] = (index + W * H + W);    // 40
				cells[index].neighbours[9] = (index + W * H + W + 1);  //41
				cells[index].neighbours[10] = (index + W * H - W);    //32
				cells[index].neighbours[11] = (index + W * H - W + 1);    //33
				cells[index].neighbours[12] = (index - W * H);      //4
				cells[index].neighbours[13] = (index - W * H + 1);    //5
				cells[index].neighbours[14] = (index - W * H + W);    // 8
				cells[index].neighbours[15] = (index - W * H + W + 1);  //9
				cells[index].neighbours[16] = (index - W * H - W);    //0
				cells[index].neighbours[17] = (index - W * H - W + 1);  //1
				cells[index].nrOfNeighbours = 18;
			}
		}
		break;
	case (W - 1) : //Right side
		if (index == W - 1) { //Top right front     (2) 
			cells[index].neighbours[1] = (index - 1);      //1
			cells[index].neighbours[2] = (index + W);      //5
			cells[index].neighbours[3] = (index + W - 1);    //4
			cells[index].neighbours[4] = (index + W*H);      //11
			cells[index].neighbours[5] = (index + W*H - 1);    //10
			cells[index].neighbours[6] = (index + W*H + W);    //14
			cells[index].neighbours[7] = (index + W*H + W - 1);  //13
			cells[index].nrOfNeighbours = 8;
		}
		else if (index == W * H - 1) { //Down right front (8)
			cells[index].neighbours[1] = (index - 1);      //7
			cells[index].neighbours[2] = (index - W);      //5
			cells[index].neighbours[3] = (index - W - 1);    //4
			cells[index].neighbours[4] = (index + W*H);      //17
			cells[index].neighbours[5] = (index + W*H - 1);    //16      
			cells[index].neighbours[6] = (index + W*H - W);    //14
			cells[index].neighbours[7] = (index + W*H - W - 1);  //13
			cells[index].nrOfNeighbours = 8;
		}
		else if (index == W * H * L - W*H + W - 1){ //Top right back (20)
			cells[index].neighbours[1] = (index - 1);      //19
			cells[index].neighbours[2] = (index + W);      //23
			cells[index].neighbours[3] = (index + W - 1);    //22
			cells[index].neighbours[4] = (index - W*H);      //11
			cells[index].neighbours[5] = (index - W*H - 1);    //10
			cells[index].neighbours[6] = (index - W*H + W);    //14
			cells[index].neighbours[7] = (index - W*H + W - 1);  //13
			cells[index].nrOfNeighbours = 8;
		}
		else if (index == W * H * L - 1){ //Down right back (26)
			cells[index].neighbours[1] = (index - 1);      //25
			cells[index].neighbours[2] = (index - W);      //23
			cells[index].neighbours[3] = (index - W - 1);    //22
			cells[index].neighbours[4] = (index - W*H);      //17
			cells[index].neighbours[5] = (index - W*H - 1);    //16
			cells[index].neighbours[6] = (index - W*H - W);    //14
			cells[index].neighbours[7] = (index - W*H - W - 1); // 13
			cells[index].nrOfNeighbours = 8;
		}
		else{ // (4*4*4)

			if (y == H - 1 && x == W - 1 && z < L - 1 && z > 0){    // (31)

				cells[index].neighbours[1] = (index - 1);      //30
				cells[index].neighbours[2] = (index - W);      //27
				cells[index].neighbours[3] = (index - W - 1);    //26
				cells[index].neighbours[4] = (index + W*H);      //47
				cells[index].neighbours[5] = (index + W*H - 1);    //46
				cells[index].neighbours[6] = (index + W*H - W);    //43
				cells[index].neighbours[7] = (index + W*H - W - 1);  //42
				cells[index].neighbours[8] = (index - W*H);      //15
				cells[index].neighbours[9] = (index - W*H - 1);    //14
				cells[index].neighbours[10] = (index - W*H - W);    //11
				cells[index].neighbours[11] = (index - W*H - W - 1);  //10  
				cells[index].nrOfNeighbours = 12;
			}
			else if (y == 0 && z < L - 1 && z > 0){       // (19)
				cells[index].neighbours[1] = (index - 1);      //18
				cells[index].neighbours[2] = (index + W);      //23
				cells[index].neighbours[3] = (index + W - 1);    //22
				cells[index].neighbours[4] = (index - W*H);      //3
				cells[index].neighbours[5] = (index - W*H - 1);    //2
				cells[index].neighbours[6] = (index - W*H + W);    //7
				cells[index].neighbours[7] = (index - W*H + W - 1);  //6
				cells[index].neighbours[8] = (index + W*H);      //35
				cells[index].neighbours[9] = (index + W*H - 1);    //34
				cells[index].neighbours[10] = (index + W*H + W);    //39
				cells[index].neighbours[11] = (index + W*H + W - 1);  //38
				cells[index].nrOfNeighbours = 12;
			}
			else if (z == L - 1 && y < H - 1 && y > 0){ // (55)
				cells[index].neighbours[1] = (index - 1);      //54
				cells[index].neighbours[2] = (index - W);      //51
				cells[index].neighbours[3] = (index - W - 1);    //50
				cells[index].neighbours[4] = (index + W);      //59
				cells[index].neighbours[5] = (index + W - 1);    //58
				cells[index].neighbours[6] = (index - W*H);      //39
				cells[index].neighbours[7] = (index - W*H - 1);    //38
				cells[index].neighbours[8] = (index - W*H - W);    //35
				cells[index].neighbours[9] = (index - W*H - W - 1);  //34
				cells[index].neighbours[10] = (index - W*H + W);    //43
				cells[index].neighbours[11] = (index - W*H + W - 1);  //42
				cells[index].nrOfNeighbours = 12;
			}
			else if (z == 0 && y > 0 && y < H - 1){//(7)
				cells[index].neighbours[1] = (index - 1);    // 6
				cells[index].neighbours[2] = (index - W);    //3
				cells[index].neighbours[3] = (index - W - 1);  //2
				cells[index].neighbours[4] = (index + W);    //11
				cells[index].neighbours[5] = (index + W - 1);  //10
				cells[index].neighbours[6] = (index + W*H);    //23
				cells[index].neighbours[7] = (index + W*H - 1);  //22
				cells[index].neighbours[8] = (index + W*H - W);  //19
				cells[index].neighbours[9] = (index + W*H - W - 1); //18
				cells[index].neighbours[10] = (index + W*H + W);  //27
				cells[index].neighbours[11] = (index + W*H + W - 1); //26
				cells[index].nrOfNeighbours = 12;
			}
			else if (y != 0 && y != H - 1 && z != 0 && z != L - 1){ //Right side (23)
				cells[index].neighbours[1] = (index - 1);          //22
				cells[index].neighbours[2] = (index + W);          //27
				cells[index].neighbours[3] = (index + W - 1);        //26
				cells[index].neighbours[4] = (index - W);          //20
				cells[index].neighbours[5] = (index - W - 1);        //19
				cells[index].neighbours[6] = (index + W*H);          //39
				cells[index].neighbours[7] = (index + W*H - 1);        //38
				cells[index].neighbours[8] = (index + W*H + W);        //43
				cells[index].neighbours[9] = (index + W*H + W - 1);      //42
				cells[index].neighbours[10] = (index + W*H - W);        //35
				cells[index].neighbours[11] = (index + W*H - W - 1);      //34
				cells[index].neighbours[12] = (index - W*H);          //7
				cells[index].neighbours[13] = (index - W*H - 1);        //6
				cells[index].neighbours[14] = (index - W*H + W);        //11
				cells[index].neighbours[15] = (index - W*H + W - 1);      //10
				cells[index].neighbours[16] = (index - W*H - W);        //3
				cells[index].neighbours[17] = (index - W*H - W - 1);      //2
				cells[index].nrOfNeighbours = 18;
			}
		}
		break;
	default:
		if (x > 0 && x < W - 1 && y > 0 && y < H - 1 && z > 0 && z < L - 1){ //(21) //Centralt

			cells[index].neighbours[1] = (index - 1);          //20
			cells[index].neighbours[2] = (index + 1);          //22
			cells[index].neighbours[3] = (index + W);          //25
			cells[index].neighbours[4] = (index + W - 1);        //24
			cells[index].neighbours[5] = (index + W + 1);        //26
			cells[index].neighbours[6] = (index - W);          //17
			cells[index].neighbours[7] = (index - W - 1);        //16
			cells[index].neighbours[8] = (index - W + 1);        //18
			cells[index].neighbours[9] = (index - W*H);          //5
			cells[index].neighbours[10] = (index - W*H - 1);        //4
			cells[index].neighbours[11] = (index - W*H + 1);        //6
			cells[index].neighbours[12] = (index - W*H - W);        //1
			cells[index].neighbours[13] = (index - W*H - W - 1);      //0
			cells[index].neighbours[14] = (index - W*H - W + 1);      //2
			cells[index].neighbours[15] = (index - W*H + W);        //9
			cells[index].neighbours[16] = (index - W*H + W - 1);      //8
			cells[index].neighbours[17] = (index - W*H + W + 1);      //10
			cells[index].neighbours[18] = (index + W*H);          //37
			cells[index].neighbours[19] = (index + W*H - 1);        //36
			cells[index].neighbours[20] = (index + W*H + 1);        //38
			cells[index].neighbours[21] = (index + W*H - W);        //33
			cells[index].neighbours[22] = (index + W*H - W - 1);      //32
			cells[index].neighbours[23] = (index + W*H - W + 1);      //34
			cells[index].neighbours[24] = (index + W*H + W);        //41
			cells[index].neighbours[25] = (index + W*H + W - 1);      //40
			cells[index].neighbours[26] = (index + W*H + W + 1);      //42
			cells[index].nrOfNeighbours =27;
		}
		else if (y == H - 1 && x > 0 && x < W - 1 && z > 0 && z < L - 1){ //(29) Bottenplatta

			cells[index].neighbours[1] = (index - 1);          //28
			cells[index].neighbours[2] = (index + 1);          //30
			cells[index].neighbours[3] = (index - W);          //25
			cells[index].neighbours[4] = (index - W - 1);        //24
			cells[index].neighbours[5] = (index - W + 1);        //26

			cells[index].neighbours[6] = (index - W*H);          //13
			cells[index].neighbours[7] = (index - W*H - 1);        //12
			cells[index].neighbours[8] = (index - W*H + 1);        //14
			cells[index].neighbours[9] = (index - W*H - W);        //9
			cells[index].neighbours[10] = (index - W*H - W - 1);      //8
			cells[index].neighbours[11] = (index - W*H - W + 1);      //10

			cells[index].neighbours[12] = (index + W*H);          //45
			cells[index].neighbours[13] = (index + W*H - 1);        //44
			cells[index].neighbours[14] = (index + W*H + 1);        //46
			cells[index].neighbours[15] = (index + W*H - W);        //41
			cells[index].neighbours[16] = (index + W*H - W - 1);      //40
			cells[index].neighbours[17] = (index + W*H - W + 1);      //42
			cells[index].nrOfNeighbours = 18;

		}
		else if (y == 0 && x < W - 1 && z > 0 && z < L - 1){  //(17) Topplatta

			cells[index].neighbours[1] = (index - 1);          //16
			cells[index].neighbours[2] = (index + 1);          //18
			cells[index].neighbours[3] = (index + W);          //21
			cells[index].neighbours[4] = (index + W - 1);        //20
			cells[index].neighbours[5] = (index + W + 1);        //22

			cells[index].neighbours[6] = (index + W*H);          //33
			cells[index].neighbours[7] = (index + W*H - 1);        //32
			cells[index].neighbours[8] = (index + W*H + 1);        //34
			cells[index].neighbours[9] = (index + W*H + W);        //37
			cells[index].neighbours[10] = (index + W*H + W - 1);      //36
			cells[index].neighbours[11] = (index + W*H + W + 1);      //38

			cells[index].neighbours[12] = (index - W*H);          //1
			cells[index].neighbours[13] = (index - W*H - 1);        //0
			cells[index].neighbours[14] = (index - W*H + 1);        //2
			cells[index].neighbours[15] = (index - W*H + W);        //5
			cells[index].neighbours[16] = (index - W*H + W - 1);      //4
			cells[index].neighbours[17] = (index - W*H + W + 1);      //6
			cells[index].nrOfNeighbours = 18;

		}
		else if (z == 0 && x > 0 && x < W - 1 && y > 0 && y < H - 1){ //(5) Bortaplatta

			cells[index].neighbours[1] = (index - 1);          //4
			cells[index].neighbours[2] = (index + 1);          //6
			cells[index].neighbours[3] = (index + W);          //9
			cells[index].neighbours[4] = (index + W - 1);        //8
			cells[index].neighbours[5] = (index + W + 1);        //10
			cells[index].neighbours[6] = (index - W);          //1
			cells[index].neighbours[7] = (index - W - 1);        //0
			cells[index].neighbours[8] = (index - W + 1);        //2

			cells[index].neighbours[9] = (index + W*H);          //21
			cells[index].neighbours[10] = (index + W*H - 1);        //20
			cells[index].neighbours[11] = (index + W*H + 1);        //22
			cells[index].neighbours[12] = (index + W*H + W);        //25
			cells[index].neighbours[13] = (index + W*H + W - 1);      //24
			cells[index].neighbours[14] = (index + W*H + W + 1);      //26
			cells[index].neighbours[15] = (index + W*H - W);        //17
			cells[index].neighbours[16] = (index + W*H - W - 1);      //16
			cells[index].neighbours[17] = (index + W*H - W + 1);      //18
			cells[index].nrOfNeighbours = 18;

		}
		else if (z == L - 1 && x > 0 && x < W - 1 && y > 0 && y < H - 1){ //(53) //Motplattan

			cells[index].neighbours[1] = (index - 1);          //52
			cells[index].neighbours[2] = (index + 1);          //54
			cells[index].neighbours[3] = (index + W);          //57
			cells[index].neighbours[4] = (index + W - 1);        //56
			cells[index].neighbours[5] = (index + W + 1);        //58

			cells[index].neighbours[6] = (index - W);          //49
			cells[index].neighbours[7] = (index - W - 1);        //48
			cells[index].neighbours[8] = (index - W + 1);        //50

			cells[index].neighbours[9] = (index - W*H);          //37
			cells[index].neighbours[10] = (index - W*H - 1);        //36
			cells[index].neighbours[11] = (index - W*H + 1);        //38

			cells[index].neighbours[12] = (index - W*H - W);        //33
			cells[index].neighbours[13] = (index - W*H - W - 1);      //32
			cells[index].neighbours[14] = (index - W*H - W + 1);      //34

			cells[index].neighbours[15] = (index - W*H + W);        //41
			cells[index].neighbours[16] = (index - W*H + W - 1);      //40
			cells[index].neighbours[17] = (index - W*H + W + 1);      //42
			cells[index].nrOfNeighbours = 18;
		}
		else if (y == H - 1 && z == 0 && x > 0 && x < W - 1){     //(13)

			cells[index].neighbours[1] = (index - 1);          //12
			cells[index].neighbours[2] = (index + 1);          //14
			cells[index].neighbours[3] = (index - W);          //9
			cells[index].neighbours[4] = (index - W - 1);        //8
			cells[index].neighbours[5] = (index - W + 1);        //10

			cells[index].neighbours[6] = (index + W*H);          //29
			cells[index].neighbours[7] = (index + W*H - 1);        //28
			cells[index].neighbours[8] = (index + W*H + 1);        //30
			cells[index].neighbours[9] = (index + W*H - W);        //25
			cells[index].neighbours[10] = (index + W*H - W - 1);      //24
			cells[index].neighbours[11] = (index + W*H - W + 1);      //26
			cells[index].nrOfNeighbours = 12;
		}
		else if (y == H - 1 && z == L - 1 && x > 0 && x < W - 1){     //(61)

			cells[index].neighbours[1] = (index - 1);          //60
			cells[index].neighbours[2] = (index + 1);          //62

			cells[index].neighbours[3] = (index - W);          //57
			cells[index].neighbours[4] = (index - W - 1);        //56
			cells[index].neighbours[5] = (index - W + 1);        //58

			cells[index].neighbours[6] = (index - W*H);          //45
			cells[index].neighbours[7] = (index - W*H - 1);        //44
			cells[index].neighbours[8] = (index - W*H + 1);        //46

			cells[index].neighbours[9] = (index - W*H - W);        //41
			cells[index].neighbours[10] = (index - W*H - W - 1);      //40
			cells[index].neighbours[11] = (index - W*H - W + 1);      //42
			cells[index].nrOfNeighbours = 12;
		}
		else if (z == L - 1 && y == 0 && x > 0 && x < W - 1){     //(49)

			cells[index].neighbours[1] = (index - 1);          //48
			cells[index].neighbours[2] = (index + 1);          //50

			cells[index].neighbours[3] = (index + W);          //53
			cells[index].neighbours[4] = (index + W - 1);        //52
			cells[index].neighbours[5] = (index + W + 1);        //54

			cells[index].neighbours[6] = (index - W*H);          //33
			cells[index].neighbours[7] = (index - W*H - 1);        //32
			cells[index].neighbours[8] = (index - W*H + 1);        //34

			cells[index].neighbours[9] = (index - W*H + W);        //37
			cells[index].neighbours[10] = (index - W*H + W - 1);      //36
			cells[index].neighbours[11] = (index + W*H + W + 1);      //38
			cells[index].nrOfNeighbours = 12;
		}
		else if (z == 0 && y == 0 && x > 0 && x < W - 1){     //(1)

			cells[index].neighbours[1] = (index - 1);          //0
			cells[index].neighbours[2] = (index + 1);          //2

			cells[index].neighbours[3] = (index + W);          //5
			cells[index].neighbours[4] = (index + W - 1);        //4
			cells[index].neighbours[5] = (index + W + 1);        //6

			cells[index].neighbours[6] = (index + W*H);          //17
			cells[index].neighbours[7] = (index + W*H - 1);        //16
			cells[index].neighbours[8] = (index + W*H + 1);        //18

			cells[index].neighbours[9] = (index + W*H + W);        //21
			cells[index].neighbours[10] = (index + W*H + W - 1);      //20
			cells[index].neighbours[11] = (index + W*H + W + 1);      //22
			cells[index].nrOfNeighbours = 12;
		}
		break;
	}
	//std::cout << "SETNEIGHBOUR Function" << cells[index].neighbours[3] << std::endl;
}

const char* TranslateOpenCLError(cl_int errorCode)
{
    switch(errorCode)
    {
    case CL_SUCCESS:                            return "CL_SUCCESS";
    case CL_DEVICE_NOT_FOUND:                   return "CL_DEVICE_NOT_FOUND";
    case CL_DEVICE_NOT_AVAILABLE:               return "CL_DEVICE_NOT_AVAILABLE";
    case CL_COMPILER_NOT_AVAILABLE:             return "CL_COMPILER_NOT_AVAILABLE";
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:      return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case CL_OUT_OF_RESOURCES:                   return "CL_OUT_OF_RESOURCES";
    case CL_OUT_OF_HOST_MEMORY:                 return "CL_OUT_OF_HOST_MEMORY";
    case CL_PROFILING_INFO_NOT_AVAILABLE:       return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case CL_MEM_COPY_OVERLAP:                   return "CL_MEM_COPY_OVERLAP";
    case CL_IMAGE_FORMAT_MISMATCH:              return "CL_IMAGE_FORMAT_MISMATCH";
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:         return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case CL_BUILD_PROGRAM_FAILURE:              return "CL_BUILD_PROGRAM_FAILURE";
    case CL_MAP_FAILURE:                        return "CL_MAP_FAILURE";
    case CL_MISALIGNED_SUB_BUFFER_OFFSET:       return "CL_MISALIGNED_SUB_BUFFER_OFFSET";                          //-13
    case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:    return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";   //-14
    //case CL_COMPILE_PROGRAM_FAILURE:            return "CL_COMPILE_PROGRAM_FAILURE";                               //-15
    //case CL_LINKER_NOT_AVAILABLE:               return "CL_LINKER_NOT_AVAILABLE";                                  //-16
    //case CL_LINK_PROGRAM_FAILURE:               return "CL_LINK_PROGRAM_FAILURE";                                  //-17
    //case CL_DEVICE_PARTITION_FAILED:            return "CL_DEVICE_PARTITION_FAILED";                               //-18
    //case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:      return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";                         //-19
    case CL_INVALID_VALUE:                      return "CL_INVALID_VALUE";
    case CL_INVALID_DEVICE_TYPE:                return "CL_INVALID_DEVICE_TYPE";
    case CL_INVALID_PLATFORM:                   return "CL_INVALID_PLATFORM";
    case CL_INVALID_DEVICE:                     return "CL_INVALID_DEVICE";
    case CL_INVALID_CONTEXT:                    return "CL_INVALID_CONTEXT";
    case CL_INVALID_QUEUE_PROPERTIES:           return "CL_INVALID_QUEUE_PROPERTIES";
    case CL_INVALID_COMMAND_QUEUE:              return "CL_INVALID_COMMAND_QUEUE";
    case CL_INVALID_HOST_PTR:                   return "CL_INVALID_HOST_PTR";
    case CL_INVALID_MEM_OBJECT:                 return "CL_INVALID_MEM_OBJECT";
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:    return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case CL_INVALID_IMAGE_SIZE:                 return "CL_INVALID_IMAGE_SIZE";
    case CL_INVALID_SAMPLER:                    return "CL_INVALID_SAMPLER";
    case CL_INVALID_BINARY:                     return "CL_INVALID_BINARY";
    case CL_INVALID_BUILD_OPTIONS:              return "CL_INVALID_BUILD_OPTIONS";
    case CL_INVALID_PROGRAM:                    return "CL_INVALID_PROGRAM";
    case CL_INVALID_PROGRAM_EXECUTABLE:         return "CL_INVALID_PROGRAM_EXECUTABLE";
    case CL_INVALID_KERNEL_NAME:                return "CL_INVALID_KERNEL_NAME";
    case CL_INVALID_KERNEL_DEFINITION:          return "CL_INVALID_KERNEL_DEFINITION";
    case CL_INVALID_KERNEL:                     return "CL_INVALID_KERNEL";
    case CL_INVALID_ARG_INDEX:                  return "CL_INVALID_ARG_INDEX";
    case CL_INVALID_ARG_VALUE:                  return "CL_INVALID_ARG_VALUE";
    case CL_INVALID_ARG_SIZE:                   return "CL_INVALID_ARG_SIZE";
    case CL_INVALID_KERNEL_ARGS:                return "CL_INVALID_KERNEL_ARGS";
    case CL_INVALID_WORK_DIMENSION:             return "CL_INVALID_WORK_DIMENSION";
    case CL_INVALID_WORK_GROUP_SIZE:            return "CL_INVALID_WORK_GROUP_SIZE";
    case CL_INVALID_WORK_ITEM_SIZE:             return "CL_INVALID_WORK_ITEM_SIZE";
    case CL_INVALID_GLOBAL_OFFSET:              return "CL_INVALID_GLOBAL_OFFSET";
    case CL_INVALID_EVENT_WAIT_LIST:            return "CL_INVALID_EVENT_WAIT_LIST";
    case CL_INVALID_EVENT:                      return "CL_INVALID_EVENT";
    case CL_INVALID_OPERATION:                  return "CL_INVALID_OPERATION";
    case CL_INVALID_GL_OBJECT:                  return "CL_INVALID_GL_OBJECT";
    case CL_INVALID_BUFFER_SIZE:                return "CL_INVALID_BUFFER_SIZE";
    case CL_INVALID_MIP_LEVEL:                  return "CL_INVALID_MIP_LEVEL";
    case CL_INVALID_GLOBAL_WORK_SIZE:           return "CL_INVALID_GLOBAL_WORK_SIZE";                           //-63
    case CL_INVALID_PROPERTY:                   return "CL_INVALID_PROPERTY";                                   //-64
    //case CL_INVALID_IMAGE_DESCRIPTOR:           return "CL_INVALID_IMAGE_DESCRIPTOR";                           //-65
    //case CL_INVALID_COMPILER_OPTIONS:           return "CL_INVALID_COMPILER_OPTIONS";                           //-66
    //case CL_INVALID_LINKER_OPTIONS:             return "CL_INVALID_LINKER_OPTIONS";                             //-67
    //case CL_INVALID_DEVICE_PARTITION_COUNT:     return "CL_INVALID_DEVICE_PARTITION_COUNT";                     //-68
//    case CL_INVALID_PIPE_SIZE:                  return "CL_INVALID_PIPE_SIZE";                                  //-69
//    case CL_INVALID_DEVICE_QUEUE:               return "CL_INVALID_DEVICE_QUEUE";                               //-70    

    default:
        return "UNKNOWN ERROR CODE";
    }
}


/* Convenient container for all OpenCL specific objects used in the sample
 *
 * It consists of two parts:
 *   - regular OpenCL objects which are used in almost each normal OpenCL applications
 *   - several OpenCL objects that are specific for this particular sample
 *
 * You collect all these objects in one structure for utility purposes
 * only, there is no OpenCL specific here: just to avoid global variables
 * and make passing all these arguments in functions easier.
 */
struct ocl_args_d_t
{
    ocl_args_d_t();
    ~ocl_args_d_t();

    // Regular OpenCL objects:
    cl_context       context;           // hold the context handler
    cl_device_id     device;            // hold the selected device handler
    cl_command_queue commandQueue;      // hold the commands-queue handler
    cl_program       program;           // hold the program handler
    cl_kernel        kernel;            // hold the kernel handler
	cl_kernel		 kernel2;
	cl_kernel		 kernel3;
	cl_kernel		 kernel4;
    float            platformVersion;   // hold the OpenCL platform version (default 1.2)
    float            deviceVersion;     // hold the OpenCL device version (default. 1.2)
    float            compilerVersion;   // hold the device OpenCL C version (default. 1.2)
    
    // Objects that are specific for algorithm implemented in this sample
    cl_mem           pos_b;              // hold first source buffer
    cl_mem           vel_b;              // hold second source buffer
	cl_mem			 pressure_force_b;
	cl_mem			 viscosity_force_b;
	cl_mem			 gravity_force_b;
	cl_mem			 pressure_b;
	cl_mem			 density_b;
	cl_mem			 cells_b;
	cl_mem			 grav_dir_b;
};

ocl_args_d_t::ocl_args_d_t():
        context(NULL),
        device(NULL),
        commandQueue(NULL),
        program(NULL),
        kernel(NULL),
		kernel2(NULL),
		kernel3(NULL),
		kernel4(NULL),
        platformVersion(OPENCL_VERSION_1_2),
        deviceVersion(OPENCL_VERSION_1_2),
        compilerVersion(OPENCL_VERSION_1_2),
        pos_b(NULL),
        vel_b(NULL),
		pressure_force_b(NULL),
		viscosity_force_b(NULL),
		gravity_force_b(NULL),
        pressure_b(NULL),
		density_b(NULL),
		cells_b(NULL),
		grav_dir_b(NULL)
{
}

/*
 * destructor - called only once
 * Release all OpenCL objects
 * This is a regular sequence of calls to deallocate all created OpenCL resources in bootstrapOpenCL.
 *
 * You may want to call these deallocation procedures in the middle of your application execution
 * (not at the end) if you don't further need OpenCL runtime.
 * You may want to do that in order to free some memory, for example,
 * or recreate OpenCL objects with different parameters.
 *
 */
ocl_args_d_t::~ocl_args_d_t()
{
    cl_int err = CL_SUCCESS;

    if (kernel)
    {
        err = clReleaseKernel(kernel);
        if (CL_SUCCESS != err)
        {
            LogError("Error: clReleaseKernel returned '%s'.\n", TranslateOpenCLError(err));
        }
    }
	if (kernel2)
	{
		err = clReleaseKernel(kernel2);
		if (CL_SUCCESS != err)
		{
			LogError("Error: clReleaseKernel returned '%s'.\n", TranslateOpenCLError(err));
		}
	}
	if (kernel3)
	{
		err = clReleaseKernel(kernel3);
		if (CL_SUCCESS != err)
		{
			LogError("Error: clReleaseKernel returned '%s'.\n", TranslateOpenCLError(err));
		}
	}
	if (kernel4)
	{
		err = clReleaseKernel(kernel4);
		if (CL_SUCCESS != err)
		{
			LogError("Error: clReleaseKernel returned '%s'.\n", TranslateOpenCLError(err));
		}
	}

    if (program)
    {
        err = clReleaseProgram(program);
        if (CL_SUCCESS != err)
        {
            LogError("Error: clReleaseProgram returned '%s'.\n", TranslateOpenCLError(err));
        }
    }
    if (pos_b)
    {
        err = clReleaseMemObject(pos_b);
        if (CL_SUCCESS != err)
        {
            LogError("Error: clReleaseMemObject returned '%s'.\n", TranslateOpenCLError(err));
        }
    }
    if (vel_b)
    {
        err = clReleaseMemObject(vel_b);
        if (CL_SUCCESS != err)
        {
            LogError("Error: clReleaseMemObject returned '%s'.\n", TranslateOpenCLError(err));
        }
    }
	if (pressure_force_b)
	{
		err = clReleaseMemObject(pressure_force_b);
		if (CL_SUCCESS != err)
		{
			LogError("Error: clReleaseMemObject returned '%s'.\n", TranslateOpenCLError(err));
		}
	}
	if (viscosity_force_b)
	{
		err = clReleaseMemObject(viscosity_force_b);
		if (CL_SUCCESS != err)
		{
			LogError("Error: clReleaseMemObject returned '%s'.\n", TranslateOpenCLError(err));
		}
	}
	if (gravity_force_b)
	{
		err = clReleaseMemObject(gravity_force_b);
		if (CL_SUCCESS != err)
		{
			LogError("Error: clReleaseMemObject returned '%s'.\n", TranslateOpenCLError(err));
		}
	}
    if (pressure_b)
    {
        err = clReleaseMemObject(pressure_b);
        if (CL_SUCCESS != err)
        {
            LogError("Error: clReleaseMemObject returned '%s'.\n", TranslateOpenCLError(err));
        }
    }
	if (density_b)
	{
		err = clReleaseMemObject(density_b);
		if (CL_SUCCESS != err)
		{
			LogError("Error: clReleaseMemObject returned '%s'.\n", TranslateOpenCLError(err));
		}
	}
	if (cells_b)
	{
		err = clReleaseMemObject(cells_b);
		if (CL_SUCCESS != err)
		{
			LogError("Error: clReleaseMemObject returned '%s'.\n", TranslateOpenCLError(err));
		}
	}
	if (grav_dir_b)
	{
		err = clReleaseMemObject(grav_dir_b);
		if (CL_SUCCESS != err)
		{
			LogError("Error: clReleaseMemObject returned '%s'.\n", TranslateOpenCLError(err));
		}
	}
    if (commandQueue)
    {
        err = clReleaseCommandQueue(commandQueue);
        if (CL_SUCCESS != err)
        {
            LogError("Error: clReleaseCommandQueue returned '%s'.\n", TranslateOpenCLError(err));
        }
    }
    if (device)
    {
        //err = clReleaseDevice(device);
        if (CL_SUCCESS != err)
        {
            LogError("Error: clReleaseDevice returned '%s'.\n", TranslateOpenCLError(err));
        }
    }
    if (context)
    {
        err = clReleaseContext(context);
        if (CL_SUCCESS != err)
        {
            LogError("Error: clReleaseContext returned '%s'.\n", TranslateOpenCLError(err));
        }
    }

    /*
     * Note there is no procedure to deallocate platform 
     * because it was not created at the startup,
     * but just queried from OpenCL runtime.
     */
}


/*
 * Check whether an OpenCL platform is the required platform
 * (based on the platform's name)
 */
bool CheckPreferredPlatformMatch(cl_platform_id platform, const char* preferredPlatform)
{
    size_t stringLength = 0;
    cl_int err = CL_SUCCESS;
    bool match = false;

    // In order to read the platform's name, we first read the platform's name string length (param_value is NULL).
    // The value returned in stringLength
    err = clGetPlatformInfo(platform, CL_PLATFORM_NAME, 0, NULL, &stringLength);
    if (CL_SUCCESS != err)
    {
        LogError("Error: clGetPlatformInfo() to get CL_PLATFORM_NAME length returned '%s'.\n", TranslateOpenCLError(err));
        return false;
    }

    // Now, that we know the platform's name string length, we can allocate enough space before read it
    std::vector<char> platformName(stringLength);

    // Read the platform's name string
    // The read value returned in platformName
    err = clGetPlatformInfo(platform, CL_PLATFORM_NAME, stringLength, &platformName[0], NULL);
    if (CL_SUCCESS != err)
    {
        LogError("Error: clGetplatform_ids() to get CL_PLATFORM_NAME returned %s.\n", TranslateOpenCLError(err));
        return false;
    }
    
    // Now check if the platform's name is the required one
    if (strstr(&platformName[0], preferredPlatform) != 0)
    {
        // The checked platform is the one we're looking for
        match = true;
    }

    return match;
}

/*
 * Find and return the preferred OpenCL platform
 * In case that preferredPlatform is NULL, the ID of the first discovered platform will be returned
 */
cl_platform_id FindOpenCLPlatform(const char* preferredPlatform, cl_device_type deviceType)
{
    cl_uint numPlatforms = 0;
    cl_int err = CL_SUCCESS;

    // Get (in numPlatforms) the number of OpenCL platforms available
    // No platform ID will be return, since platforms is NULL
    err = clGetPlatformIDs(0, NULL, &numPlatforms);
    if (CL_SUCCESS != err)
    {
        LogError("Error: clGetplatform_ids() to get num platforms returned %s.\n", TranslateOpenCLError(err));
        return NULL;
    }
    LogInfo("Number of available platforms: %u\n", numPlatforms);

    if (0 == numPlatforms)
    {
        LogError("Error: No platforms found!\n");
        return NULL;
    }

    std::vector<cl_platform_id> platforms(numPlatforms);

    // Now, obtains a list of numPlatforms OpenCL platforms available
    // The list of platforms available will be returned in platforms
    err = clGetPlatformIDs(numPlatforms, &platforms[0], NULL);
    if (CL_SUCCESS != err)
    {
        LogError("Error: clGetplatform_ids() to get platforms returned %s.\n", TranslateOpenCLError(err));
        return NULL;
    }

    // Check if one of the available platform matches the preferred requirements
    for (cl_uint i = 0; i < numPlatforms; i++)
    {
        bool match = true;
        cl_uint numDevices = 0;

        // If the preferredPlatform is not NULL then check if platforms[i] is the required one
        // Otherwise, continue the check with platforms[i]
        if ((NULL != preferredPlatform) && (strlen(preferredPlatform) > 0))
        {
            // In case we're looking for a specific platform
            match = CheckPreferredPlatformMatch(platforms[i], preferredPlatform);
        }

        // match is true if the platform's name is the required one or don't care (NULL)
        if (match)
        {
            // Obtains the number of deviceType devices available on platform
            // When the function failed we expect numDevices to be zero.
            // We ignore the function return value since a non-zero error code
            // could happen if this platform doesn't support the specified device type.
            err = clGetDeviceIDs(platforms[i], deviceType, 0, NULL, &numDevices);
            if (CL_SUCCESS != err)
            {
                LogError("clGetDeviceIDs() returned %s.\n", TranslateOpenCLError(err));
            }

            if (0 != numDevices)
            {
                // There is at list one device that answer the requirements
                return platforms[i];
            }
        }
    }

    return NULL;
}


/*
 * This function read the OpenCL platdorm and device versions
 * (using clGetxxxInfo API) and stores it in the ocl structure.
 * Later it will enable us to support both OpenCL 1.2 and 2.0 platforms and devices
 * in the same program.
 */
int GetPlatformAndDeviceVersion (cl_platform_id platformId, ocl_args_d_t *ocl)
{
    cl_int err = CL_SUCCESS;

    // Read the platform's version string length (param_value is NULL).
    // The value returned in stringLength
    size_t stringLength = 0;
    err = clGetPlatformInfo(platformId, CL_PLATFORM_VERSION, 0, NULL, &stringLength);
    if (CL_SUCCESS != err)
    {
        LogError("Error: clGetPlatformInfo() to get CL_PLATFORM_VERSION length returned '%s'.\n", TranslateOpenCLError(err));
        return err;
    }

    // Now, that we know the platform's version string length, we can allocate enough space before read it
    std::vector<char> platformVersion(stringLength);

    // Read the platform's version string
    // The read value returned in platformVersion
    err = clGetPlatformInfo(platformId, CL_PLATFORM_VERSION, stringLength, &platformVersion[0], NULL);
    if (CL_SUCCESS != err)
    {
        LogError("Error: clGetplatform_ids() to get CL_PLATFORM_VERSION returned %s.\n", TranslateOpenCLError(err));
        return err;
    }

    if (strstr(&platformVersion[0], "OpenCL 2.0") != NULL)
    {
        ocl->platformVersion = OPENCL_VERSION_2_0;
    }

    // Read the device's version string length (param_value is NULL).
    err = clGetDeviceInfo(ocl->device, CL_DEVICE_VERSION, 0, NULL, &stringLength);
    if (CL_SUCCESS != err)
    {
        LogError("Error: clGetDeviceInfo() to get CL_DEVICE_VERSION length returned '%s'.\n", TranslateOpenCLError(err));
        return err;
    }

    // Now, that we know the device's version string length, we can allocate enough space before read it
    std::vector<char> deviceVersion(stringLength);

    // Read the device's version string
    // The read value returned in deviceVersion
    err = clGetDeviceInfo(ocl->device, CL_DEVICE_VERSION, stringLength, &deviceVersion[0], NULL);
    if (CL_SUCCESS != err)
    {
        LogError("Error: clGetDeviceInfo() to get CL_DEVICE_VERSION returned %s.\n", TranslateOpenCLError(err));
        return err;
    }

    if (strstr(&deviceVersion[0], "OpenCL 2.0") != NULL)
    {
        ocl->deviceVersion = OPENCL_VERSION_2_0;
    }

    // Read the device's OpenCL C version string length (param_value is NULL).
    err = clGetDeviceInfo(ocl->device, CL_DEVICE_OPENCL_C_VERSION, 0, NULL, &stringLength);
    if (CL_SUCCESS != err)
    {
        LogError("Error: clGetDeviceInfo() to get CL_DEVICE_OPENCL_C_VERSION length returned '%s'.\n", TranslateOpenCLError(err));
        return err;
    }

    // Now, that we know the device's OpenCL C version string length, we can allocate enough space before read it
    std::vector<char> compilerVersion(stringLength);

    // Read the device's OpenCL C version string
    // The read value returned in compilerVersion
    err = clGetDeviceInfo(ocl->device, CL_DEVICE_OPENCL_C_VERSION, stringLength, &compilerVersion[0], NULL);
    if (CL_SUCCESS != err)
    {
        LogError("Error: clGetDeviceInfo() to get CL_DEVICE_OPENCL_C_VERSION returned %s.\n", TranslateOpenCLError(err));
        return err;
    }

    else if (strstr(&compilerVersion[0], "OpenCL C 2.0") != NULL)
    {
        ocl->compilerVersion = OPENCL_VERSION_2_0;
    }

    return err;
}


/*
 * Generate random value for input buffers
 */
void initParticles()
{

	for (cl_uint i = 0; i < NUM_PARTICLES; i++) {

		
		particle_pos[i] = vec4(0.f, 0.f, 0.f, 0.f);
		particle_vel[i] = vec4(0.f, 0.f, 0.f, 0.f);

		//Forces
		particle_grav_f[i] = vec4(0.f, 0.0f, 0.f, 0.f);
		particle_visc_f[i] = vec4(0.f, 0.f, 0.f, 0.f);
		particle_press_f[i] = vec4(0.f, 0.f, 0.f, 0.f);

		//Defines
		particle_density[i] = 0;
		particle_pressure[i] = 0;

	}
	// Set positions
	int x = 0, y = 0, z = 0;

	for (int i = 0; i < NUM_PARTICLES - MAX_ADDED_PARTICLES; i++){

		if (x == 10){
			y++;
			x = 0;
		}

		if (y == 10){
			z++; // z + 5;
			y = 0;
			//printf("Z position %i \n", z);
		}

		x++;
		
		//		k * x < 256, l * y < 256, m * z < 256 -> 
		//	->	k > 256 / x



		particle_pos[i] = (vec4(20.0 + x*0.1f, 100.f + y*0.1f, 20.0 + z*0.1f, 0));
	}

}


int generateNewParticles(){

	
	
	if (ADDED_PARTICLES >= MAX_ADDED_PARTICLES)
	{
		printf("Number of added Particles reached Limit \n");
		return -1;
	}
	


	cl_int i = ADDED_PARTICLES + BEGIN_PARTICLES;

	// Set positions
	particle_pos[i] = (vec4(64.f, 256.f, 256.f, 0));
	particle_vel[i] = (vec4(-550.f, -50.0, -700.f, 0));
	ADDED_PARTICLES++;



}


/*
 * This function picks/creates necessary OpenCL objects which are needed.
 * The objects are:
 * OpenCL platform, device, context, and command queue.
 *
 * All these steps are needed to be performed once in a regular OpenCL application.
 * This happens before actual compute kernels calls are performed.
 *
 * For convenience, in this application you store all those basic OpenCL objects in structure ocl_args_d_t,
 * so this function populates fields of this structure, which is passed as parameter ocl.
 * Please, consider reviewing the fields before going further.
 * The structure definition is right in the beginning of this file.
 */
int SetupOpenCL(ocl_args_d_t *ocl, cl_device_type deviceType)
{
    // The following variable stores return codes for all OpenCL calls.
    cl_int err = CL_SUCCESS;

    // Query for all available OpenCL platforms on the system
    // Here you enumerate all platforms and pick one which name has preferredPlatform as a sub-string
    cl_platform_id platformId = FindOpenCLPlatform("Intel", deviceType);
    if (NULL == platformId)
    {
        LogError("Error: Failed to find OpenCL platform.\n");
        return CL_INVALID_VALUE;
    }

    // Create context with device of specified type.
    // Required device type is passed as function argument deviceType.
    // So you may use this function to create context for any CPU or GPU OpenCL device.
    // The creation is synchronized (pfn_notify is NULL) and NULL user_data
    cl_context_properties contextProperties[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platformId, 0};
    ocl->context = clCreateContextFromType(contextProperties, deviceType, NULL, NULL, &err);
    if ((CL_SUCCESS != err) || (NULL == ocl->context))
    {
        LogError("Couldn't create a context, clCreateContextFromType() returned '%s'.\n", TranslateOpenCLError(err));
        return err;
    }

    // Query for OpenCL device which was used for context creation
    err = clGetContextInfo(ocl->context, CL_CONTEXT_DEVICES, sizeof(cl_device_id), &ocl->device, NULL);
    if (CL_SUCCESS != err)
    {
        LogError("Error: clGetContextInfo() to get list of devices returned %s.\n", TranslateOpenCLError(err));
        return err;
    }

    // Read the OpenCL platform's version and the device OpenCL and OpenCL C versions
    GetPlatformAndDeviceVersion(platformId, ocl);

    // Create command queue.
    // OpenCL kernels are enqueued for execution to a particular device through special objects called command queues.
    // Command queue guarantees some ordering between calls and other OpenCL commands.
    // Here you create a simple in-order OpenCL command queue that doesn't allow execution of two kernels in parallel on a target device.
#ifdef CL_VERSION_2_0
    if (OPENCL_VERSION_2_0 == ocl->deviceVersion)
    {
        const cl_command_queue_properties properties[] = {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};
        ocl->commandQueue = clCreateCommandQueueWithProperties(ocl->context, ocl->device, properties, &err);
    } 
    else {
        // default behavior: OpenCL 1.2
        cl_command_queue_properties properties = CL_QUEUE_PROFILING_ENABLE;
        ocl->commandQueue = clCreateCommandQueue(ocl->context, ocl->device, properties, &err);
    } 
#else
    // default behavior: OpenCL 1.2
    cl_command_queue_properties properties = CL_QUEUE_PROFILING_ENABLE;
    ocl->commandQueue = clCreateCommandQueue(ocl->context, ocl->device, properties, &err);
#endif
    if (CL_SUCCESS != err)
    {
        LogError("Error: clCreateCommandQueue() returned %s.\n", TranslateOpenCLError(err));
        return err;
    }

    return CL_SUCCESS;
}


/* 
 * Create and build OpenCL program from its source code
 */
int CreateAndBuildProgram(ocl_args_d_t *ocl)
{
    cl_int err = CL_SUCCESS;

    // Upload the OpenCL C source code from the input file to source
    // The size of the C program is returned in sourceSize
    char* source = NULL;
    size_t src_size = 0;
    err = ReadSourceFromFile("ParticleKernels.cl", &source, &src_size);
    if (CL_SUCCESS != err)
    {
        LogError("Error: ReadSourceFromFile returned %s.\n", TranslateOpenCLError(err));
        goto Finish;
    }

    // And now after you obtained a regular C string call clCreateProgramWithSource to create OpenCL program object.
    ocl->program = clCreateProgramWithSource(ocl->context, 1, (const char**)&source, &src_size, &err);
    if (CL_SUCCESS != err)
    {
        LogError("Error: clCreateProgramWithSource returned %s.\n", TranslateOpenCLError(err));
        goto Finish;
    }

    // Build the program
    // During creation a program is not built. You need to explicitly call build function.
    // Here you just use create-build sequence,
    // but there are also other possibilities when program consist of several parts,
    // some of which are libraries, and you may want to consider using clCompileProgram and clLinkProgram as
    // alternatives.
    err = clBuildProgram(ocl->program, 1, &ocl->device, "", NULL, NULL);
    if (CL_SUCCESS != err)
    {
        LogError("Error: clBuildProgram() for source program returned %s.\n", TranslateOpenCLError(err));

        // In case of error print the build log to the standard output
        // First check the size of the log
        // Then allocate the memory and obtain the log from the program
        if (err == CL_BUILD_PROGRAM_FAILURE)
        {
            size_t log_size = 0;
            clGetProgramBuildInfo(ocl->program, ocl->device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

            std::vector<char> build_log(log_size);
            clGetProgramBuildInfo(ocl->program, ocl->device, CL_PROGRAM_BUILD_LOG, log_size, &build_log[0], NULL);

            LogError("Error happened during the build of OpenCL program.\nBuild log:%s", &build_log[0]);
        }
    }

Finish:
    if (source)
    {
        delete[] source;
        source = NULL;
    }

    return err;
}


/*
 * Create OpenCL buffers from host memory
 * These buffers will be used later by the OpenCL kernel
 */
int CreateBufferArguments(ocl_args_d_t *ocl, vec4* particle_pos, vec4* particle_vel, vec4* particle_visc_f,
	vec4* particle_press_f, vec4* particle_grav_f,
	float* particle_pressure,
	cl_float* particle_density, Cell* cells)
{
    cl_int err = CL_SUCCESS;

    // Create new OpenCL buffer objects
    // As these buffer are used only for read by the kernel, you are recommended to create it with flag CL_MEM_READ_ONLY.
    // Always set minimal read/write flags for buffers, it may lead to better performance because it allows runtime
    // to better organize data copying.
    // You use CL_MEM_COPY_HOST_PTR here, because the buffers should be populated with bytes at particle_pos and particle_vel.

	//Changed from CL_MEM_READ_ONLY to CL_MEM_READ_WRITE because we want to read vel and position and change it 

	ocl->pos_b = clCreateBuffer(ocl->context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(vec4) * NUM_PARTICLES , particle_pos, &err);
    if (CL_SUCCESS != err)
    {
        LogError("Error: clCreateBuffer for pos_b returned %s\n", TranslateOpenCLError(err));
        return err;
    }
	

	ocl->vel_b = clCreateBuffer(ocl->context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(vec4) * NUM_PARTICLES, particle_vel, &err);
    if (CL_SUCCESS != err)
    {
        LogError("Error: clCreateBuffer for vel_b returned %s\n", TranslateOpenCLError(err));
        return err;
    }

	ocl->pressure_force_b = clCreateBuffer(ocl->context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(vec4) * NUM_PARTICLES, particle_visc_f, &err);
	if (CL_SUCCESS != err)
	{
		LogError("Error: clCreateBuffer for pressure_force_b returned %s\n", TranslateOpenCLError(err));
		return err;
	}
	ocl->viscosity_force_b = clCreateBuffer(ocl->context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(vec4) * NUM_PARTICLES, particle_press_f, &err);
	if (CL_SUCCESS != err)
	{
		LogError("Error: clCreateBuffer for viscosity_force_b returned %s\n", TranslateOpenCLError(err));
		return err;
	}
	ocl->gravity_force_b = clCreateBuffer(ocl->context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(vec4) * NUM_PARTICLES, particle_grav_f, &err);
	if (CL_SUCCESS != err)
	{
		LogError("Error: clCreateBuffer for gravity_force_b returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	ocl->pressure_b = clCreateBuffer(ocl->context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(cl_float) * NUM_PARTICLES, particle_pressure, &err);
    if (CL_SUCCESS != err)
    {
        LogError("Error: clCreateBuffer for pressure_b returned %s\n", TranslateOpenCLError(err));
        return err;
    }
	ocl->density_b = clCreateBuffer(ocl->context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(cl_float) * NUM_PARTICLES, particle_density, &err);
	if (CL_SUCCESS != err)
	{
		LogError("Error: clCreateBuffer for density_b returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	ocl->cells_b = clCreateBuffer(ocl->context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(Cell) * NUM_CELLS, cells , &err);
	if (CL_SUCCESS != err)
	{
		LogError("Error: clCreateBuffer for cells_b returned %s\n", TranslateOpenCLError(err));
		return err;
	}
	ocl->grav_dir_b = clCreateBuffer(ocl->context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(cl_float) * 4, gravity_dir, &err);
	if (CL_SUCCESS != err)
	{
		LogError("Error: clCreateBuffer for grav_dir_b returned %s\n", TranslateOpenCLError(err));
		return err;
	}


    return CL_SUCCESS;
}


/*
 * Set kernel arguments
 */
cl_uint SetKernelArguments(ocl_args_d_t *ocl)
{
    cl_int err = CL_SUCCESS;
	//

	////Position Arg
    err  =  clSetKernelArg(ocl->kernel, 0, sizeof(cl_mem), (void *)&ocl->pos_b);
    if (CL_SUCCESS != err)
    {
        LogError("error: Failed to set argument pos_b, returned %s\n", TranslateOpenCLError(err));
        return err;
    }
	////Position Arg AccelerationKernel
	err = clSetKernelArg(ocl->kernel2, 0, sizeof(cl_mem), (void *)&ocl->pos_b);
	if (CL_SUCCESS != err)
	{
		LogError("error: Failed to set argument pos_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}
	////Position Arg MoveParticles Kernel
	err = clSetKernelArg(ocl->kernel3, 0, sizeof(cl_mem), (void *)&ocl->pos_b);
	if (CL_SUCCESS != err)
	{
		LogError("error: Failed to set argument pos_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}
	////Position Arg setCellParticles Kernel
	err = clSetKernelArg(ocl->kernel4, 0, sizeof(cl_mem), (void *)&ocl->pos_b);
	if (CL_SUCCESS != err)
	{
		LogError("error: Failed to set argument pos_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}
	////Velocity Arg
    err  = clSetKernelArg(ocl->kernel, 1, sizeof(cl_mem), (void *)&ocl->vel_b);
    if (CL_SUCCESS != err)
    {
        LogError("Error: Failed to set argument vel_b, returned %s\n", TranslateOpenCLError(err));
        return err;
    }
	////Velocity Arg accelerationKernel
	err = clSetKernelArg(ocl->kernel2, 1, sizeof(cl_mem), (void *)&ocl->vel_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument vel_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}
	////Velocity Arg Moveparticles Kernel
	err = clSetKernelArg(ocl->kernel3, 1, sizeof(cl_mem), (void *)&ocl->vel_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument vel_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}
	//setCellParticles Arg kernel
	err = clSetKernelArg(ocl->kernel4, 1, sizeof(cl_mem), (void *)&ocl->cells_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument cells_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	//Viscosity Force Arg
	err = clSetKernelArg(ocl->kernel, 2, sizeof(cl_mem), (void *)&ocl->viscosity_force_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument viscosity_force_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}
	//Viscosity Force Arg Acceleration Kernel
	err = clSetKernelArg(ocl->kernel2, 2, sizeof(cl_mem), (void *)&ocl->viscosity_force_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument viscosity_force_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}
	//Viscosity Force Arg MoveParticles Kernel
	err = clSetKernelArg(ocl->kernel3, 2, sizeof(cl_mem), (void *)&ocl->viscosity_force_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument viscosity_force_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	//Pressure Force Arg 
	err = clSetKernelArg(ocl->kernel, 3, sizeof(cl_mem), (void *)&ocl->pressure_force_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument pressure_force_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}
	//Pressure Force Arg Acceleration Kernel
	err = clSetKernelArg(ocl->kernel2, 3, sizeof(cl_mem), (void *)&ocl->pressure_force_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument pressure_force_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}
	//Pressure Force Arg Moveparticles Kernel
	err = clSetKernelArg(ocl->kernel3, 3, sizeof(cl_mem), (void *)&ocl->pressure_force_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument pressure_force_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}


	//Gravity Force Arg
	err = clSetKernelArg(ocl->kernel, 4, sizeof(cl_mem), (void *)&ocl->gravity_force_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument gravity_force_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	//Gravity Force Arg ACceleration Kernel
	err = clSetKernelArg(ocl->kernel2, 4, sizeof(cl_mem), (void *)&ocl->gravity_force_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument gravity_force_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}
	//Gravity Force Arg MoveParticles Kernel
	err = clSetKernelArg(ocl->kernel3, 4, sizeof(cl_mem), (void *)&ocl->gravity_force_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument gravity_force_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}
	//Pressure Arg
    err  = clSetKernelArg(ocl->kernel, 5, sizeof(cl_mem), (void *)&ocl->pressure_b);
    if (CL_SUCCESS != err)
    {
        LogError("Error: Failed to set argument pressure_b, returned %s\n", TranslateOpenCLError(err));
        return err;
    }
	//Pressure Arg acceleretion Kernel
	err = clSetKernelArg(ocl->kernel2, 5, sizeof(cl_mem), (void *)&ocl->pressure_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument pressure_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}
	//Pressure Arg MoveParticles Kernel
	err = clSetKernelArg(ocl->kernel3, 5, sizeof(cl_mem), (void *)&ocl->pressure_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument pressure_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	//Density Argk
	err = clSetKernelArg(ocl->kernel, 6, sizeof(cl_mem), (void *)&ocl->density_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument density_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}
	//Density Argk Acceleration Kernel
	err = clSetKernelArg(ocl->kernel2, 6, sizeof(cl_mem), (void *)&ocl->density_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument density_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}
	//Density Argk MoveParticles Kernel
	err = clSetKernelArg(ocl->kernel3, 6, sizeof(cl_mem), (void *)&ocl->density_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument density_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	//Cell Arg Density and Pressure kernel
	err = clSetKernelArg(ocl->kernel, 7, sizeof(cl_mem), (void *)&ocl->cells_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument cells_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	//Cell Arg Acceleration kernel
	err = clSetKernelArg(ocl->kernel2, 7, sizeof(cl_mem), (void *)&ocl->cells_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument cells_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}


	//Cell Arg Move Particles kernel
	err = clSetKernelArg(ocl->kernel3, 7, sizeof(cl_mem), (void *)&ocl->cells_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument cells_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}


	//Gravity Direction Arg acceleretion Kernel
	err = clSetKernelArg(ocl->kernel2, 8, sizeof(cl_mem), (void *)&ocl->grav_dir_b);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to set argument grav_dir_b, returned %s\n", TranslateOpenCLError(err));
		return err;
	}
    return err;
}

cl_uint ExecuteSetCellParticlesKernel(ocl_args_d_t *ocl)
{
	cl_int err = CL_SUCCESS;

	// Define global iteration space for clEnqueueNDRangeKernel.
	size_t globalWorkSize[1] = { BEGIN_PARTICLES + ADDED_PARTICLES };


	// execute kernel
	err = clEnqueueNDRangeKernel(ocl->commandQueue, ocl->kernel4, 1, NULL, globalWorkSize, NULL, 0, NULL, NULL);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to run kernel, return %s\n", TranslateOpenCLError(err));
		return err;
	}

	// Wait until the queued kernel is completed by the device
	err = clFinish(ocl->commandQueue);


	if (CL_SUCCESS != err)
	{
		LogError("Error: clFinish return %s\n", TranslateOpenCLError(err));
		return err;
	}
	/*else
	{
	std::cout << "FINISHED KERNEL EXECUTE SUCCESS" << std::endl;
	}
	*/
	return CL_SUCCESS;
}

/*
 * Execute the Density and Pressure kernel
 */
cl_uint ExecuteDensAndPressKernel(ocl_args_d_t *ocl)
{
    cl_int err = CL_SUCCESS;

    // Define global iteration space for clEnqueueNDRangeKernel.
	size_t globalWorkSize[1] = { BEGIN_PARTICLES + ADDED_PARTICLES };


    // execute kernel
    err = clEnqueueNDRangeKernel(ocl->commandQueue, ocl->kernel, 1, NULL, globalWorkSize, NULL, 0, NULL, NULL);
    if (CL_SUCCESS != err)
    {
        LogError("Error: Failed to run kernel, return %s\n", TranslateOpenCLError(err));
        return err;
    }

    // Wait until the queued kernel is completed by the device
    err = clFinish(ocl->commandQueue);

	
    if (CL_SUCCESS != err)
    {
        LogError("Error: clFinish return %s\n", TranslateOpenCLError(err));
        return err;
	}
	/*else
	{
		std::cout << "FINISHED KERNEL EXECUTE SUCCESS" << std::endl;
	}
*/
    return CL_SUCCESS;
}
/*
* Execute the Density and Pressure kernel
*/
cl_uint ExecuteAccelerationKernel(ocl_args_d_t *ocl)
{
	cl_int err = CL_SUCCESS;

	// Define global iteration space for clEnqueueNDRangeKernel.
	size_t globalWorkSize[1] = { BEGIN_PARTICLES + ADDED_PARTICLES };


	// execute kernel
	err = clEnqueueNDRangeKernel(ocl->commandQueue, ocl->kernel2, 1, NULL, globalWorkSize, NULL, 0, NULL, NULL);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to run kernel, return %s\n", TranslateOpenCLError(err));
		return err;
	}

	// Wait until the queued kernel is completed by the device
	err = clFinish(ocl->commandQueue);


	if (CL_SUCCESS != err)
	{
		LogError("Error: clFinish return %s\n", TranslateOpenCLError(err));
		return err;
	}
	/*else
	{
	std::cout << "FINISHED KERNEL EXECUTE SUCCESS" << std::endl;
	}
	*/
	return CL_SUCCESS;
}

cl_uint ExecuteMoveParticleKernel(ocl_args_d_t *ocl)
{
	cl_int err = CL_SUCCESS;

	// Define global iteration space for clEnqueueNDRangeKernel.
	size_t globalWorkSize[1] = { BEGIN_PARTICLES + ADDED_PARTICLES };


	// execute kernel
	err = clEnqueueNDRangeKernel(ocl->commandQueue, ocl->kernel3, 1, NULL, globalWorkSize, NULL, 0, NULL, NULL);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to run kernel, return %s\n", TranslateOpenCLError(err));
		return err;
	}

	// Wait until the queued kernel is completed by the device
	err = clFinish(ocl->commandQueue);


	if (CL_SUCCESS != err)
	{
		LogError("Error: clFinish return %s\n", TranslateOpenCLError(err));
		return err;
	}
	/*else
	{
	std::cout << "FINISHED KERNEL EXECUTE SUCCESS" << std::endl;
	}
	*/
	return CL_SUCCESS;
}


/*
 * "Read" the result buffer (mapping the buffer to the host memory address)
 */
bool ReadAndVerify(ocl_args_d_t *ocl)
{
    cl_int err = CL_SUCCESS;
    bool result = true;

    // Enqueue a command to map the buffer object (ocl->pressure_b) into the host address space and returns a pointer to it
    // The map operation is blocking
    
	
	vec4 *resultParticle_pos = (vec4*)clEnqueueMapBuffer(ocl->commandQueue, ocl->pos_b, true, CL_MAP_READ, 0, sizeof(vec4) * NUM_PARTICLES, 0, NULL, NULL, &err);
	vec4 *resultParticle_vel = (vec4*)clEnqueueMapBuffer(ocl->commandQueue, ocl->vel_b, true, CL_MAP_READ, 0, sizeof(vec4) * NUM_PARTICLES, 0, NULL, NULL, &err);
	vec4 *resultParticle_visc_f = (vec4*)clEnqueueMapBuffer(ocl->commandQueue, ocl->viscosity_force_b, true, CL_MAP_READ, 0, sizeof(vec4) * NUM_PARTICLES, 0, NULL, NULL, &err);
	vec4 *resultParticle_press_f = (vec4*)clEnqueueMapBuffer(ocl->commandQueue, ocl->pressure_force_b, true, CL_MAP_READ, 0, sizeof(vec4) * NUM_PARTICLES, 0, NULL, NULL, &err);
	vec4 *resultParticle_grav_f = (vec4*)clEnqueueMapBuffer(ocl->commandQueue, ocl->gravity_force_b, true, CL_MAP_READ, 0, sizeof(vec4) * NUM_PARTICLES, 0, NULL, NULL, &err);
	cl_float *resultParticle_pressure = (cl_float*)clEnqueueMapBuffer(ocl->commandQueue, ocl->pressure_b, true, CL_MAP_READ, 0, sizeof(cl_float) * NUM_PARTICLES, 0, NULL, NULL, &err);
	cl_float *resultParticle_density = (cl_float*)clEnqueueMapBuffer(ocl->commandQueue, ocl->density_b, true, CL_MAP_READ, 0, sizeof(cl_float) * NUM_PARTICLES, 0, NULL, NULL, &err);
	Cell* resultCell = (Cell*)clEnqueueMapBuffer(ocl->commandQueue, ocl->cells_b, true, CL_MAP_READ, 0, sizeof(Cell) * NUM_CELLS, 0, NULL, NULL, &err);
	//GLfloat resultGravDir = (GLfloat)clEnqueueMapBuffer(ocl->commandQueue, ocl->grav_dir_b, true, CL_MAP_READ, 0, sizeof(GLfloat) * 4, 0, NULL, NULL, &err);


	particle_pos = resultParticle_pos;
	particle_vel = resultParticle_vel;
	particle_visc_f = resultParticle_visc_f;
	particle_press_f = resultParticle_press_f;
	particle_grav_f = resultParticle_grav_f;

	//particle_grav_f = resultParticle_grav_f;

	particle_pressure = resultParticle_pressure;
	particle_density = resultParticle_density;

	cells = resultCell;
	


	
	//idle();


    if (CL_SUCCESS != err)
    {
        LogError("Error: clEnqueueMapBuffer returned %s\n", TranslateOpenCLError(err));
        return false;
    }

    // Call clFinish to guarantee that output region is updated
    err = clFinish(ocl->commandQueue);
    if (CL_SUCCESS != err)
    {
        LogError("Error: clFinish returned %s\n", TranslateOpenCLError(err));
    }

    // MAKE THE MAGIC
	/*for (unsigned int k = 0; k  <NUM_PARTICLES; ++k)
	{

		printf("Density VALUE : %f\n", resultParticle_pos[k][0]);
     
    }*/

     // Unmapped the output buffer before releasing it
	err = clEnqueueUnmapMemObject(ocl->commandQueue, ocl->pos_b, resultParticle_pos, 0, NULL, NULL);
	err = clEnqueueUnmapMemObject(ocl->commandQueue, ocl->vel_b, resultParticle_vel, 0, NULL, NULL);
	err = clEnqueueUnmapMemObject(ocl->commandQueue, ocl->viscosity_force_b, resultParticle_visc_f, 0, NULL, NULL);
	err = clEnqueueUnmapMemObject(ocl->commandQueue, ocl->pressure_force_b, resultParticle_press_f, 0, NULL, NULL);
	err = clEnqueueUnmapMemObject(ocl->commandQueue, ocl->gravity_force_b, resultParticle_grav_f, 0, NULL, NULL);
	err = clEnqueueUnmapMemObject(ocl->commandQueue, ocl->pressure_b, resultParticle_pressure, 0, NULL, NULL);
	err = clEnqueueUnmapMemObject(ocl->commandQueue, ocl->density_b, resultParticle_density, 0, NULL, NULL);
	err = clEnqueueUnmapMemObject(ocl->commandQueue, ocl->cells_b, resultCell, 0, NULL, NULL);
    if (CL_SUCCESS != err)
    {
        LogError("Error: clEnqueueUnmapMemObject returned %s\n", TranslateOpenCLError(err));
    }
	

    return result;
}

int executeOnGPU(ocl_args_d_t *ocl){

	// Regularly you wish to use OpenCL in your application to achieve greater performance results
	// that are hard to achieve in other ways.
	// To understand those performance benefits you may want to measure time your application spent in OpenCL kernel execution.
	// The recommended way to obtain this time is to measure interval between two moments:
	//   - just before clEnqueueNDRangeKernel is called, and
	//   - just after clFinish is called
	// clFinish is necessary to measure entire time spending in the kernel, measuring just clEnqueueNDRangeKernel is not enough,
	// because this call doesn't guarantees that kernel is finished.
	// clEnqueueNDRangeKernel is just enqueue new command in OpenCL command queue and doesn't wait until it ends.
	// clFinish waits until all commands in command queue are finished, that suits your need to measure time.

	//EXECUTE SetCellParticles KERNEL
	// Execute (enqueue) the kernel
	if (CL_SUCCESS != ExecuteSetCellParticlesKernel(ocl))
	{
		return -1;
	}
	//Get values from Set cell particle kernel
	ReadAndVerify(ocl);

	//EXECUTE DENSITY AND PRESSURE KERNEL
	// Execute (enqueue) the kernel
	if (CL_SUCCESS != ExecuteDensAndPressKernel(ocl))
	{
		return -1;
	}
	//Get values from Dens and pressure kernel
	ReadAndVerify(ocl);

	//EXECUTE ACCELERATIONKERNEL 
;
	if (CL_SUCCESS != ExecuteAccelerationKernel(ocl))
	{
		return -1;
	}
	//Get values from Acceleration kernel
	ReadAndVerify(ocl);

	//EXECUTE MOVE PARTICLE KERNEL ;
	// Execute (enqueue) the kernel
	if (CL_SUCCESS != ExecuteMoveParticleKernel(ocl))
	{
		return -1;
	}

	//Get values from moveparticles kernel
	ReadAndVerify(ocl);

	/*for (int i = 0; i < NUM_PARTICLES; i++){
		particle_grav_f[i] = vec4(particle_grav_f[i][0] * gravity_dir[0], particle_grav_f[i][1] * gravity_dir[1], particle_grav_f[i][2] * gravity_dir[2], 0)*GRAVITY_CONST;
	}*/
	
}

/*void renderString(void * font, std::string k, int posx, int posy){

	glRasterPos2i(posx, posy);

	for (std::string::iterator i = k.begin(); i != k.end(); i++){
		char c = *i;

		glutBitmapCharacter(font, c);


	}
}*/


/*
 * main execution routine
 * Basically it consists of three parts:
 *   - generating the inputs
 *   - running OpenCL kernel
 *   - reading results of processing
 */
int _tmain(int argc, TCHAR* argv[])
{
    cl_int err;
    ocl_args_d_t ocl;
    cl_device_type deviceType = CL_DEVICE_TYPE_GPU;

    LARGE_INTEGER perfFrequency;
    LARGE_INTEGER performanceCountNDRangeStart;
    LARGE_INTEGER performanceCountNDRangeStop;

	


    //initialize Open CL objects (context, queue, etc.)
    if (CL_SUCCESS != SetupOpenCL(&ocl, deviceType))
    {
        return -1;
    }

    // allocate working buffers. 
    // the buffer should be aligned with 4K page and size should fit 64-byte cached line
	float optimizedSize = ((sizeof(vec4) * NUM_PARTICLES - 1) / 64 + 1) * 64;
	particle_pos = (vec4*)_aligned_malloc(optimizedSize, 4096);
	particle_vel = (vec4*)_aligned_malloc(optimizedSize, 4096);
	particle_visc_f = (vec4*)_aligned_malloc(optimizedSize, 4096);
	particle_press_f = (vec4*)_aligned_malloc(optimizedSize, 4096);
	particle_grav_f = (vec4*)_aligned_malloc(optimizedSize, 4096);
	particle_density = (cl_float*)_aligned_malloc(optimizedSize, 4096);
	particle_pressure = (cl_float*)_aligned_malloc(optimizedSize, 4096);
	float optimizedSizeCell = ((sizeof(Cell) * NUM_CELLS - 1) / 64 + 1) * 64;
	cells = (Cell*)_aligned_malloc(optimizedSizeCell, 4096);

	
	/*
    if (NULL == particle_pos || NULL == particle_vel)
    {
        LogError("Error: _aligned_malloc failed to allocate buffers.\n");
        return -1;
    }*/
	
    //Initialize particles , In this order!
    initParticles(); //particle_pos, particle_vel, particle_visc_f, particle_press_f, particle_grav_f,  particle_density, particle_pressure
    
	////Set Cells and neighbours
	initNeighbours();

    // Create OpenCL buffers from host memory
    // These buffers will be used later by the OpenCL kernel
    if (CL_SUCCESS != CreateBufferArguments(&ocl, particle_pos, particle_vel, particle_visc_f, particle_press_f, particle_grav_f ,particle_pressure, particle_density, cells))
    {
        return -1;
    }

     // Create and build the OpenCL program
    if (CL_SUCCESS != CreateAndBuildProgram(&ocl))
    {
        return -1;
    }

    // Program consists of kernels.
    // Each kernel can be called (enqueued) from the host part of OpenCL application.
    // To call the kernel, you need to create it from existing program.
    ocl.kernel = clCreateKernel(ocl.program, "calculateDensityAndPressure", &err);
    if (CL_SUCCESS != err)
    {
        LogError("Error: clCreateKernel returned %s\n", TranslateOpenCLError(err));
        return -1;
    }
	ocl.kernel2 = clCreateKernel(ocl.program, "calculateAccelerations", &err);
	if (CL_SUCCESS != err)
	{
		LogError("Error: clCreateKernel returned %s\n", TranslateOpenCLError(err));
		return -1;
	}
	ocl.kernel3 = clCreateKernel(ocl.program, "moveParticles", &err);
	if (CL_SUCCESS != err)
	{
		LogError("Error: clCreateKernel returned %s\n", TranslateOpenCLError(err));
		return -1;
	}

	ocl.kernel4 = clCreateKernel(ocl.program, "setCellParticles", &err);
	if (CL_SUCCESS != err)
	{
		LogError("Error: clCreateKernel returned %s\n", TranslateOpenCLError(err));
		return -1;
	}

    // Passing arguments into OpenCL kernel.
    if (CL_SUCCESS != SetKernelArguments(&ocl))
    {
        return -1;
    }

	
	
	//Execute Kernels on GPU
	executeOnGPU(&ocl);
	
	//From main projekt
	glfwInit();

	window = glfwCreateWindow(700, 700, "OpenGL", nullptr, nullptr); // Windowed
	glfwMakeContextCurrent(window);
	glfwSetWindowPos(window, 100, 20);
	glfwSwapInterval(1);


	//DEFAULT CAMERA ROTATE and TRANSLATE
	glMatrixMode(GL_MODELVIEW);
	float angle = 0.0;
	angle += 360. / 10.;

	// set camera parameters
	GLdouble eyeX = .5*cos(angle);
	GLdouble eyeY = .5*sin(angle);
	GLdouble eyeZ = 0.;
	GLdouble centerX = 128.f;
	GLdouble centerY = 128.f;
	GLdouble centerZ = 128.f;
	GLdouble upX = 0.;
	GLdouble upY = 1.;
	GLdouble upZ = 0.;

	gluLookAt(eyeX, eyeY, eyeZ,
		centerX, centerY, centerZ,
		upX, upY, upZ);
	glTranslatef(-512.f, 128.f, 0);


	// Lightning set up
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glShadeModel(GL_FLAT);

	// St lightning internsity and color
	GLfloat ambient[] = { 0.2, 0.2, 0.2, 1.0 };
	GLfloat diffuse[] = { 0.8, 0.8, 0.8, 1.0 };
	GLfloat specular[] = { 1.0, 1.0, 1.0, 1.0 };

	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT1, GL_SPECULAR, specular);

	GLfloat pos[] = { 1024.0, 1024.0, 1024.0, 1.0 };
	GLfloat pos1[] = { -1024.0, -1024.0, -1024.0, 1.0 };
	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	glLightfv(GL_LIGHT1, GL_POSITION, pos1);


	//GLUT font 
	void * font = GLUT_BITMAP_9_BY_15;
	while (!glfwWindowShouldClose(window)){

		float ratio;
		int width, height;

		glfwGetFramebufferSize(window, &width, &height);
		ratio = width / (float)height;

		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);

		

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		//Display 
		/*glDisable(GL_LIGHTING);
		glPushMatrix();
			glLoadIdentity();
			gluOrtho2D(0.0, 1024, 0.0, 1024);

			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
				glLoadIdentity();

				glColor3f(1.0, 1.0, 1.0);

				renderString(font, "Move box with arrows", 750, 1000);
				renderString(font, "Rotate box with: A, S, D and W", 750, 980);
				renderString(font, "Carl Bildt this city", 750, 960);



				glMatrixMode(GL_MODELVIEW);
			glPopMatrix();

			glMatrixMode(GL_PROJECTION);
		glPopMatrix();

		glEnable(GL_LIGHTING);*/


		//glOrtho(0.0, 512.0, 0.0, 512.0, -1, 1);//2D ORTHO
		glOrtho(0.0, 1024.0, 0.0, 1024.0, -1024.0, 1024.0);
		handleInputs();
		// Get rotation matrix
		glGetFloatv(GL_MODELVIEW_MATRIX, model);
		calculateNewGravityVec();
		//printf("NUMBEROF PARTICLE %3.4f \n", gravity_dir[0]);
		
		//drawCoordinateAxes();

		//GPU STUFF
		executeOnGPU(&ocl);

		//for (int i = 0; i < NUM_CELLS; i++){
		//	printf("NUMBEROF PARTICLE %i \n " , cells[i].nrOfParticles);
		//	//Fill array of particle IDS with unvalid default value -1
		//	//std::fill(cells[i].particleIDs, cells[i].particleIDs + 1000, -1);
		//}
		/*if (frameCounterSpecialName++ % 10 != 0) {
			continue;
		}*/

		//END GPU STUFF
		//drawParticles();
		drawParticlesContainer();
		drawCubeParticles();
		
		
		
			
			
		//drawSphereParticles();
		//drawBetaBalls();

		//Swap front and back buffers
		glfwSetWindowSizeCallback(window, reshape_window);
		glfwSwapBuffers(window);

		//Poll for and process events
		glfwPollEvents();

	}


	glfwDestroyWindow(window);
	glfwTerminate();


    _aligned_free(particle_pos);
    _aligned_free(particle_vel);
	_aligned_free(particle_visc_f);
	_aligned_free(particle_press_f);
	_aligned_free(particle_grav_f);
	_aligned_free(particle_density);
	_aligned_free(particle_pressure);
	_aligned_free(cells);

    return 0;
}
