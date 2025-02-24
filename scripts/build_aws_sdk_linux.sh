# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

CONFIGURATION=$1

export ROOT_REPO_PATH=$(cd "$(dirname "$0")/.."; pwd -P)

export AWS_SDK_CPP_TAG="1.11.481"
export AWS_SDK_PATH="${ROOT_REPO_PATH}/aws_sdk"
export SRC_DIR="${AWS_SDK_PATH}/aws_sdk_cpp"
export BUILD_DIR="${AWS_SDK_PATH}/build"
export INSTALL_DIR="${AWS_SDK_PATH}/install"

mkdir -p ${SRC_DIR} ${BUILD_DIR} ${INSTALL_DIR}
pushd $BUILD_DIR

git clone --recurse-submodules -b "$AWS_SDK_CPP_TAG" "https://github.com/aws/aws-sdk-cpp.git" ${SRC_DIR}

cmake -S ${SRC_DIR} \
  -B $BUILD_DIR \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}"\
  -DCMAKE_BUILD_TYPE="${CONFIGURATION}" \
  -DBUILD_ONLY="rds;secretsmanager;sts" \
  -DENABLE_TESTING="OFF" \
  -DBUILD_SHARED_LIBS="OFF" \
  -DCPP_STANDARD="20"

cmake --build . --config=${CONFIGURATION}
cmake --install . --config=${CONFIGURATION}

popd