#!/bin/bash

info() {
  printf "[${0%.*}][INFO]\033[1;33m${1}\033[0m\n"
}

warn() {
  printf "[${0%.*}][WARN]\033[0;31m${1}\033[0m\n"
}

lint_opts=()
untracked=false
unset against_ref
while [ $# -ge 1 ]; do
  case $1 in
    --against)
      shift
      against_ref="$1"
      shift
      ;;
    -u|--untracked)
      untracked=true
      shift
      ;;
    -v|--fix)
    # known options for `mach lint`
      lint_opts+=("$1")
      shift
      ;;
    -*)
      warn "Unknown options: $1"
      exit -1
      ;;
  esac
done

against_ref=${against_ref:-HEAD}
all_files=()
js_files=()
other_files=()

if [[ ${against_ref} == HEAD ]]; then
  # assume that all changed files are unstaged
  IFS=$'\n' read -r -d '' -a all_files < <( git diff ${against_ref} --diff-filter=ARM --name-only && printf '\0' )
  if [ ${#all_files[@]} -eq 0 ]; then
  # try the staged files
    warn "No changed files compared to ${against_ref} found, I assume you already executed 'git add'. Retry with --cached"
    IFS=$'\n' read -r -d '' -a all_files < <( git diff ${against_ref} --diff-filter=ARM --cached --name-only && printf '\0' )
  fi
else
  # get the ADDED, RENAMED and MODIFIED files compared to ${against_ref}
  info "Get files added / renamed / modified since ${against_ref}($(git rev-parse ${against_ref}))"
  IFS=$'\n' read -r -d '' -a all_files < <( git diff ${against_ref} --diff-filter=ARM --name-only && printf '\0' )
fi
if ${untracked}; then
  info "Get untracked files (that are not recorded in .gitignore)"
  for u in `git ls-files . --exclude-standard --others`; do
    all_files+=(${u})
  done
fi
if [ ${#all_files[@]} -eq 0 ]; then
  warn "No files changed ? If you want to lint all files in gecko, use 'mach lint' directly"
  exit 0
fi

for f in "${all_files[@]}"; do
  echo $f
  ext=${f##*.}
  if [[ $ext =~ ^js[m]? ]]; then
    js_files+=(${f})
  elif [[ $ext =~ ^(c|cpp|cc|css|rs|.*idl)$ ]]; then
    other_files+=(${f})
  fi
done

if [ ${#js_files[@]} -gt 0 ]; then
  info "Lint js/jsm files with eslint..."
  ./mach lint -f unix -l eslint "${lint_opts[@]}" "${js_files[@]}" || {
    warn "============================================================"
    warn "Please fix linting errors:"
    warn "Run './mach lint --fix -v ${js_files[@]}' "
    warn "for more details."
    warn "============================================================"
    exit 1
  }
fi
if [ ${#other_files[@]} -gt 0 ]; then
  info "Literally Lint the other files..."
  ./mach lint -f unix "${lint_opts[@]}" "${other_files[@]}" || {
    warn "============================================================"
    warn "Please fix linting errors:"
    warn "Run './mach lint --fix -v ${other_files[@]}'"
    warn "for more details."
    warn "============================================================"
    exit 1
  }
fi
exit 0
