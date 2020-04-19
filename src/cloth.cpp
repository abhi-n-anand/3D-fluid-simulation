#include <iostream>
#include <math.h>
#include <random>
#include <vector>

#include "cloth.h"
#include "collision/plane.h"
#include "collision/sphere.h"

using namespace std;

Cloth::Cloth(double width, double height, int num_width_points,
             int num_height_points, float thickness) {
  this->width = width;
  this->height = height;
  this->num_width_points = num_width_points;
  this->num_height_points = num_height_points;
  this->thickness = thickness;
        
  buildGrid();
  buildClothMesh();
}

Cloth::~Cloth() {
  point_masses.clear();
  springs.clear();

  if (clothMesh) {
    delete clothMesh;
  }
}

void Cloth::buildGrid() {
  // TODO (Part 1): Build a grid of masses and springs.
    for (int j = 0; j < num_height_points; j++) {
        for (int i = 0; i < num_width_points; i++) {
            float z;
            Vector3D v;
            if (orientation == HORIZONTAL) {
                v = Vector3D(i * width / num_width_points, 1.0, j * height / num_height_points);
            } else {
                z = rand() / RAND_MAX * (2 / 1000) - (1 / 1000);
                v = Vector3D(i * width / num_width_points, j * height / num_height_points, z);
            }
            
            vector<int> target = {i, j};
            bool pin = false;
            for (std::size_t k = 0; k < pinned.size(); k++) {
                if (pinned[k] == target) pin = true;
            }
            PointMass pm = PointMass(v, pin);
            point_masses.push_back(pm);
        }
    }
    
    // structural
    for (int j = 0; j < num_height_points; j++) {
        for (int i = 0; i < num_width_points; i++) {
            // conversion formula [j * num_width_points + i]
            // change in height --> (j - 1)
            // change in width --> (i - 1)
                // this and above
               if (j > 0) springs.push_back(Spring(&point_masses[j * num_width_points + i], &point_masses[(j - 1) * num_width_points + i], STRUCTURAL));
                // this and left
                if (i > 0) springs.push_back(Spring(&point_masses[j * num_width_points + i], &point_masses[j * num_width_points + i - 1], STRUCTURAL));
                // this and diag left
               if ((i > 0) && (j > 0)) springs.push_back(Spring(&point_masses[j * num_width_points + i], &point_masses[(j - 1) * num_width_points + (i - 1)], SHEARING));
                // this and diag right
                if ((j > 0) && (i < num_width_points - 1)) springs.push_back(Spring(&point_masses[j * num_width_points + i], &point_masses[(j - 1) * num_width_points + (i + 1)], SHEARING));
                // this and 2 above
                if (j > 1) springs.push_back(Spring(&point_masses[j * num_width_points + i], &point_masses[(j - 2) * num_width_points + i], BENDING));
                // this and 2 left
                if (i > 1) springs.push_back(Spring(&point_masses[j * num_width_points + i], &point_masses[j * num_width_points + i - 2], BENDING));

            
        }
    }
}


void Cloth::simulate(double frames_per_sec, double simulation_steps, ClothParameters *cp,
                     vector<Vector3D> external_accelerations,
                     vector<CollisionObject *> *collision_objects) {
    
  double mass = width * height * cp->density / num_width_points / num_height_points;
  double delta_t = 1.0f / frames_per_sec / simulation_steps;

  // TODO (Part 2): Compute total force acting on each point mass.
    // total external force
    Vector3D acceleration = Vector3D(0, 0, 0);
    for (Vector3D &accel : external_accelerations) acceleration += accel;
    for (PointMass &pm : point_masses) pm.forces = mass * acceleration;
    // Hooke's law

    for (Spring &s: springs) {
        if (s.spring_type == STRUCTURAL && cp->enable_structural_constraints) {
            Vector3D diff = s.pm_b -> position - s.pm_a -> position;
            Vector3D diffcopy = Vector3D(diff.x, diff.y, diff.z);
            diffcopy.normalize();
            Vector3D force = cp -> ks * (diff.norm() - s.rest_length) * diffcopy;
            s.pm_a -> forces += force;
            s.pm_b -> forces -= force;
        }
        if (s.spring_type == BENDING && cp->enable_bending_constraints) {
            Vector3D diff = s.pm_b -> position - s.pm_a -> position;
            Vector3D diffcopy = Vector3D(diff.x, diff.y, diff.z);
            diffcopy.normalize();
            Vector3D force = cp -> ks * (diff.norm() - s.rest_length) * diffcopy;
            s.pm_a -> forces += force;
            s.pm_b -> forces -= force;
        }
        if (s.spring_type == SHEARING && cp -> enable_shearing_constraints) {
            Vector3D diff = s.pm_b -> position - s.pm_a -> position;
            Vector3D diffcopy = Vector3D(diff.x, diff.y, diff.z);
            diffcopy.normalize();
            Vector3D force = cp -> ks * (diff.norm() - s.rest_length) * diffcopy;
            s.pm_a -> forces += force;
            s.pm_b -> forces -= force;
        }
    }

  // TODO (Part 2): Use Verlet integration to compute new point mass positions
    for (PointMass &pm : point_masses) {
        if (pm.pinned) continue;
        Vector3D lastPos = pm.last_position;
        pm.last_position = pm.position;
        pm.position = pm.last_position + (1.0 - cp -> damping / 100.0) * (pm.last_position - lastPos) + pow(delta_t, 2) * pm.forces / mass;
    }
    

  // TODO (Part 4): Handle self-collisions.


  // TODO (Part 3): Handle collisions with other primitives.


  // TODO (Part 2): Constrain the changes to be such that the spring does not change
  // in length more than 10% per timestep [Provot 1995].
    for (int i = 0; i < springs.size(); i++) {
        Spring s = springs[i];
        if (s.pm_a -> pinned && s.pm_b -> pinned) continue;
        double curLen = (s.pm_a -> position - s.pm_b -> position).norm();
        double lastLen = (s.pm_a -> last_position - s.pm_b -> last_position).norm();
        if (abs(curLen - lastLen)/lastLen > 0.1) {
            double reduction = curLen - lastLen * 1.1;
            if (s.pm_a -> pinned && !s.pm_b -> pinned) {
                s.pm_b -> position += (s.pm_a -> position - s.pm_b -> position).norm() * reduction;
            }
            if (s.pm_b -> pinned && !s.pm_a -> pinned) {
                s.pm_a -> position += (s.pm_b -> position - s.pm_a -> position).norm() * reduction;
            }
            if (!s.pm_a -> pinned && !s.pm_b -> pinned) {
                s.pm_a -> position += (s.pm_a -> position - s.pm_b -> position).norm() * reduction / 2.0;
                s.pm_b -> position += (s.pm_b -> position - s.pm_a -> position).norm() * reduction / 2.0;
            }
            // update rest length
        }
    }

}

void Cloth::build_spatial_map() {
  for (const auto &entry : map) {
    delete(entry.second);
  }
  map.clear();

  // TODO (Part 4): Build a spatial map out of all of the point masses.

}

void Cloth::self_collide(PointMass &pm, double simulation_steps) {
  // TODO (Part 4): Handle self-collision for a given point mass.

}

float Cloth::hash_position(Vector3D pos) {
  // TODO (Part 4): Hash a 3D position into a unique float identifier that represents membership in some 3D box volume.

  return 0.f; 
}

///////////////////////////////////////////////////////
/// YOU DO NOT NEED TO REFER TO ANY CODE BELOW THIS ///
///////////////////////////////////////////////////////

void Cloth::reset() {
  PointMass *pm = &point_masses[0];
  for (int i = 0; i < point_masses.size(); i++) {
    pm->position = pm->start_position;
    pm->last_position = pm->start_position;
    pm++;
  }
}

void Cloth::buildClothMesh() {
  if (point_masses.size() == 0) return;

  ClothMesh *clothMesh = new ClothMesh();
  vector<Triangle *> triangles;

  // Create vector of triangles
  for (int y = 0; y < num_height_points - 1; y++) {
    for (int x = 0; x < num_width_points - 1; x++) {
      PointMass *pm = &point_masses[y * num_width_points + x];
      // Get neighboring point masses:
      /*                      *
       * pm_A -------- pm_B   *
       *             /        *
       *  |         /   |     *
       *  |        /    |     *
       *  |       /     |     *
       *  |      /      |     *
       *  |     /       |     *
       *  |    /        |     *
       *      /               *
       * pm_C -------- pm_D   *
       *                      *
       */
      
      float u_min = x;
      u_min /= num_width_points - 1;
      float u_max = x + 1;
      u_max /= num_width_points - 1;
      float v_min = y;
      v_min /= num_height_points - 1;
      float v_max = y + 1;
      v_max /= num_height_points - 1;
      
      PointMass *pm_A = pm                       ;
      PointMass *pm_B = pm                    + 1;
      PointMass *pm_C = pm + num_width_points    ;
      PointMass *pm_D = pm + num_width_points + 1;
      
      Vector3D uv_A = Vector3D(u_min, v_min, 0);
      Vector3D uv_B = Vector3D(u_max, v_min, 0);
      Vector3D uv_C = Vector3D(u_min, v_max, 0);
      Vector3D uv_D = Vector3D(u_max, v_max, 0);
      
      
      // Both triangles defined by vertices in counter-clockwise orientation
      triangles.push_back(new Triangle(pm_A, pm_C, pm_B, 
                                       uv_A, uv_C, uv_B));
      triangles.push_back(new Triangle(pm_B, pm_C, pm_D, 
                                       uv_B, uv_C, uv_D));
    }
  }

  // For each triangle in row-order, create 3 edges and 3 internal halfedges
  for (int i = 0; i < triangles.size(); i++) {
    Triangle *t = triangles[i];

    // Allocate new halfedges on heap
    Halfedge *h1 = new Halfedge();
    Halfedge *h2 = new Halfedge();
    Halfedge *h3 = new Halfedge();

    // Allocate new edges on heap
    Edge *e1 = new Edge();
    Edge *e2 = new Edge();
    Edge *e3 = new Edge();

    // Assign a halfedge pointer to the triangle
    t->halfedge = h1;

    // Assign halfedge pointers to point masses
    t->pm1->halfedge = h1;
    t->pm2->halfedge = h2;
    t->pm3->halfedge = h3;

    // Update all halfedge pointers
    h1->edge = e1;
    h1->next = h2;
    h1->pm = t->pm1;
    h1->triangle = t;

    h2->edge = e2;
    h2->next = h3;
    h2->pm = t->pm2;
    h2->triangle = t;

    h3->edge = e3;
    h3->next = h1;
    h3->pm = t->pm3;
    h3->triangle = t;
  }

  // Go back through the cloth mesh and link triangles together using halfedge
  // twin pointers

  // Convenient variables for math
  int num_height_tris = (num_height_points - 1) * 2;
  int num_width_tris = (num_width_points - 1) * 2;

  bool topLeft = true;
  for (int i = 0; i < triangles.size(); i++) {
    Triangle *t = triangles[i];

    if (topLeft) {
      // Get left triangle, if it exists
      if (i % num_width_tris != 0) { // Not a left-most triangle
        Triangle *temp = triangles[i - 1];
        t->pm1->halfedge->twin = temp->pm3->halfedge;
      } else {
        t->pm1->halfedge->twin = nullptr;
      }

      // Get triangle above, if it exists
      if (i >= num_width_tris) { // Not a top-most triangle
        Triangle *temp = triangles[i - num_width_tris + 1];
        t->pm3->halfedge->twin = temp->pm2->halfedge;
      } else {
        t->pm3->halfedge->twin = nullptr;
      }

      // Get triangle to bottom right; guaranteed to exist
      Triangle *temp = triangles[i + 1];
      t->pm2->halfedge->twin = temp->pm1->halfedge;
    } else {
      // Get right triangle, if it exists
      if (i % num_width_tris != num_width_tris - 1) { // Not a right-most triangle
        Triangle *temp = triangles[i + 1];
        t->pm3->halfedge->twin = temp->pm1->halfedge;
      } else {
        t->pm3->halfedge->twin = nullptr;
      }

      // Get triangle below, if it exists
      if (i + num_width_tris - 1 < 1.0f * num_width_tris * num_height_tris / 2.0f) { // Not a bottom-most triangle
        Triangle *temp = triangles[i + num_width_tris - 1];
        t->pm2->halfedge->twin = temp->pm3->halfedge;
      } else {
        t->pm2->halfedge->twin = nullptr;
      }

      // Get triangle to top left; guaranteed to exist
      Triangle *temp = triangles[i - 1];
      t->pm1->halfedge->twin = temp->pm2->halfedge;
    }

    topLeft = !topLeft;
  }

  clothMesh->triangles = triangles;
  this->clothMesh = clothMesh;
}
