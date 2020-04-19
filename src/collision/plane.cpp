#include "iostream"
#include <nanogui/nanogui.h>

#include "../clothMesh.h"
#include "../clothSimulator.h"
#include "../leak_fix.h"
#include "plane.h"

using namespace std;
using namespace CGL;

#define SURFACE_OFFSET 0.0001

/*
 double distance = (pm.position - origin).norm();
 if (radius >= distance) {
     // compute where pm intersected sphere, find tangent
     Vector3D path = pm.position - origin;
     path.normalize();
     // compute correction vector to reach tangent
     Vector3D correction_vector = path * radius + origin - pm.last_position;
     // let point mass's new position be last position adjusted by correction vector, multiplied by 1-f
     pm.position = pm.last_position + correction_vector * (1.0 - friction);
 }
 */

void Plane::collide(PointMass &pm) {
  // TODO (Part 3): Handle collisions with planes.
    // does the point cross over the plane within this timestep
    bool curPositive = false;
    bool lastPositive = false;
    if (dot(point - pm.position, normal) > 0.0) curPositive = true;
    if (dot(point - pm.last_position, normal) > 0.0) lastPositive = false;
    if (curPositive != lastPositive) {
            // dot product of normal
        // compute where pm would intersect plane, find tangent
        Vector3D tangent_point = pm.position - (dot(pm.position - point, normal) - SURFACE_OFFSET) * normal;
        // compute correction vector to reach tangent, use surface offset to displace
        Vector3D correction_vector = tangent_point - pm.last_position;
        // let pm's new position be last position adjusted by corr vect mult by 1-f
        pm.position = pm.last_position + correction_vector * (1.0 - friction);
    }
}

void Plane::render(GLShader &shader) {
  nanogui::Color color(0.7f, 0.7f, 0.7f, 1.0f);

  Vector3f sPoint(point.x, point.y, point.z);
  Vector3f sNormal(normal.x, normal.y, normal.z);
  Vector3f sParallel(normal.y - normal.z, normal.z - normal.x,
                     normal.x - normal.y);
  sParallel.normalize();
  Vector3f sCross = sNormal.cross(sParallel);

  MatrixXf positions(3, 4);
  MatrixXf normals(3, 4);

  positions.col(0) << sPoint + 2 * (sCross + sParallel);
  positions.col(1) << sPoint + 2 * (sCross - sParallel);
  positions.col(2) << sPoint + 2 * (-sCross + sParallel);
  positions.col(3) << sPoint + 2 * (-sCross - sParallel);

  normals.col(0) << sNormal;
  normals.col(1) << sNormal;
  normals.col(2) << sNormal;
  normals.col(3) << sNormal;

  if (shader.uniform("u_color", false) != -1) {
    shader.setUniform("u_color", color);
  }
  shader.uploadAttrib("in_position", positions);
  if (shader.attrib("in_normal", false) != -1) {
    shader.uploadAttrib("in_normal", normals);
  }

  shader.drawArray(GL_TRIANGLE_STRIP, 0, 4);
#ifdef LEAK_PATCH_ON
  shader.freeAttrib("in_position");
  if (shader.attrib("in_normal", false) != -1) {
    shader.freeAttrib("in_normal");
  }
#endif
}
