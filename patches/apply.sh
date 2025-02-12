#!/bin/bash
cd ..
git am patches/*.patch
# PR descriptions should not contain "from:" in the beginning of lines.
# In case of problems, run
# git am --abort
