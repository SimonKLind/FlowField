/** A very simple shader, just draws all black with low opacity */

#version 450 core

void main(){
	gl_FragColor = vec4(0.0, 0.0, 0.0, 0.005);
}
