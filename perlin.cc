#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <ctime>
#include <cmath>
#include <shader.h>
#include <noise.h>
#include <random>

const int W = 1366;
const int H = 768;
const int xSize = 112;
const int ySize = 64;
const double frameStep = 0.0001;
const double axisStep = 0.01;
const int lineCount = 10000;
const double maxVel = 0.001;
const double pi = 3.1415926535;

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

struct Line{
	GLfloat x1;
	GLfloat y1;
	GLfloat x2;
	GLfloat y2;
};

class Lines{
public:
	Line *lines;
	Vector *vels;
	int size;
	Lines(int count): size(count){
		std::mt19937 rand(time(0));
		lines = new Line[count];
		vels = new Vector[count];
		for(int i=0; i<count; ++i){
			lines[i].x1 = lines[i].x2 = (float)rand()*2/rand.max()-1.0;
			lines[i].y1 = lines[i].y2 = (float)rand()*2/rand.max()-1.0;
		}
	}
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
	void setPrev(int index){
		lines[index].x2 = lines[index].x1;
		lines[index].y2 = lines[index].y1;
	}
	~Lines(){
		delete[] lines;
		delete[] vels;
	}
};

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

	GLuint vao, vbo;
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

	shader sp("vertex.shader", "fragment.shader");

	sp.use();

	/*std::mt19937 rand(time(0));
	double firstAngle = (double)rand()/rand.max()*2*pi;
	vectors[0][0].setAngle(firstAngle);
	for(int i=0; i<ySize; ++i){
		for(int j=1; j<xSize; ++j){
			int count = 0;
			double total = 0;
			for(int x = -1; x<=1; ++x){
				for(int y = -1; y<=1; ++y){
					if((x != 0 || y != 0) && i+x >= 0 && i+x < ySize && j+y >= 0 && j+y < xSize){
						++count;
						total += vectors[i+x][j+y].angle;
					}
				}
			}
			total /= count;
			vectors[i][j] = total + ((double)rand()/rand.max()*2-1)*change;
		}
	}*/
	int frames = 0;
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	while(!glfwWindowShouldClose(window)){
		glfwPollEvents();
		/*glClearColor(1.0, 1.0, 1.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);*/
		/*firstAngle = (double)rand()/rand.max()*2*pi;
		vectors[0][0].setAngle(firstAngle);
		for(int i=0; i<ySize; ++i){
			for(int j=1; j<xSize; ++j){
				int count = 0;
				double total = 0;
				for(int x = -1; x<=1; ++x){
					for(int y = -1; y<=1; ++y){
						if((x != 0 || y != 0) && i+x >= 0 && i+x < ySize && j+y >= 0 && j+y < xSize){
							++count;
							total += vectors[i+x][j+y].angle;
						}
					}
				}
				total /= count;
				vectors[i][j] = total + ((double)rand()/rand.max()*2-1)*change;
			}
		}*/
		for(int i=0; i<ySize; ++i){
			for(int j=0; j<xSize; ++j){
				vectors[i][j].setAngle(perlin(j*axisStep/*+i*frameStep*/, i*axisStep/*+j*frameStep*/, frames*frameStep)*2*pi);
				// cout << perlin(j*frameStep, i*frameStep, frames*frameStep) << endl;
			}
		}
		particles.update(vectors);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Line)*lineCount, particles.lines);
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