#!/bin/bash

dstfile=version_info.cxx
datetime=$(date -u +'%Y-%m-%d %-H:%M:%S')
githash=$(git rev-parse --short=16 HEAD 2>/dev/null || echo 1)

echo "// This is automatically generated file" > ${dstfile}
echo "#include \"../version_info.h\"" >> ${dstfile}
echo "const std::string_view build_datetime = \"${datetime}\";" >> ${dstfile}
echo "const std::string_view build_githash = \"${githash}\";" >> ${dstfile}
