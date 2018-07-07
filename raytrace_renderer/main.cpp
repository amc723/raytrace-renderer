#include <fstream>
#include <iostream>
#include <cstdio>
#include <string>
#include <thread>
#include <mutex>
#include <sstream>
#include <errno.h>
#include <random>
#include <time.h>
#include <float.h>
#include <chrono>
#include "../ext/glm/glm.hpp"
#include "materials.hpp"
#include "util.hpp"
#include "ray.hpp"
#include "hittables.hpp"
using namespace std;
using namespace glm;

const int WIDTH = 800; // 2048;
const int HEIGHT = 400; // 2048;

const int SAMPLES = 50; // 512;

const int MAX_PATH_LENGTH = 50;

const int WORKER_THREADS = 8;

class Camera {
protected:
	vec3 _pos;
	vec3 _blCorner;
	vec3 _xViewport;
	vec3 _yViewport;
public:
	// vfov is vertical field of view in degrees
	Camera(vec3 pos, vec3 lookat, vec3 vup, float vfov, float aspect) {
		_pos = pos;
		
		float theta = vfov * PI / 180.0f;
		float halfHeight = tan(theta / 2.0f);
		float halfWidth = halfHeight * aspect;
		vec3 lookDir = normalize(_pos - lookat);
		vec3 right = normalize(cross(vup, lookDir));
		vec3 up = normalize(cross(lookDir, right));

		_blCorner = _pos - halfWidth*right - halfWidth*up - lookDir;
		_xViewport = 2 * halfWidth*right;
		_yViewport = 2 * halfHeight*up;
	}

	// x, y between 0 and 1 specifying percentage distance from bl corner
	Ray RayAtViewport(float x, float y) {
		return Ray(_pos, vec3(_blCorner + x*_xViewport + y*_yViewport - _pos));
	}
};

class IPixelCalculator {
protected:
	int _imgWidth;
	int _imgHeight;

public:
	IPixelCalculator(int imgWidth, int imgHeight) {
		_imgWidth = imgWidth;
		_imgHeight = imgHeight;
	}

	virtual glm::vec3 CalcPixelColor(int row, int col) = 0;
};

class SceneRenderer : public IPixelCalculator {
	Scene* _scene;
	Camera* _camera;

protected:
	vec3 BackgroundColor(const Ray& r) {
		float x = (r.Direction().x + 1.0) / 4.0;
		float y = (r.Direction().y + 1.0) / 4.0;

		return vec3(1.0, .95, 0.68)*float((y + x)) + vec3(.62, 0.80, 0.94)*float((1 - (x + y)));
	}

	vec3 CalcPixelColor(const SceneInfo& info) {
		SceneInfo outInfo(info);
		if (_scene->Hit(info, outInfo) > 0.0) {
			if (info.iteration < MAX_PATH_LENGTH) {
				return info.color * CalcPixelColor(outInfo);
			}
			else {
				return info.color;
			}
		}
		else {
			return info.color * BackgroundColor(info.inR);
		}
	}

public:
	SceneRenderer(Scene* s, Camera* cam, int width, int height) : IPixelCalculator(width, height) {
		_scene = s;
		_camera = cam;
	}

	glm::vec3 CalcPixelColor(int row, int col) {
		vec3 sum = vec3(0, 0, 0);
		const int samples = SAMPLES;
		for (int i = 0; i < samples; i++) {
			const double jitterMin = -0.001;
			const double jitterMax = 0.001;
			vec3 jitter = vec3(RandRange(jitterMin, jitterMax), RandRange(jitterMin, jitterMax), RandRange(jitterMin, jitterMax));
			Ray r = _camera->RayAtViewport(float(col) / WIDTH, float(row) / HEIGHT);
			sum += CalcPixelColor(SceneInfo(Ray(r.Origin(), r.Direction() + jitter), vec3(1, 1, 1), 1));
		}
		return vec3(sum.x / samples, sum.y / samples, sum.z / samples);
	}
};

// TODO: how to not make this global??
std::mutex rowLock;

void WriteRow(vector<vec3>* img, int* nextRow, IPixelCalculator* calculator) {
	while (nextRow >= 0) {
		int row;
		rowLock.lock();
		row = *nextRow;
		*nextRow -= 1;
		rowLock.unlock();
		if (row < 0) {
			break;
		}
		
		for (int col = 0; col < img[row].size(); col++)
		{
			img[row][col] = calculator->CalcPixelColor(row, col);
		}

		cout << row << endl;
	}
}

// simple multithreaded write allocates a 2D array of floats (the "texture") up front
// for a 2K image, this allocates 50MB up front; 4K = 200MB, so it's kind of sustainable?
// this should be the fastest async write?? but also the worst for memory allocation
bool WritePpmAsync(char * outputPath, int width, int height, IPixelCalculator * calculator) {
	FILE * outFile;
	char errBuff[100];
	int err;
	err = fopen_s(&outFile, outputPath, "w");
	if (err != 0) {
		strerror_s(errBuff, err);
		cout << err << ", " << errBuff << endl;
		return false;
	}
	err = fprintf(outFile, "P3\n%i %i\n255\n", width, height);

	vector<vec3>* img = new vector<vec3>[height];
	for (int i = 0; i < height; i++) {
		img[i] = vector<vec3>(width);
	}

	int* nextRow = new int(height - 1);
	vector<thread> threads;
	for (int i = 0; i < WORKER_THREADS; i++) {
		threads.push_back(thread(WriteRow, img, nextRow, calculator));
	}

	// wait for all of the worker threads to finish
	for (int i = 0; i < WORKER_THREADS; i++) {
		threads[i].join();
	}

	// now actually write the image
	for (int row = height - 1; row >= 0; row--) {
		for (int col = 0; col < width; col++)
		{
			int rn, gn, bn;
			rn = int(255.9f * img[row][col].r);
			gn = int(255.9f * img[row][col].g);
			bn = int(255.9f * img[row][col].b);

			if (rn < 0 || rn > 255 || gn < 0 || gn > 255 || bn < 0 || bn > 255) {
				int foobar = 12;
			}

			err = fprintf(outFile, "%i %i %i\n", rn, gn, bn);
		}
	}

	int res = fclose(outFile);
	return res == 0;
}

int main() {

	chrono::time_point<chrono::system_clock> start, end;
	start = chrono::system_clock::now();

	// seed the random number generator
	srand((unsigned)time(NULL));

	DiffuseMaterial* diffuse = new DiffuseMaterial();
	SpecularMaterial* roughSpecular = new SpecularMaterial(0.6f);
	SpecularMaterial* shinySpecular = new SpecularMaterial(0.0f);
	DielectricMaterial* glass = new DielectricMaterial(1.3f);

	Scene* scene = new Scene();
	Camera* cam = new Camera(vec3(0, -1, 3), vec3(0, 0, -1), vec3(0, 1, 0), 45, float(WIDTH) / HEIGHT);

	// scene->Add(new Plane(roughSpecular, vec3(0, 0, -10), vec3(0.0, 1.0, 0.0), vec3(1.0, 0.0, 1.0)));

	scene->Add(new Sphere(diffuse, vec3(0.0, -203.0, -6.0), 200.0, vec3(0.2, 0.2, 0.2)));

	scene->Add(new Sphere(glass, vec3(4.0, -1.0, -6.0), 2.0, vec3(0.0, 1.0, 0.0)));
	scene->Add(new Sphere(glass, vec3(0.0, -1.0, -6.0), 2.0, vec3(1.0, 0.0, 0.0)));
	scene->Add(new Sphere(glass, vec3(-4.0, -1.0, -6.0), 2.0, vec3(1.0, 1.0, 1.0)));

	/*
	scene->Add(new Sphere(glass, vec3(-5.0, 4.0, -28.0), 6, vec3(1.0,1.0,1.0)));
	scene->Add(new Sphere(diffuse, vec3(0.0, 0.0, -15.0), 2, vec3(0.9,0.9,0.9)));
	scene->Add(new Sphere(shinySpecular, vec3(-4.0, 0.0, -15.0), 2, vec3(0.7, 0.7, 1.0)));
	scene->Add(new Sphere(roughSpecular, vec3(4.0, 0.0, -15.0), 2, vec3(1.0,0.5,0.5)));

	scene->Add(new Sphere(diffuse, vec3(0.0, -10002, -15.0), 10000, vec3(0.2, 0.2, 0.9)));
	*/

	SceneRenderer sceneRenderer(scene, cam, WIDTH, HEIGHT);
	
	bool res = WritePpmAsync("out.ppm", WIDTH, HEIGHT, &sceneRenderer);
	cout << (res ? "success" : "failure") << endl;

	end = chrono::system_clock::now();
	chrono::duration<double> elapsed = end - start;
	
	cout << "elapsed time: " << elapsed.count() << " seconds" << endl;
	cout << "enter anything to exit...";

	string foo;
	cin >> foo;
	return 0;
}