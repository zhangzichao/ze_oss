// Copyright (C) ETH Zurich, Wyss Zurich, Zurich Eye - All Rights Reserved
// Unauthorized copying of this file, via any medium is strictly prohibited
// Proprietary and confidential

#pragma once

#include <string>

#include <ze/imu/accelerometer_model.hpp>
#include <ze/imu/gyroscope_model.hpp>
#include <ze/imu/imu_yaml_serialization.hpp>
#include <ze/common/macros.hpp>
#include <ze/common/types.hpp>

namespace ze {

//! Imu Model
class ImuModel
{
public:
  ZE_POINTER_TYPEDEFS(ImuModel);

  typedef VectorX measurement_t;

  ImuModel() = delete;

  ImuModel(const AccelerometerModel::Ptr accelerometer,
           const GyroscopeModel::Ptr gyroscope);

  //! Load an imu from a yaml file. Returns a nullptr if the loading fails.
  static Ptr loadFromYaml(const std::string& path);

  void setLabel(const std::string& label) { label_ = label; }
  void setId(const std::string& id) { id_ = id; }
  std::string label() const { return label_; }
  std::string id() const { return id_; }

  Vector6 distort(const Eigen::Ref<const measurement_t>& primary,
                  const Eigen::Ref<const measurement_t>& secondary) const;
  Vector6 undistort(const Eigen::Ref<const measurement_t>& primary,
                    const Eigen::Ref<const measurement_t>& secondary) const;

  // getters
  inline const AccelerometerModel::Ptr accelerometerModel() const
  {
    return accelerometerModel_;
  }
  inline const GyroscopeModel::Ptr gyroscopeModel() const
  {
    return gyroscopeModel_;
  }

private:
  std::string id_;
  std::string label_;

  const AccelerometerModel::Ptr accelerometerModel_;
  const GyroscopeModel::Ptr gyroscopeModel_;
};

} // namespace ze