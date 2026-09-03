#pragma once

namespace robot_utils
{

class AngleUtils
{
public:
	/**
	 * @brief 将角度归一化到[-pi, pi)范围。
	 * @param angle 输入角度，单位为弧度。
	 * @return 归一化后的角度，单位为弧度。
	 */
	static double normalizeAngle(double angle);

	/**
	 * @brief 将数值限制到给定区间。
	 * @param value 输入数值。
	 * @param minimum 下限。
	 * @param maximum 上限。
	 * @return 限制后的数值。
	 */
	static double clamp(double value, double minimum, double maximum);
};

}
