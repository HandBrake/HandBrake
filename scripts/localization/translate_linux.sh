#!/usr/bin/env bash
#
# Usage: Set environment variables, then run ./translate_linux.sh
# Using your own HandBrake Fork, example:
#     export HB_GIT_REPO=https://github.com/<your-username>/HandBrake.git
#     export HB_BRANCH_NAME=translation_update
# Optional:
#     If you want to remove a translator from the files if Transifex erroneously adds it, you can add:
#     export HB_REMOVE_TRANSLATOR="# Username <user@email>, year"
# Please note, "HandBrake" is case sensitive!
#
# This works with the "Download All" zip packages transifex creates.
#     The download all, Goto the "Resources" age, 
#     Click on the resource to delve into it
#     The three dots button top right will have a Download all option.

if [[ -z ${HB_GIT_REPO} ]]; then
    echo "HB_GIT_REPO should be set to your HandBrake Repo Fork URL, including .git";
    exit 1;
fi

if [[ -z ${HB_BRANCH_NAME} ]]; then
    echo "HB_BRANCH_NAME should be set to a new unique branch name for updating translations";
    exit 1;
fi

echo ""
echo "Using: "
echo "Git Repo: $HB_GIT_REPO"
echo "Git Branch: $HB_BRANCH_NAME"
echo "Translator Removal: $HB_REMOVE_TRANSLATOR";
echo ""

# Cleanup Past Runs
echo ""
echo "- Tidyup any previous run"
rm -rf HandBrake
rm *.po

# Create a bit branch for the pull request
echo ""
echo "- Download git fork and create a translation branch."
echo $HB_GIT_REPO
git clone $HB_GIT_REPO
git branch translation_update
git checkout translation_update

# Unpack
echo ""
echo "- Unpacking the transifex files"
unzip handbrakeproject_linux-ui_ghbpot*.zip

# Rename the files
echo ""
echo "- Renaming the files"
for f in *; do
        case $f in 
               ghbpot_*.po)
                        [[ $f =~ ghbpot_(.*) ]]
                        suffix=${BASH_REMATCH[1]}
                        y="$suffix"
                        echo "Renaming $f to $y"
                        mv $f $y
                        
                        if [[ ! -z ${HB_REMOVE_TRANSLATOR} ]]; then
                            echo "   - Cleaning Translators ..."
                            grep -v "$HB_REMOVE_TRANSLATOR" $y > $y.tmp
                            rm $y
                            mv $y.tmp $y
                        fi
                        ;;
        esac
done

# Change the line endings to Windows
echo ""
echo "- Changing Line Endings"
dos2unix *.po

# Copy the files to the correct directory
echo ""
echo "- Copying files to the correct directory"
cp *.po HandBrake/gtk/po

# Commit the change
echo ""
echo "- Creating a Git commit"
cd HandBrake/gtk/po
git add *.po
cd ../../
git commit -m "Updating Linux UI Translations"

echo ""
echo "Done: git push then create a pull request on GitHub."
