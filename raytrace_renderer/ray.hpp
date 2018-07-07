#ifndef RAY_H
#define RAY_H
#include "../ext/glm/glm.hpp"
using namespace glm;

class Ray {
	vec3 origin;
	vec3 direction;

public:
	Ray(const vec3& _origin, const vec3& _direction) {
		origin = _origin;
		direction = normalize(_direction);
	}

	Ray() {}

	vec3 Origin() const { return origin; }
	vec3 Direction() const { return direction; }

	vec3 PointAt(float t) const {
		return origin + t*direction;
	}
};

#endif