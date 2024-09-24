#!/usr/bin/env groovy
pipeline {
  agent none
  agent {
    label 'docker'
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

    booleanParam(
      defaultValue: true,
      description: 'Check to enable Linux build',
      name: 'shouldBuildLinux')

    booleanParam(
      defaultValue: true,
      description: 'Check to enable Windows build',
      name: 'shouldBuildWindows')

    booleanParam(
      defaultValue: true,
      description: 'Check to enable macOS build',
      name: 'shouldBuildMac')
  }

  stages {
    stage('Build') {
      parallel {
        stage('Linux') {
          when {
            expression {
              return params.shouldBuildLinux
            }
          }
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
            script {
              if (CLEAN_BUILD) {
                sh 'rm -rf builds'
              }
              if (REBUILD_VCPKG) {
                sh 'rm -rf vcpkg/{installed,buildtree} ~/.cache/vcpkg'
              }
            }
            sh 'scripts/build-gui.sh'
          }
          post {
            always {
              zip zipFile: 'linux-clap-plugins.zip', archive: true: overwrite: true,
                glob: 'builds/ninja-vcpkg/plugins/Release/clap-plugins.clap'
              zip zipFile: 'linux-vcpkg-logs.zip', archive: true, overwrite: true,
                glob: 'vcpkg/buildtrees/**/*.log, vcpkg/installed/vcpkg/issue_body.md'
            }
          }
        }
      }
    }
  }
}
