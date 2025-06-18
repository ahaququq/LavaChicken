set -e
echo "Compiling shaders..."

for f in "$1"/src/shaders/*.????; do
  name="${f#"$1"/src/shaders}"
  glslc "$f" -o ./"$name".spv
done
echo "Shaders compiled!"