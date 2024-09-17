#!/usr/bin/env groovy
pipeline {
  agent {
    label 'linux-docker'
  }

  parameters {
    string(
      defaultValue: 'main', name: 'BRANCH_OR_COMMIT_HASH',
      description: 'Git branch name or commit hash to build. Defaults to tip of main branch.')

    booleanParam(
      defaultValue: false,
      description: 'Cleanups the build directory.',
      name: 'CLEAN_BUILD')

    booleanParam(
      defaultValue: false,
      description: 'Rebuilds VCPKG dependencies.',
      name: 'REBUILD_VCPKG')
  }

  stages {
    stage('Build') {
      agent {
        dockerfile {
          additionalBuildArgs '--target clap-plugins-build'
          filename 'Dockerfile'
          reuseNode true
        }
      }
      options {
        timeout(time: 4, unit: 'HOURS')
      }
      steps {
        if (CLEAN_BUILD) {
          sh 'rm -rf builds'
        }
        if (REBUILD_VCPKG) {
          sh 'rm -rf vcpkg/{installed,buildtree} ~/.cache/vcpkg'
        }
        sh 'scripts/build-gui.sh'
      }
    }
  }

  post {
    always {
      archiveArtifacts artifacts: 'builds/ninja-vcpkg/plugins/Release/clap-plugins.clap', fingerprint: true
    }
  }
}
