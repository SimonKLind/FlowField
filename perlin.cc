/** This creates a random flow field using my noise implementation
  * and then uses OpenGL to render particles that travel in said flow field */

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <ctime>
#include <cmath>
#include <shader.h>
#include <noise.h> // My noise implementation
#include <random>

const int W = 1366;
const int H = 768;
const int xSize = 112; // Amount of vectors along x
const int ySize = 64; // Amount of vectors along y
const double frameStep = 0.0001; // Determines how fast the "z-axis" of the noise changes
const double axisStep = 0.01; // Magnitude of x and y axis of noise
const int lineCount = 10000; // How many particles
const double maxVel = 0.001; // Max velocity of particles
const double pi = 3.1415926535;

/** A bare minimum vector class 
  * Capable of creating a vector 1/10th of 
  * maxVel in magnitude from a given angle 
  * operator+= is used to move particles */
class Vector{
public:
	GLfloat x, y;
	Vector(): x(0), y(0) {}
	void setAngle(double angle){
		x = cos(angle)*maxVel*0.01;
		y = sin(angle)*maxVel*0.01;
		// cout << "Angle: " << angle << " -> " << x << ", " << y << endl;
	}
	void operator+=(Vector other){
		x += other.x;
		y += other.y;
		double mag = sqrt(x*x+y*y);
		if(mag > maxVel){
			x *= maxVel/mag;
			y *= maxVel/mag;
		}
	}
};

/** Line struct used to pass line vertices to 
  * graphics card through vbo */
struct Line{
	GLfloat x1;
	GLfloat y1;
	GLfloat x2;
	GLfloat y2;
};

/** Lines class to keep track of all particles
  * Named Lines since a particle is a line to OpenGL */
class Lines{
public:
	Line *lines; // Particles
	Vector *vels; // Current velocities
	int size;
	
	/** Creates count number of particles and gives then random positions */
	Lines(int count): size(count){
		std::mt19937 rand(time(0));
		lines = new Line[count];
		vels = new Vector[count];
		for(int i=0; i<count; ++i){
			lines[i].x1 = lines[i].x2 = (float)rand()*2/rand.max()-1.0;
			lines[i].y1 = lines[i].y2 = (float)rand()*2/rand.max()-1.0;
		}
	}
	
	/** Updates all particles according to flow field 
	  * param: vectors - flow field vectors */
	void update(Vector **vectors){
		for(int i=0, x, y; i<size; ++i){
			// cout << lines[i].x1 << ", " << lines[i].y1 << " -> ";
			// cout << (lines[i].x1+1.0)*(float)W/2 << ", " << (lines[i].y1+1.0)*(float)H/2 << " -> ";
			x = (lines[i].x1 >= 1.0) ? xSize-1 : (int)((lines[i].x1+1.0)*(float)(W/2)/(float)(W/xSize));
			y = (lines[i].y1 >= 1.0) ? ySize-1 : (int)((lines[i].y1+1.0)*(float)(H/2)/(float)(H/ySize));
			// cout << x << ", " << y << endl;
			vels[i] += vectors[y][x];
			// cout << vels[i].x << ", " << vels[i].y << endl;
			setPrev(i);
			lines[i].x1 += vels[i].x;
			lines[i].y1 += vels[i].y;
			if(lines[i].x1 > 1.0f && vels[i].x > 0){
				lines[i].x1 = -1.0f;
				setPrev(i);
			}
			else if(lines[i].x1 < -1.0f && vels[i].x < 0){
				lines[i].x1 = 1.0f;
				setPrev(i);
			}
			if(lines[i].y1 > 1.0f && vels[i].y > 0){
				lines[i].y1 = -1.0f;
				setPrev(i);
			}
			else if(lines[i].y1 < -1.0f && vels[i].y < 0){
				lines[i].y1 = 1.0f;
				setPrev(i);
			}
			// cout << lines[i].x1 << ", " << lines[i].y1 << " <- " << lines[i].x2 << ", " << lines[i].y2 << endl;
		}
	}
	
	/** Just a little helper for updating previous particle positions */
	void setPrev(int index){
		lines[index].x2 = lines[index].x1;
		lines[index].y2 = lines[index].y1;
	}
	
	~Lines(){
		delete[] lines;
		delete[] vels;
	}
};

/** Key callback for closing window when esc is pressed */
void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mode){
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);
}

int main(){
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow *window = glfwCreateWindow(W, H, "Perlin", glfwGetPrimaryMonitor(), nullptr);
	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, keyCallback);

	glewExperimental = GL_TRUE;
	glewInit();

	glViewport(0, 0, W, H);

	Lines particles(lineCount);

	GLuint vao, vbo; // Make OpenGL vertex buffer and vertex array
	glGenBuffers(1, &vbo);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Line)*lineCount, particles.lines, GL_STREAM_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	NoiseGenerator perlin(time(0));

	Vector **vectors = new Vector*[ySize];
	for(int i=0; i<ySize; ++i) vectors[i] = new Vector[xSize];

	shader sp("vertex.shader", "fragment.shader"); // Shader program

	sp.use();
	
	int frames = 0;
	glEnable(GL_ALPHA_TEST); // enable alpha test and blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	while(!glfwWindowShouldClose(window)){
		glfwPollEvents();
		for(int i=0; i<ySize; ++i){
			for(int j=0; j<xSize; ++j){ // Update vectors according to noise
				vectors[i][j].setAngle(perlin(j*axisStep/*+i*frameStep*/, i*axisStep/*+j*frameStep*/, frames*frameStep)*2*pi);
				// cout << perlin(j*frameStep, i*frameStep, frames*frameStep) << endl;
			}
		}
		particles.update(vectors); // Update particles
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Line)*lineCount, particles.lines); // Update VBO
		glDrawArrays(GL_LINES, 0, lineCount*2);
		glBindVertexArray(0);
		glfwSwapBuffers(window);
		++frames;
	}
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glfwTerminate();
	for(int i=0; i<ySize; ++i) delete[] vectors[i];
	delete[] vectors;
	return 0;
}
