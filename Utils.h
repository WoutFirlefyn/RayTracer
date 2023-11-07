#pragma once
#include <cassert>
#include <fstream>
#include "Math.h"
#include "DataTypes.h"

namespace dae
{
	namespace GeometryUtils
	{
#pragma region Sphere HitTest
		//SPHERE HIT-TESTS
		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			const Vector3& rayToSphere{ sphere.origin - ray.origin };
			const float tCa{ Vector3::Dot(rayToSphere, ray.direction) };
			const float odSqr{ rayToSphere.SqrMagnitude() - Square(tCa) };

			if (sphere.radius * sphere.radius > odSqr) 
			{
				const float tHc{ sqrtf(Square(sphere.radius) - odSqr) };
				float t{ tCa - tHc };
				if (t < ray.min) t = tCa + tHc;
				if (t > ray.min && t < ray.max)
				{
					if (t < hitRecord.t && !ignoreHitRecord)
					{
						hitRecord.didHit = true;
						hitRecord.materialIndex = sphere.materialIndex;
						hitRecord.t = t;
						hitRecord.origin = ray.origin + ray.direction * t;
						hitRecord.normal = (hitRecord.origin - sphere.origin) / sphere.radius;
					}
					return true;
				}
			}
			return false;
		}

		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray)
		{
			const Vector3& rayToSphere{ sphere.origin - ray.origin };
			const float tCa{ Vector3::Dot(rayToSphere, ray.direction) };
			const float odSqr{ rayToSphere.SqrMagnitude() - Square(tCa) };

			if (sphere.radius * sphere.radius <= odSqr) 
				return false;

			const float tHc{ sqrtf(Square(sphere.radius) - odSqr) };
			float t{ tCa - tHc };

			if (t < ray.min) t = tCa + tHc;
			return (t > ray.min && t < ray.max);
		}
#pragma endregion
#pragma region Plane HitTest
		//PLANE HIT-TESTS
		inline bool HitTest_Plane(const Plane& plane, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			const float t{ Vector3::Dot(plane.origin - ray.origin, plane.normal) / Vector3::Dot(ray.direction, plane.normal) };
			
			if (t > ray.min && t < ray.max)
			{
				if (t < hitRecord.t && !ignoreHitRecord)
				{
					hitRecord.didHit = true;
					hitRecord.t = t;
					hitRecord.materialIndex = plane.materialIndex;
					hitRecord.origin = ray.origin + ray.direction * t;
					hitRecord.normal = plane.normal;
				}
				return true;
			}
			return false;
		}

		inline bool HitTest_Plane(const Plane& plane, const Ray& ray)
		{
			const float t{ Vector3::Dot(plane.origin - ray.origin, plane.normal) / Vector3::Dot(ray.direction, plane.normal) };
			return (t > ray.min && t < ray.max);
		}
#pragma endregion
#pragma region Triangle HitTest
		//TRIANGLE HIT-TESTS
		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			const float dot{ Vector3::Dot(triangle.normal, ray.direction) };
			switch (triangle.cullMode)
			{
			case TriangleCullMode::FrontFaceCulling:
				if (dot < 0) return false;
				break;
			case TriangleCullMode::BackFaceCulling:
				if (dot > 0) return false;
				break;
			default:
				break;
			}
			if (AreEqual(dot, 0.f))
				return false;
			const float t{ Vector3::Dot(triangle.v0 - ray.origin, triangle.normal) / dot };
			if (t < ray.min || t > ray.max)
				return false;
			const Vector3& hitPoint{ ray.origin + ray.direction * t };

			Vector3 e = triangle.v1 - triangle.v0;
			Vector3 p = hitPoint - triangle.v0;
			if (Vector3::Dot(Vector3::Cross(e, p), triangle.normal) < 0)
				return false;

			e = triangle.v2 - triangle.v1;
			p = hitPoint - triangle.v1;
			if (Vector3::Dot(Vector3::Cross(e, p), triangle.normal) < 0)
				return false;

			e = triangle.v0 - triangle.v2;
			p = hitPoint - triangle.v2;
			if (Vector3::Dot(Vector3::Cross(e, p), triangle.normal) < 0)
				return false;

			if (t < hitRecord.t && !ignoreHitRecord)
			{
				hitRecord.didHit = true;
				hitRecord.t = t;
				hitRecord.materialIndex = triangle.materialIndex;
				hitRecord.origin = hitPoint;
				hitRecord.normal = triangle.normal;
			}
			return true;
		}

		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray)
		{
			const float dot{ Vector3::Dot(triangle.normal, ray.direction) };
			switch (triangle.cullMode)
			{
			case TriangleCullMode::FrontFaceCulling:
				if (-dot < 0) return false;
				break;
			case TriangleCullMode::BackFaceCulling:
				if (-dot > 0) return false;
				break;
			default:
				break;
			}
			if (AreEqual(dot, 0.f))
				return false;
			const float t{ Vector3::Dot(triangle.v0 - ray.origin, triangle.normal) / dot };
			if (t < ray.min || t > ray.max)
				return false;
			const Vector3& hitPoint{ ray.origin + ray.direction * t };

			Vector3 e = triangle.v1 - triangle.v0;
			Vector3 p = hitPoint - triangle.v0;
			if (Vector3::Dot(Vector3::Cross(e, p), triangle.normal) < 0)
				return false;
			
			e = triangle.v2 - triangle.v1;
			p = hitPoint - triangle.v1;
			if (Vector3::Dot(Vector3::Cross(e, p), triangle.normal) < 0)
				return false;

			e = triangle.v0 - triangle.v2;
			p = hitPoint - triangle.v2;
			if (Vector3::Dot(Vector3::Cross(e, p), triangle.normal) < 0)
				return false;

			return true;
		}
#pragma endregion
#pragma region TriangleMesh HitTest
		inline bool SlabTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			float t1 = (mesh.transformedMinAABB.x - ray.origin.x) / ray.direction.x;
			float t2 = (mesh.transformedMaxAABB.x - ray.origin.x) / ray.direction.x;

			float tmin = std::min(t1, t2);
			float tmax = std::max(t1, t2);

			if (tmax < tmin)
				return false;

			t1 = (mesh.transformedMinAABB.y - ray.origin.y) / ray.direction.y;
			t2 = (mesh.transformedMaxAABB.y - ray.origin.y) / ray.direction.y;

			tmin = std::max(tmin, std::min(t1, t2));
			tmax = std::min(tmax, std::max(t1, t2));

			if (tmax < tmin)
				return false;

			t1 = (mesh.transformedMinAABB.z - ray.origin.z) / ray.direction.z;
			t2 = (mesh.transformedMaxAABB.z - ray.origin.z) / ray.direction.z;

			tmin = std::max(tmin, std::min(t1, t2));
			tmax = std::min(tmax, std::max(t1, t2));

			return tmax > 0 && tmax >= tmin;
		}

		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			if (!SlabTest_TriangleMesh(mesh,ray)) return false;
			Triangle currTriangle{};
			currTriangle.cullMode = mesh.cullMode;
			currTriangle.materialIndex = mesh.materialIndex;

			for (int i{}; i < mesh.indices.size(); i += 3)
			{
				currTriangle.v0 = mesh.transformedPositions[mesh.indices[i]];
				currTriangle.v1 = mesh.transformedPositions[mesh.indices[i + 1]];
				currTriangle.v2 = mesh.transformedPositions[mesh.indices[i + 2]];
				currTriangle.normal = mesh.transformedNormals[i / 3];
				HitTest_Triangle(currTriangle, ray, hitRecord, ignoreHitRecord);
			}
			return hitRecord.didHit;
		}

		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			if (!SlabTest_TriangleMesh(mesh, ray)) return false;
			Triangle currTriangle{};
			currTriangle.cullMode = mesh.cullMode;

			for (int i{}; i < mesh.indices.size(); i += 3)
			{
				currTriangle.v0 = mesh.transformedPositions[mesh.indices[i]];
				currTriangle.v1 = mesh.transformedPositions[mesh.indices[i + 1]];
				currTriangle.v2 = mesh.transformedPositions[mesh.indices[i + 2]];
				currTriangle.normal = mesh.transformedNormals[i / 3];
				if (HitTest_Triangle(currTriangle, ray))
					return true;
			}
			return false;
		}
#pragma endregion
	}

	namespace LightUtils
	{
		//Direction from target to light
		inline Vector3 GetDirectionToLight(const Light& light, const Vector3& origin)
		{
			return { light.origin - origin };
		}

		inline ColorRGB GetRadiance(const Light& light, const Vector3& target)
		{
			ColorRGB radiance{};
			radiance = light.color * light.intensity;
			if (light.type == LightType::Point)
				radiance /= (light.origin - target).SqrMagnitude();
			return radiance;
		}
	}

	namespace Utils
	{
		//Just parses vertices and indices
#pragma warning(push)
#pragma warning(disable : 4505) //Warning unreferenced local function
		static bool ParseOBJ(const std::string& filename, std::vector<Vector3>& positions, std::vector<Vector3>& normals, std::vector<int>& indices)
		{
			std::ifstream file(filename);
			if (!file)
				return false;

			std::string sCommand;
			// start a while iteration ending when the end of file is reached (ios::eof)
			while (!file.eof())
			{
				//read the first word of the string, use the >> operator (istream::operator>>) 
				file >> sCommand;
				//use conditional statements to process the different commands	
				if (sCommand == "#")
				{
					// Ignore Comment
				}
				else if (sCommand == "v")
				{
					//Vertex
					float x, y, z;
					file >> x >> y >> z;
					positions.push_back({ x, y, z });
				}
				else if (sCommand == "f")
				{
					float i0, i1, i2;
					file >> i0 >> i1 >> i2;

					indices.push_back((int)i0 - 1);
					indices.push_back((int)i1 - 1);
					indices.push_back((int)i2 - 1);
				}
				//read till end of line and ignore all remaining chars
				file.ignore(1000, '\n');

				if (file.eof()) 
					break;
			}

			//Precompute normals
			for (uint64_t index = 0; index < indices.size(); index += 3)
			{
				uint32_t i0 = indices[index];
				uint32_t i1 = indices[index + 1];
				uint32_t i2 = indices[index + 2];

				Vector3 edgeV0V1 = positions[i1] - positions[i0];
				Vector3 edgeV0V2 = positions[i2] - positions[i0];
				Vector3 normal = Vector3::Cross(edgeV0V1, edgeV0V2);

				if(isnan(normal.x))
				{
					int k = 0;
				}

				normal.Normalize();
				if (isnan(normal.x))
				{
					int k = 0;
				}

				normals.push_back(normal);
			}

			return true;
		}
#pragma warning(pop)
	}
}