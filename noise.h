#ifndef NOISE_H
#define NOISE_H

#include <random>
// #include <iostream>

#define NUM_VECTORS 256

class NoiseGenerator{
	struct Vector{
		double x;
		double y;
		double z;
	};

	std::mt19937 rand;
	Vector g[NUM_VECTORS];
	int octaves = 8;
	double persistence = 0.5;

	double lerp(double a, double b, double x){
		return a+x*(b-a);
	}

	double fade(double x){
		return x*x*x*(x*(x*6-15)+10);
	}

	/*double grad(double x, double y, double z){
		double a = (double)rand()/rand.max()*2-1;
		double b = (double)rand()/rand.max()*2-1;
		double c = (double)rand()/rand.max()*2-1;
		return a*x+b*y+c*z;
	}*/

	double grad(int hash, double x, double y, double z){
		// int index = hash&0xFF;
		int index = hash%NUM_VECTORS;
		return g[index].x*x + g[index].y*y + g[index].z*z;
	}

public:

	NoiseGenerator(int seed){
		rand.seed(seed);
		for(int i=0; i<NUM_VECTORS; ++i){
			g[i].x = ((double)rand()/rand.max())*2-1;
			g[i].y = ((double)rand()/rand.max())*2-1;
			g[i].z = ((double)rand()/rand.max())*2-1;
			// std::cout << g[i].x << ", " << g[i].y << ", " << g[i].z << '\n'; 
		}
	}

	double perlin(double x, double y = 0, double z = 0){
		int hash = 7 + (int)x;
		hash = hash*31 + (int)y;
		hash = hash*31 + (int)z;
		// std::cout << hash << ", ";
		// rand.seed(hash);
		x -= (int)x;
		y -= (int)y;
		z -= (int)z;
		double u = fade(x);
		double v = fade(y);
		double w = fade(z);
		double x1, x2, y1, y2;
		x1 = lerp(grad(hash, x-1, y+1, z+1), grad(hash, x+1, y+1, z+1), u);
		x2 = lerp(grad(hash, x-1, y+1, z-1), grad(hash, x+1, y+1, z-1), u);
		y1 = lerp(x1, x2, v);
		x1 = lerp(grad(hash, x-1, y-1, z+1), grad(hash, x+1, y-1, z+1), u);
		x2 = lerp(grad(hash, x-1, y-1, z-1), grad(hash, x+1, y-1, z-1), u);
		y2 = lerp(x1, x2, v);
		return (lerp(y1, y2, w)+1)/2;
	}

	double operator()(double x, double y = 0, double z = 0){
		double total = 0, amp = 1, freq = 1, maxVal = 0;
		for(int i=0; i<octaves; ++i){
			total += perlin(x*freq, y*freq, z*freq)*amp;
			// std::cout << total << ", " << perlin(x*freq, y*freq, z*freq) << '\n';
			maxVal += amp;
			amp *= persistence;
			freq *= 2;
		}
		return total/maxVal;
	}

	void set(int octaves, double persistence){
		this->octaves = octaves;
		this->persistence = persistence;
	}
};

#endif