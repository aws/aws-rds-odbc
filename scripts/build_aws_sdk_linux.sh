
#
# // Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# //
# // This library is free software; you can redistribute it and/or
# // modify it under the terms of the GNU Library General Public
# // License as published by the Free Software Foundation; either
# // version 2 of the License, or (at your option) any later version.
# //
# // This library is distributed in the hope that it will be useful,
# // but WITHOUT ANY WARRANTY; without even the implied warranty of
# // MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# // Library General Public License for more details.
# //
# // You should have received a copy of the GNU Library General Public
# // License along with this library; if not, write to the Free Software
# // Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#

#This program is free software; you can redistribute it and/or modify
#it under the terms of the GNU General Public License, version 2.0
#(GPLv2), as published by the Free Software Foundation, with the
#following additional permissions:

#This program is distributed with certain software that is licensed
#under separate terms, as designated in a particular file or component
#or in the license documentation. Without limiting your rights under
#the GPLv2, the authors of this program hereby grant you an additional
#permission to link the program and your derivative works with the
#separately licensed software that they have included with the program.

#Without limiting the foregoing grant of rights under the GPLv2 and
#additional permission as to separately licensed software, this
#program is also subject to the Universal FOSS Exception, version 1.0,
#a copy of which can be found along with its FAQ at
#http://oss.oracle.com/licenses/universal-foss-exception.

#This program is distributed in the hope that it will be useful, but
#WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#See the GNU General Public License, version 2.0, for more details.

#You should have received a copy of the GNU General Public License
#along with this program. If not, see 
#http://www.gnu.org/licenses/gpl-2.0.html.

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