#!/bin/sh
# AES pre-commit hook - runs quick quality gates
echo "Running AES pre-commit checks..."

# Check for TODO/FIXME in source
if grep -r "TODO\|FIXME" --include="*.cpp" --include="*.h" --include="*.hpp" src/ 2>/dev/null; then
    echo "WARNING: TODO/FIXME found in source files"
fi

# Check for console.log/debug in source
if grep -r "console\.log\|std::cout.*debug\|printf.*debug" --include="*.cpp" --include="*.h" --include="*.hpp" src/ 2>/dev/null; then
    echo "WARNING: Debug output found in source files"
fi

exit 0
