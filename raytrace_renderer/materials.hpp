#ifndef MATERIALS_HPP
#define MATERIALS_HPP

#include "../ext/glm/glm.hpp"
#include "ray.hpp"
#include "util.hpp"
using namespace glm;

class IMaterial {
public:
	virtual void Scatter(const vec3& hitPoint, const vec3& normal, const SceneInfo& info, SceneInfo& outInfo) = 0;
};

class DiffuseMaterial : public IMaterial {
public:
	void Scatter(const vec3& hitPoint, const vec3& normal, const SceneInfo& info, SceneInfo& outInfo) {
		// use the hit point and normal to figure out where
		// a unit sphere would rest, so that the bounce point can be calculated
		Ray sphereRay(hitPoint, normal);
		vec3 sphereCenter = sphereRay.PointAt(1.0001);
		vec3 nextDir = glm::normalize(sphereCenter + RandomPointInUnitSphere() - hitPoint);

		outInfo.inR = Ray(hitPoint, nextDir);
	}
};

class SpecularMaterial : public IMaterial {
	float _roughness;
public:
	SpecularMaterial(float roughness) {
		_roughness = roughness;
	}

	void Scatter(const vec3& hitPoint, const vec3& normal, const SceneInfo& info, SceneInfo& outInfo) {
		vec3 inDir = info.inR.Direction();
		vec3 jitter = _roughness * RandomPointInUnitSphere();

		vec3 partial = inDir - 2 * dot(inDir, normal)*normal;
		vec3 nextDir = partial + jitter;
		// vec3 nextDir = inDir - 2*dot(inDir, normal)*normal + jitter;

		if (dot(nextDir, normal) < 0) {
			nextDir = partial - jitter;
			// nextDir = Ray(hitPoint, -((inDir - 2 * dot(inDir, normal)*normal) + jitter));
			
			// cout << "u fukt up" << endl;
		}
		outInfo.inR = Ray(hitPoint, nextDir);
	}
};

class DielectricMaterial : public IMaterial {
	float _ior;
protected:

	

public:

	DielectricMaterial(float ior) {
		_ior = ior;
	}

	void Scatter(const vec3& hitPoint, const vec3& normal, const SceneInfo& info, SceneInfo& outInfo) {
		float niOverNt;
		vec3 outN;
		vec3 refracted;
		float cosine;
		float reflectProb;
		float inDotN = dot(info.inR.Direction(), normal);
		if (inDotN > 0) {
			outN = -normal;
			niOverNt = _ior / info.ior;
			cosine = _ior*inDotN;
		}
		else {
			outN = normal;
			niOverNt = info.ior / _ior;
			cosine = info.ior*(-inDotN);
		}
		if (Refract(outN, info.inR.Direction(), niOverNt, refracted)) {
			reflectProb = Shlick(cosine, _ior);
		}
		else {
			reflectProb = 1.0;
		}
		if (Rand01() < reflectProb) {
			outInfo.inR = Ray(hitPoint, reflect(info.inR.Direction(), normal));
			outInfo.ior = info.ior;
		}
		else {
			outInfo.inR = Ray(hitPoint, refracted);
			outInfo.ior = _ior;
		}
	}
};

#endif