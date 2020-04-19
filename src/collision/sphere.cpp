#include <nanogui/nanogui.h>

#include "../clothMesh.h"
#include "../misc/sphere_drawing.h"
#include "sphere.h"

using namespace nanogui;
using namespace CGL;

void Sphere::collide(PointMass &pm) {
  // TODO (Part 3): Handle collisions with spheres.
    // determine if intersects / is inside sphere
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
}

void Sphere::render(GLShader &shader) {
  // We decrease the radius here so flat triangles don't behave strangely
  // and intersect with the sphere when rendered
  m_sphere_mesh.draw_sphere(shader, origin, radius * 0.92);
}
