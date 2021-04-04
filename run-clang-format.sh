find . \( -iname "*.h" -o -iname "*.cpp" \) -not -path "./cmake-build-debug/*" -not -path "./build/*" | xargs clang-format -i
