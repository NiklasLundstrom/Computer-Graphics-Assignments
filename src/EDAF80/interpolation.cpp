#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
	//! \todo Implement this function
	glm::vec2 rV = glm::vec2(1.0f, x);
	glm::mat2 M = glm::mat2(1, 0, -1, 1);
	glm::mat2x3 cV = glm::mat2x3(p0, p1);

	return cV * M * rV;
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0, glm::vec3 const& p1,
                              glm::vec3 const& p2, glm::vec3 const& p3,
                              float const t, float const x)
{
	//! \todo Implement this function
	glm::vec4 rV = glm::vec4(1, x, glm::pow(x,2) , glm::pow(x,3));
	glm::mat4 M = glm::mat4(0, 1, 0, 0,
							-t, 0, t, 0,
							2*t, 3-t, 3-2*t, -t,
							-t, 2-t, t-2, t);
	glm::mat4x3 cV = glm::mat4x3(p0, p1, p2, p3);

	return cV* M * rV;
}
