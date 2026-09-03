#include "robot_utils/angle_utils.hpp"

#include <algorithm>
#include <cmath>

namespace robot_utils
{

double AngleUtils::normalizeAngle(double angle)
{
	constexpr double pi = 3.14159265358979323846;
	constexpr double two_pi = 2.0 * pi;
	angle = std::fmod(angle + pi, two_pi);

	if (angle < 0.0)
	{
		angle += two_pi;
	}

	return angle - pi;
}

double AngleUtils::clamp(double value, double minimum, double maximum)
{
	return std::clamp(value, minimum, maximum);
}

}
