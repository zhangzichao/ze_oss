// Copyright (c) 2015-2016, ETH Zurich, Wyss Zurich, Zurich Eye
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the ETH Zurich, Wyss Zurich, Zurich Eye nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL ETH Zurich, Wyss Zurich, Zurich Eye BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <ze/vi_simulation/imu_bias_simulator.hpp>
#include <ze/vi_simulation/imu_simulator.hpp>
#include <ze/vi_simulation/trajectory_simulator.hpp>
#include <ze/common/test_entrypoint.hpp>
#include <ze/splines/bspline_pose_minimal.hpp>
#include <ze/common/types.hpp>
#include <ze/common/random_matrix.hpp>

TEST(TrajectorySimulator, testSplineScenario)
{
  using namespace ze;

  std::shared_ptr<BSplinePoseMinimalRotationVector> bs =
      std::make_shared<BSplinePoseMinimalRotationVector>(3);


  bs->initPoseSpline(10.0, 20.0,
                     bs->curveValueToTransformation(Vector6::Random()),
                     bs->curveValueToTransformation(Vector6::Random()));

  SplineTrajectorySimulator::Ptr scenario =
      std::make_shared<SplineTrajectorySimulator>(bs);

  // dependencies:
  ImuBiasSimulator::Ptr bias(std::make_shared<ConstantBiasSimulator>());
  RandomVectorSampler<3>::Ptr acc_noise =
      RandomVectorSampler<3>::sigmas(Vector3(1e-5, 1e-5, 1e-5));
  RandomVectorSampler<3>::Ptr gyr_noise =
      RandomVectorSampler<3>::sigmas(Vector3(1e-5, 1e-5, 1e-5));
  real_t gravity_magnitude = 9.81;

  // test runner
  ImuSimulator imu_simulator(
        scenario, bias, acc_noise, gyr_noise,
        100, 100, gravity_magnitude);

  for (real_t t = 10.0; t < 20.0; t += 0.1)
  {
    // the probability is really high that this test passes... but not guaranteed
    EXPECT_TRUE(EIGEN_MATRIX_NEAR(
                  imu_simulator.angularVelocityCorrupted(t),
                  imu_simulator.angularVelocityActual(t),
                  0.3));
    EXPECT_TRUE(EIGEN_MATRIX_NEAR(
                  imu_simulator.specificForceActual(t),
                  imu_simulator.specificForceCorrupted(t),
                  0.3));
  }
}

TEST(TrajectorySimulator, testConsistency)
{
  using namespace ze;

  std::shared_ptr<BSplinePoseMinimalRotationVector> bs =
      std::make_shared<BSplinePoseMinimalRotationVector>(3);

  Vector6 v1; v1 << 0, 0, 0, 1, 1, 1.;
  Vector6 v2; v2 << 0, 0, 0, 1, 1, 1.;

  bs->initPoseSpline(10.0, 20.0,
                     bs->curveValueToTransformation(v1),
                     bs->curveValueToTransformation(v2));

  SplineTrajectorySimulator::Ptr scenario =
      std::make_shared<SplineTrajectorySimulator>(bs);

  // dependencies:
  ImuBiasSimulator::Ptr bias(std::make_shared<ConstantBiasSimulator>());
  RandomVectorSampler<3>::Ptr acc_noise =
      RandomVectorSampler<3>::sigmas(Vector3(0, 0, 0));
  RandomVectorSampler<3>::Ptr gyr_noise =
      RandomVectorSampler<3>::sigmas(Vector3(0, 0, 0));
  real_t gravity_magnitude = 9.81;

  // test runner
  ImuSimulator imu_simulator(
        scenario, bias, acc_noise, gyr_noise, 100, 100, gravity_magnitude);

  EXPECT_TRUE(EIGEN_MATRIX_NEAR(
                imu_simulator.specificForceActual(11),
                Vector3(9.38534, -1.7953, 2.219960), 1e-4));

  Matrix3 R_B_I;
  R_B_I.col(2) = imu_simulator.specificForceActual(11).normalized();
  R_B_I.col(0) = - (R_B_I.col(2).cross(Vector3::UnitY())).normalized();
  R_B_I.col(1) = R_B_I.col(2).cross(R_B_I.col(0));
  R_B_I = R_B_I.transpose().eval();

  EXPECT_TRUE(EIGEN_MATRIX_NEAR(
                R_B_I.eulerAngles(2, 1, 0),
                Vector3(0.6585, -1.27549, -0.68003), 1e-4));
  EXPECT_TRUE(EIGEN_MATRIX_NEAR(
                bs->orientation(15).eulerAngles(2, 1, 0),
                Vector3(2.46156, -1.86611, 2.46156), 1e-4));

}

ZE_UNITTEST_ENTRYPOINT
