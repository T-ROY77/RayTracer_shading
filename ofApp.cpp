#include "ofApp.h"

#include <glm/gtx/intersect.hpp>


// Intersect Ray with Plane  (wrapper on glm::intersect*
//
bool Plane::intersect(const Ray& ray, glm::vec3& point, glm::vec3& normalAtIntersect) {
	float dist;
	bool insidePlane = false;
	bool hit = glm::intersectRayPlane(ray.p, ray.d, position, this->normal, dist);
	if (hit) {
		Ray r = ray;
		point = r.evalPoint(dist);
		intersectionPoint = r.evalPoint(dist + .5);
		normalAtIntersect = this->normal;
		glm::vec2 xrange = glm::vec2(position.x - width / 2, position.x + width / 2);
		glm::vec2 zrange = glm::vec2(position.z - height / 2, position.z + height / 2);
		if (point.x < xrange[1] && point.x > xrange[0] && point.z < zrange[1] && point.z > zrange[0]) {
			insidePlane = true;
		}
	}
	return insidePlane;
}


// Convert (u, v) to (x, y, z) 
// We assume u,v is in [0, 1]
//
glm::vec3 ViewPlane::toWorld(float u, float v) {
	float w = width();
	float h = height();
	return (glm::vec3((u * w) + min.x, (v * h) + min.y, position.z));
}

// Get a ray from the current camera position to the (u, v) position on
// the ViewPlane
//
Ray RenderCam::getRay(float u, float v) {
	glm::vec3 pointOnPlane = view.toWorld(u, v);
	return(Ray(position, glm::normalize(pointOnPlane - position)));
}


//--------------------------------------------------------------
void ofApp::setup() {
	image.allocate(imageWidth, imageHeight, ofImageType::OF_IMAGE_COLOR);

	gui.setup();
	gui.add(intensity.setup("Light intensity", 1.0, .1, 5));

	gui.add(power.setup("Phong p", 100, 10, 10000));
	bHide = true;

	theCam = &mainCam;
	mainCam.setDistance(10);
	mainCam.setNearClip(.1);
	sideCam.setPosition(glm::vec3(5, 0, 0));
	sideCam.lookAt(glm::vec3(0, 0, 0));
	sideCam.setNearClip(.1);
	previewCam.setFov(90);
	previewCam.setPosition(0, 0, 10);
	previewCam.lookAt(glm::vec3(0, 0, -1));

	


	cout << "h to toggle GUI" << endl;
	cout << "t to start ray tracer" << endl;
	cout << "d to show render" << endl;

}

//--------------------------------------------------------------
void ofApp::update() {

}

//--------------------------------------------------------------
void ofApp::rayTrace() {

	cout << "drawing..." << endl;

	ofColor closest;
	float distance = FLT_MIN;
	float close = FLT_MAX;
	int c = 0;
	for (int i = 0; i < image.getWidth(); i++) {
		for (int j = 0; j < image.getHeight(); j++) {
			background = true;																//reset variables every pixel
			distance = FLT_MIN;
			close = FLT_MAX;
			for (int k = 0; k < scene.size(); k++) {
				float u = (i + .5) / image.getWidth();
				float v = 1 - (j + .5) / image.getHeight();

				Ray r = renderCam.getRay(u, v);

				if (scene[k]->intersect(r, glm::vec3(i, j, 0), glm::vec3(0, 1, 0))) {
					background = false;														//if intersected with scene object, pixel is not background

					distance = glm::distance(r.p, scene[k]->position);						//calculate distance of intersection

					if (distance < close)													//if current object is closest to viewplane
					{
						c = k;
						close = distance;													//set threshold to new closest distance
					}
				}
				closest = scene[c]->diffuseColor;									//set to closest object color
				closest = lambert(r.evalPoint(close), scene[c]->getNormal(glm::vec3(0, 0, 0)), closest, distance);
				closest += phong(r.evalPoint(close), scene[c]->getNormal(glm::vec3(0, 0, 0)), closest, ofColor::white, power, distance);
				image.setColor(i, j, closest);
			}
			if (background) {																//if pixel is background set to black
				image.setColor(i, j, ofColor::black);
			}
		}
	}

	image.save("output.png");
	image.load("output.png");

	cout << "render saved" << endl;
}

//--------------------------------------------------------------
ofColor ofApp::lambert(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, float r) {
	ofColor lambert = ofColor(0, 0, 0);
	ofColor k = diffuse;
	float zero = 0.0;

	for (int i = 0; i < light.size(); i++) {
		/*
		Ray shadowRay = Ray(scene[0]->intersectionPoint, light[i]->position - p);
		for (int j = 0; j < scene.size(); j++) {
			if (scene[j]->intersect(shadowRay, scene[j]->position, scene[j]->getNormal(p))) {
				return ofColor::black;
			}
		}
		*/
		glm::vec3 l = glm::normalize(light[i]->position - p);
		lambert += k * (light[i]->intensity / r * r) * (glm::max(zero, glm::dot(norm, l)));
	}
	return lambert;
}


//--------------------------------------------------------------
ofColor ofApp::phong(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, const ofColor specular, float power, float r) {
	ofColor phong = ofColor(0, 0, 0);
	glm::vec3 h = glm::vec3(0);
	float zero = 0.0;
	phong = diffuse;
	for (int i = 0; i < light.size(); i++) {
		glm::vec3 l = glm::normalize(light[i]->position - p);
		glm::vec3 v = glm::normalize(renderCam.position - p);
		h = glm::normalize(l + v);
		phong += lambert(p, norm, diffuse, r) + specular * (light[i]->intensity / r * r) * glm::pow(glm::max(zero, glm::dot(norm, h)), power);
	}
	return phong;
}

//--------------------------------------------------------------


void ofApp::draw() {



	ofSetDepthTest(true);

	theCam->begin();


	scene.clear();

	scene.push_back(new Plane(glm::vec3(0, -5, 0), glm::vec3(0, 1, 0), ofColor::darkOliveGreen, 600, 400));		//ground plane

	scene.push_back(new Plane(glm::vec3(0, 1, -50), glm::vec3(0, 0, 1), ofColor::red, 600, 400));		//wall plane

	scene.push_back(new Sphere(glm::vec3(0, 1, 1), 1, ofColor::purple));											//purple sphere

	scene.push_back(new Sphere(glm::vec3(.5, 0, 2), 1, ofColor::green));											//green sphere

	scene.push_back(new Sphere(glm::vec3(-1, 0, 3), 1, ofColor::blue));												//blue sphere

	light.clear();

	light.push_back(new Light(glm::vec3(10, 15, 20), .5));

	light.push_back(new Light(glm::vec3(-10, 30, 5), 2));

	light.push_back(new Light(glm::vec3(-5, 10, 5), 1));


	//draw all scene objects
	for (int i = 0; i < scene.size(); i++) {
		ofColor color = scene[i]->diffuseColor;
		ofSetColor(color);
		scene[i]->draw();
	}

	for (int i = 0; i < light.size(); i++) {
		light[i]->setIntensity(intensity);
		light[i]->draw();
	}

	theCam->end();
	if (!bHide) {
		ofSetDepthTest(false);

		gui.draw();
	}
	//draw render
	if (drawImage) {
		ofSetColor(ofColor::white);
		image.draw(0, 0);
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	switch (key) {
	case OF_KEY_F1:
		theCam = &mainCam;
		break;
	case OF_KEY_F2:
		theCam = &sideCam;
		break;
	case OF_KEY_F3:
		theCam = &previewCam;
		break;
	case 'd':
		drawImage = !drawImage;
		break;
	case 't':
		rayTrace();
		break;
	case 'h':
		bHide = !bHide;
		break;
	default:
		break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}


