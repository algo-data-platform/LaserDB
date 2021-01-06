#!/bin/bash
# Usage: sh build.sh [commit-id|ci] [build-mod] [all|control|batch_update_manager]

SERVICE_NAME=laser_control
NEXUS_URL="http://example/repository/ad-core"
NEXUS_AUTH="auth_token"
PRODUCTION_MODE="prod"
STAGE_MODE="stage"
BATCH_UPDATE_MANAGER="batch_update_manager"
BATCH_UPDATE_MANAGER_SERVICE_NAME="laser_batch_update_manager"

function check_commit() {
  LAST_COMMIT_ORIGIN=$(get_last_commit_id $1)
  LAST_COMMIT_HEAD=$(git rev-parse --short HEAD)
  LAST_COMMIT_HEAD=${LAST_COMMIT_HEAD:0:7}
  if [ $LAST_COMMIT_ORIGIN != $LAST_COMMIT_HEAD ]; then
    echo "Last commit origin $LAST_COMMIT_ORIGIN not equal head $LAST_COMMIT_HEAD"
    exit 1
  fi

  if [ "0" != $(git status -s | wc -l)"" ]; then
    echo "Current git repo has untrack or not commit file."
    git status
    exit 1
  fi
}

function get_last_commit_id() {
  LAST_COMMIT_ORIGIN=$(git rev-parse --short origin/master)
  if [ "" != ""$1 ]; then
    LAST_COMMIT_ORIGIN=${1:0:7}
  fi
  echo $LAST_COMMIT_ORIGIN
}

function build_control() {
  cd $RUN_DIR/
  ret=$(cp -r ./templates/* $BUILD_DIR/templates)
  echo "cp template to package, ret : $ret"
  go env -w GO111MODULE=on
  go env -w GOPROXY=https://goproxy.io,direct
  go get github.com/swaggo/swag/cmd/swag
  GOPATH=$(go env GOPATH)
  export PATH=$GOPATH/bin:$PATH

  swag init -g apis/router.go
  go build main.go

  if [ $? -ne 0 ]; then
    echo "Build main.go fail."
    exit 1
  fi

  mv main $BUILD_DIR/bin/$SERVICE_NAME

  cd $RUN_DIR/frontend

  npm install

  if [ -n "$1" ]; then
    if [ "$1" == "$STAGE_MODE" ]; then
      echo "Build in staging mode..."
      npm run build:stage
    elif [ "$1" == "$PRODUCTION_MODE" ]; then
      echo "Build in production mode..."
      npm run build:prod
    else
      echo "Unknown build mode, build failed!"
      exit 1
    fi
  else
    echo "Build in production mode..."
    npm run build:prod
  fi

  if [ $? -ne 0 ]; then
    echo "Build frontend fail."
    exit 1
  fi

  mv dist $BUILD_DIR/frontend/
  if [ $TAG_NAME != "ci" ]; then
    cd $(dirname $BUILD_DIR)
    tar -zcf $TAG_NAME.tar.gz $TAG_NAME/frontend $TAG_NAME/$SERVICE_NAME $TAG_NAME/templates
    curl -v --user $NEXUS_AUTH --upload-file $TAG_NAME.tar.gz $NEXUS_URL/release/$SERVICE_NAME/$TAG_NAME.tar.gz
  fi
}

function build_batch_upload_manager() {
  cd $RUN_DIR/$BATCH_UPDATE_MANAGER
  mvn package
  if [ $? -ne 0 ]; then
    echo "Build batch update manager fail."
    exit 1
  fi
  BATCH_UPLOAD_MANAGER_VERSION=$(mvn help:evaluate -Dexpression=project.version -q -DforceStdout)
  BATCH_UPLOAD_MANAGER_PACKAGE_NAME=$BATCH_UPDATE_MANAGER"-"$BATCH_UPLOAD_MANAGER_VERSION".jar"
  mv ./target/$BATCH_UPLOAD_MANAGER_PACKAGE_NAME $BUILD_DIR/bin/$BATCH_UPDATE_MANAGER_SERVICE_NAME.jar
  rm -rf ./target

  if [ $TAG_NAME != "ci" ]; then
    cd $(dirname $BUILD_DIR)
    tar -zcf $TAG_NAME.tar.gz $TAG_NAME/$BATCH_UPDATE_MANAGER_SERVICE_NAME.jar
    curl -v --user $NEXUS_AUTH --upload-file $TAG_NAME.tar.gz $NEXUS_URL/release/$BATCH_UPDATE_MANAGER_SERVICE_NAME/$TAG_NAME.tar.gz
  fi
}

if [ $# -eq 0 ]; then
  TAG_NAME=test
elif [ "ci" == ""$1 ]; then
  TAG_NAME=ci
else
  check_commit $1
  TAG_NAME=$(get_last_commit_id $1)
fi

RUN_DIR=$(
  cd $(dirname $0)
  pwd
)
BUILD_DIR=$RUN_DIR/build/$TAG_NAME

if [ -d $BUILD_DIR ]; then
  rm -rf $BUILD_DIR
fi
mkdir -p $BUILD_DIR/bin
mkdir -p $BUILD_DIR/frontend
mkdir -p $BUILD_DIR/templates

# build package
if [[ ! -n "$3" ]] || [[ "$3" == "all" ]]; then
  build_control $2
  build_batch_upload_manager
elif [ "$3" == "control" ]; then
  build_control $2
elif [ "$3" == "batch_update_manager" ]; then
  build_batch_upload_manager
else
  echo "Unknown build module, build failed!"
  exit 1
fi
