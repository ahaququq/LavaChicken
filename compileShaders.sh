set -e
echo "Compiling shaders..."

for f in "$1"/src/shaders/*.slang; do
  name="${f##"$1"/src/shaders}"
  name="${name%%.slang}"
  name="$name.spv"
  #glslc "$f" -o ./"$name".spv
  slangc "$f" -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry fragMain -o ./"$name"
  echo "$f" "->" "$name"
done
echo "Shaders compiled!"