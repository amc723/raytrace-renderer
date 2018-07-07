#ifndef HITTABLES_HPP
#define HITTABLES_HPP

#include "../ext/glm/glm.hpp"
#include "util.hpp"
#include "ray.hpp"
#include "hittables.hpp"
#include "materials.hpp"
using namespace std;
using namespace glm;

class IHittable {
protected:
	IMaterial* _material;
public:
	IHittable(IMaterial* material) {
		_material = material;
	}

	virtual float Hit(const SceneInfo& info, SceneInfo& outInfo) = 0;

	virtual const vec3& Color() const = 0;
};

class Sphere : public IHittable {
	vec3 _pos;
	float _radius;
	vec3 _color;

public:
	Sphere(IMaterial* material, vec3 pos, float radius, vec3 color)
		: IHittable(material) {
		_pos = pos;
		_radius = radius;
		_color = color;
	}

	float PointOnSphere(const Ray& r) {
		vec3 oc = r.Origin() - _pos;
		float a = dot(r.Direction(), r.Direction());
		float b = 2 * dot(r.Direction(), oc);
		float c = dot(oc, oc) - _radius * _radius;
		float discriminant = b*b - 4 * a*c;
		if (discriminant < 0) {
			return -1;
		}
		else {
			return (-b - sqrt(discriminant)) / (2.0*a);
		}
	}

	virtual float Hit(const SceneInfo& info, SceneInfo& outInfo) {
		float t = PointOnSphere(info.inR);
		if (t > 0) {
			vec3 hitPoint = info.inR.PointAt(t);
			vec3 normal = normalize(hitPoint - _pos);

			_material->Scatter(hitPoint, normal, info, outInfo);

			// outInfo.inR = Ray(hitPoint, _material->Scatter(hitPoint, normal, outInfo));
			// outInfo.inR = Ray(hitPoint, normal);
			 
		}
		return t;
	}

	const vec3& Pos() const {
		return _pos;
	}

	float Radius() const {
		return _radius;
	}

	const vec3& Color() const {
		return _color;
	}

};

class Plane : public IHittable {
	vec3 _normal;
	float _d;
	vec3 _color;

public:
	Plane(IMaterial* mat, vec3 pos, vec3 normal, vec3 color) : IHittable(mat) {
		_normal = normalize(normal);
		_d = sqrt(dot(pos, pos));
		_color = color;
	}

	virtual float Hit(const SceneInfo& info, SceneInfo& outInfo) {
		float num = _normal.x * info.inR.Origin().x + _normal.y * info.inR.Origin().y + _normal.z * info.inR.Origin().z + _d;
		float denom = _normal.x * info.inR.Direction().x + _normal.y * info.inR.Direction().y + _normal.z * info.inR.Direction().z;
		// div by 0 - just return no hit?
		if (denom == 0.0f) {
			return -1;
		}

		float t = -(num / denom);
		if (t > 0) {
			vec3 hitPoint = info.inR.PointAt(t);
			// outInfo.inR = Ray(hitPoint, _material->Scatter(hitPoint, _normal, outInfo));
		}
		return t;
	}

	const vec3& Color() const {
		return _color;
	}
};

class Scene {
	vector<IHittable*> _objects;

public:
	void Add(IHittable* object) {
		_objects.push_back(object);
	}

	float Hit(const SceneInfo& info, SceneInfo& outInfo) {
		float minT = FLT_MAX;
		bool hit = false;
		for (int i = 0; i < _objects.size(); i++) {
			SceneInfo tempOutInfo(info);
			float t = _objects[i]->Hit(info, tempOutInfo);
			if (t > 0 && t < minT) {
				outInfo = tempOutInfo;
				outInfo.color = _objects[i]->Color();
				outInfo.iteration += 1;
				minT = t;
				hit = true;

				/*
				if (dot(info.inR.Direction(), outInfo.inR.Direction()) > 0) {
					cout << "UFK" << endl;
				}
				*/
			}
		}

		if (hit) {
			return minT;
		}
		else {
			return -1;
		}
	}
};

#endif