#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
	glm::vec2 rV = glm::vec2(1.0f, x);
	glm::mat2 M = glm::mat2(1.0f, 0.0f, 1.0f, -1.0f);
	glm::mat2x3 cV = glm::mat2x3(p0, p1);
	
	return cV * M * rV;
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0, glm::vec3 const& p1,
                              glm::vec3 const& p2, glm::vec3 const& p3,
                              float const t, float const x)
{
	//! \todo Implement this function
	return glm::vec3();
}
