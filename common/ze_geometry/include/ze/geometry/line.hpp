// Copyright (C) ETH Zurich, Wyss Zurich, Zurich Eye - All Rights Reserved
// Unauthorized copying of this file, via any medium is strictly prohibited
// Proprietary and confidential

#pragma once

#include <ze/common/transformation.hpp>
#include <ze/common/types.hpp>

namespace ze {

// fwd
class Camera;
class Line;

// Convenience typedefs:
using Lines = std::vector<Line>;

inline real_t distanceToLine(const Position& pos,
                                const Position& line_anchor,
                                const Vector3& line_direction)
{
  return (pos - line_anchor).cross(line_direction).norm();
}

inline LineMeasurement lineMeasurementFromBearings(const Vector3& bearing_1,
                                                   const Vector3& bearing_2)
{
  return bearing_1.cross(bearing_2).normalized();
}

Matrix26 dLineMeasurement_dPose(const Transformation& T_B_W,
                                const Transformation& T_C_B,
                                const LineMeasurement& measurement_W,
                                const Position& line_anchor,
                                const Vector3& line_direction);


std::pair<Positions, Positions> generateRandomVisibleLines(
    const Camera& cam, const Transformation& T_W_C,
    size_t num_lines, Lines& lines_W);

Lines generateLinesFromEndpoints(const Positions& startpoints,
                                const Positions& endpoints);

class Line
{
public:
  Line() = default;

  Line(Quaternion orientation, real_t distance)
  : orientation_(orientation)
  , distance_(distance) {}


  inline Vector3 direction() const
  {
    return orientation_.rotate(Vector3::UnitX());
  }

  inline Position anchorPoint() const
  {
    return distance_ * orientation_.rotate(Vector3::UnitZ());
  }

  inline real_t distanceToLine(const Position& pos) const
  {
    return ze::distanceToLine(pos, anchorPoint(), direction());
  }

  Vector2 calculateMeasurementError(const Vector3& measurement_W,
                                    const Vector3& camera_position_W) const;

private:
  Quaternion orientation_;
  real_t distance_ = 0.0;
};

} // namespace ze
