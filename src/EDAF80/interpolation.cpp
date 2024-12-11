#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
	//! \todo Implement this function
	auto X = glm::vec2(1.0f, x);
	auto M = glm::mat2(glm::vec2(1.0f, -1.0f), glm::vec2(0.0f, 1.0f));
	auto P = glm::mat2x3(p0, p1);
	return X * M * glm::transpose(P);
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0, glm::vec3 const& p1,
	glm::vec3 const& p2, glm::vec3 const& p3,
	float const t, float const x)
{
	//! \todo Implement this function
	auto X = glm::vec4(1.0f, x, glm::pow(x, 2.0f), glm::pow(x, 3.0f));
	auto M = glm::mat4(glm::vec4(0.0f, -t, 2 * t, -t),
		glm::vec4(1.0f, 0.0f, t - 3.0f, 2.0f - t),
		glm::vec4(0.0f, t, 3.0f - 2.0f * t, t - 2.0f),
		glm::vec4(0.0f, 0.0f, -t, t));
	auto P = glm::mat4x3(p0, p1, p2, p3);
	return X * M * glm::transpose(P);
}
