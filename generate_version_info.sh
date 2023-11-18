#!/bin/bash

datetime=$(date -u +'%Y-%m-%d %-H:%M:%S')
githash=$(git rev-parse --short=16 HEAD 2>/dev/null || echo 1)

echo "// This is automatically generated file" > version_info.cpp
echo "#include \"../version_info.h\"" >> version_info.cpp
echo "const std::string_view build_datetime = \"${datetime}\";" >> version_info.cpp
echo "const std::string_view build_githash = \"${githash}\";" >> version_info.cpp
