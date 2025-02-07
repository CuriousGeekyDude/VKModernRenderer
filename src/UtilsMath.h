#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <vector>
#include <array>
#include "GeometryConverter.hpp"

using glm::vec3;
using glm::vec4;

namespace Math
{
	static constexpr float PI = 3.14159265359f;
	static constexpr float TWOPI = 6.28318530718f;
}

struct BoundingBox
{
	vec3 min_;
	vec3 max_;
	BoundingBox() = default;
	BoundingBox(const vec3& min, const vec3& max) : min_(glm::min(min, max)), max_(glm::max(min, max)) {}
	BoundingBox(const vec3* points, size_t numPoints)
	{
		vec3 vmin(std::numeric_limits<float>::max());
		vec3 vmax(std::numeric_limits<float>::lowest());

		for (size_t i = 0; i != numPoints; i++)
		{
			vmin = glm::min(vmin, points[i]);
			vmax = glm::max(vmax, points[i]);
		}
		min_ = vmin;
		max_ = vmax;
	}
	vec3 getSize() const { return vec3(max_[0] - min_[0], max_[1] - min_[1], max_[2] - min_[2]); }
	vec3 getCenter() const { return 0.5f * vec3(max_[0] + min_[0], max_[1] + min_[1], max_[2] + min_[2]); }
	void transform(const glm::mat4& t)
	{
		vec3 corners[] = {
			vec3(min_.x, min_.y, min_.z),
			vec3(min_.x, max_.y, min_.z),
			vec3(min_.x, min_.y, max_.z),
			vec3(min_.x, max_.y, max_.z),
			vec3(max_.x, min_.y, min_.z),
			vec3(max_.x, max_.y, min_.z),
			vec3(max_.x, min_.y, max_.z),
			vec3(max_.x, max_.y, max_.z),
		};
		for (auto& v : corners)
			v = vec3(t * vec4(v, 1.0f));
		*this = BoundingBox(corners, 8);
	}
	BoundingBox getTransformed(const glm::mat4& t) const
	{
		BoundingBox b = *this;
		b.transform(t);
		return b;
	}
	void combinePoint(const vec3& p)
	{
		min_ = glm::min(min_, p);
		max_ = glm::max(max_, p);
	}
};

template <typename T>
T clamp(T v, T a, T b)
{
	if (v < a) return a;
	if (v > b) return b;
	return v;
}

inline float random01()
{
	return (float)rand() / (float)RAND_MAX;
}

inline float randomFloat(float min, float max)
{
	return min + (max- min) * random01();
}

inline glm::vec3 randomVec(const glm::vec3& min, const glm::vec3& max)
{
	return glm::vec3(randomFloat(min.x, max.x), randomFloat(min.y, max.y), randomFloat(min.z, max.z));
}

inline glm::vec3 randVec()
{
	return randomVec(glm::vec3(-5, -5, -5), glm::vec3(5, 5, 5));
}

inline void getFrustumPlanes(const glm::mat4& mvp, std::array<glm::vec3, 6>& planes)
{
	using glm::vec3;
	auto lv_newmvp = glm::transpose(mvp);

	auto lv_left = lv_newmvp[3] + lv_newmvp[0];
	auto lv_right = lv_newmvp[3] - lv_newmvp[0];
	auto lv_bottom = lv_newmvp[3] + lv_newmvp[1];
	auto lv_top = lv_newmvp[3] - lv_newmvp[1];
	auto lv_near = lv_newmvp[3] + lv_newmvp[2];
	auto lv_far = lv_newmvp[3] - lv_newmvp[2];

	planes[0] = vec3(lv_left.x/lv_left.w, lv_left.y/lv_left.w, lv_left.z/lv_left.w); // left
	planes[1] = vec3(lv_right.x/lv_right.w, lv_right.y/lv_right.w, lv_right.z/lv_right.w); // right
	planes[2] = (1.f/lv_bottom.w)*vec3(lv_bottom.x, lv_bottom.y, lv_bottom.z); // bottom
	planes[3] = (1.f/lv_top.w)*vec3(lv_top.x, lv_top.y, lv_top.z); // top
	planes[4] = (1.f/lv_near.w)*vec3(lv_near.x, lv_near.y, lv_near.z); // near
	planes[5] = (1.f/lv_far.w)*vec3(lv_far.x, lv_far.y, lv_far.z); // far

}

inline void getFrustumCorners(const glm::mat4& mvp, std::array<glm::vec3, 8>& points)
{
	using glm::vec4;

	const vec4 corners[] = {
		vec4(-1, -1, -1, 1), vec4(1, -1, -1, 1), vec4(1, 1, -1, 1), vec4(-1, 1, -1, 1),
		vec4(-1, -1, 1, 1),  vec4(1, -1, 1, 1),  vec4(1, 1, 1, 1),  vec4(-1, 1, 1, 1)
	};

	const glm::mat4 invMVP = glm::inverse(mvp);

	for (int i = 0; i < 8; i++) {
		vec4 q = invMVP * corners[i];
		q = q / q.w;
		points[i] = glm::vec3{ q.x, q.y, q.z };
	}
}

inline bool isBoxInFrustum(const std::array<glm::vec3, 6>& frustumPlanes, 
	const std::array<glm::vec3, 8>& frustumCorners, 
	const MeshConverter::GeometryConverter::BoundingBox& box)
{
	using glm::dot;
	using glm::vec3;

	for ( int i = 0; i < 6; i++ ) {
		int r = 0;
		r += (dot(glm::vec4(frustumPlanes[i], 1.f), glm::vec4(box.m_min.x, box.m_min.y, box.m_min.z, 1.f)) < 0.0) ? 1 : 0;
		r += (dot(glm::vec4(frustumPlanes[i], 1.f), glm::vec4(box.m_max.x, box.m_min.y, box.m_min.z, 1.f)) < 0.0) ? 1 : 0;
		r += (dot(glm::vec4(frustumPlanes[i], 1.f), glm::vec4(box.m_min.x, box.m_max.y, box.m_min.z, 1.f)) < 0.0) ? 1 : 0;
		r += (dot(glm::vec4(frustumPlanes[i], 1.f), glm::vec4(box.m_max.x, box.m_max.y, box.m_min.z, 1.f)) < 0.0) ? 1 : 0;
		r += (dot(glm::vec4(frustumPlanes[i], 1.f), glm::vec4(box.m_min.x, box.m_min.y, box.m_max.z, 1.f)) < 0.0) ? 1 : 0;
		r += (dot(glm::vec4(frustumPlanes[i], 1.f), glm::vec4(box.m_max.x, box.m_min.y, box.m_max.z, 1.f)) < 0.0) ? 1 : 0;
		r += (dot(glm::vec4(frustumPlanes[i], 1.f), glm::vec4(box.m_min.x, box.m_max.y, box.m_max.z, 1.f)) < 0.0) ? 1 : 0;
		r += (dot(glm::vec4(frustumPlanes[i], 1.f), glm::vec4(box.m_max.x, box.m_max.y, box.m_max.z, 1.f)) < 0.0) ? 1 : 0;
		if (r == 8) return false;
	}

	// check frustum outside/inside box
	int r = 0;
	r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].x > box.m_max.x) ? 1 : 0); if (r == 8) return false;
	r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].x < box.m_min.x) ? 1 : 0); if (r == 8) return false;
	r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].y > box.m_max.y) ? 1 : 0); if (r == 8) return false;
	r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].y < box.m_min.y) ? 1 : 0); if (r == 8) return false;
	r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].z > box.m_max.z) ? 1 : 0); if (r == 8) return false;
	r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].z < box.m_min.z) ? 1 : 0); if (r == 8) return false;

	return true;
}

inline BoundingBox combineBoxes(const std::vector<BoundingBox>& boxes)
{
	std::vector<vec3> allPoints;
	allPoints.reserve(boxes.size() * 8);

	for (const auto& b: boxes)
	{
		allPoints.emplace_back(b.min_.x, b.min_.y, b.min_.z);
		allPoints.emplace_back(b.min_.x, b.min_.y, b.max_.z);
		allPoints.emplace_back(b.min_.x, b.max_.y, b.min_.z);
		allPoints.emplace_back(b.min_.x, b.max_.y, b.max_.z);

		allPoints.emplace_back(b.max_.x, b.min_.y, b.min_.z);
		allPoints.emplace_back(b.max_.x, b.min_.y, b.max_.z);
		allPoints.emplace_back(b.max_.x, b.max_.y, b.min_.z);
		allPoints.emplace_back(b.max_.x, b.max_.y, b.max_.z);
	}

	return BoundingBox(allPoints.data(), allPoints.size());
}
