#ifndef UTIL_HPP
#define UTIL_HPP

#include "ray.hpp"
#include "../ext/glm/glm.hpp"
#include <iostream>
#include <float.h>
using namespace std;
using namespace glm;

// util includes a bunch of random constants and functions for now because laziness
const double PI = acos(-1.0);
const float AIR_IOR = 1.00029;

double Rand01() {
	return (double(rand()) / double(RAND_MAX));
}

double RandRange(double min, double max) {
	return Rand01() * (max - min) + min;
}

float Shlick(float cosine, float ior) {
	float r0 = (1 - ior) / (1 + ior);
	r0 = r0*r0;
	return r0 + (1 - r0)*pow((1 - cosine), 5);
}

// returns a random point in a unit sphere centered at (0, 0, 0) and radius 1
// method from: http://mathworld.wolfram.com/SpherePointPicking.html
vec3 RandomPointInUnitSphere() {
	
	/* // REJECTION SAMPLING
	vec3 out;
	do {
		out = 2.0f*vec3(Rand01(), Rand01(), Rand01()) - vec3(1.0, 1.0, 1.0);
	} while (length(out)*length(out) >= 1.0f);
	return out;
	*/

	double theta = 2.0 * PI * Rand01();
	double phi = acos(2.0 * Rand01() - 1);
	double u = cos(phi);
	double root = sqrt(1 - u*u);

	return vec3(root * cos(theta), root * sin(theta), u);
}

vec3 reflect(const vec3& inDir, const vec3& normal) {
	return inDir - 2 * dot(inDir, normal)*normal;
}

bool Refract(const vec3& normal, const vec3& inDir, float niOverNt, vec3& refracted) {
	float dt = dot(inDir, normal);
	float discriminant = 1.0 - niOverNt*niOverNt*(1 - dt*dt);
	if (discriminant > 0) {
		refracted = niOverNt*(inDir - normal*dt) - normal*sqrt(discriminant);
		return true;
	}
	else {
		return false;
	}
}

void PrintVec3(vec3 v) {
	cout << v.x << " " << v.y << " " << v.z << endl;
}

struct SceneInfo {
	vec3 color;
	Ray inR;
	float ior;
	int iteration;

	SceneInfo(Ray _inR, vec3 _color, int _iteration, float _ior = AIR_IOR)  {
		color = _color;
		inR = _inR;
		ior = AIR_IOR;
		iteration = _iteration;
	}
};

#endif