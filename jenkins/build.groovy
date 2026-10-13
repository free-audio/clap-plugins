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
                        timeout(time: 1, unit: 'HOURS')
                    }
                    steps {
                        script {
                            if (params.CLEAN_BUILD) {
                                sh 'rm -rf builds'
                            }
                        }
                        sh 'cmake --preset ninja && cmake --build --preset ninja --config Release'
                    }
                    post {
                        always {
                            zip zipFile: 'linux-clap-plugins.zip', archive: true, overwrite: true,
                                glob: '*.clap',
                                dir: 'builds/ninja/plugins/Release/'
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
                        timeout(time: 1, unit: 'HOURS')
                    }
                    steps {
                        script {
                            if (params.CLEAN_BUILD) {
                                sh 'rm -rf builds'
                            }
                        }
                        sh 'cmake --preset ninja && cmake --build --preset ninja --config Release'
                    }
                    post {
                        always {
                            // macOS produces a bundle which is a directory itself
                            zip zipFile: 'macOS-clap-plugins.zip', archive: true, overwrite: true,
                                glob: '*.clap/**',
                                dir: 'builds/ninja/plugins/Release/'
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
                        timeout(time: 1, unit: 'HOURS')
                    }
                    steps {
                        powershell 'cmake --preset vs; cmake --build --preset vs --config Release'
                    }
                    post {
                        always {
                            zip zipFile: 'windows-clap-plugins.zip', archive: true, overwrite: true,
                                glob: '*.clap',
                                dir: 'builds/vs/plugins/Release/'
                        }
                    }
                }
            }
        }
    }
}
