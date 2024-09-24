#!/usr/bin/env groovy
/* groovylint-disable NestedBlockDepth */
pipeline {
    agent none

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
                            if (params.CLEAN_BUILD) {
                                sh 'rm -rf builds'
                            }
                            if (params.REBUILD_VCPKG) {
                                sh 'rm -rf vcpkg/{installed,buildtree} ~/.cache/vcpkg'
                            }
                        }
                        sh 'scripts/build-gui.sh'
                    }
                    post {
                        always {
                            zip zipFile: 'linux-vcpkg-logs.zip', archive: true, overwrite: true,
                                glob: 'vcpkg/buildtrees/**/*.log, vcpkg/installed/vcpkg/issue_body.md',
                                dir: 'vcpkg/'

                            zip zipFile: 'linux-clap-plugins.zip', archive: true, overwrite: true,
                                glob: 'builds/ninja-vcpkg/plugins/Release/*.clap',
                                dir: 'builds/ninja-vcpkg/plugins/Release/'
                        }
                    }
                }
                stage('macOS') {
                    when {
                        expression {
                            return params.shouldBuildMac
                        }
                    }
                    agent {
                        label 'macos-build'
                    }
                    options {
                        timeout(time: 4, unit: 'HOURS')
                    }
                    steps {
                        script {
                            if (params.CLEAN_BUILD) {
                                sh 'rm -rf builds'
                            }
                            if (params.REBUILD_VCPKG) {
                                sh 'rm -rf vcpkg/{installed,buildtree} ~/.cache/vcpkg'
                            }
                        }
                        sh 'scripts/build-gui.sh'
                    }
                    post {
                        always {
                            zip zipFile: 'macOS-vcpkg-logs.zip', archive: true, overwrite: true,
                                glob: 'vcpkg/buildtrees/**/*.log, vcpkg/installed/vcpkg/issue_body.md',
                                dir: 'vcpkg/'

                            zip zipFile: 'macOS-clap-plugins.zip', archive: true, overwrite: true,
                                glob: 'builds/ninja-vcpkg/plugins/Release/*.clap',
                                dir: 'builds/ninja-vcpkg/plugins/Release/'
                        }
                    }
                }
                stage('Windows') {
                    when {
                        expression {
                            return params.shouldBuildWindows
                        }
                    }
                    agent {
                        label 'windows-build'
                    }
                    options {
                        timeout(time: 4, unit: 'HOURS')
                    }
                    steps {
                        // script {
                        //     if (params.CLEAN_BUILD) {
                        //         sh 'rm -rf builds'
                        //     }
                        //     if (params.REBUILD_VCPKG) {
                        //         sh 'rm -rf vcpkg/{installed,buildtree} ~/.cache/vcpkg'
                        //     }
                        // }
                        // sh 'scripts/build-gui.sh'

                        powershell 'Powershell.exe -noexit -executionpolicy bypass -File ./scripts/build-gui.ps1'
                    }
                    post {
                        always {
                            zip zipFile: 'windows-vcpkg-logs.zip', archive: true, overwrite: true,
                                glob: 'vcpkg/buildtrees/**/*.log, vcpkg/installed/vcpkg/issue_body.md',
                                dir: 'vcpkg/'

                            zip zipFile: 'windows-clap-plugins.zip', archive: true, overwrite: true,
                                glob: 'builds/vs-vcpkg/plugins/Release/*.clap',
                                dir: 'builds/vs-vcpkg/plugins/Release/'
                        }
                    }
                }
            }
        }
    }
}
