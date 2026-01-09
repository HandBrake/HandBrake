#!/usr/bin/env bash
# Usage: Set environment variables, then run ./translate_mac.sh
# Using your own HandBrake Fork, example:
#     export HB_GIT_REPO=https://github.com/<your-username>/HandBrake.git
#     export HB_BRANCH_NAME=translation_update
# This works with the "Download All" zip packages transifex creates.
#     The download all, Goto the "Resources" age, 
#     Click on the resource to delve into it
#     The three dots button top right will have a Download all option.

# Preconditions
if [[ -z "${HB_GIT_REPO:-}" ]]; then
    echo "HB_GIT_REPO should be set to your HandBrake Repo Fork URL, including .git"
    exit 1
fi

if [[ -z "${HB_BRANCH_NAME:-}" ]]; then
    echo "HB_BRANCH_NAME should be set to a new unique branch name for updating translations"
    exit 1
fi

echo ""
echo "Using: "
echo "Git Repo:   $HB_GIT_REPO"
echo "Git Branch: $HB_BRANCH_NAME"
echo ""

# Cleanup Past Runs
echo ""
echo "- Tidyup any previous run"
rm -rf HandBrake Translations
mkdir Translations

# Create a bit branch for the pull request
echo ""
echo "- Download git fork and create a translation branch."
git clone "$HB_GIT_REPO" HandBrake
cd HandBrake
git checkout -b "$HB_BRANCH_NAME"
cd ..

# Unpack
echo ""
echo "- Unpacking the transifex files"
unzip handbrakeproject_mac-ui_*.zip -d Translations

cd HandBrake || exit 1

# Supported languages (add all available languages in macosx here)
SUPPORTED_LANGS=("bg" "co" "de" "fr" "it" "ja_jp" "ko" "pt_br" "ru" "sv_se" "uk_ua" "zh") 

# Function to check whether a language is supported
is_supported_lang() {
    local lang="$1"
    for l in "${SUPPORTED_LANGS[@]}"; do
        [[ "$l" == "$lang" ]] && return 0
    done
    return 1
}

# Process XLIFF files
find ../Translations -name "*.xlf" | while read -r xlf; do
    filename=$(basename "$xlf")

    if [[ "$filename" =~ ^enxliff-([A-Za-z0-9_]+)\.xlf$ ]]; then
        lang="${BASH_REMATCH[1]}"
    else
        echo "Skipping unrecognized file: $filename"
        continue
    fi

    if ! is_supported_lang "$lang"; then
        echo "Skipping unsupported language: $lang (from $filename)"
        continue
    fi

    echo "Importing localization for language: $lang"

    xcodebuild \
        -importLocalizations \
        -project macosx/HandBrake.xcodeproj \
        -localizationPath "$xlf"

    src="macosx/build/release/external/macosx/$lang.lproj/Localizable.strings"
    dst="macosx/$lang.lproj/Localizable.strings"

    if [[ -f "$src" ]]; then
        mkdir -p "macosx/$lang.lproj"
        mv "$src" "$dst"
    else
        echo "No output generated for language: $lang"
    fi
done

# Fix CocoaBindingsConnection bug
echo "Fixing CocoaBindingsConnection placeholders"

find macosx -name "MainWindow.strings" | while read -r f; do
    sed -i '' \
        's/^\(".*\.ibShadowedIsNilPlaceholder"\)[[:space:]]*=[[:space:]]*".*";/\1 = "";/' \
        "$f"
done

# Commit the change
echo ""
echo "- Creating a Git commit"
git add macosx
git commit -m "Update Mac UI translations"

echo ""
echo "Done: git push then create a pull request on GitHub."
