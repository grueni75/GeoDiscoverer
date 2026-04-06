# GeoDiscoverer Android Build Environment
# Extends the shell base image with all tools needed for Android development
# Based on the GCloud setup scripts

FROM docker/sandbox-templates:shell

USER root

# Install base dependencies
RUN apt-get update && apt-get install -y \
    git \
    openjdk-8-jdk \
    openjdk-11-jdk \
    openjdk-17-jdk \
    wget \
    unzip \
    make \
    rsync \
    inkscape \
    curl \
    vim \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Set Java 11 as default (needed for Gradle builds)
RUN update-alternatives --set java /usr/lib/jvm/java-11-openjdk-amd64/bin/java

USER agent

# Create directory structure
RUN mkdir -p ~/Apps/android/android-sdk ~/Android

# Download and install Android Command Line Tools
RUN cd ~/Apps/android && \
    wget -q https://dl.google.com/android/repository/commandlinetools-linux-7583922_latest.zip && \
    unzip -q commandlinetools-linux-7583922_latest.zip -d ~/Apps/android/android-sdk && \
    rm commandlinetools-linux-7583922_latest.zip

# Install Android SDK components (NDK, Platform, Build Tools)
# Note: Using yes to accept licenses
RUN cd ~/Apps/android/android-sdk && \
    yes | cmdline-tools/bin/sdkmanager --sdk_root=. "ndk;28.2.13676358" "platforms;android-33" "build-tools;35.0.0"

# Create symlink for Android SDK (standard location expected by Android Studio)
RUN ln -s ~/Apps/android/android-sdk ~/Android/Sdk

# Create androidrc file for environment setup
RUN echo '# Android environment' > ~/.androidrc && \
    echo 'export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64' >> ~/.androidrc && \
    echo 'export ANDROID_HOME=~/Apps/android/android-sdk' >> ~/.androidrc && \
    echo 'export ANDROID_SDK_ROOT=~/Apps/android/android-sdk' >> ~/.androidrc && \
    echo 'export PATH=$PATH:$ANDROID_HOME/cmdline-tools/latest/bin' >> ~/.androidrc && \
    echo 'export PATH=$PATH:$ANDROID_HOME/platform-tools' >> ~/.androidrc && \
    echo 'export PATH=$PATH:$ANDROID_HOME/ndk/28.2.13676358' >> ~/.androidrc

# Source androidrc in bashrc
RUN echo 'source ~/.androidrc' >> ~/.bashrc

# Configure git
RUN git config --global user.email "mina.mindclaw@example.com" && \
    git config --global user.name "Mina Mindclaw"

# Configure git to use HTTPS instead of SSH
RUN git config --global url."https://github.com/".insteadOf "git@github.com:"

WORKDIR /home/agent
