#!/bin/bash

# 获取当前系统的架构
arch=$(uname -m)

# 根据架构选择下载文件的 URL
case $arch in
    "x86_64")
        download_url="https://example.com/file_x86_64"
        ;;
    "armv7l")
        download_url="https://example.com/file_armv7l"
        ;;
    "aarch64")
        download_url="https://example.com/file_aarch64"
        ;;
    *)
        echo "Unsupported architecture: $arch"
        exit 1
        ;;
esac

# 下载文件
echo $download_url
