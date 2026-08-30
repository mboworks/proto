#!/usr/bin/env bash

# SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -euo pipefail

function die() {
  echo "ERROR: ${*}" 1>&2
  exit 1
}

[[ ${#} == 1 ]] || die "Must provide a version argument."

git fetch origin main # Make sure the below is relevant

# Must actually be on `main` at exactly origin/main. The tree-diff checks below
# pass for any branch whose tree matches main (e.g. a just-squash-merged feature
# branch), so on their own they would let a release be cut off the wrong commit.
BRANCH="$(git rev-parse --abbrev-ref HEAD)"
if [[ "${BRANCH}" != "main" ]]; then
  die "Must be run from the 'main' branch (currently on '${BRANCH}')."
fi
if [[ "$(git rev-parse HEAD)" != "$(git rev-parse origin/main)" ]]; then
  die "HEAD ($(git rev-parse --short HEAD)) is not at origin/main ($(git rev-parse --short origin/main)); pull first."
fi

if [[ -n "$(git status --porcelain)" ]]; then
  # Non empty output means non clean branch.
  die "Must be run from clean 'main' branch."
fi
if [[ -n "$(git diff origin/main --numstat)" ]]; then
  die "Must be run from clean 'main' branch."
fi
if [[ -n "$(git diff origin/main --cached --numstat)" ]]; then
  die "Must be run from clean 'main' branch."
fi

VERSION="${1}"

# Releases are strictly numeric <major>.<minor>.<patch>.
if [[ ! "${VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  die "Version must be numeric <major>.<minor>.<patch> (got '${VERSION}')."
fi

BAZELMOD_VERSION="$(sed -rne 's,.*version = "([0-9]+([.][0-9]+)+.*)".*,\1,p' <MODULE.bazel | head -n1)"
CHANGELOG_VERSION="$(sed -rne 's,^# ([0-9]+([.][0-9]+)+.*)$,\1,p' <CHANGELOG.md | head -n1)"
NEXT_VERSION="$(echo "${VERSION}" | awk -F. '/^(0|[1-9][0-9]*)([.](0|[1-9][0-9]*))([.](0|[1-9][0-9]*))+([-+]|$)/{print $1"."$2"."(($3)+1)}')"

if [[ "${BAZELMOD_VERSION}" != "${CHANGELOG_VERSION}" ]]; then
  die "MODULE.bazel (${BAZELMOD_VERSION}) != CHANGELOG.md (${CHANGELOG_VERSION})."
fi

if [[ "${VERSION}" != "${BAZELMOD_VERSION}" ]]; then
  die "Provided version argument (${VERSION}) different from merged version (${BAZELMOD_VERSION})."
fi

if [[ -z "${NEXT_VERSION}" ]]; then
  die "Could not determine next version from input (${VERSION})."
fi

grep "${VERSION}" < <(git tag -l) && die "Version tag is already in use."

# Pre-flight: release_prep.sh applies .github/workflows/bazelmod.patch to the
# worktree before archiving (it shapes the released MODULE.bazel by commenting
# out the dev-only includes). If it no longer applies (e.g. context drift after a
# dependency bump) the release would tag and then fail mid-build. Catch it here,
# before we tag anything.
patch -p1 --dry-run -f -i .github/workflows/bazelmod.patch >/dev/null 2>&1 \
  || die "Patch .github/workflows/bazelmod.patch no longer applies; regenerate it before releasing."

git tag -s -a "${VERSION}" \
  -m "New release tag version: '${VERSION}'." \
  -m "$(awk '/^#/{if(NR>1)exit}/^[^#]/{print}' <CHANGELOG.md)"
git push origin --tags

echo "Next version: ${NEXT_VERSION}"

# Bump the module version (the first `version = "X"` line). Portable across BSD
# (macOS) and GNU sed: BSD `sed -i` needs a backup-suffix arg and `0,/re/` is a
# GNU-only address, so write to a temp file and use the portable `1,/re/` range.
sed "1,/version = \"${VERSION}\"/ s/version = \"${VERSION}\"/version = \"${NEXT_VERSION}\"/" MODULE.bazel >MODULE.bazel.tmp
mv MODULE.bazel.tmp MODULE.bazel

# Prepend a new top section for the next version (portable; no `sed -i`).
{
  printf '# %s\n\n' "${NEXT_VERSION}"
  cat CHANGELOG.md
} >CHANGELOG.md.tmp
mv CHANGELOG.md.tmp CHANGELOG.md

NEXT_BRANCH="chore/bump_version_to_${NEXT_VERSION}"

git checkout -b "${NEXT_BRANCH}"
git add MODULE.bazel
git add CHANGELOG.md
git commit -m "Bump version to ${NEXT_VERSION}"
git push -u origin "${NEXT_BRANCH}"

# Open the version-bump PR and stop. We deliberately do NOT auto-approve/merge it:
# GitHub forbids approving your own PR, so a second person must review and merge.
if which gh >/dev/null 2>&1; then
  gh pr create \
    --title "Bump version from ${VERSION} to ${NEXT_VERSION}" \
    --body "Automated version bump from ${VERSION} to ${NEXT_VERSION} created by ${0}. Please review and merge." \
    || echo "Could not create the PR automatically; open one from branch '${NEXT_BRANCH}'."
  echo "Opened the version-bump PR for '${NEXT_BRANCH}'. Have another maintainer review and merge it."
else
  echo "Pushed '${NEXT_BRANCH}'. Open a version-bump PR for it and have another maintainer merge it."
fi

# Leave the checkout back on a clean main (the bump lands via the PR above).
git checkout main
