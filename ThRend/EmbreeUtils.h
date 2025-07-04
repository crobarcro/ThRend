#ifndef EmbreeUtils
#define EmbreeUtils

#include <cmath>
#include <embree3/rtcore.h>
#include <math.h>
#include <string>
#include <vector>
#include <glm.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

const float EPS = 1e-4f;

int triOffset = 0;
unsigned int triID;
unsigned int quadID;
RTCScene eScene;

typedef struct{
	float val;
	unsigned int gID;
	unsigned int pID;
} Result;

void intersectionFilter(const RTCFilterFunctionNArguments* args)
{
	/* avoid crashing when debug visualizations are used */
	if (args->context == nullptr) return;

	assert(args->N == 1);
	int* valid = args->valid;
	RTCRay* ray = (RTCRay*)args->ray;
	RTCHit* hit = (RTCHit*)args->hit;

	/* ignore inactive rays */
	if (valid[0] != -1) return;

	/* ignore hit if self hit */

	int globalID = hit->primID;
	if (hit->geomID == quadID)
		globalID += triOffset;

	int self = ray->flags;
	if (globalID == self)
		valid[0] = 0;
}

RTCScene buildSceneEmbree(std::vector<glm::vec3> &sc_vertices,
							std::vector<int> &sc_triangles,
							std::vector<int> &sc_quads,
							std::vector<int> &matIDs,
							std::vector<float> &temps) {
	std::cout << "Building Embree scene..." << std::endl;
	RTCDevice device = rtcNewDevice("threads=0");
	
	// Check for device errors
	RTCError deviceError = rtcGetDeviceError(device);
	if (deviceError != RTC_ERROR_NONE) {
		std::cout << "Device error: " << deviceError << std::endl;
	}
	
	//	rtcSetDeviceProperty(device, RTC_DEVICE_PROPERTY_BACKFACE_CULLING_ENABLED, 1);
	//	ssize_t pr = rtcGetDeviceProperty(device, RTC_DEVICE_PROPERTY_BACKFACE_CULLING_ENABLED);
	//	std::cout << "BFC prop is " <<  pr<<  " \n";
	eScene = rtcNewScene(device);
	
	// Check for scene creation errors
	RTCError sceneError = rtcGetDeviceError(device);
	if (sceneError != RTC_ERROR_NONE) {
		std::cout << "Scene creation error: " << sceneError << std::endl;
	}

	RTCBuffer vertices =
		rtcNewSharedBuffer(device, sc_vertices.data(),
		sizeof(glm::vec3) * sc_vertices.size());

	if (sc_triangles.size() > 0) {
		std::cout << "Adding " << sc_triangles.size() / 3 << " triangles..." << std::endl;
		RTCGeometry triGeom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
		rtcSetGeometryBuffer(triGeom, RTC_BUFFER_TYPE_VERTEX, 0,
			RTC_FORMAT_FLOAT3, vertices, 0, sizeof(glm::vec3),
			sc_triangles.size());

		GLuint* indexT = (GLuint*)rtcSetNewGeometryBuffer(
			triGeom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
			3 * sizeof(GLuint), sc_triangles.size() / 3);

		for (GLuint i = 0; i < sc_triangles.size(); i++) {
			indexT[i] = sc_triangles[i];
		}
		//filter to avoid self hit
		rtcSetGeometryIntersectFilterFunction(triGeom, intersectionFilter);

		rtcSetGeometryVertexAttributeCount(triGeom, 1);
		//		rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT, hair_indices, 0, sizeof(unsigned int), 1);
		rtcSetSharedGeometryBuffer(triGeom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT, temps.data(), 0, sizeof(float), temps.size());

		rtcCommitGeometry(triGeom);
		triID = rtcAttachGeometry(eScene, triGeom);
		triOffset = sc_triangles.size() / 3;
		std::cout << "Triangles attached with ID: " << triID << std::endl;
	}
	if (sc_quads.size() > 0) {
		std::cout << "Adding " << sc_quads.size() / 4 << " quads..." << std::endl;
		RTCGeometry quadGeom =
			rtcNewGeometry(device, RTC_GEOMETRY_TYPE_QUAD);
		rtcSetGeometryBuffer(quadGeom, RTC_BUFFER_TYPE_VERTEX, 0,
			RTC_FORMAT_FLOAT3, vertices, 0, sizeof(glm::vec3),
			sc_quads.size());
		GLuint* indexQ = (GLuint*)rtcSetNewGeometryBuffer(
			quadGeom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT4,
			4 * sizeof(GLuint), sc_quads.size() / 4);
		for (GLuint i = 0; i < sc_quads.size(); i++) {
			indexQ[i] = sc_quads[i];
		}
		//filter to avoid self hit
		rtcSetGeometryIntersectFilterFunction(quadGeom, intersectionFilter);

		rtcSetGeometryVertexAttributeCount(quadGeom, 1);
		//		rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT, hair_indices, 0, sizeof(unsigned int), 1);
		rtcSetSharedGeometryBuffer(quadGeom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT, temps.data(), 0, sizeof(float), temps.size());

		rtcCommitGeometry(quadGeom);
		quadID = rtcAttachGeometry(eScene, quadGeom);
		std::cout << "Quads attached with ID: " << quadID << std::endl;
	}

	std::cout << "Committing scene..." << std::endl;
	rtcCommitScene(eScene);
	
	// Check for commit errors
	RTCError commitError = rtcGetDeviceError(device);
	if (commitError != RTC_ERROR_NONE) {
		std::cout << "Scene commit error: " << commitError << std::endl;
	} else {
		std::cout << "Scene committed successfully" << std::endl;
	}

	return eScene;
}


#endif

