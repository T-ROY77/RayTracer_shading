#include "ofApp.h"

#include <glm/gtx/intersect.hpp>

//render is too bright?
//does phong call lambert?
//
// 
// Intersect Ray with Plane  (wrapper on glm::intersect*
//

bool Plane::intersect(const Ray& ray, glm::vec3& point, glm::vec3& normalAtIntersect) {
	float dist;
	bool insidePlane = false;
	bool hit = glm::intersectRayPlane(ray.p, ray.d, position, this->normal, dist);
	if (hit) {
		Ray r = ray;
		point = r.evalPoint(dist);
		setIntersectionPoint(r.evalPoint(dist));

		normalAtIntersect = this->normal;
		glm::vec2 xrange = glm::vec2(position.x - width / 2, position.x + width
			/ 2);
		glm::vec2 zrange = glm::vec2(position.z - height / 2, position.z +
			height / 2);
		if (point.x < xrange[1] && point.x > xrange[0] && point.z < zrange[1]
			&& point.z > zrange[0]) {
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
	gui.add(intensity.setup("Light intensity", .5, .1, 1));

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
			c = 0;
			float u = (i + .5) / image.getWidth();
			float v = 1 - (j + .5) / image.getHeight();

			Ray r = renderCam.getRay(u, v);
			for (int k = 0; k < scene.size(); k++) {
				if (scene[k]->intersect(r, scene[k]->intersectionPoint, glm::vec3(0, 1, 0))) {
					background = false;														//if intersected with scene object, pixel is not background

					distance = glm::distance(r.p, scene[k]->position);						//calculate distance of intersection
					if (distance < close)													//if current object is closest to viewplane
					{
						c = k;																//save index of closest object
						close = distance;													//set threshold to new closest distance
					}
				}
			}
			if (!background) {
				closest = shade(r.evalPoint(close), scene[c]->getNormal(glm::vec3(0, 0, 0)), scene[c]->diffuseColor, distance, ofColor::gray, power, r);
				image.setColor(i, j, closest);
			}
			else if (background) {																//if pixel is background set to black
				image.setColor(i, j, ofColor::black);
			}
		}
	}

	image.save("output.png");
	image.load("output.png");

	cout << "render saved" << endl;
}

//--------------------------------------------------------------
ofColor ofApp::lambert(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, float distance, Ray r, Light light) {
	ofColor lambert = ofColor(0, 0, 0);
	float zero = 0.0;

	glm::vec3 l = glm::normalize(light.position - p);
	lambert += diffuse * (light.intensity / distance * distance) * (glm::max(zero, glm::dot(norm, l)));
	
	return lambert;
}


//--------------------------------------------------------------
ofColor ofApp::phong(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, const ofColor specular, float power, float distance, Ray r, Light light) {
	ofColor phong = ofColor(0, 0, 0);
	glm::vec3 h = glm::vec3(0);
	float zero = 0.0;

	glm::vec3 l = glm::normalize(light.position - p);
	glm::vec3 v = glm::normalize(renderCam.position - p);
	h = glm::normalize(l + v);
	//phong += lambert(p, norm, diffuse, distance, r, light) + specular * (light.intensity / distance * distance) * glm::pow(glm::max(zero, glm::dot(norm, h)), power);
	phong += specular * (light.intensity / distance * distance) * glm::pow(glm::max(zero, glm::dot(norm, h)), power);
	
	return phong;
}

//--------------------------------------------------------------
ofColor ofApp::shade(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, float distance, const ofColor specular, float power, Ray r){
	ofColor shaded = (0,0,0);
	glm::vec3 p1 = p;
	//loop through all lights
	for (int i = 0; i < light.size(); i++) {
		blocked = false;

		if (scene[0]->intersect(r, p1, glm::vec3(0, 1, 0))) {													//check if current point intersected with ground plane

			//test for shadows

			Ray shadowRay = Ray(scene[0]->getIntersectionPoint(), light[i]->position - scene[0]->getIntersectionPoint());

			//check all sphere objects
			for (int j = 2; j < scene.size(); j++) {
				if (scene[j]->intersect(shadowRay, p1, scene[j]->getNormal(p1))) {
					blocked = true;
				}
			}
		}
		if (!blocked) {
			//add shading contribtion
			shaded += lambert(p, norm, diffuse, distance, r, *light[i]);
			shaded += phong(p, norm, shaded, specular, power, distance, r, *light[i]);
		}
	}
	return shaded;
}
//--------------------------------------------------------------
void ofApp::draw() {

	ofSetDepthTest(true);

	theCam->begin();


	scene.clear();

	scene.push_back(new Plane(glm::vec3(0, -5, 0), glm::vec3(0, 1, 0), ofColor::darkBlue, 600, 400));				//ground plane

	scene.push_back(new Plane(glm::vec3(0, 1, -50), glm::vec3(0, 0, 1), ofColor::darkGray, 600, 400));	        	//wall plane

	scene.push_back(new Sphere(glm::vec3(0, 1, -2), 1, ofColor::purple));											//purple sphere

	scene.push_back(new Sphere(glm::vec3(-1, 0, 1), 1, ofColor::blue));												//blue sphere

	scene.push_back(new Sphere(glm::vec3(.5, 0, 0), 1, ofColor::green));											//green sphere


	light.clear();

	light.push_back(new Light(glm::vec3(100, 150, 150), .2));

	light.push_back(new Light(glm::vec3(-100, 300, 100), .2));

	light.push_back(new Light(glm::vec3(-50, 100, 500), .2));


	//draw all scene objects
	for (int i = 0; i < scene.size(); i++) {
		ofColor color = scene[i]->diffuseColor;
		ofSetColor(color);
		scene[i]->draw();
	}

	//draw all lights
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


